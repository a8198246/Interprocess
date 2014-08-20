#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_FIXED_BUFFER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_FIXED_BUFFER

#pragma once

#include "Chunk.hpp"

#include <sal.h>

#include <array>
#include <memory.h>

#include <boost/cstdint.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	//	Ready to be a process-shared object.
	template<::boost::uint32_t tp_Capacity>
	class t_FixedBuffer
	{
		protected: typedef ::std::array<char, tp_Capacity> t_buffer;

		#pragma region Fields

		protected: t_buffer                   m_buffer;
		protected: volatile ::boost::uint32_t m_size;
		
		#pragma	endregion
		
		public: auto Is_Empty(void) const throw() -> bool
		{
			return(0 == m_size);
		}

		public: auto Is_Not_Empty(void) const throw() -> bool
		{
			return(0 != m_size);
		}

		public: auto Get_Size(void) const throw() -> size_t
		{
			return(m_size);
		}

		public: auto Get_Capacity(void) const throw() -> size_t
		{
			return(tp_Capacity);
		}

		public: void Clear(void)
		{
			m_size = 0;
		}

		public: void Store(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			assert(bc_data <= tp_Capacity);
			auto const new_size = m_size + bc_data;
			if(new_size <= tp_Capacity)
			{
				memcpy(m_buffer.data() + m_size, p_data, bc_data);
				m_size = static_cast<::boost::uint32_t>(new_size);
			}
		}

		public: void Store(_In_ t_Chunk chunk)
		{
			assert(chunk.Is_Not_Empty());
			auto const new_size = m_size + chunk.Get_Size();
			if(new_size <= tp_Capacity)
			{
				memcpy(m_buffer.data() + m_size, chunk.Get_Data(), chunk.Get_Size());
				m_size = static_cast<::boost::uint32_t>(new_size);
			}
		}

		public: auto Retrieve_Data(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const size_t bc_buffer_capacity) -> size_t
		{
			assert(nullptr != p_buffer);
			assert(0 < bc_buffer_capacity);
			assert(m_size <= bc_buffer_capacity);
			size_t bc_written = 0;
			if(m_size <= bc_buffer_capacity)
			{
				memcpy(p_buffer, m_buffer.data(), m_size);
				bc_written = m_size;
			}
			return(bc_written);
		}

		public: auto Retrieve_Chunk(void) -> t_Chunk
		{
			assert(Is_Not_Empty());
			t_Chunk result(m_buffer.data(), m_size);
			m_size = 0;
			return(result);
		}
	};
}
}
}

#endif