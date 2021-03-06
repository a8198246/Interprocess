#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SLAVE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SLAVE

#pragma once

#include "Main.hpp"
#include "Chunk.hpp"
#include "Broker.hpp"
#include "Write Chunk To Buffer.hpp"
#include "Threaded Logger.hpp"

#include "../Application Identifier.hpp"
#include "../Event Identifier.hpp"

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <stdexcept>
#include <map>
#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_Slave
	{
		protected: typedef ::std::map<t_ApplicationId, t_Chunk>
		t_PendingOutputsFromMaster;

		#pragma region Fields

		protected: t_Broker                        m_broker;
		//	slaves to master
		protected: ::boost::asio::io_service       m_input_service;
		protected: ::boost::asio::io_service::work m_input_service_work;
		protected: ::boost::thread                 m_input_service_thread; // runs input service loop which handles writing of data into slaves to master pipe
		//	master to slave
		protected: t_PendingOutputsFromMaster      m_pending_outputs;
		protected: ::boost::mutex                  m_outputs_sync;

		#pragma endregion

		public:
		t_Slave(void)
		:	m_input_service_work(m_input_service)
		,	m_input_service_thread(::boost::bind(&::boost::asio::io_service::run, &m_input_service))
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
				logger.Print_Prefix() << "input service thread started with id " << m_input_service_thread.get_id() << ::std::endl;
			}
		#endif
		}

		private:
		t_Slave(t_Slave const &) = delete;
		
		private:
		t_Slave(t_Slave &&) = delete;

		public:
		~t_Slave(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			m_input_service.stop();
		//	m_input_service_thread.join(); // this will cause a deadlock since threads started from dll will be waiting for it to unload
			::boost::this_thread::sleep(::boost::posix_time::seconds(1)); // let's hope that user-mode code in m_input_service_thread and m_output_service_thread will be completed during this period
		}

		private: void
		operator =(t_Slave const &) = delete;
		
		private: void
		operator =(t_Slave &&) = delete;
		
		//	Returns number of bytes written into the buffer.
		//	To be called from user threads
		public: auto
		Receive_From_Master(_In_ const t_ApplicationId application_id, _Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity) -> size_t
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			t_Broker::t_MasterToSlavePipe * p_pipe;
			t_Chunk * p_pending_output;
			{
				::boost::lock_guard<::boost::mutex> lock(m_outputs_sync);
				p_pipe = &m_broker.Get_MasterToSlavePipe(application_id);
				p_pending_output = &m_pending_outputs[application_id];
			}
			size_t bc_received(0);
			for(;;)
			{
				Write_Chunk_To_Buffer(*p_pending_output, p_buffer, bc_buffer_capacity, bc_received, *p_pending_output);
				if(bc_received == bc_buffer_capacity)
				{
					break;
				}
				assert(p_pending_output->Is_Empty());
				if(p_pipe->Is_Empty())
				{
					break;
				}
				*p_pending_output = p_pipe->Read(0);
			}
			assert(bc_received <= bc_buffer_capacity);
			return(bc_received);
		}
		
		//	To be called from input service thread
		private: void
		Handle_Input_To_Master(_In_ t_Chunk chunk)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto & pipe = m_broker.Get_SlavesToMasterPipe();
			pipe.Write(chunk);
		}

		//	To be called from user threads
		public: void
		Send_To_Master(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(nullptr != p_data);
			assert(0 < bc_data);
			m_input_service.post(::boost::bind(&t_Slave::Handle_Input_To_Master, this, t_Chunk(p_data, bc_data)));
		}

		//	To be called from user threads
		public: auto
		ReceiveCommon_From_Master(_In_ const t_EventId event_id, _Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(nullptr != p_buffer);
			assert(0 < bc_buffer_capacity);
			auto const bc_received = m_broker.Get_CommonPipeForReading(event_id)->Read(p_buffer, bc_buffer_capacity, timeout_msec);
			assert(bc_received <= bc_buffer_capacity);
			return(bc_received);
		}
	};
}
}
}

#endif
