#ifndef HEADER_VTT_INTERPROCESS_CONFIGURATION
#define HEADER_VTT_INTERPROCESS_CONFIGURATION

#pragma once

//	Maximum length of the master-to-slave shared memory buffer.
#define VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT 65535

//	Maximum number of different applications for which
//	data exchange can be performed. Affects:
//	1) number of master-to-slave shared memory buffers
//	2) magnitude of slave-to-host shared memory buffer
#define VTT_INTERPROCESS_EC_APPLICATIONS_MAX 100

#endif