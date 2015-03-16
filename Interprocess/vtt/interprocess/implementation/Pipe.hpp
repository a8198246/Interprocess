#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_PIPE
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_PIPE

#pragma once

#include "Named Mutex.hpp"
#include "Event.hpp"
#include "Shared Memory.hpp"

#include <sal.h>

#include <string>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_Pipe
	{
		#pragma region Fields

		protected: t_NamedMutex   m_sync;
		protected: t_Event        m_flag;
		protected: t_SharedMemory m_shared_memory;
		
		#pragma endregion

		private:
		t_Pipe(void) = delete;

		protected:
		t_Pipe(_In_ ::std::string name, _In_ const size_t bc_shared_memory, _In_ const bool reset_event_manually = false)
		:	m_sync(name)
		,	m_flag(name, reset_event_manually)
		,	m_shared_memory(name, m_sync, bc_shared_memory)
		{
			//	Do nothing
		}

		private:
		t_Pipe(t_Pipe const &) = delete;

		private:
		t_Pipe(t_Pipe &&) = delete;

		private: void
		operator =(t_Pipe const &) = delete;

		private: void
		operator =(t_Pipe &&) = delete;
	};
}
}
}

#endif
