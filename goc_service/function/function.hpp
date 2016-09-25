#if !defined( __function_hpp_kenny_jir2jior2jifjifji8__ )
#define __function_hpp_kenny_jir2jior2jifjifji8__

#include <string>
#include <map>

namespace nsHudplay
{
	//------------------------------------------------------------------------------------
	typedef enum
	{
		emEVT_ACT_UNKOWN,

		emEVT_ACT_COUNT
	} tevt_active_type;

	//------------------------------------------------------------------------------------
	typedef enum
	{
		emEVT_REPALY_UNKOWN,

		emEVT_REPALY_COUNT
	} tevt_repaly_type;

	//------------------------------------------------------------------------------------
	struct tcommand { virtual tevt_active_type type() const = 0; };
	struct trepaly  { virtual tevt_repaly_type type() const = 0; };

	//------------------------------------------------------------------------------------
	struct tevt_handler_active
	{
		virtual void handle( tevt_active_type active, const char* b, const char* e ) = 0;
	};

	//------------------------------------------------------------------------------------
	struct tstate
	{
		virtual bool enter() = 0;
		virtual bool leavse() = 0;

		typedef std::map< tcommand, tstate > tree_;
	};

	//------------------------------------------------------------------------------------
	struct tservice
	{
		struct tdata;

		tdata* data_;

		bool send( const tcommand& command );

		template< typename _evt_active >
		void run( _evt_active& active );
	};
} //namespace nsHudplay

#endif //__function_hpp_kenny_jir2jior2jifjifji8__

