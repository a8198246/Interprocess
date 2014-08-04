#ifndef HEADER_VTT_INTERPROCESS_DETAILS_PIPE
#define HEADER_VTT_INTERPROCESS_DETAILS_PIPE

#pragma once

#include "Chunk.hpp"
#include "Fixed Buffer.hpp"

#include <sal.h>

#include <string>

#include <boost/thread.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	//	Provides FIFO pipe behavior using managed shared memory buffer.
	template<::boost::uint32_t tp_Capacity>
	class t_Pipe
	{
		protected: typedef ::boost::interprocess::named_mutex t_Mutex;

		protected: typedef ::boost::interprocess::scoped_lock<t_Mutex> t_Lock;

		protected: typedef ::boost::interprocess::interprocess_condition t_Condition;

		protected: typedef t_FixedBuffer<tp_Capacity> t_Buffer;

		#pragma region Fields

		protected: t_Mutex    m_mutex;
		protected: t_Buffer * m_p_buffer = nullptr; 

		#pragma endregion

		public: t_Pipe
		(
			::std::string const &                          name
		,	::boost::interprocess::managed_shared_memory & memory
		)
		:	m_mutex(::boost::interprocess::open_or_create, name.c_str())
		{
			t_Lock lock(m_mutex);
			m_p_buffer = memory.find<t_Buffer>(name.c_str()).first;
			if(nullptr == m_p_buffer)
			{
				m_p_buffer = memory.construct<t_Buffer>(name.c_str())();
			}
		}

		private: t_Pipe(t_Pipe const &) = delete;

		public: ~t_Pipe(void)
		{
			//	Do nothing
		}

		private: void operator = (t_Pipe const &) = delete;

		public: auto Read(void) -> t_Chunk
		{
			t_Lock lock(m_mutex);
			while(m_p_buffer->Is_Empty())
			{
				t_Condition cond;
				cond.timed_wait(lock, ::boost::get_system_time() + ::boost::posix_time::milliseconds(100));
				::boost::this_thread::interruption_point();
			}
			return(m_p_buffer->Retrieve_Chunk());
		}

		public: void Write(t_Chunk chunk)
		{
			t_Lock lock(m_mutex);
			m_p_buffer->Store_Chunk(chunk);
			t_Condition cond;
			cond.notify_one();
		}
	};
}
}
}

#endif