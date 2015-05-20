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

#define WINVER         0x0601
#define _WIN32_WINDOWS 0x0601
#define _WIN32_WINNT   0x0601
#define _WIN32_IE      0x0800
#define NTDDI_VERSION  NTDDI_WIN7

#define BOOST_ALL_NO_LIB

#include <vld.h>

#include <sal.h>

#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

#define GROUP             "224.0.0.108"
#define PORT              12345
#define BC_MESSAGE_MAX    512
#define SEND_RATE         1   // messages will be send once in m_send_rate times
#define INTERVAL_MSECONDS 1000 // interval between recive / send attempts
#define BUFFER_SIZE       512

inline void
ErrorCode_To_String(_In_ const int error_code, _Inout_ ::std::wstring & report)
{
	report.push_back(L'#');
	report += ::boost::lexical_cast<::std::wstring>(error_code);
	report.push_back(L' ');
	report.push_back(L'\"');
	auto psz_error_description = static_cast<wchar_t *>(nullptr);
	auto flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
	auto formatted = ::FormatMessageW(flags, NULL, error_code, NULL, reinterpret_cast<::LPWSTR>(&psz_error_description), NULL, NULL);
	DBG_UNREFERENCED_LOCAL_VARIABLE(formatted);
	if(psz_error_description != nullptr)
	{
		report += psz_error_description;
		::LocalFree(reinterpret_cast<::HLOCAL>(psz_error_description));
		psz_error_description = nullptr;
	}
	else
	{
		report += L"(failed to retrieve error description)";
	}
	while((L'\r' == report.back()) || (L'\n' == report.back()))
	{
		report.pop_back();
	}
	report.push_back(L'\"');
}

inline auto
ErrorCode_To_String(_In_ const int error_code) -> ::std::wstring
{
	::std::wstring report;
	ErrorCode_To_String(error_code, report);
	return(::std::move(report));
}

auto g_need_to_stop = false;

class t_Worker
{
	private: typedef ::std::vector<char> t_Buffer;

	#pragma region Fields

	private: t_Buffer           m_buffer;
	private: ::SOCKET           m_socket = INVALID_SOCKET;
	private: struct sockaddr_in m_addr;
	private: int &              m_result;
	
	#pragma endregion

	private: t_Worker(void) = delete;

	public: explicit t_Worker(int & result)
	:	m_buffer(BC_MESSAGE_MAX)
	,	m_result(result)
	{
		//	create socket
		{
			m_socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if(INVALID_SOCKET == m_socket)
			{
				m_result = ::WSAGetLastError();
				::std::wcerr << L"failed to create socket: " << ErrorCode_To_String(m_result) << ::std::endl;
				g_need_to_stop = true;
			}
		}
		//	enable multicasting
		{
			unsigned char multicastTTL = 1;
			auto option_set = ::setsockopt(m_socket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char *>(&multicastTTL), sizeof(multicastTTL));
			if(ERROR_SUCCESS != option_set)
			{
				m_result = ::WSAGetLastError();
				::std::wcerr << L"failed to set socket options: " << ErrorCode_To_String(m_result) << ::std::endl;
				g_need_to_stop = true;
			}
		}
		//	fill address struct
		{
			::memset(&m_addr, 0, sizeof(m_addr));
			m_addr.sin_family = AF_INET;
			::inet_pton(AF_INET, GROUP, &m_addr.sin_addr.s_addr);
			m_addr.sin_port = ::htons(PORT);
		}
	}

	private: t_Worker(t_Worker const &) = delete;

	private: void operator=(t_Worker const &) = delete;

	public: ~t_Worker(void)
	{
		if(INVALID_SOCKET != m_socket)
		{
			auto closed = ::closesocket(m_socket);
			m_socket = INVALID_SOCKET;
			if(ERROR_SUCCESS != closed)
			{
				m_result = ::WSAGetLastError();
				::std::wcerr << L"failed to close socket: " << ErrorCode_To_String(m_result) << ::std::endl;
				g_need_to_stop = true;
			}
		}
	}

	public: void Routine(void)
	{
		while(!g_need_to_stop)
		{
			::Sleep(INTERVAL_MSECONDS);
			auto need_to_send = (rand() % SEND_RATE) == 0;
			if(need_to_send)
			{
				Send();
			}
		}	
	}

	private: void Send(void)
	{
		Generate_Message();
		auto const bc_message = static_cast<int>(m_buffer.size());
		auto bc_sent = ::sendto(m_socket, m_buffer.data(), bc_message, 0, reinterpret_cast<sockaddr const *>(&m_addr), sizeof(m_addr));
		if(SOCKET_ERROR == bc_sent)
		{
			m_result = ::WSAGetLastError();
			::std::wcerr << L"failed to send data: " << ErrorCode_To_String(m_result) << ::std::endl;
			g_need_to_stop = true;
			return;
		}
		if(bc_message != bc_sent)
		{
			m_result = ::WSAGetLastError();
			::std::wcerr << L"failed to send data: " << ErrorCode_To_String(m_result) << L", only " << bc_sent << L" bytes out of " << bc_message << L" bytes were sent" << ::std::endl;
			g_need_to_stop = true;
			return;
		}
		::std::wcout << L"\tsuccessfully sent " << bc_message << L" bytes:" << ::std::endl;
		::std::cout.write(m_buffer.data(), bc_message);
		::std::cout.flush();
	}

	private: void Generate_Message(void)
	{
		m_buffer.clear();
		auto const bc_message = (rand() % BC_MESSAGE_MAX);
		if(0 != bc_message)
		{
			m_buffer.resize(bc_message);
			::std::generate(m_buffer.begin(), m_buffer.end(), [](void) -> char {return(static_cast<char>('a' + (rand() % 26)));});
			m_buffer.back() = '\n';
		}
	}
};

::BOOL WINAPI ConsoleHandler(::DWORD CEvent)
{
	switch(CEvent)
	{
		case CTRL_C_EVENT:
		{
			g_need_to_stop = true;
			break;
		}
	}
	return(TRUE);
}

int main(int, char *[], char *[])
{
	int result = ERROR_SUCCESS;
	//	some preparations
	{
		setlocale(LC_CTYPE, ".866");
		srand(42);
		::SetConsoleCtrlHandler(ConsoleHandler,TRUE);
	}
	//	initialize win sockets
	{
		::WSADATA wsa_data;
		auto sockets_version = MAKEWORD(2, 2);
		auto result = ::WSAStartup(sockets_version, &wsa_data);
		if(ERROR_SUCCESS != result)
		{
			::std::wcerr << L"failed to init Windows Sockets: " << ErrorCode_To_String(result) << ::std::endl;
			::std::wcout << L"press any key to exit..." << ::std::endl;
			::std::getchar();
			return(result);
		}
	}
	//	work...
	try
	{
		t_Worker worker(result);
		worker.Routine();
	}
	catch(...)
	{
		//	Do nothing
	}
	//	finalize win sockets
	{
		auto cleanuped = ::WSACleanup();
		if(ERROR_SUCCESS != cleanuped)
		{
			result =  ::WSAGetLastError();
			::std::wcerr << L"failed to shutdown Windows Sockets: " << ErrorCode_To_String(result) << ::std::endl;
		}
	}
	::std::wcout << L"press any key to exit..." << ::std::endl;
	::std::getchar();
	return(result);
}