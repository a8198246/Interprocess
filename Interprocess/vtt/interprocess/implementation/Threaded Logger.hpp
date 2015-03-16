#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_THREADED_LOGGER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_THREADED_LOGGER

#pragma once

#include "Main.hpp"
#include "Main Windows SDK Control.hpp"

#ifdef _DEBUG_LOGGING

#include <sal.h>

#include <Windows.h>

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <cassert>
#include <string>
#include <ctime>
#include <array>
#include <sys/timeb.h>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class
	t_LoggerGuard;

	class
	t_ThreadedLogger
	:	public ::std::ofstream
	{
		friend class
		t_LoggerGuard;

		#pragma region Fields

		private: ::boost::mutex m_sync;
		private: ::DWORD        m_pid = ::GetCurrentProcessId();
		private: int            m_sent_message_counter      = 0;
		private: int            m_sent_bytes_counter        = 0;
		private: int            m_received_messages_counter = 0;
		private: int            m_received_bytes_counter    = 0;
		private: bool           m_master;

		#pragma endregion

		private:
		t_ThreadedLogger(void) = delete;

		private:
		t_ThreadedLogger(t_ThreadedLogger const &) = delete;

		private:
		t_ThreadedLogger(t_ThreadedLogger &&) = delete;

		public: explicit
		t_ThreadedLogger(_In_ const bool master)
		:	m_master(master)
		{
			//char temp_path[MAX_PATH];
			//auto const cch_temp_path = ::GetTempPathA(MAX_PATH, temp_path);
			//DBG_UNREFERENCED_LOCAL_VARIABLE(cch_temp_path);
			//assert(0 != cch_temp_path);
			//assert(cch_temp_path < MAX_PATH);

			::std::string log_file_name;
			log_file_name.assign(VTT_INTERPROCESS_SZ_LOG_FOLDER_PATH);
			if(master)
			{
				log_file_name += "\\interprocess master";
			}
			else
			{
				log_file_name += "\\interprocess slave pid ";
				log_file_name += ::boost::lexical_cast<::std::string>(m_pid);
			}
			log_file_name += " log.txt";
			auto flags = (master ? (::std::ofstream::out | ::std::ofstream::app) : (::std::ofstream::out | ::std::ofstream::trunc));
			open(log_file_name.c_str(), flags);
			Print_Prefix() << (master ? "master process" : "slave process") << ::std::endl;
		}

		private: void
		operator =(t_ThreadedLogger const &) = delete;

		private: void
		operator =(t_ThreadedLogger &&) = delete;
		
		public: auto
		Print_Prefix(void) -> t_ThreadedLogger &
		{
			//	time stamp with milliseconds
			{
				struct timeb tp;
				ftime(&tp);
				struct tm lt;
				localtime_s(&lt, &tp.time);
				::std::array<char, 256> sz_time;
				strftime(sz_time.data(), sz_time.size(), "%H:%M:%S", &lt);
				(*this) << sz_time.data() << ".";
				this->width(3);
				this->fill('0');
				(*this) << tp.millitm;
			}

			(*this)	<< " pid #";
			this->width(8);
			this->fill('0');
			(*this) << ::std::hex << m_pid;

			(*this)	<< " thread #";
			this->width(8);
			this->fill('0');
			(*this) << ::std::hex << ::GetCurrentThreadId();

			(*this) << ::std::dec << " | ";
			return(*this);
		}

		public: void
		Write_Block(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			Print_Prefix();
			(*this) << "data block of " << bc_data << " bytes size" << ::std::hex;

			auto i = static_cast<size_t>(0);
			do
			{
				if((i % 32) == 0)
				{
					(*this) << ::std::endl << "\t\t\t\t\t\t\t\t\t\t\t  ";
				}
				else if((i % 8) == 0)
				{
					(*this) << "   ";
				}
				this->width(2);
				this->fill('0');
				(*this) << (static_cast<unsigned short>(p_data[i]) & 0x00ff) << " ";

				++i;
			}
			while(bc_data != i);
			(*this) << ::std::dec << ::std::endl;
		}

		public: static void
		Print_Message(::std::string const & message);

		public: static void
		Print_InvalidFunctionParameterError(void);

		public: static void
		Print_Exception(::std::system_error & e);

		public: static void
		Print_Exception(::std::logic_error & e);

		public: static void
		Print_Exception(::std::exception & e);

		public: static void
		Print_UnknownException(void);

		public: static auto
		Get_Instance(void) -> t_ThreadedLogger &;
	};

	class
	t_LoggerGuard
	{
		#pragma region Fields

		private: t_ThreadedLogger & m_logger;

		#pragma endregion

		private: t_LoggerGuard(void) = delete;

		public: explicit
		t_LoggerGuard(t_ThreadedLogger & logger)
		:	m_logger(logger)
		{
			m_logger.m_sync.lock();
		}

		private:
		t_LoggerGuard(t_LoggerGuard const &) = delete;

		private:
		t_LoggerGuard(t_LoggerGuard &&) = delete;

		public:
		~t_LoggerGuard(void)
		{
			m_logger.m_sync.unlock();
		}

		private: void
		operator =(t_LoggerGuard const &) = delete;

		private: void
		operator =(t_LoggerGuard &&) = delete;
	};

	inline void t_ThreadedLogger::
	Print_Message(::std::string const & message)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << message << ::std::endl;
	}

	inline void t_ThreadedLogger::
	Print_InvalidFunctionParameterError(void)
	{
		Print_Message("function parameter is invalid");
	}

	inline void t_ThreadedLogger::
	Print_Exception(::std::system_error & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "system_error exception \"" << e.what() << "\" " << e.code() << ::std::endl;
	}

	inline void t_ThreadedLogger::
	Print_Exception(::std::logic_error & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "logic_error exception \"" << e.what() << "\"" << ::std::endl;
	}

	inline void t_ThreadedLogger::
	Print_Exception(::std::exception & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "exception \"" << e.what() << "\"" << ::std::endl;
	}
	
	inline void t_ThreadedLogger::
	Print_UnknownException(void)
	{
		Print_Message("unknown exception...");
	}
}
}
}

#endif

#endif
