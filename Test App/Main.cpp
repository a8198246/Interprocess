#include "Precompiled.hpp"

#include <sal.h>

#include <Windows.h>

#include <boost/lexical_cast.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <cassert>

#ifdef _DEBUG
#	pragma comment(lib, "libboost_system-vc120-mt-sgd-1_58")
#	pragma comment(lib, "libboost_thread-vc120-mt-sgd-1_58")
#else
#	pragma comment(lib, "libboost_system-vc120-mt-s-1_58")
#	pragma comment(lib, "libboost_thread-vc120-mt-s-1_58")
#endif

#ifdef RUNTIME_DYNAMIC_LINKING

#define VTT_INTERPROCESS_CALLING_CONVENTION __stdcall

void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_send         )(const int, char const *, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_receive      )(char * const, const int, const int);
void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_send_to_all  )(const int, char const *, const int);
void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_send          )(char const *, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_receive       )(const int, char * const, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_receive_common)(int, char * const, const int, const int, long long * const);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *udp_multicast_receive            )(wchar_t const *, const int, char * const, const int, const int, int * const);

#define VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT 65535

#else

#include <vtt/Interprocess.hpp>

#endif

#define APPLICATION_ID_MAX    12
#define WORK_TIME_SECONDS     30
#define BC_MESSAGE_MAX        20
#define BC_SOCKET_MESSAGE_MAX 512
#define SEND_RATE             1  // messages will be send once in m_send_rate times
#define INTERVAL_MSECONDS     100 // interval between recive / send attempts
#define BUFFER_SIZE           VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT * 100
#define GROUP                 L"224.0.0.108"
#define PORT                  12345

#define EVENT_ID              45

