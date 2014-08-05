#ifndef HEADER_VTT_INTERPROCESS_DETAILS_NAMED_MUTEX
#define HEADER_VTT_INTERPROCESS_DETAILS_NAMED_MUTEX

#pragma once

#include "Owned Handle.hpp"

#include <sal.h>

#include <string>
#include <cassert>
#include <utility>
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

		#pragma region Fields

		private: ::std::string m_name;

		#pragma endregion

		public: t_NamedMutex(void) = delete;

		public: t_NamedMutex(_In_ ::std::string && name)
		:	m_name(::std::move(name))
		{
			assert(!m_name.empty());
			auto first_try = true;
			for(;;)
			{
				::SetLastError(ERROR_SUCCESS);
				m_handle = ::CreateMutexA(NULL, FALSE, m_name.c_str());
				auto last_error = ::GetLastError();
				if(Is_Initialized())
				{
					break;
				}
				else if (ERROR_ACCESS_DENIED == last_error)
				{
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create interprocess named mutex: ERROR_ACCESS_DENIED"));
				}
				else if(first_try)
				{
					first_try = false;
				}
				else
				{
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create interprocess named mutex: UNEXPECTED"));
				}
			}
		}

		public: t_NamedMutex(t_NamedMutex const &) = delete;

		public: void operator = (t_NamedMutex const &) = delete;

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
				case WAIT_FAILED:
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

		public: auto Get_Name(void) const throw() -> ::std::string const &
		{
			return(m_name);
		}
	};
}
}
}

#endif