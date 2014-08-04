#ifndef HEADER_VTT_INTERPROCESS_DETAILS_WRITE_CHUNK_TO_BUFFER
#define HEADER_VTT_INTERPROCESS_DETAILS_WRITE_CHUNK_TO_BUFFER

#pragma once

#include "Chunk.hpp"

#include <sal.h>

#include <memory.h>
#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	inline void Write_Chunk_To_Buffer(_In_ t_Chunk recieved, char * p_buffer, _In_ const size_t bc_buffer_capacity, _Inout_ size_t & bc_written, _Inout_ t_Chunk & pending)
	{
		assert(pending.Is_Empty());
		assert(nullptr != p_buffer);
		assert(0 < bc_buffer_capacity);
		if(recieved.Is_Not_Empty())
		{
			auto bc_to_write = recieved.Get_Size();
			if(bc_buffer_capacity < bc_written + bc_to_write)
			{
				bc_to_write = bc_buffer_capacity - bc_written;
			}
			assert(bc_written + bc_to_write <= bc_buffer_capacity);
			memcpy(p_buffer + bc_written, recieved.Get_Data(), bc_to_write);
			bc_written += bc_to_write;
			if(bc_to_write < recieved.Get_Size())
			{
				assert(bc_written == bc_buffer_capacity);
				pending = t_Chunk(recieved.Get_Data() + bc_to_write, recieved.Get_Size() - bc_to_write);
			}
		}
		assert(bc_written <= bc_buffer_capacity);
	}
}
}
}

#endif