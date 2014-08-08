#ifndef HEADER_VTT_INTERPROCESS_DETAILS_EVENT
#define HEADER_VTT_INTERPROCESS_DETAILS_EVENT

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
	class t_Event
	:	public t_OwnedHandle
	{
		friend class t_ConditionalVariable;

		public: t_Event(void) = delete;

		public: explicit t_Event(_Inout_ ::std::string & name)
		{
			name.push_back('e'); // event name must differ from named mutex / file mappings name
			auto first_try = true;
			for(;;)
			{
				m_handle = ::CreateEventA(NULL, FALSE, FALSE, name.c_str());
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
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create interprocess named event"));
				}
			}
			name.pop_back();
		}

		public: t_Event(t_Event const &) = delete;

		public: void operator=(t_Event const &) = delete;

		public: void Set(void) throw()
		{
			auto set = ::SetEvent(m_handle);
			DBG_UNREFERENCED_LOCAL_VARIABLE(set);
			assert(FALSE != set);
		}

		public: void Reset(void) throw()
		{
			auto reset = ::ResetEvent(m_handle);
			DBG_UNREFERENCED_LOCAL_VARIABLE(reset);
			assert(FALSE != reset);
		}
	};
}
}
}

#endif