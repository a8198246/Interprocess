#define BOOST_ALL_NO_LIB

#include <vld.h>

#include <sal.h>

#include <random>
#include <vector>
#include <algorithm>
#include <iostream>
#include <memory>
#include <cassert>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#include <vtt/Interprocess.hpp>

#ifdef _DEBUG
#	pragma comment(lib, "boost_thread-vc120-mt-gd-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-gd-1_55.lib")
#else
#	pragma comment(lib, "boost_thread-vc120-mt-1_55.lib")
#	pragma comment(lib, "boost_system-vc120-mt-1_55.lib")
#endif

#define EC_WORKERS_MAX    8
#define WORK_TIME_SECONDS 10
#define BC_MESSAGE_MAX    128
#define SEND_RATE         5   // messages will be send once in m_send_rate times
#define INTERVAL_MSECONDS 100 // interval between recive and send attempts

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
	,	m_buffer(VTT_INTERPROCESS_BC_MESSAGE_LIMIT)
	{
		m_thread = ::boost::thread([this](){Routine();});
	}

	private: t_Worker(t_Worker const &) = delete;

	private: void operator = (t_Worker const &) = delete;

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
		for(;;)
		{
			auto need_to_recieve = true;
			if(need_to_recieve)
			{
				Recieve();
			}
			auto need_to_send = (rand() % SEND_RATE) == 0;
			if(need_to_send)
			{
				Send();
			}
			::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(INTERVAL_MSECONDS));
		}
	}

	private: void Recieve(void)
	{
		m_buffer.resize(VTT_INTERPROCESS_BC_MESSAGE_LIMIT);
		size_t bc_recieved;
		if(m_is_master)
		{
			bc_recieved = static_cast<size_t>(interprocess_master_recieve(m_buffer.data(), static_cast<int>(m_buffer.size())));
		}
		else
		{
			bc_recieved = static_cast<size_t>(interprocess_slave_recieve(m_buffer.data(), static_cast<int>(m_buffer.size())));
		}
		assert(bc_recieved <= VTT_INTERPROCESS_BC_MESSAGE_LIMIT);
		if(0 != bc_recieved)
		{
			t_Lock lock(m_sync);
			::std::cout << "thread #" << ::boost::this_thread::get_id() << "recieved " << bc_recieved << " bytes:" << ::std::endl;
			::std::cout.write(m_buffer.data(), bc_recieved);
			::std::cout.flush();
		}
	}

	private: void Send(void)
	{
		Generate_Message();
		{
			t_Lock lock(m_sync);
			::std::cout << "thread #" << ::boost::this_thread::get_id() << " sends " << m_buffer.size() << " bytes:" << ::std::endl;
			::std::cout.write(m_buffer.data(), m_buffer.size());
			::std::cout.flush();
		}
		if(m_is_master)
		{
			interprocess_master_send(0, m_buffer.data(), static_cast<int>(m_buffer.size()));
		}
		else
		{
			interprocess_slave_send(m_application_id, m_buffer.data(), static_cast<int>(m_buffer.size()));
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

typedef ::std::unique_ptr<t_Worker> t_pWorker;

typedef ::std::vector<t_pWorker> t_Workers;

int main(int argc, char * args[], char *[])
{
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
			::std::cout <<  "application_id is invalid" << args[1] << ::std::endl;
			return(0);
		}
	}

	srand(42);
	
	::boost::mutex sync;
	{
		t_Workers workers;
		auto const ec_workers = 1 + (rand() % EC_WORKERS_MAX);
		workers.resize(ec_workers);
		::std::generate(workers.begin(), workers.end(), [is_master, application_id, &sync](void) -> t_pWorker {return(t_pWorker(new t_Worker(is_master, application_id, sync)));});
		::boost::this_thread::sleep(::boost::get_system_time() + ::boost::posix_time::milliseconds(WORK_TIME_SECONDS * 1000));
	}
	::std::getchar();
	return(0);
}