#ifndef HEADER_VTT_INTERPROCESS_DETAILS_PATRON
#define HEADER_VTT_INTERPROCESS_DETAILS_PATRON

#pragma once

#include "Master.hpp"
#include "Slave.hpp"
#include "Static Instace.hpp"

#include <memory>
#include <stdexcept>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_Patron
	:	public t_StaticInstace<t_Patron>
	{
		template<typename>
		friend class t_StaticInstace;

		#pragma region Fields

		protected: ::std::unique_ptr<t_Master> m_p_master;
		protected: ::std::unique_ptr<t_Slave>  m_p_slave;

		#pragma endregion

		private: t_Patron(void)
		{
			//	Do nothing
		}

		public: static auto Get_Master(void) -> t_Master &
		{
			auto & patron = Get_Instace();
			if(!patron.m_p_master)
			{
				if(patron.m_p_slave.get() != nullptr)
				{
					throw(::std::logic_error("Trying to act as Interprocess Master while already have Slave role."));
				}
				patron.m_p_master.reset(new t_Master());
				atexit(Explicit_Cleanup);
			}
			return(*patron.m_p_master);
		}

		public: static auto Get_Slave(void) -> t_Slave &
		{
			auto & patron = Get_Instace();
			if(!patron.m_p_slave)
			{
				if(patron.m_p_master.get() != nullptr)
				{
					throw(::std::logic_error("Trying to act as Interprocess Slave while already have Master role."));
				}
				patron.m_p_slave.reset(new t_Slave());
				atexit(Explicit_Cleanup);
			}
			return(*patron.m_p_slave);
		}

		private: static void Explicit_Cleanup(void)
		{
			auto & patron = Get_Instace();
			patron.m_p_slave.reset();
			patron.m_p_master.reset();
		}
	};
}
}
}

#endif