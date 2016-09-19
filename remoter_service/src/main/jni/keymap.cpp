#include <sys/time.h>
#include <stdio.h>
#include "keymap.hpp"
#include "serialport/serialport.hpp"

//--------------------------------------------------------------------------------
#ifndef LOGI
#define LOGI printf
#endif

namespace nsHudplay
{
	namespace nsBase
	{
		//--------------------------------------------------------------------------------
		enum tenum_key_state
		{
			emKEY_STATE_UP,
			emKEY_STATE_DOWN,
			emKEY_STATE_KEEP
		};

		//--------------------------------------------------------------------------------
		struct tevent_key { unsigned int value; unsigned int state; };
		struct tkey_info { tevent_key scal, key; };
		struct tkey_map_item { unsigned int origin, target; };

		//--------------------------------------------------------------------------------
		class tkeymap
		{
		public:
			typedef const tkey_map_item titem;

		public:
			tkeymap( titem *b, titem *e ) { load( b, e ); }
			~tkeymap() {}

			void load( titem *b, titem *e )
			{
				b_ = b, e_ = e;
			}

			bool push( const tevent_key& in, tevent_key& out )
			{
				titem *ret = 0;

				for( titem *tmp = b_; tmp != e_; ++tmp )
				{
					if( tmp->origin == in.value )
					{
						ret = tmp;
						break;
					}
				}

				if( ret == 0 ) return false;

				out = in;

				out.value = ret->target;

				return true;
			}
		
		private:
			titem *b_, *e_;
		};
	} //namespace nsBase

	//--------------------------------------------------------------------------------
	template< typename _event >
	class tevent_key_loop
	{
	public:
		typedef _event tevent;

	public:
		tevent_key_loop( const char* port, unsigned int speed = 9200,
				unsigned int bits = 8, unsigned int stop = 1, char parity = 'N' );

		void run( tevent& evt, typename tevent::tdata data );

		void exit() { to_exit = true; }

	private:
		unsigned int timeout( unsigned int mode );

	private:
		nsBase::tserialport sport_;
		bool to_exit;
	};

	//--------------------------------------------------------------------------------
	template< typename _event >
	tevent_key_loop< _event >::tevent_key_loop( const char* port, unsigned int speed,
		unsigned int bits, unsigned int stop, char parity ) :
		sport_( port, speed, false, bits, stop, parity ), to_exit( false )
	{
	}

	//--------------------------------------------------------------------------------
	template< typename _event >
	unsigned int tevent_key_loop< _event >::timeout( unsigned int mode )
	{
		unsigned int timeouts[] = {
			5000,
			500,
			50
		};

		return 1000 * timeouts[ mode % ( sizeof( timeouts ) / sizeof( unsigned int ) ) ];	
	}

	//--------------------------------------------------------------------------------
	template< typename _event >
	void tevent_key_loop< _event >::run( tevent& evt, typename tevent::tdata data )
	{
		int ret = 0;

		static const unsigned int siz = 8;

		char buf[siz], upflag = -1;

		unsigned int mode = 0;

		nsHudplay::nsBase::tkey_info info = { { 0, nsBase::emKEY_STATE_UP }, { 0, nsBase::emKEY_STATE_UP } };

		//printf( "%s()\n", __func__ );

		while( !to_exit && ( ret = sport_.select( timeout( mode ) ) ) >= 0 )
		{
			//printf( "%s() select return mode %d.\n", __func__, mode );

			if( ret == 0 )
			{
				if( mode > 0 )
				{
					info.scal.state = nsBase::emKEY_STATE_KEEP;

					evt.on_event( data, info );

					mode = 2;
				}

				continue;
			}

			ret = sport_.read( buf, siz );

			for( int t = 0; t < ret; ++t )
			{
				//printf( "%s: read %d remoter code: 0x%x\n", __func__, ret, 0xff & (unsigned int)buf[t] );

				if( mode == 0 )
				{
					if( buf[t] != upflag )
					{
						info.scal.value = 0xff & buf[t];
						info.scal.state = nsBase::emKEY_STATE_DOWN;
						evt.on_event( data, info );
						mode = 1;
					}
				} else {
					if( buf[t] == upflag )
					{
						info.scal.state = nsBase::emKEY_STATE_UP;
						evt.on_event( data, info );
						info.scal.value = 0;
						mode = 0;
					}
				}
			}
		}
	}

	//--------------------------------------------------------------------------------
	static const nsHudplay::nsBase::tkey_map_item key_table[] = 
	{
		{ 0x81,  	212 },
		{ 0x82,  	103 },
		{ 0x83,  	213 },
		{ 0x84,  	028 },
		{ 0x85,  	105 },
		{ 0x86,  	158 },
		{ 0x87,  	106 },
		{ 0x9a, 	108 },
		{ 0x9b, 	139 },

		{ 0x01,  	212 },
		{ 0x02,  	103 },
		{ 0x03,  	213 },
		{ 0x04,  	028 },
		{ 0x05,  	105 },
		{ 0x06,  	158 },
		{ 0x07,  	106 },
		{ 0x1a, 	108 },
		{ 0x1b, 	139 }
	};

	//--------------------------------------------------------------------------------
	struct tkey_handler
	{
		typedef nsHudplay::nsBase::tkeymap& tdata;

		nsHudplay::nsBase::tkey_info info_;

		timeval tv_;

		FILE* fp_;

		tkey_handler( const char *fn ) : fp_( fopen( fn, "w" ) ) {}

		~tkey_handler() { fclose( fp_ ); fp_ = NULL; }

		inline void send_key( const timeval& tv, const nsBase::tkey_info& key )
		{
			if( fp_ == NULL ) return;

			char buffer[12];
			unsigned long tmp = 0;

			tmp |= (!!key.scal.state) << 24;
			tmp |= (key.scal.value & 0xfff) << 12;
			tmp |= key.key.value & 0xfff;

			snprintf( buffer, 12, "0x%08lx\n", tmp );

			buffer[11] = '\0';

			fwrite( buffer, sizeof(char), sizeof(buffer), fp_ );

			fflush( fp_ );

			//printf( "%s: write key %lx.\n", __func__, tmp );
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
#ifdef ANDROID_BUILD_REMOTER
	nsHudplay::tkey_handler handler( "/sys/devices/platform/hudplay_rc/rc" );
	nsHudplay::tevent_key_loop< nsHudplay::tkey_handler > keyloop( "/dev/ttyMT3", 9600 );
#else
	nsHudplay::tkey_handler handler( "/tmp/key_event.log" );
	nsHudplay::tevent_key_loop< nsHudplay::tkey_handler > keyloop( "/dev/ttyUSB0", 9600 );
#endif

	nsHudplay::nsBase::tkeymap km( nsHudplay::key_table,
			nsHudplay::key_table + ( sizeof(nsHudplay::key_table) / sizeof(nsHudplay::nsBase::tkey_map_item) ) );

	keyloop.run( handler, km );
}

