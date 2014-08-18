#ifndef HEADER_VTT_INTERPROCESS_DETAILS_CONDITIONAL_VARIABLE
#define HEADER_VTT_INTERPROCESS_DETAILS_CONDITIONAL_VARIABLE

#pragma once

#include "Scoped Lock.hpp"
#include "Event.hpp"

#include <sal.h>

#include <Windows.h>

#include <system_error>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_ConditionalVariable
	{
		public: t_ConditionalVariable(void) = delete;

		public: ~t_ConditionalVariable(void) = delete;

		public: t_ConditionalVariable(t_ConditionalVariable const &) = delete;

		public: void operator=(t_ConditionalVariable const &) = delete;

		public: static auto Timed_Wait(_Inout_ t_ScopedLock & lock, _In_ t_Event & event, _In_ const int timeout_msec) -> bool
		{
			assert(lock.m_locked);
			lock.m_mutex.Unlock();
			lock.m_locked = false;
			event.Reset();
			auto wait_result = ::WaitForSingleObject(event.m_handle, timeout_msec);
			switch(wait_result)
			{
				case WAIT_ABANDONED:
				case WAIT_OBJECT_0:
				case WAIT_TIMEOUT:
				{
					lock.m_locked = true;
					lock.m_mutex.Lock();
					return(WAIT_TIMEOUT != wait_result);
				}
				case WAIT_FAILED:
				default:
				{
					auto last_error = ::GetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to wait for the event"));
				}
			}
		}

		public: static void Wait(_Inout_ t_ScopedLock & lock, _In_ t_Event & event)
		{
			Timed_Wait(lock, event, INFINITE);
		}
	};
}
}
}

#endif