#if !defined( __serialport_hpp_kjfd89fu9uew9ufe9u_kenny__ )
#define __serialport_hpp_kjfd89fu9uew9ufe9u_kenny__

namespace nsHudplay
{
	namespace nsBase
	{
		class tserialport
		{
		public:
			tserialport() : file_( -1 ) {}
			tserialport( const char* filename, int speed, bool block );
			tserialport( const char* filename, int speed, bool block, int bits,int stop,int parity );

			~tserialport();

			/**
			 * *@brief   打开串口设备文件, 返回设备句柄
			 * *@param  filename     类型  const char*  打开的串口文件名
			 * */
			bool open( const char* filename );

			/**
			 * *@brief  关闭串口
			 * */
			void close();

			/**
			 * *@brief   设置串口数据位，停止位和效验位
		 	 * *@param  speed  类型 int  串口速度
		 	 * *@param  block  类型 bool  串口读写是否阻塞模式
			 * *@param  bits 类型  int 数据位   取值 为 7 或者8
			 * *@param  stop 类型  int 停止位   取值为 1 或者2
			 * *@param  parity  类型  int  效验类型 取值为N,E,O,,S
			 * */
			bool settings( int speed, bool block, int bits,int stop,int parity );

			/**
			 * *@brief  从串口读取数据
			 * *@param  buffer     类型 char*  读取数据缓存
			 * *@param  size     类型 unsigned int  读取数据缓存
			 * */
			unsigned int read( char* buffer, unsigned int size );

			/**
			 * *@brief  等待串口可读写
			 * *@param  timeout     类型 unsigned int  读取数据缓存
			 * *@return  >0 可读写, =0 超时,  <0 错误
			 * */
			int select( unsigned int timeout );

			/**
			 * *@brief return handle of tty device file
			 * */
			operator int() const;

		private:
			int file_;

		};

	} //namespace nsBase
} //namespace nsHudplay

#endif //__serialport_hpp_kjfd89fu9uew9ufe9u_kenny__

