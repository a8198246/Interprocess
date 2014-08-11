#ifndef HEADER_VTT_INTERPROCESS_DETAILS_MASTER
#define HEADER_VTT_INTERPROCESS_DETAILS_MASTER

#pragma once

#include "Chunk.hpp"
#include "Broker.hpp"
#include "Write Chunk To Buffer.hpp"

#include "../Application Identifier.hpp"

#include <cassert>

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#ifdef _DEBUG_LOGGING
#include <fstream>
#endif

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_Master
	{
		#pragma region Fields

		protected: t_Broker                        m_Broker;
		//	master-to-slaves
		protected: ::boost::asio::io_service       m_input_service;
		protected: ::boost::thread                 m_input_service_thread; // runs input service loop which handles writing of data into master-to-slave pipes
		protected: ::boost::asio::io_service::work m_input_service_work;
		//	slaves-to-master
		protected: t_Chunk                         m_pending_output;
	#ifdef _DEBUG_LOGGING
		protected: ::boost::mutex                  m_logger_sync;
		private: ::std::ofstream                   m_logger;
	#endif

		#pragma endregion

		public: t_Master(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		{
		#ifdef _DEBUG_LOGGING
			{
				::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
				m_logger.open("master.log");
				m_logger << "master initialized" << ::std::endl;
			}
		#endif
		}
		
		public: ~t_Master(void)
		{
			m_input_service.stop();
		//	m_input_service_thread.join(); // this will cause a deadlock since threads started from dll will be waiting for it to unload
			::boost::this_thread::sleep(::boost::posix_time::seconds(1)); // let's hope that user-mode code in m_input_service_thread will be completed during this period
		}

		//	Returns number of bytes written into the buffer.
		//	To be called from user threads
		public: auto Recieve_From_Slaves(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
			assert(nullptr != p_buffer);
			assert(0 < bc_buffer_capacity);
			size_t bc_written = 0;
			auto have_pending_output_from_prvious_call_to_this_function = m_pending_output.Is_Not_Empty();
			if(have_pending_output_from_prvious_call_to_this_function)
			{
				Write_Chunk_To_Buffer(m_pending_output, p_buffer, bc_buffer_capacity, bc_written, m_pending_output);
			}
			else
			{
				assert(m_pending_output.Is_Empty());
				auto & pipe = m_Broker.Get_SlavesToMasterPipe();
				Write_Chunk_To_Buffer(pipe.Read(timeout_msec), p_buffer, bc_buffer_capacity, bc_written, m_pending_output);
			}
			assert(bc_written <= bc_buffer_capacity);
			return(bc_written);
		}

		//	To be called from input service thread
		private: void Handle_Input_To_Slave(_In_ const t_ApplicationId application_id, _In_ t_Chunk chunk)
		{
			auto & pipe = m_Broker.Get_MasterToSlavePipe(application_id);
			pipe.Write(chunk);
		#ifdef _DEBUG_LOGGING
			{
				::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
				m_logger << "written " << chunk.Get_Size() << "bytes of data for " << application_id << ::std::endl;
			}
		#endif
		}

		//	To be called from user threads
		public: void Send_To_Slave(_In_ const t_ApplicationId application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			m_input_service.post(::boost::bind(&t_Master::Handle_Input_To_Slave, this, application_id, t_Chunk(p_data, bc_data)));
		}
	};
}
}
}

#endif