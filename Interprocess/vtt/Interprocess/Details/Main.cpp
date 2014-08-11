#include "Precompiled.hpp"
#include "Patron.hpp"
#include "Sockets User.hpp"
#include "UDP Multicast Socket.hpp"

#include "../Interface.hpp"

#include <Windows.h>

#ifdef _DEBUG
#	pragma comment(lib, "boost_thread-vc120-mt-gd-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-gd-1_55.lib")
#else
#	pragma comment(lib, "boost_thread-vc120-mt-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-1_55.lib")
#endif

using namespace n_vtt;
using namespace n_interprocess;
using namespace n_details;

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send(_In_ const int application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
	{
		try
		{
			t_Patron::Get_Master().Send_To_Slave(application_id, p_data, static_cast<size_t>(bc_data));
		}
		catch(...)
		{
			//	Do nothing
		}
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_recieve(_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity, _In_ const int timeout_msec)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		try
		{
			bc_written = static_cast<int>(t_Patron::Get_Master().Recieve_From_Slaves(p_buffer, static_cast<size_t>(bc_buffer_capacity), timeout_msec));
		}
		catch(...)
		{
			assert(0 == bc_written);
		}
	}
	return(bc_written);
}

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
	{
		try
		{
			t_Patron::Get_Slave().Send_To_Master(p_data, static_cast<size_t>(bc_data));
		}
		catch(...)
		{
			//	Do nothing
		}
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		try
		{
			bc_written = static_cast<int>(t_Patron::Get_Slave().Recieve_From_Master(application_id, p_buffer, static_cast<size_t>(bc_buffer_capacity)));
		}
		catch(...)
		{
			assert(0 == bc_written);
		}
	}
	return(bc_written);
}

int VTT_INTERPROCESS_CALLING_CONVENTION
udp_multicast_recieve
(
	_In_z_ wchar_t const *                        psz_host
,	_In_range_(0, 65535) const int                port
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char * p_buffer
,	_In_ const int                                bc_buffer_capacity
,	_In_ const int                                timeout_msec
,	_Out_ int *                                   p_error_code
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