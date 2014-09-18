#include "Precompiled.hpp"
#include "Patron.hpp"
#include "Sockets User.hpp"
#include "UDP Multicast Socket.hpp"
#include "Threaded Logger.hpp"

#include "../Interface.hpp"

#include <Windows.h>

#include <stdexcept>
#include <system_error>

#ifdef _DEBUG
#	pragma comment(lib, "libboost_system-vc120-mt-sgd-1_56")
#	pragma comment(lib, "libboost_thread-vc120-mt-sgd-1_56")
#else
#	pragma comment(lib, "libboost_system-vc120-mt-s-1_56")
#	pragma comment(lib, "libboost_thread-vc120-mt-s-1_56")
#endif

using namespace n_vtt;
using namespace n_interprocess;
using namespace n_implementation;

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send(_In_ const int application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(true);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " application_id " << application_id
				<< ", p_data " << ::std::hex << static_cast<void const *>(p_data) << ::std::dec
				<< ", bc_data " << bc_data << ::std::endl;
			logger.Print_Prefix() << "block to be send:" << ::std::endl;
			logger.Write_Block(p_data, bc_data);
		}
	#endif
		if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
		{
			auto & master = t_Patron::Get_Master();
			master.Send_To_Slave(application_id, p_data, static_cast<size_t>(bc_data));
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_recieve(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity, _In_ const int timeout_msec)
{
	auto bc_written = 0;
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(true);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " p_buffer " << ::std::hex << static_cast<void const *>(p_buffer) << ::std::dec
				<< ", bc_buffer_capacity " << bc_buffer_capacity
				<< ", timeout_msec " << timeout_msec << ::std::endl;
		}
	#endif
		if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
		{
			auto & master = t_Patron::Get_Master();
			bc_written = static_cast<int>(master.Receive_From_Slaves(p_buffer, static_cast<size_t>(bc_buffer_capacity), timeout_msec));
		#ifdef _DEBUG_LOGGING
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			if(0 != bc_written)
			{
				logger.Print_Prefix() << "received block:" << ::std::endl;
				logger.Write_Block(p_buffer, bc_written);
			}
			else
			{
				logger.Print_Prefix() << "nothing received..." << ::std::endl;
			}
		#endif
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
		assert(0 == bc_written);
	}
	return(bc_written);
}

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send_to_all(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(true);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " p_data " << ::std::hex << static_cast<void const *>(p_data) << ::std::dec
				<< ", bc_data " << bc_data << ::std::endl;
			logger.Print_Prefix() << "block to be send:" << ::std::endl;
			logger.Write_Block(p_data, bc_data);
		}
	#endif
		if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
		{
			auto & master = t_Patron::Get_Master();
			master.Send_To_AllSlaves(p_data, bc_data);
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
	}
}

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(false);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " p_data " << ::std::hex << static_cast<void const *>(p_data) << ::std::dec
				<< ", bc_data " << bc_data << ::std::endl;
			logger.Print_Prefix() << "block to be send:" << ::std::endl;
			logger.Write_Block(p_data, bc_data);
		}
	#endif
		if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
		{
			auto & slave = t_Patron::Get_Slave();
			slave.Send_To_Master(p_data, static_cast<size_t>(bc_data));
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity)
{
	auto bc_written = 0;
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(false);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " application_id " << application_id 
				<< ", p_buffer " << ::std::hex << static_cast<void const *>(p_buffer) << ::std::dec
				<< ", bc_buffer_capacity " << bc_buffer_capacity << ::std::endl;
		}
	#endif
		if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
		{
			auto & slave = t_Patron::Get_Slave();
			bc_written = static_cast<int>(slave.Receive_From_Master(application_id, p_buffer, static_cast<size_t>(bc_buffer_capacity)));
		#ifdef _DEBUG_LOGGING
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			if(0 != bc_written)
			{
				logger.Print_Prefix() << "received block:" << ::std::endl;
				logger.Write_Block(p_buffer, bc_written);
			}
			else
			{
				logger.Print_Prefix() << "nothing received..." << ::std::endl;
			}
		#endif
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
		assert(0 == bc_written);
	}
	return(bc_written);
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve_common(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity, _In_ const int timeout_msec)
{
	auto bc_written = 0;
	try
	{
	#ifdef _DEBUG_LOGGING
		t_Patron::Init_Logger(false);
		{
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			logger.Print_Prefix() << __FUNCSIG__ << ::std::endl;
			logger.Print_Prefix() << "parameters:"
				<< " p_buffer " << ::std::hex << static_cast<void const *>(p_buffer) << ::std::dec
				<< ", bc_buffer_capacity " << bc_buffer_capacity
				<< ", timeout_msec " << timeout_msec
				<< ::std::endl;
		}
	#endif
		if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
		{
			auto & slave = t_Patron::Get_Slave();
			bc_written = static_cast<int>(slave.ReceiveCommon_From_Master(p_buffer, static_cast<size_t>(bc_buffer_capacity), timeout_msec));
		#ifdef _DEBUG_LOGGING
			auto & logger = t_ThreadedLogger::Get_Instance();
			t_LoggerGuard guard(logger);
			if(0 != bc_written)
			{
				logger.Print_Prefix() << "received block:" << ::std::endl;
				logger.Write_Block(p_buffer, bc_written);
			}
			else
			{
				logger.Print_Prefix() << "nothing received..." << ::std::endl;
			}
		#endif
		}
		else
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_InvalidFunctionParameterError();
		#endif
		}
	}
#ifdef _DEBUG_LOGGING
	catch(::std::system_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::logic_error & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
	catch(::std::exception & e)
	{
		t_ThreadedLogger::Print_Exception(e);
	}
#endif
	catch(...)
	{
	#ifdef _DEBUG_LOGGING
		t_ThreadedLogger::Print_UnknownException();
	#endif
		assert(0 == bc_written);
	}
	return(bc_written);
}

int VTT_INTERPROCESS_CALLING_CONVENTION
udp_multicast_recieve
(
	_In_z_ wchar_t const *                            psz_host
,	_In_range_(0, 65535) const int                    port
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer
,	_In_ const int                                    bc_buffer_capacity
,	_In_ const int                                    timeout_msec
,	_Out_ int *                                       p_error_code
)
{
	auto bc_written = 0;
	int error_code;
	if((nullptr != psz_host) && (L'\0' != *psz_host) && (0 <= port) && (port <= 65535) && (nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		try
		{
			n_system::n_windows::t_SocketsUser sockets_user;
			t_UDPMulticastSocket socket(psz_host, static_cast<unsigned short>(port));
			bc_written = socket.Recieve(p_buffer, bc_buffer_capacity, timeout_msec);
			error_code = ERROR_SUCCESS;
		}
		catch(::std::system_error & e)
		{
			assert(0 == bc_written);
			error_code = e.code().value();
		}
		catch(...)
		{
			assert(0 == bc_written);
			error_code = E_UNEXPECTED;
		}
	}
	else
	{
		error_code = ERROR_BAD_ARGUMENTS;
	}
	if(nullptr != p_error_code)
	{
		*p_error_code = error_code;
	}
	return(bc_written);
}