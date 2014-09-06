#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MAIN_WINDOWS_SDK_CONTROL
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_MAIN_WINDOWS_SDK_CONTROL

#pragma once

#ifndef _WINDOWS
#define _WINDOWS
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef STRICT
#define STRICT
#endif

#ifndef _USRDLL
#define _USRDLL
#endif

#define WINVER         0x0601
#define _WIN32_WINDOWS 0x0601
#define _WIN32_WINNT   0x0601
#define _WIN32_IE      0x0800
#define NTDDI_VERSION  NTDDI_WIN7

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#endif