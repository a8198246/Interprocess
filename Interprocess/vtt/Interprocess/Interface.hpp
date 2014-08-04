#ifndef HEADER_VTT_INTERPROCESS_INTERFACE
#define HEADER_VTT_INTERPROCESS_INTERFACE

#pragma once

#include "Configuration.hpp"
#include "Export Control.hpp"

#include <sal.h>

VTT_EXTERN_C_ZONE_BEGIN

//	������, ���������� � ������� ��������

//	����� ���������� ������� ���������, ����� �������� msg �������� �������� � ��������������� ���������� id.
void VTT_INTERPROCESS_DLL_API
interprocess_master_send(_In_ const int application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data);

//	����� ���������� ������� ���������, ����� ������� ������ ���������� �������� ����������.
//	����� �����������, �� ��������� ������� ���������, ������� ����� �������.
//	� msgs ��������� ������ ������������ ���������.
int VTT_INTERPROCESS_DLL_API
interprocess_master_recieve(_Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity);

//	������, ���������� � ������� ��������

//	����� �������� msg �������� ��������.
void VTT_INTERPROCESS_DLL_API
interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data);

//	����� ��������� ������ ���������� ���������, ��������������� �������� � ��������������� id.
//	� msgs ��������� ������ ������������ ���������.
int VTT_INTERPROCESS_DLL_API
interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity); 

VTT_EXTERN_C_ZONE_END

#endif