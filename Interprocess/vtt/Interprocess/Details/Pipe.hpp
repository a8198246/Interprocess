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
			m_shared_memory.Obtain<t_Buffer>().Store(chunk);
			m_flag.Set();
		}
	};

	template<::boost::uint32_t tp_Capacity>
	class t_MultiReaderPipe
	{
		protected:
		template<::boost::uint32_t tp_Capacity>
		struct t_MultiuserBuffer
		:	public t_FixedBuffer<tp_Capacity>
		{
			#pragma region Fields

			volatile long m_ec_readers;

			#pragma endregion
		};

		protected: typedef t_MultiuserBuffer<tp_Capacity> t_Buffer;

		#pragma region Fields
		
		protected: t_NamedMutex   m_sync;
		protected: t_Event        m_flag;
		protected: t_SharedMemoty m_shared_memory;
		
		#pragma endregion

		private: t_MultiReaderPipe(void) = delete;

		public: explicit t_MultiReaderPipe(_In_ ::std::string name)
		:	m_sync(name)
		,	m_flag(name, true)
		,	m_shared_memory(name, m_sync, sizeof(t_Buffer))
		{
			//	Do nothing
		}

		private: t_MultiReaderPipe(t_MultiReaderPipe const &) = delete;

		private: void operator=(t_MultiReaderPipe const &) = delete;
		
		public: auto Read(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
			auto p_ec_readers = &(m_shared_memory.Obtain<t_Buffer>().m_ec_readers);
			assert(0 == (reinterpret_cast<ptrdiff_t>(p_ec_readers) % 4)); // variables used in atomic operations must be aligned by 32 bits (4 bytes that is)

			InterlockedIncrement(p_ec_readers);

			size_t bc_read = 0;
			if(m_flag.Timed_Wait(timeout_msec))
			{
				bc_read = m_shared_memory.Obtain<t_Buffer>().Retrieve_Data(p_buffer, bc_buffer_capacity);
			}
			assert(0 < *p_ec_readers);
			auto ec_readers = InterlockedCompareExchange(p_ec_readers, 0, 1);
			auto this_was_not_last_reader = (1 != ec_readers);
			if(this_was_not_last_reader)
			{
				InterlockedDecrement(p_ec_readers);
			}
			else
			{
				m_flag.Reset(); // last reader resets event
			}
			return(bc_read);
		}

		public: void Write(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			//	assuming all the read operations were finished by this time
			m_flag.Reset(); // resetting event just in case if no readers were present last time
			m_shared_memory.Obtain<t_Buffer>().Clear();
			m_shared_memory.Obtain<t_Buffer>().Store(p_data, bc_data);
			m_flag.Set();
		}
	};
}
}
}

#endif