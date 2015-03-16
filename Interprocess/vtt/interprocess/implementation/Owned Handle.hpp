#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_OWNED_HANDLE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_OWNED_HANDLE

#pragma once

#include <sal.h>

#include <Windows.h>

#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_OwnedHandle
	{
		#pragma region Fields

		protected: ::HANDLE m_handle = NULL;

		#pragma endregion

		public:
		t_OwnedHandle(void) throw()
		{
			//	Do nothing
		}

		private:
		t_OwnedHandle(t_OwnedHandle const &) = delete;

		private:
		t_OwnedHandle(t_OwnedHandle &&) = delete;

		public: ~t_OwnedHandle(void) throw()
		{
			if(Is_Initialized())
			{
				auto const closed = ::CloseHandle(m_handle);
				DBG_UNREFERENCED_LOCAL_VARIABLE(closed);
				assert(FALSE != closed);
			}
		}

		private: void
		operator =(t_OwnedHandle const &) = delete;

		private: void
		operator =(t_OwnedHandle &&) = delete;

		public: auto
		Is_Initialized(void) const throw() -> bool
		{
			return(NULL != m_handle);
		}

		public: auto
		Is_Not_Initialized(void) const throw() -> bool
		{
			return(NULL == m_handle);
		}
	};
}
}
}

#endif
