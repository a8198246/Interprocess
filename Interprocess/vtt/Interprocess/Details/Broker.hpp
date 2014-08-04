#ifndef HEADER_VTT_INTERPROCESS_DETAILS_BROKER
#define HEADER_VTT_INTERPROCESS_DETAILS_BROKER

#pragma once

#include "../Configuration.hpp"
#include "../Application Identifier.hpp"

#include "Pipe.hpp"

#include <sal.h>

#include <string>
#include <map>
#include <memory>

#include <boost/lexical_cast.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>

namespace n_vtt
{
namespace n_interprocess
{
namespace n_details
{
	class t_Broker
	{
		protected: typedef ::boost::interprocess::managed_shared_memory t_SharedMemory;

		protected: typedef t_Pipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT * VTT_INTERPROCESS_EC_APPLICATIONS_MAX> t_SlavesToMasterPipe;

		protected: typedef t_Pipe<VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT> t_MasterToSlavePipe;

		protected: typedef ::std::unique_ptr<t_MasterToSlavePipe> t_pPipe;

		protected: typedef ::std::map<t_ApplicationId, t_pPipe> t_MasterToSlavePipesMap;

		#pragma region Fields

		protected: t_SharedMemory          m_shared_buffer;
		protected: t_SlavesToMasterPipe    m_slaves_to_master_pipe;
		protected: t_MasterToSlavePipesMap m_master_to_slaves_pipes_map;

		#pragma endregion

		public: t_Broker(void)
		:	m_shared_buffer
			(
				::boost::interprocess::open_or_create
			,	"vtt interprocess"
			,	VTT_INTERPROCESS_BC_MESSAGE_BUFFER_LIMIT * (VTT_INTERPROCESS_EC_APPLICATIONS_MAX + 1) * 2
			)
		,	m_slaves_to_master_pipe("s to m", m_shared_buffer)
		{
			
		}

		private: t_Broker(t_Broker const &) = delete;

		public: ~t_Broker(void)
		{
			//	Do nothing
		}

		private: void operator = (t_Broker const &) = delete;

		public: auto Get_SlavesToMasterPipe(void) -> t_SlavesToMasterPipe &
		{
			return(m_slaves_to_master_pipe);
		}

		public: auto Get_MasterToSlavePipePtr(_In_ const t_ApplicationId application_id) -> t_MasterToSlavePipe *
		{
			auto it_pair = m_master_to_slaves_pipes_map.find(application_id);
			if(m_master_to_slaves_pipes_map.end() == it_pair)
			{
				if(VTT_INTERPROCESS_EC_APPLICATIONS_MAX == m_master_to_slaves_pipes_map.size())
				{
					return(nullptr);
				}
				it_pair = m_master_to_slaves_pipes_map.insert
				(
					t_MasterToSlavePipesMap::value_type
					(
						application_id
					,	t_pPipe
						(
							new t_MasterToSlavePipe
							(
								"m to s" + ::boost::lexical_cast<::std::string>(application_id)
							,	m_shared_buffer
							)
						)
					)
				).first;
			}
			return(it_pair->second.get());
		}
	};
}
}
}

#endif