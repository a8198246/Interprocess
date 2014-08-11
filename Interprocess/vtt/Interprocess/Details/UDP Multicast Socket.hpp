#ifndef HEADER_VTT_INTERPROCESS_DETAILS_UDP_MULTICAST_SOCKET
#define HEADER_VTT_INTERPROCESS_DETAILS_UDP_MULTICAST_SOCKET

#pragma once

#include "../Configuration.hpp"

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
namespace n_details
{
	class t_Address
	{
		#pragma region Fields

		protected: const wchar_t * m_psz_host = nullptr;
		protected: unsigned short  m_port = 0;
		protected: bool            m_owns_ptr = false;

		#pragma endregion

		private: t_Address(void) = delete;

		public: t_Address(_In_z_ wchar_t const * psz_host, _In_ const unsigned short port, _In_ const bool copy)
		:	m_port(port)
		,	m_owns_ptr(copy)
		{
			assert(nullptr != psz_host);
			assert(L'\0' != psz_host);
			if(copy)
			{
				m_psz_host = Copy(psz_host);
			}
			else
			{
				m_psz_host = psz_host;
			}
		}

		private: t_Address(t_Address const & that) = delete;

		public: t_Address(t_Address && that)
		:	m_psz_host(that.m_psz_host)
		,	m_port(that.m_port)
		,	m_owns_ptr(that.m_owns_ptr)
		{
			that.m_psz_host = nullptr;
			that.m_port     = 0;
			that.m_owns_ptr = false;
		}

		private: void operator=(t_Address const &) = delete;

		public: ~t_Address(void)
		{
			if(m_owns_ptr)
			{
				delete[](m_psz_host);
			}
			m_psz_host = nullptr;
			m_port = 0;
			m_owns_ptr = false;
		}

		public: auto Get_Host(void) const throw() -> wchar_t const *
		{
			return(m_psz_host);
		}

		public: auto Get_Port(void) const throw() -> unsigned short const &
		{
			return(m_port);
		}

		friend auto operator<(t_Address const & left, t_Address const & right) throw() -> bool
		{
			auto cmp = wcscmp(left.m_psz_host, right.m_psz_host);
			if(0 != cmp)
			{
				return(cmp < 0);
			}
			else
			{
				return(left.m_port < right.m_port);
			}
		}

		private: static auto Copy(_In_z_ wchar_t const * psz_host) ->  wchar_t const *
		{
			auto cch_host = ::wcslen(psz_host) + 1;
			auto psz_copy = new wchar_t[cch_host];
			::memcpy(psz_copy, psz_host, sizeof(wchar_t) * cch_host);
			return(psz_copy);
		}
	};

	class t_UDPMulticastSocket
	{
		#pragma region Fields

		private: ::SOCKET           m_socket = INVALID_SOCKET;
		private: struct sockaddr_in m_addr;
		private: WSAEVENT           m_net_event = ::WSACreateEvent();;

		#pragma endregion

		private: t_UDPMulticastSocket(void) = delete;

		public: explicit t_UDPMulticastSocket(t_Address const & address)
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
				localAddr.sin_port        = ::htons(address.Get_Port());
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
				auto it_char = address.Get_Host();
				do
				{
					multicast_group.push_back(static_cast<char>(*it_char));
					++it_char;
				}
				while(L'\0' != *it_char);
				struct ip_mreq multicastRequest;
				multicastRequest.imr_multiaddr.s_addr = ::inet_addr(multicast_group.c_str());
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

		private: void operator=(t_UDPMulticastSocket const &) = delete;

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