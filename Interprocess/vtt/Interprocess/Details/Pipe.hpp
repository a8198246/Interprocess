#ifndef HEADER_VTT_INTERPROCESS_DETAILS_PIPE
#define HEADER_VTT_INTERPROCESS_DETAILS_PIPE

#pragma once

#include "Chunk.hpp"
#include "Fixed Buffer.hpp"
#include "Named Mutex.hpp"
#include "Event.hpp"
#include "Scoped Lock.hpp"
#include "Conditional Variable.hpp"
#include "Shared Memory.hpp"

#include <sal.h>

#include <string>
#include <utility>

#include <boost/thread.hpp>

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
		protected: typedef t_FixedBuffer<tp_Capacity> t_Buffer;

		#pragma region Fields

		protected: t_NamedMutex   m_sync;
		protected: t_Event        m_flag;
		protected: t_SharedMemoty m_shared_memory;
		
		#pragma endregion

		private: t_Pipe(void) = delete;

		public: explicit t_Pipe(_In_ ::std::string name)
		:	m_sync(name)
		,	m_flag(name)
		,	m_shared_memory(name, m_sync, sizeof(t_Buffer))
		{
			//	Do nothing
		}

		private: t_Pipe(t_Pipe const &) = delete;

		private: void operator=(t_Pipe const &) = delete;

		public: auto Is_Empty(void) const throw() -> bool
		{
			return(m_shared_memory.Obtain<t_Buffer>().Is_Empty());
		}

		public: auto Is_Not_Empty(void) const throw() -> bool
		{
			return(m_shared_memory.Obtain<t_Buffer>().Is_Not_Empty());
		}

		public: auto Read(_In_ const int timeout_msec) -> t_Chunk
		{
			t_ScopedLock lock(m_sync);
			if(Is_Empty())
			{
				t_ConditionalVariable::Timed_Wait(lock, m_flag, timeout_msec);
			}
			if(Is_Empty())
			{
				return(t_Chunk());
			}
			else
			{
				return(m_shared_memory.Obtain<t_Buffer>().Retrieve_Chunk());
			}
		}

		public: auto Read(void) -> t_Chunk
		{
			t_ScopedLock lock(m_sync);
			while(Is_Empty())
			{
				t_ConditionalVariable::Timed_Wait(lock, m_flag, 100);
				::boost::this_thread::interruption_point();
			}
			return(m_shared_memory.Obtain<t_Buffer>().Retrieve_Chunk());
		}

		public: void Write(_In_ t_Chunk chunk)
		{
			t_ScopedLock lock(m_sync);
			m_shared_memory.Obtain<t_Buffer>().Store_Chunk(chunk);
			m_flag.Set();
		}
	};
}
}
}

#endif