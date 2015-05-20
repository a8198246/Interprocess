#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SINGLE_WRITER_MULTI_READER_PIPE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SINGLE_WRITER_MULTI_READER_PIPE

#pragma once

#include "Pipe.hpp"
#include "Fixed Buffer.hpp"
#include "Conditional Variable.hpp"

#include <sal.h>

#include <string>
#include <utility>
#include <cstdint>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	template<::std::uint32_t tp_Capacity> class
	t_SingleWriterMultiReaderPipe
	:	public t_Pipe
	{
		protected: template<::std::uint32_t tp_Capacity> struct
		t_MultiuserBuffer
		:	public t_FixedBuffer<tp_Capacity>
		{
			#pragma region Fields

			volatile long m_magic;

			#pragma endregion
		};

		protected: template<typename tp_Callable> class
		t_AtScopeExitExecutor
		{
			#pragma region Fields

			private: tp_Callable m_callable;

			#pragma endregion

			private:
			t_AtScopeExitExecutor(void) = delete;

			private:
			t_AtScopeExitExecutor(t_AtScopeExitExecutor const &) = delete;

			private:
			t_AtScopeExitExecutor(t_AtScopeExitExecutor &&) = delete;

			public: explicit
			t_AtScopeExitExecutor(tp_Callable callable)
			:	m_callable(callable)
			{
				//	Do nothing...
			}

			public:
			~t_AtScopeExitExecutor(void)
			{
				m_callable();
			}

			private: void
			operator =(t_AtScopeExitExecutor const &) = delete;

			private: void
			operator =(t_AtScopeExitExecutor &&) = delete;
		};

		protected: typedef t_MultiuserBuffer<tp_Capacity>
		t_Buffer;

		#pragma region Fields

		protected: bool m_event_fired_during_last_read = false;

		#pragma endregion
				
		private:
		t_SingleWriterMultiReaderPipe(void) = delete;

		private:
		t_SingleWriterMultiReaderPipe(t_SingleWriterMultiReaderPipe const &) = delete;

		private:
		t_SingleWriterMultiReaderPipe(t_SingleWriterMultiReaderPipe &&) = delete;

		public: explicit
		t_SingleWriterMultiReaderPipe(_In_ ::std::string name)
		:	t_Pipe(name, sizeof(t_Buffer), true)
		{
			//	Do nothing
		}

		private: void
		operator =(t_SingleWriterMultiReaderPipe const &) = delete;

		private: void
		operator =(t_SingleWriterMultiReaderPipe &&) = delete;
		
		public: auto
		Read(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
			auto & buffer = m_shared_memory.Obtain<t_Buffer>();
			if(m_event_fired_during_last_read)
			{
				m_event_fired_during_last_read = false;
				::Sleep(1000);
			}
			volatile const long initial_magic = buffer.m_magic;
			auto wait_result = m_flag.Timed_Wait(timeout_msec);
			volatile const long new_magic = buffer.m_magic;
			auto bc_read = static_cast<size_t>(0);
			if(new_magic != initial_magic)
			{
				bc_read = buffer.Retrieve_Data(p_buffer, bc_buffer_capacity);
				assert(0 < bc_read);
				m_event_fired_during_last_read = true;
			}
			if(wait_result)
			{
				m_event_fired_during_last_read = true;
			}
			return(bc_read);
		}

		public: void
		Write(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			auto & buffer = m_shared_memory.Obtain<t_Buffer>();
			buffer.Clear();
			buffer.Store(p_data, bc_data);
			// variables used in atomic operations must be aligned by 32 bits (4 bytes that is)
			assert(0 == (reinterpret_cast<ptrdiff_t>(&buffer.m_magic) % 4));
			InterlockedIncrement(&buffer.m_magic);
			m_flag.Set();
			::Sleep(500);
			m_flag.Reset();
		}
	};
}
}
}

#endif
