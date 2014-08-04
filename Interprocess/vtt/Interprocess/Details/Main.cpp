#include "Precompiled.hpp"
#include "Patron.hpp"

#include "../Interface.hpp"

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

void interprocess_master_send(_In_ const int application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
	{
		t_Patron::Get_Master().Send_To_Slave(application_id, p_data, bc_data);
	}
}

int interprocess_master_recieve(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer))
	{
		bc_written = t_Patron::Get_Master().Recieve_From_Slaves(p_buffer, bc_buffer);
	}
	return(bc_written);
}

void interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data)
{
	if((nullptr != p_data) && (0 < bc_data) && (bc_data <= VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT))
	{
		t_Patron::Get_Slave().Send_To_Master(p_data, bc_data);
	}
}

int interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer)
{
	auto bc_written = 0;
	if((nullptr != p_buffer) && (0 < bc_buffer))
	{
		bc_written = t_Patron::Get_Slave().Recieve_From_Master(application_id, p_buffer, bc_buffer);
	}
	return(bc_written);
}