#ifndef HEADER_VTT_INTERPROCESS_DETAILS_MASTER
#define HEADER_VTT_INTERPROCESS_DETAILS_MASTER

#pragma once

#include "Chunk.hpp"
#include "Broker.hpp"

#include "../Configuration.hpp"

#include <cassert>
#include <memory.h>

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#ifdef _DEBUG
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
		//	master to slaves
		protected: ::boost::thread                 m_input_service_thread; // runs input service loop which handles writing of data into master to slave pipes
		protected: ::boost::asio::io_service       m_input_service;
		protected: ::boost::asio::io_service::work m_input_service_work;
		//	debug logging
	//#ifdef _DEBUG
	//	protected: ::boost::mutex                  m_logger_sync;
	//	private: ::std::ofstream                   m_logger;
	//#endif

		#pragma endregion

		public: t_Master(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		{
		//#ifdef _DEBUG
		//	{
		//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
		//		m_logger.open("master.log");
		//		m_logger << "master initialized" << ::std::endl;
		//	}
		//#endif
		}

		//	Method to be called from user threads
		//	Returns number of bytes written into the buffer.
		public: auto Recieve_From_Slaves(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer_capacity) -> int
		{
			auto & pipe = m_Broker.Get_SlavesToMasterPipe();
			auto recieved_block = pipe.Read();
			int bc_written;
			if(recieved_block.Get_Size() <= static_cast<size_t>(bc_buffer_capacity))
			{
				memcpy(p_buffer, recieved_block.Get_Data(), recieved_block.Get_Size());
				bc_written = static_cast<int>(recieved_block.Get_Size());
			}
			else
			{
			//#ifdef _DEBUG
			//	{
			//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
			//		m_logger << "insufficient buffer size to read output from slave processes: "
			//			<< bc_buffer_capacity << " bytes, reqired " << recieved_block.Get_Size() << ::std::endl;
			//	}
			//#endif
				bc_written = 0;
			}
			return(bc_written);
		}

		//	To be called from input service thread
		private: void Handle_Input_To_Slave(_In_ const t_ApplicationId application_id, _In_ t_Chunk chunk)
		{
			auto p_pipe = m_Broker.Get_MasterToSlavePipe(application_id);
			if(nullptr != p_pipe)
			{
			//#ifdef _DEBUG
			//	{
			//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
			//		m_logger << "written " << chunk.Get_Size() << "bytes of data for " << application_id << ::std::endl;
			//	}
			//#endif
				p_pipe->Write(chunk);
			}
			else
			{
			//#ifdef _DEBUG
			//	{
			//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
			//		m_logger << "failed to write data for " << application_id << ": application cap reached" <<::std::endl;
			//	}
			//#endif
			}
		}

		//	To be called from user threads
		public: void Send_To_Slave(_In_ const t_ApplicationId application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_ const int bc_data)
		{
			m_input_service.post(::boost::bind(&t_Master::Handle_Input_To_Slave, this, application_id, t_Chunk(p_data, bc_data)));
		}
	};
}
}
}

#endif