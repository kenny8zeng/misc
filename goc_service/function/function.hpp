#if !defined( __function_hpp_kenny_jir2jior2jifjifji8__ )
#define __function_hpp_kenny_jir2jior2jifjifji8__

namespace nsHudplay
{
	//------------------------------------------------------------------------------------
	typedef enum
	{
		emEVT_ACT_UNKOWN,

		emEVT_ACT_COUNT
	} tevt_active_type;

	//------------------------------------------------------------------------------------
	struct tcommand {};

	//------------------------------------------------------------------------------------
	struct trepaly {};

	//------------------------------------------------------------------------------------
	struct tevt_handler_active
	{
		virtual void handle( tevt_active_type active, const char* b, const char* b ) = 0;
	};

	//------------------------------------------------------------------------------------
	struct tservice
	{
		struct tdata;

		tdata* data_;

		bool send( const std::string& cmd );

		template< typename _evt_active >
		void run( _evt_active& active );
	}
} //namespace nsHudplay

#endif //__function_hpp_kenny_jir2jior2jifjifji8__
