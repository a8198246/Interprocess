#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_PATRON
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_PATRON

#pragma once

#include "Master.hpp"
#include "Slave.hpp"
#include "Static Instace.hpp"
#include "Threaded Logger.hpp"

#include <boost/thread/mutex.hpp>

#include <memory>
#include <stdexcept>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_Patron
	:	public t_StaticInstace<t_Patron>
	{
		template<typename> friend class
		t_StaticInstace;

	#ifdef _DEBUG_LOGGING
		friend class
		t_ThreadedLogger;
	#endif

		#pragma region Fields

		protected: ::std::unique_ptr<t_Master>         m_p_master;
		protected: ::std::unique_ptr<t_Slave>          m_p_slave;
		protected: ::boost::mutex                      m_slave_creation_sync;
	#ifdef _DEBUG_LOGGING
		protected: ::std::unique_ptr<t_ThreadedLogger> m_p_logger;
	#endif

		#pragma endregion

		private:
		t_Patron(void)
		{
			//	Do nothing
		}

	#ifdef _DEBUG_LOGGING
		public: static void
		Init_Logger(_In_ const bool master)
		{
			auto & patron = Get_Instace();
			if(!patron.m_p_logger)
			{
				patron.m_p_logger.reset(new t_ThreadedLogger(master));
			}
		}
	#endif

		public: static auto
		Get_Master(void) -> t_Master &
		{
			auto & patron = Get_Instace();
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			}
		#endif
			if(!patron.m_p_master)
			{
				if(patron.m_p_slave)
				{
					throw(::std::logic_error("Trying to act as Interprocess Master while already have Slave role."));
				}
				patron.m_p_master.reset(new t_Master());
				atexit(Explicit_Cleanup);
			}
			return(*patron.m_p_master);
		}

		public: static auto
		Get_Slave(void) -> t_Slave &
		{
			auto & patron = Get_Instace();
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			}
		#endif
			{
				::boost::lock_guard<::boost::mutex> lock(patron.m_slave_creation_sync);
				if(!patron.m_p_slave)
				{
					if(patron.m_p_master)
					{
						throw(::std::logic_error("Trying to act as Interprocess Slave while already have Master role."));
					}
					patron.m_p_slave.reset(new t_Slave());
					atexit(Explicit_Cleanup);
				}
			}
			return(*patron.m_p_slave);
		}

		private: static void
		Explicit_Cleanup(void)
		{
			auto & patron = Get_Instace();
		#ifdef _DEBUG_LOGGING
			if(patron.m_p_logger)
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			}
		#endif
			patron.m_p_slave.reset();
			patron.m_p_master.reset();
		#ifdef _DEBUG_LOGGING
			patron.m_p_logger.reset();
		#endif
		}
	};

#ifdef _DEBUG_LOGGING

	inline auto t_ThreadedLogger::
	Get_Instance(void) -> t_ThreadedLogger &
	{
		assert(nullptr != t_Patron::Get_Instace().m_p_logger.get());
		return(*t_Patron::Get_Instace().m_p_logger);
	}

#endif
}
}
}

#endif
