#define BOOST_ALL_NO_LIB

#define RUNTIME_DYNAMIC_LINKING

#include <vld.h>

#include <sal.h>

#include <Windows.h>

#include <random>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#ifdef _DEBUG
#	pragma comment(lib, "boost_thread-vc120-mt-gd-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-gd-1_55.lib")
#else
#	pragma comment(lib, "boost_thread-vc120-mt-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-1_55.lib")
#endif

#ifdef RUNTIME_DYNAMIC_LINKING

#define VTT_INTERPROCESS_CALLING_CONVENTION __stdcall

void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_send         )(const int, char const *, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_recieve      )(char *, const int, const int);
void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_master_send_to_all  )(char *, const int);
void (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_send          )(char const *, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_recieve       )(const int, char *, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *interprocess_slave_recieve_common)(char *, const int, const int);
int  (VTT_INTERPROCESS_CALLING_CONVENTION *udp_multicast_recieve            )(wchar_t const *, const int, char *, const int, const int, int *);

#define VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT 65535

#else

#include <vtt/Interprocess.hpp>

#endif

#define APPLICATION_ID_MAX    100
#define WORK_TIME_SECONDS     30
#define BC_MESSAGE_MAX        128
#define BC_SOCKET_MESSAGE_MAX 512
#define SEND_RATE             20  // messages will be send once in m_send_rate times
#define INTERVAL_MSECONDS     100 // interval between recive / send attempts
#define BUFFER_SIZE           512
#define GROUP                 L"224.0.0.108"
#define PORT                  12345

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

class t_Worker
{
	private: typedef ::boost::mutex t_Mutex;

	private: typedef ::boost::lock_guard<t_Mutex> t_Lock;

	private: typedef ::boost::thread t_Thread;

	private: typedef ::std::vector<char> t_Buffer;

	#pragma region Fields

	private: static int m_seed;             // shared rand seed used for message generation
	private: const bool m_is_master = false;
	private: const int  m_application_id;
	private: t_Mutex &  m_sync;             // sync for ::std::cout and rand
	private: t_Buffer   m_buffer;
	private: t_Thread   m_thread;
	
	#pragma endregion

	private: t_Worker(void) = delete;

	public: t_Worker(_In_ bool is_master, _In_ const int application_id, _In_ t_Mutex & cout_sync)
	:	m_is_master(is_master)
	,	m_application_id(application_id)
	,	m_sync(cout_sync)
	,	m_buffer(BUFFER_SIZE)
	{
		m_thread = ::boost::thread([this](){Routine();});
	}

	private: t_Worker(t_Worker const &) = delete;

	private: void operator=(t_Worker const &) = delete;

	public: ~t_Worker(void)
	{
		m_thread.interrupt();
		m_thread.join();
	}

