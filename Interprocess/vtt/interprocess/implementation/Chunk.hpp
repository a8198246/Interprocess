#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_CHUNK
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_CHUNK

#pragma once

#include "Threaded Logger.hpp"

#include <sal.h>

#include <algorithm>
#include <memory.h>

#include <boost/scoped_array.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	//	Helper class to fake move behavior with copy semantics.
	//	A workaround for the lack of support for move semantics in boost::asio
	class
	t_Chunk
	{
		protected: typedef ::boost::scoped_array<char>
		t_pData;

		#pragma region Fields

		protected: t_pData m_p_data;
		protected: size_t  m_bc_data = 0;

		#pragma endregion

		public:
		t_Chunk(void)
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			}
		#endif
		}

		public:
		t_Chunk(_In_reads_bytes_(bc_data) char const * p_data, _In_ const size_t bc_data)
		:	m_p_data(new char[bc_data])
		,	m_bc_data(bc_data)
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
				logger.Print_Prefix() << "pointer " << reinterpret_cast<void const *>(p_data) << " size " << bc_data << ::std::endl;
				logger.Print_Prefix() << "current pointer " << reinterpret_cast<void const *>(m_p_data.get()) << " size " << m_bc_data << ::std::endl;
			}
		#endif
			memcpy(m_p_data.get(), p_data, bc_data);
		}

		public:
		t_Chunk(_Inout_ t_Chunk const & that)
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
				logger.Print_Prefix() << "pointer " << reinterpret_cast<void const *>(that.m_p_data.get()) << " size " << that.m_bc_data << ::std::endl;
			}
		#endif
			auto & that_w = const_cast<t_Chunk &>(that);
			m_p_data.swap(that_w.m_p_data);
			::std::swap(m_bc_data, that_w.m_bc_data);
			assert(0 == that.m_bc_data);
			assert(nullptr == that.m_p_data.get());
		}

		public:
		~t_Chunk(void)
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
				logger.Print_Prefix() << "current pointer " << reinterpret_cast<void const *>(m_p_data.get()) << " size " << m_bc_data << ::std::endl;
			}
		#endif
		}

		public: void
		operator =(_Inout_ t_Chunk const & that)
		{
		#ifdef _DEBUG_LOGGING
			{
				auto & logger = t_ThreadedLogger::Get_Instance();
				t_LoggerGuard guard(logger);
				logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
				logger.Print_Prefix() << "pointer " << reinterpret_cast<void const *>(that.m_p_data.get()) << " size " << that.m_bc_data << ::std::endl;
				logger.Print_Prefix() << "current pointer " << reinterpret_cast<void const *>(m_p_data.get()) << " size " << m_bc_data << ::std::endl;
			}
		#endif
			auto & that_w = const_cast<t_Chunk &>(that);
			m_p_data.swap(that_w.m_p_data);
			::std::swap(m_bc_data, that_w.m_bc_data);
			assert(0 == that.m_bc_data);
			assert(nullptr == that.m_p_data.get());
		}

		public: auto
		Is_Empty(void) const throw() -> bool
		{
			return(0 == m_bc_data);
		}

		public: auto
		Is_Not_Empty(void) const throw() -> bool
		{
			return(0 != m_bc_data);
		}

		public: auto
		Get_Data(void) const throw() -> char const *
		{
			return(m_p_data.get());
		}

		public: auto
		Get_Size(void) const throw() -> size_t const &
		{
			return(m_bc_data);
		}
	};
}
}
}

#endif
