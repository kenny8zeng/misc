#include <sys/time.h>
#include <cstdio>
#include "keymap.hpp"

#ifdef ANDROID_BUILD_REMOTER
#include <input/InputManager.h>
#endif

//--------------------------------------------------------------------------------
#ifndef LOGI
#define LOGI printf
#endif

namespace nsHudplay
{
	//--------------------------------------------------------------------------------
	static const nsHudplay::nsBase::tkey_map_item key_table[] = 
	{
		{ 0x81,  	0xa1 },
		{ 0x82,  	0xa2 },
		{ 0x83,  	0xa3 },
		{ 0x84,  	0xa4 },
		{ 0x85,  	0xa5 },
		{ 0x86,  	0xa6 },
		{ 0x87,  	0xa7 },
		{ 0x9a, 	0xa8 },
		{ 0x9b, 	0xa9 },

		{ 0x1,  	0xa1 },
		{ 0x2,  	0xa2 },
		{ 0x3,  	0xa3 },
		{ 0x4,  	0xa4 },
		{ 0x5,  	0xa5 },
		{ 0x6,  	0xa6 },
		{ 0x7,  	0xa7 },
		{ 0x1a, 	0xa8 },
		{ 0x1b, 	0xa9 }
	};

	//--------------------------------------------------------------------------------
	struct tkey_handler
	{
		typedef nsHudplay::nsBase::tkeymap& tdata;

		nsHudplay::nsBase::tkey_info info_;

		timeval tv_;

		inline void send_key( const timeval& tv, const nsBase::tkey_info& key )
		{
#ifdef ANDROID_BUILD_REMOTER
#endif
		}

		inline void on_event( tdata data, const nsHudplay::nsBase::tkey_info& key )
		{
			info_ = key;

			if( data.push( info_.scal, info_.key ) )
			{
				gettimeofday( &tv_, NULL );

				send_key( tv_, info_ );

				LOGI( "[ %u.%06u ] %s( { { 0x%x, 0x%x }, { 0x%x, 0x%x } } )\n",
						(unsigned int)tv_.tv_sec, (unsigned int)tv_.tv_usec, __func__,
						info_.scal.value, info_.scal.state, info_.key.value, info_.key.state );
			} else {
				printf( "unkown key.\n");
			}
		}
	};

} //namespace nsHudplay

//--------------------------------------------------------------------------------
void key_loop(void)
{
	nsHudplay::tkey_handler handler;

	nsHudplay::nsBase::tkeymap km( nsHudplay::key_table,
			nsHudplay::key_table + ( sizeof(nsHudplay::key_table) / sizeof(nsHudplay::nsBase::tkey_map_item) ) );

	nsHudplay::tevent_key_loop< nsHudplay::tkey_handler > keyloop( "/dev/ttyUSB0", 9200 );

	keyloop.run( handler, km );
}

