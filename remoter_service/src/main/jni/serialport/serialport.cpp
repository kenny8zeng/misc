#include  <stdio.h>      /*标准输入输出定义*/
#include  <stdlib.h>     /*标准函数库定义*/
#include  <unistd.h>     /*Unix 标准函数定义*/
#include  <sys/types.h>  
#include  <sys/stat.h>   
#include  <fcntl.h>      /*文件控制定义*/
#include  <termios.h>    /*PPSIX 终端控制定义*/
#include  <errno.h>      /*错误号定义*/

#include  "serialport.hpp"

namespace nsHudplay
{
	namespace nsBase
	{
		//--------------------------------------------------------------------------------
		static const int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400, B19200, B9600, B4800, B2400, B1200, B300, };

		//--------------------------------------------------------------------------------
		static const int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300, 38400, 19200,  9600, 4800, 2400, 1200,  300, };

		//--------------------------------------------------------------------------------
		struct tserialport_base
		{
			//--------------------------------------------------------------------------------
			static bool set_speed(int fd, int speed)
			{
				int i;
				struct termios opt;    //定义termios结构

				if(tcgetattr(fd, &opt) != 0)
				{
					return false;
				}

				for(i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
				{
					if(speed == name_arr[i])
					{
						tcflush(fd, TCIOFLUSH);

						cfsetispeed(&opt, speed_arr[i]);
						cfsetospeed(&opt, speed_arr[i]);

						if(tcsetattr(fd, TCSANOW, &opt) != 0)
						{
							return false;
						}
						
						tcflush(fd, TCIOFLUSH);
					}
				}

				return true;
			}

			//--------------------------------------------------------------------------------
			static bool set_parity(int fd, bool block, int databits, int stopbits, int parity)
			{
				struct termios opt;

				if(tcgetattr(fd, &opt) != 0)
				{
					return false;
				}

				opt.c_cflag |= (CLOCAL | CREAD);        //一般必设置的标志

				switch(databits)        //设置数据位数
				{
					case 7:
						opt.c_cflag &= ~CSIZE;
						opt.c_cflag |= CS7;
						break;
					case 8:
						opt.c_cflag &= ~CSIZE;
						opt.c_cflag |= CS8;
						break;
					default:
						return false;
				}

				switch(parity)            //设置校验位
				{
					case 'n':
					case 'N':
						opt.c_cflag &= ~PARENB;        //清除校验位
						opt.c_iflag &= ~INPCK;        //enable parity checking
						break;
					case 'o':
					case 'O':
						opt.c_cflag |= PARENB;        //enable parity
						opt.c_cflag |= PARODD;        //奇校验
						opt.c_iflag |= INPCK;         //disable parity checking
						break;
					case 'e':
					case 'E':
						opt.c_cflag |= PARENB;        //enable parity
						opt.c_cflag &= ~PARODD;        //偶校验
						opt.c_iflag |= INPCK;            //disable pairty checking
						break;
					case 's':
					case 'S':
						opt.c_cflag &= ~PARENB;        //清除校验位
						opt.c_cflag &= ~CSTOPB;        //??????????????
						opt.c_iflag |= INPCK;            //disable pairty checking
						break;
					default:
						return false;    
				}

				switch(stopbits)        //设置停止位
				{
					case 1:
						opt.c_cflag &= ~CSTOPB;
						break;
					case 2:
						opt.c_cflag |= CSTOPB;
						break;
					default:
						return false;
				}

				opt.c_cflag |= (CLOCAL | CREAD);
				opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
				opt.c_oflag &= ~OPOST;
				opt.c_oflag &= ~(ONLCR | OCRNL);    //添加的
				opt.c_iflag &= ~(ICRNL | INLCR);
				opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //添加的

				tcflush(fd, TCIFLUSH);

				opt.c_cc[VTIME] = 0;        //设置超时为15sec
				opt.c_cc[VMIN] = block? 1 : 0;

				if(tcsetattr(fd, TCSANOW, &opt) != 0)
				{
					return false;
				}

				return true;
			}
		};

		//--------------------------------------------------------------------------------
		bool tserialport::open( const char* filename )
		{
			file_ = ::open( filename, O_RDWR | O_NOCTTY | O_NDELAY );

			if( fcntl( file_, F_SETFL, 0 ) < 0 )
			{
				printf("fcntl failed!\n");
			}

			if( isatty( STDIN_FILENO ) == 0 )
			{
				printf("isatty fail!\n");
			}

			return -1 != file_;
		}

		//--------------------------------------------------------------------------------
		void tserialport::close()
		{
			if( file_ != -1 ) { ::close( file_ ); file_ = -1; }
		}

		//--------------------------------------------------------------------------------
		tserialport::tserialport( const char* filename, int speed, bool block ) : file_( -1 ) {
			this->open( filename );
			settings( speed, block, 8, 1, 'N' );
		}

		//--------------------------------------------------------------------------------
		tserialport::tserialport( const char* filename, int speed, bool block,
			int bits,int stop,int parity )
		{
			this->open( filename );
			settings( speed, block, bits, stop, parity );
		}

		//--------------------------------------------------------------------------------
		tserialport::~tserialport() {
			this->close();
		}

		//--------------------------------------------------------------------------------
		bool tserialport::settings( int speed, bool block, int bits,int stop,int parity )
		{
			return tserialport_base::set_speed( file_, speed ) &&
				tserialport_base::set_parity( file_, block, bits, stop, parity );
		}

		//--------------------------------------------------------------------------------
		unsigned int tserialport::read( char* buffer, unsigned int size )
		{
			if( file_ == -1 || size == 0 ) return 0;

			int ret = 0;

			return ::read( file_, &(buffer[0]), size );
		}

		//--------------------------------------------------------------------------------
		int tserialport::select( unsigned int timeout )
		{
			int ret = 0;
			fd_set rfds;

			struct timeval tv = {
				.tv_sec = timeout / 1000000,
				.tv_usec = timeout % 1000000,
			};

			FD_ZERO( &rfds );
			FD_SET( file_, &rfds );

			ret = ::select(file_ + 1, &rfds, NULL, NULL, &tv );
			
			return ( ret > 0 && !FD_ISSET( file_, &rfds ) )? 0 : ret;
		}

		//--------------------------------------------------------------------------------
		tserialport::operator int() const
		{
			return file_;
		}

	} //namespace nsBase
} //namespace nsHudplay

