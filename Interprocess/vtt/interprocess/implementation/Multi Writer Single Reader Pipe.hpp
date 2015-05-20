#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MULTI_WRITER_SINGLE_READER_PIPE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MULTI_WRITER_SINGLE_READER_PIPE

#pragma once

#include "Pipe.hpp"
#include "Conditional Variable.hpp"
#include "Scoped Lock.hpp"
#include "Fixed Buffer.hpp"
#include "Chunk.hpp"
#include "Threaded Logger.hpp"

#include <sal.h>

#include <string>
#include <utility>

#include <boost/thread/thread.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	template<::boost::uint32_t tp_Capacity> class
	t_MultiWriterSingleReaderPipe
	:	public t_Pipe
	{
		protected: typedef t_FixedBuffer<tp_Capacity>
		t_Buffer;
		
		private:
		t_MultiWriterSingleReaderPipe(void) = delete;

		private:
		t_MultiWriterSingleReaderPipe(t_MultiWriterSingleReaderPipe const &) = delete;

		private:
		t_MultiWriterSingleReaderPipe(t_MultiWriterSingleReaderPipe &&) = delete;

		public: explicit
		t_MultiWriterSingleReaderPipe(_In_ ::std::string name)
		:	t_Pipe(::std::move(name), sizeof(t_Buffer), false)
		{
			//	Do nothing
		}

		private: void
		operator =(t_MultiWriterSingleReaderPipe const &) = delete;

		private: void
		operator =(t_MultiWriterSingleReaderPipe &&) = delete;

		public: auto
		Is_Empty(void) const throw() -> bool
		{
			return(m_shared_memory.Obtain<t_Buffer>().Is_Empty());
		}

		public: auto
		Is_Not_Empty(void) const throw() -> bool
		{
			return(m_shared_memory.Obtain<t_Buffer>().Is_Not_Empty());
		}

		public: auto
		Read(_In_ const int timeout_msec) -> t_Chunk
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

		public: auto
		Read(void) -> t_Chunk
		{
			t_ScopedLock lock(m_sync);
			while(Is_Empty())
			{
				t_ConditionalVariable::Timed_Wait(lock, m_flag, 100);
				::boost::this_thread::interruption_point();
			}
			return(m_shared_memory.Obtain<t_Buffer>().Retrieve_Chunk());
		}

		public: void
		Write(_In_ t_Chunk chunk)
		{
			t_ScopedLock lock(m_sync);
			m_shared_memory.Obtain<t_Buffer>().Store(chunk);
			m_flag.Set();
		}
	};
}
}
}

#endif
