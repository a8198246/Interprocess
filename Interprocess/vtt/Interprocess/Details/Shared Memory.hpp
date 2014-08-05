#ifndef HEADER_VTT_INTERPROCESS_SHARED_MEMORY
#define HEADER_VTT_INTERPROCESS_SHARED_MEMORY

#pragma once

#include "Named Mutex.hpp"
#include "Scoped Lock.hpp"
#include "Owned Handle.hpp"

#include <sal.h>

#include <string>
#include <stdexcept>
#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_SharedMemoty
	:	public t_OwnedHandle
	{
		#pragma region Fields

		protected: void * m_p_memory = nullptr;

		#pragma endregion

		private: t_SharedMemoty(void) = delete;

		public: t_SharedMemoty(_In_ t_NamedMutex & sync, _In_ const size_t bc_capacity)
		{
			auto name = sync.Get_Name() + "f"; // name must differ from mutex name
			t_ScopedLock lock(sync);
			m_handle = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
			if(Is_Not_Initialized())
			{
				m_handle = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast<::DWORD>(bc_capacity), name.c_str());
				if(Is_Not_Initialized())
				{
					auto last_error = ::GetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create named file mapping: ERROR_UNEXPECTED"));
				}
			}
			m_p_memory = ::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, bc_capacity);
			if(nullptr == m_p_memory)
			{
				auto last_error = ::GetLastError();
				::CloseHandle(m_handle);
				m_handle = 0;
				throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to map view of named file mapping: ERROR_UNEXPECTED"));
			}
		}

		private: t_SharedMemoty(t_SharedMemoty const &) = delete;

		private: void operator = (t_SharedMemoty const &) = delete;

		public: ~t_SharedMemoty(void)
		{
			if(Is_Initialized())
			{
				auto unmapped = ::UnmapViewOfFile(m_p_memory);
				DBG_UNREFERENCED_LOCAL_VARIABLE(unmapped);
				assert(FALSE != unmapped);
			}
		}

		public: template<typename tp_Object>
		auto Construct(void) -> tp_Object *
		{
			return(new (m_p_memory) tp_Object());
		}
	};
}
}
}

#endif