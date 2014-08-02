#ifndef HEADER_VTT_INTERPROCESS_INTERFACE
#define HEADER_VTT_INTERPROCESS_INTERFACE

#pragma once

#include "Export Control.hpp"

#include <sal.h>

#define VTT_INTERPROCESS_BC_MESSAGE_LIMIT 65535

VTT_EXTERN_C_ZONE_BEGIN

//	������, ���������� � ������� ��������

//	����� ���������� ������� ���������, ����� �������� msg �������� �������� � ��������������� ���������� id.
void VTT_INTERPROCESS_DLL_API
interprocess_master_send(_In_ const int slave_app_id, _In_reads_bytes_(bc_data) void const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_LIMIT) const int bc_data);

//	����� ���������� ������� ���������, ����� ������� ������ ���������� �������� ����������.
//	����� �����������, �� ��������� ������� ���������, ������� ����� �������.
//	� msgs ��������� ������ ������������ ���������.
int VTT_INTERPROCESS_DLL_API
interprocess_master_recieve(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer);

//	������, ���������� � ������ ��������

//	����� �������� msg �������� ��������.
void VTT_INTERPROCESS_DLL_API
interprocess_slave_send(_In_ const int slave_app_id, _In_reads_bytes_(bc_data) void const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_LIMIT) const int bc_data);

//	����� ��������� ������ ���������� ���������, ��������������� �������� � ��������������� id.
//	� msgs ��������� ������ ������������ ���������.
int VTT_INTERPROCESS_DLL_API
interprocess_slave_recieve(_Out_writes_bytes_(bc_buffer) char * p_buffer, _In_ const int bc_buffer); 

VTT_EXTERN_C_ZONE_END

#endif