inline void
ErrorCode_To_String(_In_ const int error_code, _Inout_ ::std::wstring & report)
{
	report.push_back(L'#');
	report += ::boost::lexical_cast<::std::wstring>(error_code);
	report.push_back(L' ');
	report.push_back(L'\"');
	wchar_t * psz_error_description = nullptr;
	const ::DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
	auto const formatted_length = ::FormatMessageW(flags, NULL, error_code, NULL, reinterpret_cast<::LPWSTR>(&psz_error_description), NULL, NULL);
	if((0 < formatted_length) && (psz_error_description != nullptr))
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

bool g_continue_running = true;

::BOOL WINAPI
ConsoleHandler(::DWORD dwType)
{
	switch(dwType)
	{
		case CTRL_C_EVENT:
		{
			g_continue_running = false;
			break;
		}
	}
	return TRUE;
}

class
t_Worker
{
	private: typedef ::boost::mutex
	t_Mutex;

	private: typedef ::boost::lock_guard<t_Mutex>
	t_Lock;

	private: typedef ::boost::thread
	t_Thread;

	private: typedef ::std::vector<char>
	t_Buffer;

	#pragma region Fields

	private: static int           m_seed;             // shared rand seed used for message generation
	private: const bool           m_is_master = false;
	private: const int            m_appid;
	private: const ::DWORD        m_pid = ::GetCurrentProcessId();
	private: ::DWORD              m_tid = 0;
	private: t_Mutex &            m_sync;             // sync for ::std::cout and rand
	private: t_Buffer             m_buffer;
	private: ::std::string        m_last_message_text;
	private: ::std::map<int, int> m_appid_to_expected_received_message_index;
	private: ::std::map<int, int> m_appid_to_sent_message_couter;
	private: int                  m_common_sent_message_counter = 0;
	private: t_Thread             m_thread;

	#pragma endregion

	private:
	t_Worker(void) = delete;

	private:
	t_Worker(t_Worker const &) = delete;

	private:
	t_Worker(t_Worker &&) = delete;

	public:
	t_Worker(_In_ bool is_master, _In_ const int application_id, _In_ t_Mutex & cout_sync)
	:	m_is_master(is_master)
	,	m_appid(application_id)
	,	m_sync(cout_sync)
	,	m_buffer(BUFFER_SIZE)
	,	m_thread(&t_Worker::Routine, this)
	{
		//	Do nothing...
	}

	public: ~t_Worker(void)
	{
		m_thread.interrupt();
		m_thread.join();
	}

	private: void
	operator =(t_Worker const &) = delete;

	private: void
	operator =(t_Worker &&) = delete;

	private: void
	Routine(void)
	{
		m_tid = ::GetCurrentThreadId();
		{
			t_Lock lock(m_sync);
			::std::cout << "thread #" << ::boost::this_thread::get_id() << " started" << ::std::endl;
		}
		try
		{
			while(g_continue_running)
			{
				auto need_to_recieve = true;
				if(need_to_recieve)
				{
					Recieve();
				}

				auto need_to_send = (rand() % (m_is_master ? SEND_RATE : (SEND_RATE * 20))) == 0;
				if(need_to_send)
				{
					Send();
				}

				auto need_to_use_common = false;
				if(need_to_use_common)
				{
					Common();
				}

				auto need_to_recieve_from_socket = false;
				if(need_to_recieve_from_socket)
				{
					Recieve_From_Soket();
					::boost::this_thread::interruption_point();
				}

				::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(INTERVAL_MSECONDS));
			}
		}
		catch(::std::system_error & e)
		{
			t_Lock lock(m_sync);
			::std::cerr << "caught an exception " << e.what() << ": " << ::std::endl;
			::std::wcerr << ErrorCode_To_String(e.code().value()) << ::std::endl;
		}
	}

	private: void
	Common(void)
	{
		if(m_is_master)
		{
			auto need_to_send = (rand() % SEND_RATE) == 0;
			if(need_to_send)
			{
				Generate_Message(m_common_sent_message_counter);
				++m_common_sent_message_counter;
				::LARGE_INTEGER start_ts;
				::QueryPerformanceCounter(&start_ts);
				interprocess_master_send_to_all(EVENT_ID, m_buffer.data(), static_cast<int>(m_buffer.size()));
				::LARGE_INTEGER end_ts;
				::QueryPerformanceCounter(&end_ts);
				{
					t_Lock lock(m_sync);
					::std::cout << "\t >> thread #" << ::boost::this_thread::get_id() << " sent " << m_buffer.size() << " bytes to all the slaves at "
						<< start_ts.QuadPart << " - " << end_ts.QuadPart << " interval:" << ::std::endl;
					::std::cout.write(m_buffer.data() + 12, m_buffer.size() - 12);
					::std::cout.flush();
				}
			}
		}
		else
		{
			m_buffer.resize(BUFFER_SIZE);
			::LARGE_INTEGER start_ts;
			::QueryPerformanceCounter(&start_ts);
			auto const event_id = m_appid;
			auto bc_received = interprocess_slave_receive_common(event_id, m_buffer.data(), static_cast<int>(m_buffer.size()), 20000, nullptr);
			::LARGE_INTEGER end_ts;
			::QueryPerformanceCounter(&end_ts);
			{
				t_Lock lock(m_sync);
				if(0 != bc_received)
				{
					::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " received " << bc_received << " bytes at "
						<< start_ts.QuadPart << " - " << end_ts.QuadPart << " interval" << ::std::endl;
					::std::cout.write(m_buffer.data() + 12, bc_received - 12);
				}
				else
				{
					::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " tried to receive at "
						<< start_ts.QuadPart << " - " << end_ts.QuadPart << " interval" << ::std::endl;
				}
				::std::cout.flush();
			}
		}
	}

	private: void
	Recieve_From_Soket(void)
	{
		m_buffer.resize(BUFFER_SIZE);
		int error_code;
		size_t bc_received = static_cast<size_t>(udp_multicast_receive(GROUP, PORT, m_buffer.data(), static_cast<int>(m_buffer.size()), 100, &error_code));
		assert(bc_received <= BUFFER_SIZE);
		if(ERROR_SUCCESS != error_code)
		{
			throw(::std::system_error(error_code, ::std::system_category(), "udp_multicast_receive failed"));
		}
		if(0 != bc_received)
		{
			t_Lock lock(m_sync);
			::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " received " << bc_received << " bytes from UDP socket:" << ::std::endl;
			::std::cout.write(m_buffer.data(), bc_received);
			::std::cout.flush();
		}
		else
		{
			t_Lock lock(m_sync);
			::std::cout << "\t << received nothing..." << ::std::endl;
		}
	}

	private: void
	Recieve(void)
	{
		m_buffer.resize(BUFFER_SIZE);
		size_t bc_received = 0;
		static int attempt = 0;
		if(m_is_master)
		{
		//	t_Lock lock(m_sync);
		//	::std::cout << " #" << attempt << " attempt to recieve" << ::std::endl;
		}
		++attempt;
		if(m_is_master)
		{
			bc_received = static_cast<size_t>(interprocess_master_receive(m_buffer.data(), 1000, 2000));
		}
		else
		{
			bc_received = static_cast<size_t>(interprocess_slave_receive(m_appid, m_buffer.data(), static_cast<int>(m_buffer.size())));
		}
		assert(bc_received <= BUFFER_SIZE);
		if(0 != bc_received)
		{
			assert(13 <= bc_received); // 4 bytes pid 4 bytes messages number 4 bytes message length + 1 symbol + \n
			t_Lock lock(m_sync);
			int offset = 0;
			while(offset != static_cast<int>(bc_received))
			{
				assert(offset + sizeof(int) < static_cast<int>(bc_received));
				auto appid = *reinterpret_cast<int *>(m_buffer.data() + offset);
				offset += sizeof(int);
				m_appid_to_expected_received_message_index.insert(::std::make_pair(appid, 0));

				assert(offset + sizeof(int) < static_cast<int>(bc_received));
				auto message_index = *reinterpret_cast<int *>(m_buffer.data() + offset);
				offset += sizeof(int);

				assert(offset + sizeof(int) < static_cast<int>(bc_received));
				auto bc_message = *reinterpret_cast<int *>(m_buffer.data() + offset);
				offset += sizeof(int);

				assert(offset + bc_message <= static_cast<int>(bc_received));

				auto it_expected_message_index = m_appid_to_expected_received_message_index.find(appid);
				auto & expected_message_index = it_expected_message_index->second;
				if(expected_message_index != message_index)
				{
					::std::cout << "ERROR: message index mismatch, expected " << expected_message_index << " got " << message_index << " from " << appid << ::std::endl;
				}
				expected_message_index = message_index + 1;

				m_last_message_text.assign(m_buffer.data() + offset, bc_message);
				offset += bc_message;
				assert(offset <= static_cast<int>(bc_received));

				::std::cout << "<<  tid#" << m_tid << " app#" << m_appid <<
					" received " << (sizeof(int) * 3 + bc_message) << " bytes" <<
					" from " "app#" << appid << " in message #" << ::std::setw(2) << message_index << " :" << m_last_message_text << ::std::endl;
			}
		}
	}

	private: void
	Send(void)
	{
		auto const target_application_id = m_is_master ? (rand() % (APPLICATION_ID_MAX)) : 0;
		auto const insertion_result = m_appid_to_sent_message_couter.insert(::std::make_pair(target_application_id, 0));
		auto & last_sent_message_index(insertion_result.first->second);
		Generate_Message(last_sent_message_index);
		{
			t_Lock lock(m_sync);
			::std::cout << ">>  tid#" << m_tid << " app#" << m_appid << " sends "
				"message #" << ::std::setw(2) << last_sent_message_index << " of " << m_buffer.size() << " bytes "
				"to " << target_application_id << ": " << m_last_message_text << ::std::endl;
		}
		++last_sent_message_index;
		if(m_is_master)
		{
			interprocess_master_send(target_application_id, m_buffer.data(), static_cast<int>(m_buffer.size()));
		}
		else
		{
			interprocess_slave_send(m_buffer.data(), static_cast<int>(m_buffer.size()));
		}
	}

	private: void
	Generate_Message(_In_ const int sent_messages_counter)
	{
		t_Lock lock(m_sync); // otherwise different threads will spam the same messages
		srand(m_seed);
		auto const bc_message = 1 + (rand() % BC_MESSAGE_MAX);
		m_last_message_text.resize(bc_message);
		::std::generate(m_last_message_text.begin(), m_last_message_text.end(), [](void) -> char {return(static_cast<char>('a' + (rand() % 26)));});

		m_buffer.resize(sizeof(int) * 3 + bc_message);
		*reinterpret_cast<int *>(m_buffer.data() + sizeof(int) * 0) = static_cast<int>(m_appid);
		*reinterpret_cast<int *>(m_buffer.data() + sizeof(int) * 1) = sent_messages_counter;
		*reinterpret_cast<int *>(m_buffer.data() + sizeof(int) * 2) = bc_message;
		memcpy(m_buffer.data() + sizeof(int) * 3, m_last_message_text.data(), m_last_message_text.size());

		m_seed = rand();
	}
};

