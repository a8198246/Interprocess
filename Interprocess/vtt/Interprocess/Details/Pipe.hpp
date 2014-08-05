#ifndef HEADER_VTT_INTERPROCESS_DETAILS_PIPE
#define HEADER_VTT_INTERPROCESS_DETAILS_PIPE

#pragma once

#include "Chunk.hpp"
#include "Fixed Buffer.hpp"
#include "Named Mutex.hpp"
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

		protected: t_NamedMutex   m_mutex;
		protected: t_SharedMemoty m_shared;
		protected: t_Buffer *     m_p_buffer = nullptr; 

		#pragma endregion

		public: explicit t_Pipe(_Inout_ ::std::string && name)
		:	m_mutex(::std::move(name))
		,	m_shared(m_mutex, sizeof(t_Buffer))
		,	m_p_buffer(m_shared.Construct<t_Buffer>())
		{
			//	Do nothing
		}

		private: t_Pipe(t_Pipe const &) = delete;

		private: void operator = (t_Pipe const &) = delete;

		public: auto Read(void) -> t_Chunk
		{
			t_ScopedLock lock(m_mutex);
			while(m_p_buffer->Is_Empty())
			{
				t_ConditionalVariable cond;
				cond.Timed_Wait(lock, 100);
				::boost::this_thread::interruption_point();
			}
			return(m_p_buffer->Retrieve_Chunk());
		}

		public: void Write(_In_ t_Chunk chunk)
		{
			t_ScopedLock lock(m_mutex);
			m_p_buffer->Store_Chunk(chunk);
			t_ConditionalVariable cond;
			cond.Notify(lock);
		}

		public: auto Get_Name(void) const throw() -> ::std::string const &
		{
			return(m_mutex.Get_Name());
		}
	};
}
}
}

#endif