#ifndef HEADER_VTT_INTERPROCESS_CONFIGURATION
#define HEADER_VTT_INTERPROCESS_CONFIGURATION

#pragma once

//	Maximum length of the master-to-slave shared memory buffer.
#define VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT 65535

//	Magnitude of slave-to-host shared memory buffer
//	compared to the maximum length of master-to-slave shared memory buffer.
#define VTT_INTERPROCESS_SLAVE_TO_MASTER_MESSAGE_BUFFER_SIZE_MAGNITUDE 100

#define VTT_INTERPROCESS_CALLING_CONVENTION __stdcall

#endif