int t_Worker::m_seed;

int
main(int argc, char * args[], char *[])
{
	setlocale(LC_CTYPE, ".866");
	srand(42);

	auto const console_ctrl_handler_set(::SetConsoleCtrlHandler(ConsoleHandler, TRUE));
	DBG_UNREFERENCED_LOCAL_VARIABLE(console_ctrl_handler_set);
	assert(FALSE != console_ctrl_handler_set);

#ifdef RUNTIME_DYNAMIC_LINKING
	auto h_interprocess_library = ::LoadLibraryW(L"Interprocess.dll");
	if(NULL != h_interprocess_library)
	{
		interprocess_master_send          = reinterpret_cast<decltype(interprocess_master_send         )>(::GetProcAddress(h_interprocess_library, "interprocess_master_send"         ));
		interprocess_master_receive       = reinterpret_cast<decltype(interprocess_master_receive      )>(::GetProcAddress(h_interprocess_library, "interprocess_master_receive"      ));
		interprocess_master_send_to_all   = reinterpret_cast<decltype(interprocess_master_send_to_all  )>(::GetProcAddress(h_interprocess_library, "interprocess_master_send_to_all"  ));
		interprocess_slave_send           = reinterpret_cast<decltype(interprocess_slave_send          )>(::GetProcAddress(h_interprocess_library, "interprocess_slave_send"          ));
		interprocess_slave_receive        = reinterpret_cast<decltype(interprocess_slave_receive       )>(::GetProcAddress(h_interprocess_library, "interprocess_slave_receive"       ));
		interprocess_slave_receive_common = reinterpret_cast<decltype(interprocess_slave_receive_common)>(::GetProcAddress(h_interprocess_library, "interprocess_slave_receive_common"));
		udp_multicast_receive             = reinterpret_cast<decltype(udp_multicast_receive            )>(::GetProcAddress(h_interprocess_library, "udp_multicast_receive"            ));
		assert(nullptr != interprocess_master_send         );
		assert(nullptr != interprocess_master_receive      );
		assert(nullptr != interprocess_master_send_to_all  );
		assert(nullptr != interprocess_slave_send          );
		assert(nullptr != interprocess_slave_receive       );
		assert(nullptr != interprocess_slave_receive_common);
		assert(nullptr != udp_multicast_receive            );
	}
	else
	{
		::std::cerr << "failed to load " "Interprocess.dll";
		return(ERROR_APP_INIT_FAILURE);
	}
#endif

	auto const this_process_is_master(1 == argc);
	if(this_process_is_master)
	{
		::std::cout << "master process started" << ::std::endl;
		::boost::mutex sync;
		t_Worker worker(this_process_is_master, 0, sync);
		::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(WORK_TIME_SECONDS * 1000));
	}
	else
	{
		::std::cout << "slave process started" << ::std::endl;
		int application_id;
		try
		{
			application_id = ::boost::lexical_cast<decltype(application_id)>(args[1]);
			::std::cout <<  "application_id = " << args[1] << ::std::endl;
		}
		catch(::boost::bad_lexical_cast &)
		{
			::std::cout <<  "application_id \"" << args[1] << "\" is invalid" << ::std::endl;
			return(ERROR_APP_INIT_FAILURE);
		}
		if(APPLICATION_ID_MAX < application_id)
		{
			::std::cout <<  "application_id \"" << args[1] << "\" is invalid, must be within 0 - " << APPLICATION_ID_MAX << " range" << ::std::endl;
			return(ERROR_APP_INIT_FAILURE);
		}
		::boost::mutex sync;
		t_Worker worker1(this_process_is_master, application_id, sync);
		t_Worker worker2(this_process_is_master, application_id + 1, sync);
		t_Worker worker3(this_process_is_master, application_id + 2, sync);
		::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(WORK_TIME_SECONDS * 1000));
	}

#ifdef RUNTIME_DYNAMIC_LINKING
	auto const unloaded = ::FreeLibrary(h_interprocess_library);
	DBG_UNREFERENCED_LOCAL_VARIABLE(unloaded);
	assert(FALSE != unloaded);
	h_interprocess_library = NULL;
#endif
	getchar();
	return(0);
}