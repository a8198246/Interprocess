#ifndef HEADER_VTT_INTERPROCESS_INTERFACE
#define HEADER_VTT_INTERPROCESS_INTERFACE

#pragma once

#include "Configuration.hpp"
#include "Export Control.hpp"

#include <sal.h>

VTT_EXTERN_C_ZONE_BEGIN

//--------------------------------------------------------------------------------------------------
//	Методы, вызываемые в ведущем процессе:

//	Передача блока данных из ведущего процесса в процесс с идентификатором application_id.
void VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send
(
	_In_                                                    const int    application_id
,	_In_reads_bytes_(bc_data)                               char const * p_data
,	_In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int    bc_data
);

//	Прием блока данных, посланных из ведомых процессов.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_recieve
(
	_Out_writes_bytes_opt_(bc_buffer_capacity) char *    p_buffer
,	_In_                                       const int bc_buffer_capacity
,	_In_                                       const int timeout_msec
);

//	Передача блока данных всем ведомым процессам, ожидающим событие с идентификатором event_id.
void VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send_to_all
(
	_In_                                                    const int    event_id
,	_In_reads_bytes_(bc_data)                               char const * p_data
,	_In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int    bc_data
);

//--------------------------------------------------------------------------------------------------
//	Методы, вызываемые в ведомом процессе:

//	Передача блока данных из ведомого процесса в ведущий.
void VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_send
(
	_In_reads_bytes_(bc_data)                               char const * p_data
,	_In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int    bc_data
);

//	Прием блока данных, посланных из ведущего процесса в процесс с идентификатором application_id.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve
(
	_In_                                       const int application_id
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char *    p_buffer
,	_In_                                       const int bc_buffer_capacity
);

//	Прием блока данных, посланных из ведущего процесса и относящихся к событию с идентификатором event_id.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_recieve_common
(
	_In_                                       const int   event_id
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char *      p_buffer
,	_In_                                       const int   bc_buffer_capacity
,	_In_                                       const int   timeout_msec
,	_In_opt_                                   long long * p_ticks
);

//--------------------------------------------------------------------------------------------------
//	Прочие методы:

//	Прием UDP мультикаст сообщения.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
udp_multicast_recieve
(
	_In_z_                                     wchar_t const * psz_host
,	_In_range_(0, 65535)                       const int       port
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char *          p_buffer
,	_In_                                       const int       bc_buffer_capacity
,	_In_                                       const int       timeout_msec
,	_Out_opt_                                  int * const     p_error_code
);

VTT_EXTERN_C_ZONE_END

#endif
