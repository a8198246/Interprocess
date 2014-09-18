#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_UDP_MULTICAST_SOCKET
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_UDP_MULTICAST_SOCKET

#pragma once

#include <sal.h>

#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#include <cstring>
#include <cassert>
#include <system_error>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_UDPMulticastSocket
	{
		#pragma region Fields

		private: ::SOCKET           m_socket = INVALID_SOCKET;
		private: struct sockaddr_in m_addr;
		private: WSAEVENT           m_net_event = ::WSACreateEvent();;

		#pragma endregion

		private: t_UDPMulticastSocket(void) = delete;

		public: explicit t_UDPMulticastSocket(_In_z_ const wchar_t * m_psz_host, _In_ const unsigned short port)
		{
			//	create socket
			{
				m_socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if(INVALID_SOCKET == m_socket)
				{
					auto last_error = ::WSAGetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to create a socket"));
				}
			}
			//	bind socket to local port
			{
				::sockaddr_in localAddr;
				::memset(&localAddr, 0, sizeof(localAddr));
				localAddr.sin_family      = AF_INET;
				localAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
				localAddr.sin_port        = ::htons(port);
				auto bind_result = bind(m_socket, reinterpret_cast<sockaddr *>(&localAddr), sizeof(sockaddr_in));
				if(SOCKET_ERROR == bind_result)
				{
					auto last_error = ::WSAGetLastError();
					Close();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to bind a socket to local port"));
				}
			}
			//	join multicast group
			{
				::std::string multicast_group;
				auto it_char = m_psz_host;
				do
				{
					multicast_group.push_back(static_cast<char>(*it_char));
					++it_char;
				}
				while(L'\0' != *it_char);
				struct ip_mreq multicastRequest;
				{
					auto conversion_result = ::inet_pton(AF_INET, multicast_group.c_str(), &(multicastRequest.imr_multiaddr.s_addr));
					DBG_UNREFERENCED_LOCAL_VARIABLE(conversion_result);
					assert(1 == conversion_result);
				}
				multicastRequest.imr_interface.s_addr = ::htonl(INADDR_ANY);
				auto option_set = setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char const *>(&multicastRequest), sizeof(multicastRequest));
				if(SOCKET_ERROR == option_set)
				{
					auto last_error = ::WSAGetLastError();
					Close();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to set socket options"));
				}
			}
			//
			{
				auto selection_result = ::WSAEventSelect(m_socket, m_net_event, FD_READ);
				if(SOCKET_ERROR == selection_result)
				{
					auto last_error = ::WSAGetLastError();
					Close();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to set net event"));
				}
			}
		}

		private: t_UDPMulticastSocket(t_UDPMulticastSocket const &) = delete;

		public: ~t_UDPMulticastSocket(void)
		{
			Close();
		}

		private: void operator =(t_UDPMulticastSocket const &) = delete;

		protected: void Close(void)
		{
			if(INVALID_SOCKET != m_socket)
			{
				auto closed = ::closesocket(m_socket);
				m_socket = INVALID_SOCKET;
				if(ERROR_SUCCESS != closed)
				{
					auto last_error = ::WSAGetLastError();
					DBG_UNREFERENCED_PARAMETER(last_error);
				}
			}
			if(WSA_INVALID_EVENT != m_net_event)
			{
				auto closed = ::WSACloseEvent(m_net_event);
				DBG_UNREFERENCED_LOCAL_VARIABLE(closed);
				assert(FALSE != closed);
			}
		}

		public: auto Recieve(char * p_buffer, _In_ const int bc_buffer, _In_ const int timeout_msec) -> int
		{
			auto bc_written = 0;
			auto wait_result = ::WSAWaitForMultipleEvents(1, &m_net_event, 1, timeout_msec, FALSE);
			switch(wait_result)
			{
				case WSA_WAIT_EVENT_0:
				{
					bc_written = ::recvfrom(m_socket, p_buffer, bc_buffer, 0, static_cast<sockaddr*>(nullptr), 0);
					if(SOCKET_ERROR == bc_written)
					{
						auto last_error = ::WSAGetLastError();
						throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to recieve data"));
					}
					{
						auto reset = ::WSAResetEvent(m_net_event);
						DBG_UNREFERENCED_LOCAL_VARIABLE(reset);
						assert(FALSE != reset);
					}
					break;
				}
				case WSA_WAIT_FAILED:
				{
					auto last_error = ::WSAGetLastError();
					throw(::std::system_error(static_cast<int>(last_error), ::std::system_category(), "failed to wait for net event"));
				}
				default:
				{
					//	Do nothing
					break;	
				}
			}
			return(bc_written);
		}
	};
}
}
}

#endif