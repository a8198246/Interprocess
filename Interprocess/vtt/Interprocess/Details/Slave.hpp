#ifndef HEADER_VTT_INTERPROCESS_DETAILS_SLAVE
#define HEADER_VTT_INTERPROCESS_DETAILS_SLAVE

#pragma once

#include "Chunk.hpp"
#include "Broker.hpp"
#include "../Application Identifier.hpp"

#include "../Configuration.hpp"

#include <cassert>
#include <memory.h>
#include <stdexcept>

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
	class t_Slave
	{
		#pragma region Fields

		protected: t_Broker                        m_Broker;
		//	slaves to master
		protected: ::boost::thread                 m_input_service_thread; // runs input service loop which handles writing of data into slaves to master pipe
		protected: ::boost::asio::io_service       m_input_service;
		protected: ::boost::asio::io_service::work m_input_service_work;
		//	master to slave
		protected: ::boost::thread                 m_output_service_thread; // continiously reads data from master to slave pipe and sends it to output service
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
		private: volatile t_ApplicationId          m_application_id = 0;
		private: volatile bool                     m_application_set = false;
		private: ::boost::mutex                    m_application_id_sync;

		#pragma endregion

		public: t_Slave(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		,	m_output_service_work(m_output_service)
		{
		//#ifdef _DEBUG
		//	{
		//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
		//		m_logger.open("slave.log");
		//		m_logger << "slave initialized" << ::std::endl;
		//	}
		//#endif
		}

		//	output service thread routine
		private: void Retrieve_Output_From_Master(void)
		{
			assert(m_application_set);
			auto p_pipe = m_Broker.Get_MasterToSlavePipe(m_application_id);
			if(nullptr != p_pipe)
			{
				auto & pipe = *p_pipe;
				for(;;)
				{
					m_output_service.post(::boost::bind(&t_Slave::Handle_Ouput_From_Master, this, pipe.Read()));
				}
			}
		}

		//	Method to be called from user threads
		private: void Handle_Ouput_From_Master(_In_ t_Chunk chunk)
		{
			assert(m_application_set);
		//#ifdef _DEBUG
		//	{
		//		::boost::lock_guard<::boost::mutex> lock(m_logger_sync);
		//		m_logger << "slave got " << chunk.Get_Size() << " bytes of data from master process" << ::std::endl;
		//	}
		//#endif
			m_pending_output_chunk = chunk;
		}

		//	Method to be called from user threads
		//	Returns number of bytes written into the buffer.
		public: auto Recieve_From_Master(_In_ const t_ApplicationId application_id, _Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer_capacity) -> int
		{
			if(!m_application_set)
			{
				::boost::lock_guard<decltype(m_application_id_sync)> lock(m_application_id_sync);
				if(!m_application_set)
				{
					m_application_id = application_id;
					m_application_set = true;
					m_output_service_thread = ::boost::thread(::boost::bind(&t_Slave::Retrieve_Output_From_Master, this));
				}
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
						m_output_service.dispatch(::boost::bind(&t_Slave::Handle_Ouput_From_Master, this, m_pending_output_chunk));
						break;
					}
				}
			}
			auto bc_written = static_cast<int>(p_write_cursor - p_buffer);
			assert(bc_written <= bc_buffer_capacity);
			return(bc_written);
		}

		//	To be called from input service thread
		private: void Handle_Input_To_Master(_In_ t_Chunk chunk)
		{
			auto & pipe = m_Broker.Get_SlavesToMasterPipe();
			pipe.Write(chunk);
		}

		//	To be called from user threads
		public: void Send_To_Master(_In_reads_bytes_(bc_data) char const * p_data, _In_ const int bc_data)
		{
			m_input_service.post(::boost::bind(&t_Slave::Handle_Input_To_Master, this, t_Chunk(p_data, bc_data)));
		}
	};
}
}
}

#endif