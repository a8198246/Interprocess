#ifndef HEADER_VTT_INTERPROCESS_SHARED_MEMORY
#define HEADER_VTT_INTERPROCESS_SHARED_MEMORY

#pragma once

#include "Named Mutex.hpp"
#include "Scoped Lock.hpp"
#include "Owned Handle.hpp"

#include <sal.h>

#include <Windows.h>

#include <string>
#include <stdexcept>
#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_SharedMemoty
	:	public t_OwnedHandle
	{
		#pragma region Fields

		protected: void * m_p_memory = nullptr;
	#ifdef _DEBUG
		protected: const size_t m_bc_memory;
	#endif

		#pragma endregion

		private: t_SharedMemoty(void) = delete;

		public: t_SharedMemoty(_Inout_ ::std::string & name, _In_ t_NamedMutex & sync, _In_ const size_t bc_capacity)
	#ifdef _DEBUG
		:	m_bc_memory(bc_capacity)
	#endif
		{
			name.push_back('b'); // name must differ from named mutex / event names
			t_ScopedLock lock(sync);
			m_handle = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
			if(NULL == m_handle)
			{
				m_handle = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast<::DWORD>(bc_capacity), name.c_str());
				if(NULL == m_handle)
				{
					auto last_error = ::GetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create named file mapping"));
				}
			}
			m_p_memory = ::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, bc_capacity);
			if(nullptr == m_p_memory)
			{
				auto last_error = ::GetLastError();
				throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to map view of the named file mapping"));
			}
			name.pop_back();
		}

		private: t_SharedMemoty(t_SharedMemoty const &) = delete;

		private: void operator =(t_SharedMemoty const &) = delete;

		public: ~t_SharedMemoty(void)
		{
			if(Is_Initialized())
			{
				assert(nullptr != m_p_memory);
				auto unmapped = ::UnmapViewOfFile(m_p_memory);
				DBG_UNREFERENCED_LOCAL_VARIABLE(unmapped);
				assert(FALSE != unmapped);
			}
		}

		public: template<typename tp_Object>
		auto Obtain(void) throw() -> tp_Object &
		{
			assert(Is_Initialized());
			assert(nullptr != m_p_memory);
			assert(sizeof(tp_Object) == m_bc_memory);
			return(*static_cast<tp_Object *>(m_p_memory));
		}

		public: template<typename tp_Object>
		auto Obtain(void) const throw() -> tp_Object const &
		{
			assert(Is_Initialized());
			assert(nullptr != m_p_memory);
			assert(sizeof(tp_Object) == m_bc_memory);
			return(*static_cast<tp_Object const *>(m_p_memory));
		}
	};
}
}
}

#endif