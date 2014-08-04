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
		//	slaves to master
		protected: ::boost::thread                 m_output_service_thread; // continiously reads data from slaves to master pipe and sends it to output service
		protected: ::boost::asio::io_service       m_output_service;
		protected: ::boost::asio::io_service::work m_output_service_work;
		//	TODO replace with stack or TLS?
		protected: ::boost::mutex                  m_pending_output_chunk_sync;
		protected: t_Chunk                         m_pending_output_chunk;
		//	debug logging
	//#ifdef _DEBUG
	//	protected: ::boost::mutex                  m_logger_sync;
	//	private: ::std::ofstream                   m_logger;
	//#endif

		#pragma endregion

		public: t_Master(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		,	m_output_service_work(m_output_service)
		,	m_output_service_thread(::boost::bind(&t_Master::Retrieve_Output_From_Slaves, this))
		{
		//#ifdef _DEBUG
		//	{
		//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
		//		m_logger.open("master.log");
		//		m_logger << "master initialized" << ::std::endl;
		//	}
		//#endif
		}

		//	output service thread routine
		private: void Retrieve_Output_From_Slaves(void)
		{
			auto & pipe = m_Broker.Get_SlavesToMasterPipe();
			for(;;)
			{
				m_output_service.post(::boost::bind(&t_Master::Handle_Ouput_From_Slaves, this, pipe.Read()));
			}
		}

		//	Method to be called from user threads
		private: void Handle_Ouput_From_Slaves(_In_ t_Chunk chunk)
		{
		//#ifdef _DEBUG
		//	{
		//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
		//		m_logger << "master got " << chunk.Get_Size() << " bytes of data from slave process" << ::std::endl;
		//	}
		//#endif
			m_pending_output_chunk = chunk;
		}

		//	Method to be called from user threads
		//	Returns number of bytes written into the buffer.
		public: auto Recieve_From_Slaves(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer_capacity) -> int
		{
			auto p_write_cursor = p_buffer;
			{
				::boost::lock_guard<::boost::mutex> lock(m_pending_output_chunk_sync);
				auto const p_buffer_end = p_buffer + bc_buffer_capacity;
				for(;;)
				{
					m_pending_output_chunk.Clear();
					m_output_service.poll_one();
					if(m_pending_output_chunk.Is_Empty())
					{
						break;
					}
					auto p_write_end = p_write_cursor + m_pending_output_chunk.Get_Size();
					if(p_write_end <= p_buffer_end)
					{
						memcpy(p_write_cursor, m_pending_output_chunk.Get_Data(), m_pending_output_chunk.Get_Size());
						p_write_cursor = p_write_end;
						if(p_write_end == p_buffer_end)
						{
							break;
						}
					}
					else
					{
						m_output_service.dispatch(::boost::bind(&t_Master::Handle_Ouput_From_Slaves, this, m_pending_output_chunk));
						break;
					}
				}
			}
			auto bc_written = static_cast<int>(p_write_cursor - p_buffer);
			assert(bc_written <= bc_buffer_capacity);
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