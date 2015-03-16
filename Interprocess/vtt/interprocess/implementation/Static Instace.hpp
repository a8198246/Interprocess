#ifndef HEADER_VTT_STATIC_INSTANCE
#define HEADER_VTT_STATIC_INSTANCE

#pragma once

namespace n_vtt
{
	template<typename tp_Derived> class
	t_StaticInstace
	{
		#pragma region Fields

		protected: static tp_Derived m_instance;

		#pragma endregion

		public: static auto
		Get_Instace(void) throw() -> tp_Derived &
		{
			return(m_instance);
		}
	};

	template<typename tp_Derived>
	tp_Derived t_StaticInstace<tp_Derived>::m_instance;
}

#endif
