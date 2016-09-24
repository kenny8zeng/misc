#include "function.hpp"

namespace nsHudplay
{
	//------------------------------------------------------------------------------------
	struct tevent_source_buffer
	{
		template< typename _evt_buffer >
		void run( _evt_buffer& evt );
	};

	//------------------------------------------------------------------------------------
	struct tservice::tdata
	{
		tevent_source_buffer evt_raw_;
	};

	//------------------------------------------------------------------------------------
	bool tservice::send( const std::string& command )
	{
		return false;
	}

} //namespace nsHudplay
