#ifndef HEADER_VTT_SYSTEM_WINDOWS_SOCKETS_USER
#define HEADER_VTT_SYSTEM_WINDOWS_SOCKETS_USER

#pragma once

#include <sal.h>

#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#include <system_error>

namespace n_vtt
{
namespace n_system
{
namespace n_windows
{
	class t_SocketsUser
	{
		public: t_SocketsUser(void)
		{
			::WSADATA wsa_data;
			auto sockets_version = MAKEWORD(2, 2);
			auto result = ::WSAStartup(sockets_version, &wsa_data);
			if(ERROR_SUCCESS != result)
			{
				throw(::std::system_error(static_cast<int>(result), ::std::system_category(), "failed to initialize Windows Sockets"));
			}
		}

		private: t_SocketsUser(t_SocketsUser const &) = delete;

		public: ~t_SocketsUser(void)
		{
			auto cleanuped = ::WSACleanup();
			if(SOCKET_ERROR == cleanuped)
			{
				auto last_error = ::WSAGetLastError();
				DBG_UNREFERENCED_PARAMETER(last_error);
			}
		}

		private: void operator =(t_SocketsUser const &) = delete;
	};
}
}
}

#endif