#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MAIN
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MAIN

#pragma once

#define VTT_INTERPROCESS_DLL_EXPORTS

#define VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "vtt interprocess "

#define _DEBUG_LOGGING

#ifndef _DEBUG
#undef _DEBUG_LOGGING
#endif

//	folder to store logs in
//	must exist before starting logging
#define VTT_INTERPROCESS_SZ_LOG_FOLDER_PATH "c:\\logs"

#endif