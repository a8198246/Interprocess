#ifndef HEADER_VTT_INTERPROCESS_DETAILS_NAMED_MUTEX
#define HEADER_VTT_INTERPROCESS_DETAILS_NAMED_MUTEX

#pragma once

#include "Owned Handle.hpp"

#include <sal.h>

#include <string>
#include <cassert>
#include <system_error>

#include <Windows.h>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	//	Replacement for broken boost::interprocess::named_mutex
	class t_NamedMutex
	:	public t_OwnedHandle
	{
		friend class t_ConditionalVariable;

		public: t_NamedMutex(void) = delete;

		public: t_NamedMutex(_Inout_ ::std::string & name)
		{
			assert(!name.empty());
			name.push_back('m'); // name must differ from event / mappings names
			auto first_try = true;
			for(;;)
			{
				m_handle = ::CreateMutexA(NULL, FALSE, name.c_str());
				if(NULL != m_handle)
				{
					break;
				}
				if(first_try)
				{
					first_try = false;
				}
				else
				{
					auto last_error = ::GetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create interprocess named mutex: UNEXPECTED"));
				}
			}
			name.pop_back();
		}

		public: t_NamedMutex(t_NamedMutex const &) = delete;

		public: void operator=(t_NamedMutex const &) = delete;

		//	returns true if lock was acquired at the specified preriod of time
		//	returns false if a timeout occurred and lock was not acquired
		public: auto Timed_Lock(_In_ const int timeout_msec) -> bool
		{
			assert(Is_Initialized());
			auto result = ::WaitForSingleObject(m_handle, timeout_msec);
			switch(result)
			{
				case WAIT_ABANDONED:
				case WAIT_OBJECT_0:
				{
					return(true);
				}
				case WAIT_TIMEOUT:
				{
					return(false);
				}
				default:
				{
					throw(::std::system_error(static_cast<int>(result), ::std::system_category(), "failed to lock interprocess named mutex: UNEXPECTED"));
				}
			}
		}

		public: void Lock(void)
		{
			auto locked = Timed_Lock(INFINITE);
			DBG_UNREFERENCED_LOCAL_VARIABLE(locked);
			assert(locked);
		}

		public: void Unlock(void) throw()
		{
			assert(Is_Initialized());
			auto released = ::ReleaseMutex(m_handle);
			DBG_UNREFERENCED_LOCAL_VARIABLE(released);
			assert(FALSE != released);
		}
	};
}
}
}

#endif