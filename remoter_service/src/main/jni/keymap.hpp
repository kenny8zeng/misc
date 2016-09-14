#if !defined( __keymap_hpp_kjfd89fu9uew9ufe9u_kenny__ )
#define __keymap_hpp_kjfd89fu9uew9ufe9u_kenny__

#include "serialport.hpp"

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

} //namespace nsHudplay

#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------------------------------------------
void key_loop(void);
#ifdef __cplusplus
}
#endif


#endif //__keymap_hpp_kjfd89fu9uew9ufe9u_kenny__

