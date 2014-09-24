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
#include <boost/scope_exit.hpp>

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

		protected: typedef t_MultiuserBuffer<tp_Capacity> t_Buffer;
		
		private: t_SingleWriterMultiReaderPipe(void) = delete;

		public: explicit t_SingleWriterMultiReaderPipe(_In_ ::std::string name)
		:	t_Pipe(::std::move(name), sizeof(t_Buffer), true)
		{
			//	Do nothing
		}

		private: t_SingleWriterMultiReaderPipe(t_SingleWriterMultiReaderPipe const &) = delete;

		private: void operator =(t_SingleWriterMultiReaderPipe const &) = delete;
		
		public: auto Read(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity, _In_ const int timeout_msec) -> size_t
		{
			auto & buffer = m_shared_memory.Obtain<t_Buffer>();
			volatile const long initial_magic = buffer.m_magic;
			auto p_ec_readers = &(buffer.m_ec_readers);
			// variables used in atomic operations must be aligned by 32 bits (4 bytes that is)
			assert(0 == (reinterpret_cast<ptrdiff_t>(p_ec_readers) % 4));
			InterlockedIncrement(p_ec_readers);
			BOOST_SCOPE_EXIT(p_ec_readers, this_)
			{
				assert(0 < *p_ec_readers);
				auto ec_readers = InterlockedCompareExchange(p_ec_readers, 0, 1);
				if(1 == ec_readers)
				{
					this_->m_flag.Reset(); // last reader resets event
				}
				else if(1 < ec_readers)
				{
					InterlockedDecrement(p_ec_readers);
				}
			}
			BOOST_SCOPE_EXIT_END;
			auto bc_read = static_cast<size_t>(0);
			m_flag.Timed_Wait(timeout_msec);
			//auto const wait_result = m_flag.Timed_Wait(timeout_msec);
			//if(false != wait_result)
			{
				volatile const long new_magic = buffer.m_magic;
				if(new_magic != initial_magic)
				{
					bc_read = buffer.Retrieve_Data(p_buffer, bc_buffer_capacity);
				}
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