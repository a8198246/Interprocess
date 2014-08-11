#include "Precompiled.hpp"
#include "Patron.hpp"

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
		t_Patron::Get_Master().Send_To_Slave(application_id, p_data, static_cast<size_t>(bc_data));
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_recieve(_Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		bc_written = static_cast<int>(t_Patron::Get_Master().Recieve_From_Slaves(p_buffer, static_cast<size_t>(bc_buffer_capacity)));
	}
	return(bc_written);
}

void VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
	{
		t_Patron::Get_Slave().Send_To_Master(p_data, static_cast<size_t>(bc_data));
	}
}

int VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		bc_written = static_cast<int>(t_Patron::Get_Slave().Recieve_From_Master(application_id, p_buffer, static_cast<size_t>(bc_buffer_capacity)));
	}
	return(bc_written);
}

int VTT_INTERPROCESS_CALLING_CONVENTION
udp_multicast_recieve(_In_z_ wchar_t const * psz_host, _In_range_(0, 65535) const int port, _Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity)
{
	auto bc_written = 0;
	if((nullptr != psz_host) && (L'\0' != *psz_host) && (0 <= port) && (port <= 65535) && (nullptr != p_buffer) && (0 < bc_buffer_capacity))
	{
		t_Address address(psz_host, static_cast<unsigned short>(port), false);
		bc_written = static_cast<int>(t_Patron::Get_NetBroker()[address].Recieve(p_buffer, static_cast<size_t>(bc_buffer_capacity), VTT_INTERPROCESS_SOCKET_RECIEVE_TIMEOUT_MSECONDS));
	}
	return(bc_written);
}