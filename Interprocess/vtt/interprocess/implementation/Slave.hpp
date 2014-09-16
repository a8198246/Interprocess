#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SLAVE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SLAVE

#pragma once

#include "Main.hpp"
#include "Chunk.hpp"
#include "Broker.hpp"
#include "Write Chunk To Buffer.hpp"
#include "Threaded Logger.hpp"

#include "../Application Identifier.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_Slave
	{
		#pragma region Fields

		protected: t_Broker                        m_Broker;
		//	slaves to master
		protected: ::boost::asio::io_service       m_input_service;
		protected: ::boost::thread                 m_input_service_thread; // runs input service loop which handles writing of data into slaves to master pipe
		protected: ::boost::asio::io_service::work m_input_service_work;
		//	master to slave
		protected: ::boost::asio::io_service       m_output_service;
		protected: ::boost::thread                 m_output_service_thread; // continiously reads data from master to slave pipe and sends it to output service
		protected: t_Chunk                         m_pending_output;

		private: volatile t_ApplicationId          m_application_id = 0;
		private: volatile bool                     m_application_id_set = false;

		#pragma endregion

		public: t_Slave(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
		}

		public: ~t_Slave(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			m_input_service.stop();
		//	m_input_service_thread.join(); // this will cause a deadlock since threads started from dll will be waiting for it to unload
			m_output_service_thread.interrupt();
		//	m_output_service_thread.join(); // this will cause a deadlock since threads started from dll will be waiting for it to unload
			::boost::this_thread::sleep(::boost::posix_time::seconds(1)); // let's hope that user-mode code in m_input_service_thread and m_output_service_thread will be completed during this period
		}
		
		//	output service thread routine
		private: void Retrieve_Output_From_Master(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(m_application_id_set);
			auto & pipe = m_Broker.Get_MasterToSlavePipe(m_application_id);
			for(;;)
			{
				auto chunk = pipe.Read();
				m_output_service.post(::boost::bind(&t_Slave::Handle_Ouput_From_Master, this, chunk));
			}
		}

		//	To be called from user threads
		private: void Handle_Ouput_From_Master(_In_ t_Chunk chunk)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(m_application_id_set);
			assert(m_pending_output.Is_Empty());
			m_pending_output = chunk;
		}

		//	Returns number of bytes written into the buffer.
		//	To be called from user threads
		public: auto Receive_From_Master(_In_ const t_ApplicationId application_id, _Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity) -> size_t
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			if(!m_application_id_set)
			{
				m_application_id = application_id;
				m_application_id_set = true;
				auto & pipe = m_Broker.Get_MasterToSlavePipe(m_application_id); // this will initialize the pipe
				if(pipe.Is_Not_Empty())
				{
					m_pending_output = pipe.Read();
				}
				m_output_service_thread = ::boost::thread(::boost::bind(&t_Slave::Retrieve_Output_From_Master, this));
			}
			if(application_id != m_application_id)
			{
				throw
				(
					::std::logic_error
					(
						"during attempt to read master process output an application ID specified "
						"(" + ::boost::lexical_cast<::std::string>(application_id) + ")"
						" differs from the application ID specified in previous such attempt "
						"(" + ::boost::lexical_cast<::std::string>(m_application_id) + ")"
					)
				);
			}
			size_t bc_written = 0;
			for(;;)
			{
				Write_Chunk_To_Buffer(m_pending_output, p_buffer, bc_buffer_capacity, bc_written, m_pending_output);
				if(bc_written == bc_buffer_capacity)
				{
					break;
				}
				m_output_service.run_one();
				if(m_pending_output.Is_Empty())
				{
					m_output_service.poll_one();
				}
				if(m_pending_output.Is_Empty())
				{
					break;
				}
			}
			assert(bc_written <= bc_buffer_capacity);
			return(bc_written);
		}

		//	To be called from input service thread
		private: void Handle_Input_To_Master(_In_ t_Chunk chunk)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto & pipe = m_Broker.Get_SlavesToMasterPipe();
			pipe.Write(chunk);
		}

		//	To be called from user threads
		public: void Send_To_Master(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(nullptr != p_data);
			assert(0 < bc_data);
			m_input_service.post(::boost::bind(&t_Slave::Handle_Input_To_Master, this, t_Chunk(p_data, bc_data)));
		}

		//	To be called from user threads
		public: auto ReceiveCommon_From_Master(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(nullptr != p_buffer);
			assert(0 < bc_buffer_capacity);
			auto bc_written = m_Broker.Get_CommonPipe().Read(p_buffer, bc_buffer_capacity, timeout_msec);
			assert(bc_written <= bc_buffer_capacity);
			return(bc_written);
		}
	};
}
}
}

#endif