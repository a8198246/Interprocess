#include "Precompiled.hpp"

#include "../Interface.hpp"

#ifdef _DEBUG
#	pragma comment(lib, "boost_thread-vc120-mt-gd-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-gd-1_55.lib")
#else
#	pragma comment(lib, "boost_thread-vc120-mt-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-1_55.lib")
#endif

void
interprocess_master_send(_In_ const int slave_app_id, _In_reads_bytes_(bc_data) void const * p_data, _In_range_(0, 65535) const int bc_data)
{
	((void) slave_app_id);
	((void) p_data);
	((void) bc_data);
}

int
interprocess_master_recieve(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer)
{
	((void) p_buffer);
	((void) bc_buffer);
	return(0);
}

void
interprocess_slave_send(_In_ const int slave_app_id, _In_reads_bytes_(bc_data) void const * p_data, _In_range_(0, 65535) const int bc_data)
{
	((void) slave_app_id);
	((void) p_data);
	((void) bc_data);
}

int VTT_INTERPROCESS_DLL_API
interprocess_slave_recieve(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer)
{
	((void) p_buffer);
	((void) bc_buffer);
	return(0);
}