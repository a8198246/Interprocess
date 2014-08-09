#ifndef HEADER_VTT_INTERPROCESS_EXPORT_CONTROL
#define HEADER_VTT_INTERPROCESS_EXPORT_CONTROL

#pragma once

#ifdef VTT_INTERPROCESS_DLL_API
#	error VTT_INTERPROCESS_DLL_API is an internal macro, do not define it manually
#endif

#ifdef VTT_INTERPROCESS_DLL_EXPORTS
#	if defined(_MSC_VER)
#		define VTT_INTERPROCESS_DLL_API// __declspec(dllexport) // using .def file instead
#	elif defined(__GNUC__)
#		error Unsupported platform
#	else
#		error Unsupported platform
#	endif
#else
#	if defined(_MSC_VER)
#		define VTT_INTERPROCESS_DLL_API// __declspec(dllimport) // using .def file instead
#		ifdef _DEBUG
#			if defined(VTT_X32)
#				pragma comment(lib, "x32/Debug/Interprocess.lib")
#			elif defined(VTT_X64)
#				pragma comment(lib, "x64/Debug/Interprocess.lib")
#			else
#				error Unsupported platform
#			endif
#		else
#			if defined(VTT_X32)
#				pragma comment(lib, "x32/Release/Interprocess.lib")
#			elif defined(VTT_X64)
#				pragma comment(lib, "x64/Release/Interprocess.lib")
#			else
#				error Unsupported platform
#			endif
#		endif
#	elif defined(__GNUC__)
#		error Unsupported platform
#	else
#		error Unsupported platform
#	endif
#endif

#ifdef __cplusplus
#	define VTT_EXTERN_C_ZONE_BEGIN extern "C" {
#	define VTT_EXTERN_C_ZONE_END }
#else
#	define VTT_EXTERN_C_ZONE_BEGIN
#	define VTT_EXTERN_C_ZONE_END
#endif

#endif