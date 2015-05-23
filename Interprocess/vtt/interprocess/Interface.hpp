#ifndef HEADER_VTT_INTERPROCESS_INTERFACE
#define HEADER_VTT_INTERPROCESS_INTERFACE

#pragma once

#include "Configuration.hpp"
#include "Export Control.hpp"

#include <sal.h>

VTT_EXTERN_C_ZONE_BEGIN

//--------------------------------------------------------------------------------------------------
//	Методы, вызываемые в ведущем процессе:
//	(эти методы можно вызвать из одного потока внутри одного процесса)

//	Передача блока данных из ведущего процесса в процесс с идентификатором application_id.
void VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_send
(
	_In_                                                    const int    application_id
,	_In_reads_bytes_(bc_data)                               char const * p_data
,	_In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int    bc_data
);

//	Прием блока данных, посланных из ведомых процессов.
//	Возвращает количество байт, которые были получены и записаны в предоставленный буфер.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_master_receive
(
	_Out_writes_bytes_opt_(bc_buffer_capacity) char * const p_buffer
,	_In_                                       const int    bc_buffer_capacity
,	_In_                                       const int    timeout_msec
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
//	(эти методы можно вызывать из одного или нескольких потоков внутри одного или нескольких процессов)

//	Передача блока данных из ведомого процесса в ведущий.
void VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_send
(
	_In_reads_bytes_(bc_data)                               char const * p_data
,	_In_range_(0, VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT) const int    bc_data
);

//	Прием блока данных, посланных из ведущего процесса в процесс с идентификатором application_id.
//	Возвращает количество байт, которые были получены и записаны в предоставленный буфер.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_receive
(
	_In_                                       const int    application_id
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char * const p_buffer
,	_In_                                       const int    bc_buffer_capacity
);

//	Прием блока данных, посланных из ведущего процесса и относящихся к событию с идентификатором event_id.
//	Возвращает количество байт, которые были получены и записаны в предоставленный буфер.
//	Перед возвратом записывает значение высокоточного таймера в p_ticks.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
interprocess_slave_receive_common
(
	_In_                                       const int   event_id
,	_Out_writes_bytes_opt_(bc_buffer_capacity) char *      p_buffer
,	_In_                                       const int   bc_buffer_capacity
,	_In_                                       const int   timeout_msec
,	_Out_opt_                                  long long * p_ticks
);

//--------------------------------------------------------------------------------------------------
//	Прочие методы:

//	Прием UDP мультикаст сообщения.
//	Возвращает количество байт, которые были получены и записаны в предоставленный буфер.
//	При возникновении ошибки записывает ее код в p_error_code.
int VTT_INTERPROCESS_DLL_API VTT_INTERPROCESS_CALLING_CONVENTION
udp_multicast_receive
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
