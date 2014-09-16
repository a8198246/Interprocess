#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_FIXED_BUFFER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_FIXED_BUFFER

#pragma once

#include "Chunk.hpp"
#include "Threaded Logger.hpp"

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
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(chunk.Is_Not_Empty());
			auto const new_size = m_size + chunk.Get_Size();
			if(new_size <= tp_Capacity)
			{
				memcpy(m_buffer.data() + m_size, chunk.Get_Data(), chunk.Get_Size());
				{
				#ifdef _DEBUG_LOGGING
					auto & logger = t_ThreadedLogger::Get_Instance();
					t_LoggerGuard logger_guard(logger);
					logger.Print_Prefix() << "stored " << chunk.Get_Size() << " bytes"
						<< ", initial buffer size " << m_size << " bytes"
						<< ", new buffer size " << new_size << " bytes"
						<< ", buffer capacity " << Get_Capacity() << " bytes"
						<< ::std::endl;
					logger.Write_Block(chunk.Get_Data(), chunk.Get_Size());
				#endif
				}
				m_size = static_cast<::boost::uint32_t>(new_size);
			}
			else
			{
			#ifdef _DEBUG_LOGGING
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard logger_guard(logger);
				logger.Print_Prefix() << "unable to store " << chunk.Get_Size() << " bytes"
					<< ", initial buffer size " << m_size << " bytes"
					<< ", buffer capacity " << Get_Capacity() << " bytes"
					<< ::std::endl;
			#endif
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
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			assert(Is_Not_Empty());
			t_Chunk result(m_buffer.data(), m_size);
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard logger_guard(logger);
				logger.Print_Prefix() << "retrieved " << m_size << " bytes"
					<< ", buffer capacity " << Get_Capacity() << " bytes"
					<< ::std::endl;
				logger.Write_Block(result.Get_Data(), result.Get_Size());
			}
		#endif
			m_size = 0;
			return(result);
		}
	};
}
}
}

#endif