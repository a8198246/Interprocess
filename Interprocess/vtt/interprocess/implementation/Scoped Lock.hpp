#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SCOPED_LOCK
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_SCOPED_LOCK

#pragma once

#include "Named Mutex.hpp"

#include <sal.h>

#include <cassert>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_ScopedLock
	{
		#pragma region Fields

		private: t_NamedMutex & m_mutex;
		private: volatile bool  m_locked = false;

		#pragma endregion

		private: t_ScopedLock(void) = delete;

		public: explicit t_ScopedLock(t_NamedMutex & mutex)
		:	m_mutex(mutex)
		{
			Lock();
		}

		private: t_ScopedLock(t_ScopedLock const &) = delete;

		public: ~t_ScopedLock(void)
		{
			if(m_locked)
			{
				Unlock();
			}
		}

		private: void operator =(t_ScopedLock const &) = delete;

		public: void Lock(void)
		{
			assert(!m_locked);
			m_locked = true;
			m_mutex.Lock();
		}

		public: void Unlock(void)
		{
			assert(m_locked);
			m_mutex.Unlock();
			m_locked = false;
		}
	};
}
}
}

#endif