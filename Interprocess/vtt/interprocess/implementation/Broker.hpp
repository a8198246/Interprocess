#ifndef HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER
#define HEADER_VTT_INTERPROCESS_IMPLEMENTATION_BROKER

#pragma once

#include "../Configuration.hpp"
#include "../Application Identifier.hpp"

#include "Multi Writer Single Reader Pipe.hpp"
#include "Single Writer Multi Reader Pipe.hpp"

#include <sal.h>

#include <string>
#include <map>
#include <memory>

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

		protected: typedef ::std::unique_ptr<t_MasterToSlavePipe> t_pPipe;

		protected: typedef ::std::map<t_ApplicationId, t_pPipe> t_MasterToSlavePipesMap;

		#pragma region Fields

		protected: t_SlavesToMasterPipe    m_slaves_to_master_pipe;
		protected: t_CommonPipe            m_commom_pipe;
		protected: t_MasterToSlavePipesMap m_master_to_slaves_pipes_map;

		#pragma endregion

		public: t_Broker(void)
		:	m_slaves_to_master_pipe(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "s to m"))
		,	m_commom_pipe(::std::string(VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "common"))
		{
			//	Do nothing
		}

		private: t_Broker(t_Broker const &) = delete;

		private: void operator =(t_Broker const &) = delete;

		public: auto Get_SlavesToMasterPipe(void) -> t_SlavesToMasterPipe &
		{
			return(m_slaves_to_master_pipe);
		}
		
		public: auto Get_CommonPipe(void) -> t_CommonPipe &
		{
			return(m_commom_pipe);
		}

		public: auto Get_MasterToSlavePipe(_In_ const t_ApplicationId application_id) -> t_MasterToSlavePipe &
		{
			auto it_pair = m_master_to_slaves_pipes_map.find(application_id);
			if(m_master_to_slaves_pipes_map.end() == it_pair)
			{
				it_pair = m_master_to_slaves_pipes_map.insert
				(
					t_MasterToSlavePipesMap::value_type
					(
						application_id
					,	t_pPipe
						(
							new t_MasterToSlavePipe
							(
								VTT_SZ_INTERPROCESS_NAMED_OBJECTS_PREFIX "m to s" + ::boost::lexical_cast<::std::string>(application_id)
							)
						)
					)
				).first;
			}
			return(*(it_pair->second.get()));
		}
	};
}
}
}

#endif