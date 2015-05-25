#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER

#pragma once

#include "../Configuration.hpp"
#include "../Application Identifier.hpp"
#include "../Event Identifier.hpp"

#include "Threaded Logger.hpp"
#include "Multi Writer Single Reader Pipe.hpp"
#include "Single Writer Multi Reader Pipe.hpp"

#include <sal.h>

#include <string>
#include <memory>
#include <map>
#include <deque>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_Broker
	{
		public: typedef t_MultiWriterSingleReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT * VTT_INTERPROCESS_SLAVE_TO_MASTER_MESSAGE_BUFFER_SIZE_MAGNITUDE>
		t_SlavesToMasterPipe;

		public: typedef t_MultiWriterSingleReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT>
		t_MasterToSlavePipe;
		
		protected: typedef ::std::map<t_ApplicationId, t_MasterToSlavePipe>
		t_MasterToSlavePipesMap;

		public: typedef t_SingleWriterMultiReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT>
		t_CommonPipe;

		protected: typedef ::std::unique_ptr<t_CommonPipe>
		t_CommonPipePointer;
	
		protected: typedef ::std::map<t_EventId, t_CommonPipe>
		t_CommonPipesMap;

		protected: typedef ::std::deque<t_EventId>
		t_EventsIds;

		protected: typedef ::std::pair<int, t_EventId>
		t_msAgeEventIdPair;

		protected: typedef ::std::deque<t_msAgeEventIdPair>
		t_FiredEventsIds;

		#pragma region Fields

		protected: static const int m_common_pipes_test_interval = 100;

		protected: t_SlavesToMasterPipe    m_slaves_to_master_pipe;
		protected: t_MasterToSlavePipesMap m_master_to_slaves_pipes_map;
		protected: t_SlavesToMasterPipe    m_notifications_from_listening_slaves;
		// to be used by master processes
		protected: t_CommonPipesMap        m_common_pipes_map; 
		protected: t_EventsIds             m_events_ids;
		protected: t_FiredEventsIds        m_fired_events_ids;
		protected: ::boost::thread         m_notifications_thread;
		protected: ::boost::mutex          m_common_pipes_map_sync;

		#pragma endregion

		public:
		t_Broker(void)
		:	m_slaves_to_master_pipe(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "s to m"))
		,	m_notifications_from_listening_slaves(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "notifications from s"))
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
		}

		private:
		t_Broker(t_Broker const &) = delete;

		private:
		t_Broker(t_Broker &&) = delete;
		
		private: void
		operator =(t_Broker const &) = delete;

		private: void
		operator =(t_Broker &&) = delete;

		public: auto
		Get_SlavesToMasterPipe(void) -> t_SlavesToMasterPipe &
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			return(m_slaves_to_master_pipe);
		}

		public: auto
		Get_MasterToSlavePipe(_In_ const t_ApplicationId application_id) -> t_MasterToSlavePipe &
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto it_pair = m_master_to_slaves_pipes_map.find(application_id);
			if(m_master_to_slaves_pipes_map.end() == it_pair)
			{
			#ifdef _DEBUG_LOGGING
				t_ThreadedLogger::Print_Message("creating master to slave pipe for application id = " + ::boost::lexical_cast<::std::string>(application_id));
			#endif
				it_pair = m_master_to_slaves_pipes_map.emplace
				(
					::std::piecewise_construct
				,	::std::forward_as_tuple(application_id)
				,	::std::forward_as_tuple(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "m to s" + ::boost::lexical_cast<::std::string>(application_id))
				).first;
			}
			return(it_pair->second);
		}

		protected: void
		Waiting_For_SlavesThreadProc(void)
		{
			for(;;)
			{
				Pull_Notifications(m_common_pipes_test_interval);
				Reset_PendingCommonPipeFlags();
				::boost::this_thread::interruption_point();
			}
		}

		protected: void
		Pull_Notifications(_In_ const int timeout_msec)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto notifications = m_notifications_from_listening_slaves.Read(timeout_msec);
			if(notifications.Is_Not_Empty())
			{
				auto const event_ids_count = notifications.Get_Size() / sizeof(t_EventId);
				auto const event_ids_begin = reinterpret_cast<t_EventId const *>(notifications.Get_Data());
				auto const event_ids_end = event_ids_begin + event_ids_count;
				auto event_id_iterator = event_ids_begin;
				while(event_ids_end != event_id_iterator)
				{
					auto const & event_id = *event_id_iterator;
					auto const pipe_iterator = m_common_pipes_map.find(event_id);
					if(m_common_pipes_map.end() == pipe_iterator)
					{
						::boost::lock_guard<::boost::mutex> lock(m_common_pipes_map_sync);
						if(m_common_pipes_map.size() == VTT_INTERPROCESS_EVENTS_QUEUE_SIZE_LIMIT)
						{
							auto const oldest_event_id = m_events_ids.front();
							m_events_ids.pop_front();
							auto const common_pipe_to_be_erased_iterator = m_common_pipes_map.find(oldest_event_id);
							common_pipe_to_be_erased_iterator->second.Reset_Flag();
							m_common_pipes_map.erase(common_pipe_to_be_erased_iterator);
						}
						m_events_ids.push_back(event_id);
						m_common_pipes_map.emplace
						(
							::std::piecewise_construct
						,	::std::forward_as_tuple(event_id)
						,	::std::forward_as_tuple(Make_CommonPipeName(event_id))
						);
					}
					++event_id_iterator;
				}
			}
		}

		protected: void
		Reset_PendingCommonPipeFlags(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			::boost::lock_guard<::boost::mutex> lock(m_common_pipes_map_sync);
			if(!m_fired_events_ids.empty())
			{
				bool continue_iterating;
				auto fired_event_id_iterator = m_fired_events_ids.begin();
				do
				{
					auto & current_fired_event_id_ms_age = fired_event_id_iterator->first;
					auto const current_fired_event_id = fired_event_id_iterator->second;
					++fired_event_id_iterator;
					continue_iterating = (m_fired_events_ids.end() != fired_event_id_iterator);
					current_fired_event_id_ms_age += m_common_pipes_test_interval;
					if(500 <= current_fired_event_id_ms_age)
					{
						auto const pipe_iterator = m_common_pipes_map.find(current_fired_event_id);
						if(m_common_pipes_map.end() != pipe_iterator)
						{
							pipe_iterator->second.Reset_Flag();
						}
						m_fired_events_ids.pop_front();
					}
				}
				while(continue_iterating);
			}
		}

		public: void
		Start_Waiting_For_Slaves(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			Pull_Notifications(0);
			m_notifications_thread = ::boost::thread(::boost::bind(&t_Broker::Waiting_For_SlavesThreadProc, this));
		}

		public: void
		Stop_Waiting_For_Slaves(void)
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			m_notifications_thread.interrupt();
		}

		public: auto
		Get_CommonPipesSync(void) -> ::boost::mutex &
		{
			return(m_common_pipes_map_sync);
		}

		//	m_common_pipes_map_sync should be locked before calling this method.
		public: auto
		Get_CommonPipeForWritingPointer(_In_ const int event_id) -> t_CommonPipe *
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto const pipe_iterator = m_common_pipes_map.find(event_id);
			if(m_common_pipes_map.end() != pipe_iterator)
			{
				m_fired_events_ids.emplace_back(0, event_id);
				return(&pipe_iterator->second);
			}
			else
			{
				return(nullptr);
			}
		}
				
		public: auto
		Get_CommonPipeForReading(_In_ const int event_id) -> ::std::unique_ptr<t_CommonPipe>
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto p_common_pipe = ::std::make_unique<t_CommonPipe>(Make_CommonPipeName(event_id));
			m_notifications_from_listening_slaves.Write(t_Chunk(reinterpret_cast<char const *>(&event_id), sizeof(event_id)));
			return(::std::move(p_common_pipe));
		}

		protected: static auto
		Make_CommonPipeName(_In_ const int event_id) -> ::std::string
		{
			return(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "common" + ::boost::lexical_cast<::std::string>(event_id));
		}
	};
}
}
}

#endif
