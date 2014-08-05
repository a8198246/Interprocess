#ifndef HEADER_VTT_INTERPROCESS_DETAILS_CONDITIONAL_VARIABLE
#define HEADER_VTT_INTERPROCESS_DETAILS_CONDITIONAL_VARIABLE

#pragma once

#include "Scoped Lock.hpp"
#include "Event.hpp"

#include <sal.h>

#include <string>
#include <stdexcept>

#include <Windows.h>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_ConditionalVariable
	{
		public: t_ConditionalVariable(void) throw()
		{
			//	Do nothing
		}

		public: t_ConditionalVariable(t_ConditionalVariable const &) = delete;

		public: void operator = (t_ConditionalVariable const &) = delete;

		public: void Timed_Wait(_Inout_ t_ScopedLock & lock, _In_ const int timeout_msec)
		{
			assert(lock.m_locked);
			t_Event event(lock.m_mutex.m_name);
			::SetLastError(ERROR_SUCCESS);
			lock.m_mutex.Unlock();
			lock.m_locked = false;
			auto result = ::WaitForSingleObject(event.m_handle, timeout_msec);
			switch(result)
			{
				case WAIT_ABANDONED:
				case WAIT_OBJECT_0:
				case WAIT_TIMEOUT:
				{
					lock.m_locked = true;
					lock.m_mutex.Lock();
					return;
				}
				case WAIT_FAILED:
				default:
				{
					auto last_error = ::GetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to wait for event: UNEXPECTED"));
				}
			}
		}

		public: void Wait(_Inout_ t_ScopedLock & lock)
		{
			Timed_Wait(lock, INFINITE);
		}

		public: void Notify(_Inout_ t_ScopedLock & lock)
		{
			t_Event event(lock.m_mutex.m_name);
			event.Set();
		}
	};
}
}
}

#endif