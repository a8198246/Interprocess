#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER

#pragma once

#include "../Configuration.hpp"
#include "../Application Identifier.hpp"

#include "Threaded Logger.hpp"
#include "Multi Writer Single Reader Pipe.hpp"
#include "Single Writer Multi Reader Pipe.hpp"

#include <sal.h>

#include <string>
#include <map>

#include <boost/lexical_cast.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_implementation
{
	class t_Broker
	{
		protected: typedef t_MultiWriterSingleReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT * VTT_INTERPROCESS_SLAVE_TO_MASTER_MESSAGE_BUFFER_SIZE_MAGNITUDE> t_SlavesToMasterPipe;

		protected: typedef t_MultiWriterSingleReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT> t_MasterToSlavePipe;

		protected: typedef t_SingleWriterMultiReaderPipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT> t_CommonPipe;
		
		protected: typedef ::std::map<t_ApplicationId, t_MasterToSlavePipe> t_MasterToSlavePipesMap;

		#pragma region Fields

		protected: t_SlavesToMasterPipe    m_slaves_to_master_pipe;
		protected: t_CommonPipe            m_commom_pipe;
		protected: t_MasterToSlavePipesMap m_master_to_slaves_pipes_map;

		#pragma endregion

		public: t_Broker(void)
		:	m_slaves_to_master_pipe(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "s to m"))
		,	m_commom_pipe(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "common"))
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
		}

		private: t_Broker(t_Broker const &) = delete;

		private: void operator =(t_Broker const &) = delete;

		public: auto Get_SlavesToMasterPipe(void) -> t_SlavesToMasterPipe &
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			return(m_slaves_to_master_pipe);
		}
		
		public: auto Get_CommonPipe(void) -> t_CommonPipe &
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			return(m_commom_pipe);
		}

		public: auto Get_MasterToSlavePipe(_In_ const t_ApplicationId application_id) -> t_MasterToSlavePipe &
		{
		#ifdef _DEBUG_LOGGING
			t_ThreadedLogger::Print_Message(__FUNCSIG__);
		#endif
			auto it_pair = m_master_to_slaves_pipes_map.find(application_id);
			if(m_master_to_slaves_pipes_map.end() == it_pair)
			{
			#ifdef _DEBUG_LOGGING
				t_ThreadedLogger::Print_Message("creating master to slave pipe for application id = " + ::boost::lexical_cast<::std::string>(application_id));
			#endif
				it_pair = m_master_to_slaves_pipes_map.emplace
				(
					::std::piecewise_construct
				,	::std::forward_as_tuple(application_id)
				,	::std::forward_as_tuple(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "m to s" + ::boost::lexical_cast<::std::string>(application_id))
				).first;
			}
			return(it_pair->second);
		}
	};
}
}
}

#endif