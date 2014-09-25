#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SINGLE_WRITER_MULTI_READER_PIPE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SINGLE_WRITER_MULTI_READER_PIPE

#pragma once

#include "Pipe.hpp"
#include "Fixed Buffer.hpp"
#include "Conditional Variable.hpp"

#include <sal.h>

#include <string>
#include <utility>

#include <boost/cstdint.hpp>
#include <boost/thread.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	template<::boost::uint32_t tp_Capacity>
	class t_SingleWriterMultiReaderPipe
	:	public t_Pipe
	{
		protected:
		template<::boost::uint32_t tp_Capacity>
		struct t_MultiuserBuffer
		:	public t_FixedBuffer<tp_Capacity>
		{
			#pragma region Fields

			volatile long m_ec_readers;
			volatile long m_magic;

			#pragma endregion
		};

		protected: template<typename tp_Callable>
		class t_AtScopeExitExecutor
		{
			#pragma region Fields

			private: tp_Callable m_callable;

			#pragma endregion

			private: t_AtScopeExitExecutor(void) = delete;

			private: t_AtScopeExitExecutor(t_AtScopeExitExecutor const &) = delete;

			public: explicit t_AtScopeExitExecutor(tp_Callable callable)
			:	m_callable(callable)
			{
				//	Do nothing...
			}

			public: ~t_AtScopeExitExecutor(void)
			{
				m_callable();
			}

			private: void operator =(t_AtScopeExitExecutor const &) = delete;
		};

		protected: typedef t_MultiuserBuffer<tp_Capacity> t_Buffer;
				
		private: t_SingleWriterMultiReaderPipe(void) = delete;

		public: explicit t_SingleWriterMultiReaderPipe(_In_ ::std::string name)
		:	t_Pipe(name, sizeof(t_Buffer), true)
		{
			//	Do nothing
		}

		private: t_SingleWriterMultiReaderPipe(t_SingleWriterMultiReaderPipe const &) = delete;

		private: void operator =(t_SingleWriterMultiReaderPipe const &) = delete;
		
		public: auto Read(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
			auto & buffer = m_shared_memory.Obtain<t_Buffer>();
			volatile const long initial_magic = buffer.m_magic;
			{
				t_ScopedLock lock(m_sync);
				if(0 == buffer.m_ec_readers)
				{
					m_flag.Reset(); // first reader resets event in case if writer already had written something
				}
				++buffer.m_ec_readers;
			}
			auto ScopeExitActions = [&buffer, this](void)
			{
				t_ScopedLock lock(m_sync);
				--(buffer.m_ec_readers);
				if(0 == buffer.m_ec_readers)
				{
					m_flag.Reset(); // last reader resets event
				}
			};
			t_AtScopeExitExecutor<decltype(ScopeExitActions)> at_scope_exit(ScopeExitActions);
			m_flag.Timed_Wait(timeout_msec);
			volatile const long new_magic = buffer.m_magic;
			auto bc_read = static_cast<size_t>(0);
			if(new_magic != initial_magic)
			{
				bc_read = buffer.Retrieve_Data(p_buffer, bc_buffer_capacity);
			}
			return(bc_read);
		}

		public: void Write(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			//	assuming all the read operations were finished by this time
			m_flag.Reset(); // resetting event just in case if no readers were present last time
			auto & buffer = m_shared_memory.Obtain<t_Buffer>();
			buffer.Clear();
			buffer.Store(p_data, bc_data);
			// variables used in atomic operations must be aligned by 32 bits (4 bytes that is)
			assert(0 == (reinterpret_cast<ptrdiff_t>(&buffer.m_magic) % 4));
			InterlockedIncrement(&buffer.m_magic);
			m_flag.Set();
		}
	};
}
}
}

#endif