#ifndef HEADER_VTT_INTERPROCESS_DETAILS_NET_BROKER
#define HEADER_VTT_INTERPROCESS_DETAILS_NET_BROKER

#pragma once

#include "../Configuration.hpp"

#include "UDP Multicast Socket.hpp"

#include <sal.h>

#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#include <map>
#include <memory>
#include <system_error>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_NetBroker
	{
		protected: typedef ::std::unique_ptr<t_UDPMulticastSocket> t_pUDPMulticastSocket;
		
		protected: typedef ::std::map<t_Address, t_pUDPMulticastSocket> t_sockets;

		#pragma region Fields

		private: t_sockets m_sockets;

		#pragma endregion

		public: t_NetBroker(void)
		{
			::WSADATA wsa_data;
			auto sockets_version = MAKEWORD(2, 2);
			auto result = ::WSAStartup(sockets_version, &wsa_data);
			if(ERROR_SUCCESS != result)
			{
				throw(::std::system_error(static_cast<int>(result), ::std::system_category(), "failed to initialize Windows Sockets"));
			}
		}

		private: t_NetBroker(t_NetBroker const &) = delete;

		public: ~t_NetBroker(void)
		{
			m_sockets.clear();
			auto cleanuped = ::WSACleanup();
			if(SOCKET_ERROR == cleanuped)
			{
				auto last_error = ::WSAGetLastError();
				DBG_UNREFERENCED_PARAMETER(last_error);
			}
		}

		private: void operator=(t_NetBroker const &) = delete;

		public: auto operator[](_In_ const t_Address & address) -> t_UDPMulticastSocket &
		{
			auto it_socket = m_sockets.find(address);
			if(m_sockets.end() == it_socket)
			{
				it_socket = m_sockets.insert
				(
					::std::make_pair
					(
						t_Address(address.Get_Host(), address.Get_Port(), true)
					,	t_pUDPMulticastSocket(new t_UDPMulticastSocket(address))
					)
				).first;
			}
			return(*(it_socket->second.get()));
		}
	};
}
}
}

#endif