	private: void Routine(void)
	{
		{
			t_Lock lock(m_sync);
			::std::cout << "thread #" << ::boost::this_thread::get_id() << " started" << ::std::endl;
		}
		try
		{
			for(;;)
			{
				//auto need_to_recieve_from_socket = true;
				//if(need_to_recieve_from_socket)
				//{
				//	Recieve_From_Soket();
				//	::boost::this_thread::interruption_point();
				//}
				//auto need_to_recieve = true;
				//if(need_to_recieve)
				//{
				//	Recieve();
				//}
				//auto need_to_send = (rand() % SEND_RATE) == 0;
				//if(need_to_send)
				//{
				//	Send();
				//}
				Common();
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

	private: void Common(void)
	{
		if(m_is_master)
		{
			auto need_to_send = (rand() % SEND_RATE) == 0;
			if(need_to_send)
			{
				Generate_Message();
				{
					t_Lock lock(m_sync);
					::std::cout << "\t >> thread #" << ::boost::this_thread::get_id() << " sends " << m_buffer.size() << " bytes to all the slaves:" << ::std::endl;
					::std::cout.write(m_buffer.data(), m_buffer.size());
					::std::cout.flush();
				}
				interprocess_master_send_to_all(m_buffer.data(), static_cast<int>(m_buffer.size()));
			}
		}
		else
		{
			m_buffer.resize(BUFFER_SIZE);
			auto bc_recieved = interprocess_slave_recieve_common(m_buffer.data(), static_cast<int>(m_buffer.size()), 50);
			if(0 != bc_recieved)
			{
				t_Lock lock(m_sync);
				::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " recieved " << bc_recieved << " bytes:" << ::std::endl;
				::std::cout.write(m_buffer.data(), bc_recieved);
				::std::cout.flush();
			}
		}
	}

	private: void Recieve_From_Soket(void)
	{
		m_buffer.resize(BUFFER_SIZE);
		int error_code;
		size_t bc_recieved = static_cast<size_t>(udp_multicast_recieve(GROUP, PORT, m_buffer.data(), static_cast<int>(m_buffer.size()), 100, &error_code));
		assert(bc_recieved <= BUFFER_SIZE);
		if(ERROR_SUCCESS != error_code)
		{
			throw(::std::system_error(error_code, ::std::system_category(), "udp_multicast_recieve failed"));
		}
		if(0 != bc_recieved)
		{
			t_Lock lock(m_sync);
			::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " recieved " << bc_recieved << " bytes from UDP socket:" << ::std::endl;
			::std::cout.write(m_buffer.data(), bc_recieved);
			::std::cout.flush();
		}
		else
		{
			t_Lock lock(m_sync);
			::std::cout << "\t << recieved nothing..." << ::std::endl;
		}
	}

	private: void Recieve(void)
	{
		m_buffer.resize(BUFFER_SIZE);
		size_t bc_recieved = 0;
		static int attempt = 0;
		if(m_is_master)
		{
			t_Lock lock(m_sync);
			::std::cout << "\t #" << attempt << " attempt to recieve" << ::std::endl;
		}
		++attempt;
		if(m_is_master)
		{
			bc_recieved = static_cast<size_t>(interprocess_master_recieve(m_buffer.data(), static_cast<int>(m_buffer.size()), 100));
		}
		else
		{
			bc_recieved = static_cast<size_t>(interprocess_slave_recieve(m_application_id, m_buffer.data(), static_cast<int>(m_buffer.size())));
		}
		assert(bc_recieved <= BUFFER_SIZE);
		if(0 != bc_recieved)
		{
			t_Lock lock(m_sync);
			::std::cout << "\t << thread #" << ::boost::this_thread::get_id() << " recieved " << bc_recieved << " bytes:" << ::std::endl;
			::std::cout.write(m_buffer.data(), bc_recieved);
			::std::cout.flush();
		}
	}

	private: void Send(void)
	{
		Generate_Message();
		if(m_is_master)
		{
			auto target_application_id = (rand() % (APPLICATION_ID_MAX + 1));
			{
				t_Lock lock(m_sync);
				::std::cout << "\t >> thread #" << ::boost::this_thread::get_id() << " sends " << m_buffer.size() << " bytes to " << target_application_id << ":" << ::std::endl;
				::std::cout.write(m_buffer.data(), m_buffer.size());
				::std::cout.flush();
			}
			interprocess_master_send(target_application_id, m_buffer.data(), static_cast<int>(m_buffer.size()));
		}
		else
		{
			{
				t_Lock lock(m_sync);
				::std::cout << "\t >> thread #" << ::boost::this_thread::get_id() << " sends " << m_buffer.size() << " bytes to master:" << ::std::endl;
				::std::cout.write(m_buffer.data(), m_buffer.size());
				::std::cout.flush();
			}
			interprocess_slave_send(m_buffer.data(), static_cast<int>(m_buffer.size()));
		}
	}

	private: void Generate_Message(void)
	{
		m_buffer.clear();
		t_Lock lock(m_sync); // otherwise different threads will spam the same messages
		srand(m_seed);
		auto const bc_message = (rand() % BC_MESSAGE_MAX);
		if(0 != bc_message)
		{
			m_buffer.resize(bc_message);
			::std::generate(m_buffer.begin(), m_buffer.end(), [](void) -> char {return(static_cast<char>('a' + (rand() % 26)));});
			m_buffer.back() = '\n';
		}
		m_seed = rand();
	}
};

int t_Worker::m_seed;

int main(int argc, char * args[], char *[])
{
	setlocale(LC_CTYPE, ".866");
	srand(42);

#ifdef RUNTIME_DYNAMIC_LINKING
	auto h_interprocess_library = ::LoadLibraryW(L"Interprocess.dll");
	if(NULL != h_interprocess_library)
	{
		interprocess_master_send          = reinterpret_cast<decltype(interprocess_master_send         )>(::GetProcAddress(h_interprocess_library, "interprocess_master_send"         ));
		interprocess_master_recieve       = reinterpret_cast<decltype(interprocess_master_recieve      )>(::GetProcAddress(h_interprocess_library, "interprocess_master_recieve"      ));
		interprocess_master_send_to_all   = reinterpret_cast<decltype(interprocess_master_send_to_all  )>(::GetProcAddress(h_interprocess_library, "interprocess_master_send_to_all"  ));
		interprocess_slave_send           = reinterpret_cast<decltype(interprocess_slave_send          )>(::GetProcAddress(h_interprocess_library, "interprocess_slave_send"          ));
		interprocess_slave_recieve        = reinterpret_cast<decltype(interprocess_slave_recieve       )>(::GetProcAddress(h_interprocess_library, "interprocess_slave_recieve"       ));
		interprocess_slave_recieve_common = reinterpret_cast<decltype(interprocess_slave_recieve_common)>(::GetProcAddress(h_interprocess_library, "interprocess_slave_recieve_common"));
		udp_multicast_recieve             = reinterpret_cast<decltype(udp_multicast_recieve            )>(::GetProcAddress(h_interprocess_library, "udp_multicast_recieve"            ));
		assert(nullptr != interprocess_master_send         );
		assert(nullptr != interprocess_master_recieve      );
		assert(nullptr != interprocess_master_send_to_all  );
		assert(nullptr != interprocess_slave_send          );
		assert(nullptr != interprocess_slave_recieve       );
		assert(nullptr != interprocess_slave_recieve_common);		
		assert(nullptr != udp_multicast_recieve            );
	}
	else
	{
		::std::cerr << "failed to load " "Interprocess.dll";
		return(ERROR_APP_INIT_FAILURE);
	}
#endif

	auto is_master = (1 == argc);
	if(is_master)
	{
		::std::cout << "master process started" << ::std::endl;
	}
	else
	{
		::std::cout << "slave process started" << ::std::endl;
	}
	int application_id;
	if(is_master)
	{
		application_id = 0;
	}
	else
	{
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
	}
	
	{
		::boost::mutex sync;
		t_Worker worker(is_master, application_id, sync);
		::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(WORK_TIME_SECONDS * 1000));
	}

#ifdef RUNTIME_DYNAMIC_LINKING
	auto unloaded = ::FreeLibrary(h_interprocess_library);
	DBG_UNREFERENCED_LOCAL_VARIABLE(unloaded);
	assert(FALSE != unloaded);
	h_interprocess_library = NULL;
#endif
	return(0);
}