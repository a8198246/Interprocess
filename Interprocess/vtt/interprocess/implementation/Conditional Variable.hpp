#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_CONDITIONAL_VARIABLE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_CONDITIONAL_VARIABLE

#pragma once

#include "Scoped Lock.hpp"
#include "Event.hpp"

#include <sal.h>

#include <Windows.h>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_ConditionalVariable
	{
		private:
		t_ConditionalVariable(void) = delete;

		private:
		t_ConditionalVariable(t_ConditionalVariable const &) = delete;

		private:
		t_ConditionalVariable(t_ConditionalVariable &&) = delete;

		private:
		~t_ConditionalVariable(void) = delete;

		private: void
		operator =(t_ConditionalVariable const &) = delete;

		private: void
		operator =(t_ConditionalVariable &&) = delete;

		public: static auto
		Timed_Wait(_Inout_ t_ScopedLock & lock, _In_ t_Event & event, _In_ const int timeout_msec) -> bool
		{
			lock.Unlock();
			auto wait_result = event.Timed_Wait(timeout_msec);
			lock.Lock();
			return(wait_result);
		}

		public: static void
		Wait(_Inout_ t_ScopedLock & lock, _In_ t_Event & event)
		{
			Timed_Wait(lock, event, INFINITE);
		}
	};
}
}
}

#endif
