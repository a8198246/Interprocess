#ifndef HEADER_VTT_INTERPROCESS_INTERFACE
#define HEADER_VTT_INTERPROCESS_INTERFACE

#pragma once

#include "Configuration.hpp"
#include "Export Control.hpp"

#include <sal.h>

VTT_EXTERN_C_ZONE_BEGIN

//	Методы, вызываемые в ведущем процессе:

//	Метод вызывается ведущим процессом, чтобы передать msg рабочему процессу с идентификатором приложения id.
void VTT_INTERPROCESS_DLL_API
interprocess_master_send(_In_ const int application_id, _In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data);

//	Метод вызывается ведущим процессом, чтобы принять данные переданные рабочими процессами.
//	Метод блокирующий, до появления первого сообщения, которое можно забрать.
//	В msgs суммарные данные передаваемых сообщений.
int VTT_INTERPROCESS_DLL_API
interprocess_master_recieve(_Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity);

//	Методы, вызываемые в ведомом процессе:

//	Метод передает msg ведущему процессу.
void VTT_INTERPROCESS_DLL_API
interprocess_slave_send(_In_reads_bytes_(bc_data) char const * p_data, _In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int bc_data);

//	Метод принимает раннее переданные сообщения, предназначенные процессу с идентификатором id.
//	В msgs суммарные данные передаваемых сообщений.
int VTT_INTERPROCESS_DLL_API
interprocess_slave_recieve(_In_ const int application_id, _Out_writes_bytes_(bc_buffer_capacity) char * p_buffer, _In_ const int bc_buffer_capacity); 

VTT_EXTERN_C_ZONE_END

#endif