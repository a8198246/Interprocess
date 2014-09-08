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

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_LoggerGuard;

	class t_ThreadedLogger
	:	public ::std::ofstream
	{
		friend class t_LoggerGuard;

		#pragma region Fields

		private: ::std::string  m_temp_path;
		private: ::std::string  m_temp_file_path;
		private: ::boost::mutex m_sync;
		private: ::DWORD        m_pid = ::GetCurrentProcessId();
		private: int            m_sent_message_counter      = 0;
		private: int            m_sent_bytes_counter        = 0;
		private: int            m_received_messages_counter = 0;
		private: int            m_received_bytes_counter    = 0;
		private: bool           m_master;

		#pragma endregion

		private: t_ThreadedLogger(void) = delete;

		public: explicit t_ThreadedLogger(_In_ const bool master)
		:	m_master(master)
		{
			char temp_path[MAX_PATH];
			auto const cch_temp_path = ::GetTempPathA(MAX_PATH, temp_path);
			DBG_UNREFERENCED_LOCAL_VARIABLE(cch_temp_path);
			assert(0 != cch_temp_path);
			assert(cch_temp_path < MAX_PATH);
			m_temp_path.assign(temp_path, cch_temp_path);

			auto & log_file_name = m_temp_file_path;
			log_file_name += m_temp_path;
			log_file_name += master ? "\\master #" : "\\slave #";
			log_file_name += ::boost::lexical_cast<::std::string>(m_pid);
			log_file_name += " log.txt";
			open(log_file_name.c_str(), ::std::ofstream::out | ::std::ofstream::trunc);
			Print_Prefix() << (master ? "master process" : "slave process") << ::std::endl;
		}

		private: t_ThreadedLogger(t_ThreadedLogger const &) = delete;

		private: void operator =(t_ThreadedLogger const &) = delete;

		public: auto Print_Prefix(void) -> t_ThreadedLogger &
		{
			auto old_width = width();
			width(20);
			auto old_fill = fill();
			fill(' ');
			operator <<("thread #") << ::GetCurrentThreadId() << " | ";
			fill(old_fill);
			width(old_width);
			return(*this);
		}

		public: void Write_SentBlock(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			m_temp_file_path.clear();
			auto & dump_file_name = m_temp_file_path;
			dump_file_name += m_temp_path;
			dump_file_name += m_master ? "\\master #" : "\\slave #";
			dump_file_name += ::boost::lexical_cast<::std::string>(m_pid);
			dump_file_name += " sent block #";
			dump_file_name += ::boost::lexical_cast<::std::string>(m_sent_message_counter);
			dump_file_name += ".bin";
			Write(dump_file_name, p_data, bc_data);
			++m_sent_message_counter;
			m_sent_bytes_counter += static_cast<int>(bc_data);
		}

		public: void Write_ReceivedBlock(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			m_temp_file_path.clear();
			auto & dump_file_name = m_temp_file_path;
			dump_file_name += m_temp_path;
			dump_file_name += m_master ? "\\master #" : "\\slave #";
			dump_file_name += ::boost::lexical_cast<::std::string>(m_pid);
			dump_file_name += " received block #";
			dump_file_name += ::boost::lexical_cast<::std::string>(m_received_messages_counter);
			dump_file_name += ".bin";
			Write(dump_file_name, p_data, bc_data);
			++m_received_messages_counter;
			m_received_bytes_counter += static_cast<int>(bc_data);
		}

		private: void Write(_In_ ::std::string const & dump_file_name, _In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		{
			assert(nullptr != p_data);
			assert(0 < bc_data);
			::std::ofstream dump_file;
			dump_file.open(dump_file_name.c_str(), ::std::ofstream::out | ::std::ofstream::trunc | ::std::ofstream::binary);
			dump_file.write(p_data, bc_data);
		}

		public: static void Print_Message(::std::string const & message);

		public: static void Print_InvalidFunctionParameterError(void);

		public: static void Print_Exception(::std::system_error & e);

		public: static void Print_Exception(::std::logic_error & e);

		public: static void Print_Exception(::std::exception & e);

		public: static void Print_UnknownException(void);

		public: static auto Get_Instance(void) -> t_ThreadedLogger &;
	};

	class t_LoggerGuard
	{
		#pragma region Fields

		private: t_ThreadedLogger & m_logger;

		#pragma endregion

		private: t_LoggerGuard(void) = delete;

		public: explicit t_LoggerGuard(t_ThreadedLogger & logger)
		:	m_logger(logger)
		{
			m_logger.m_sync.lock();
		}

		private: t_LoggerGuard(t_LoggerGuard const &) = delete;

		public: ~t_LoggerGuard(void)
		{
			m_logger.m_sync.unlock();
		}

		private: void operator =(t_LoggerGuard const &) = delete;
	};

	inline void t_ThreadedLogger::Print_Message(::std::string const & message)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << message << ::std::endl;
	}

	inline void t_ThreadedLogger::Print_InvalidFunctionParameterError(void)
	{
		Print_Message("function parameter is invalid");
	}

	inline void t_ThreadedLogger::Print_Exception(::std::system_error & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "system_error exception \"" << e.what() << "\" " << e.code() << ::std::endl;
	}

	inline void t_ThreadedLogger::Print_Exception(::std::logic_error & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "logic_error exception \"" << e.what() << "\"" << ::std::endl;
	}

	inline void t_ThreadedLogger::Print_Exception(::std::exception & e)
	{
		auto & logger = t_ThreadedLogger::Get_Instance();
		t_LoggerGuard guard(logger);
		logger.Print_Prefix() << "exception \"" << e.what() << "\"" << ::std::endl;
	}
	
	inline void t_ThreadedLogger::Print_UnknownException(void)
	{
		Print_Message("unknown exception...");
	}
}
}
}

#endif

#endif