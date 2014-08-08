#ifndef HEADER_VTT_INTERPROCESS_DETAILS_SCOPED_LOCK
#define HEADER_VTT_INTERPROCESS_DETAILS_SCOPED_LOCK

#pragma once

#include "Named Mutex.hpp"

#include <sal.h>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_ScopedLock
	{
		friend class t_ConditionalVariable;

		#pragma region Fields

		private: t_NamedMutex & m_mutex;
		private: volatile bool  m_locked = false;

		#pragma endregion

		private: t_ScopedLock(void) = delete;

		public: explicit t_ScopedLock(t_NamedMutex & mutex)
		:	m_mutex(mutex)
		{
			m_locked = true;
			m_mutex.Lock();
		}

		private: t_ScopedLock(t_ScopedLock const &) = delete;

		public: ~t_ScopedLock(void)
		{
			if(m_locked)
			{
				m_mutex.Unlock();
				m_locked = false;
			}
		}

		private: void operator=(t_ScopedLock const &) = delete;
	};
}
}
}

#endif