//==================================================================
//	Copyright (C) 2006  Davide Pasca
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//==================================================================
//= Creation: Davide Pasca 2002
//=
//=
//=
//=
//==================================================================

#include "drtysock.h"
#include "compak.h"
#include "socksupport.h"
#include "appbase3.h"

#define PRINT_SOCK_ERR
static u_int	_ticket_cnt=1;	// 0 is reserved for no ticket

#define DEBUG_TRAFFIC

//==================================================================
typedef enum
{
	NOT_CONNECTED=0,
	CONNECTING=1,
	CONNECTED=2
} status_e;

enum {
	TH_STATUS_NULL,
	TH_STATUS_RUN,
	TH_STATUS_REQ_QUIT,
	TH_STATUS_REQ_RECYCLE,	// makes sure that the thread is staring a fresh look with the new FD
	TH_STATUS_DID_QUIT
};

//==================================================================
Compak::Compak() :
	_status(NOT_CONNECTED),
	_connected_as_caller(false),
	_fd(-1),
	_listen_fd(-1),
	_top_pack_done_bytes(0),
	_n_outpacks(0),
	_top_out_idx(0),
	_io_mutex_h(NULL),
	_hservice(NULL),
	_thread_status(TH_STATUS_NULL),
	_need_disconnect(false),
	_on_pack_pkid(0),
	_on_pack_userdatap(NULL),
	_on_pack_callback(NULL),
	_connection_started_time(0)
{
	DWORD	thread_id;

	_io_mutex_h = CreateMutex( NULL, 0, "outpack mutex" );
	//InitializeCriticalSection( &cp->_cri_section );
	_hservice = (void *)CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)Compak::ioThread_s, this, 0, &thread_id );
	_thread_status = TH_STATUS_RUN;
}

//==================================================================
void CompakStats::Reset()
{
	PSYS_ZEROMEM( this );

	double timed = psys_timer_get_d();
	_start_timed = timed;
	_start_timed_window = timed;
	_window_size = 5 * 1000;
}

//==================================================================
void CompakStats::UpdateWindow( double timed )
{
double	delta;

	delta = timed - _start_timed_window;
	if ( delta >= _window_size )
	{
		delta = 1000 / delta;
		_send_bytes_per_sec_window = (float)(_total_send_bytes_window * delta);
		_recv_bytes_per_sec_window = (float)(_total_recv_bytes_window * delta);
		_total_send_bytes_window = 0;
		_total_recv_bytes_window = 0;
		_start_timed_window = timed;
	}
}

//==================================================================
static int setup_nodelay_noblock( int fd )
{
unsigned long int	on = 0;
u_long					blockflg;

//	if ( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on) ) < 0 )
//	{
//		return -1;
//	}

	blockflg = 1;
	if ( ioctlsocket( fd, FIONBIO, &blockflg ) < 0 )
	{
		PRINT_SOCK_ERR;
		return -1;
	}

	return 0;
}
/*
//==================================================================
static void quit_thread( Compak *T )
{
	//EnterCriticalSection( &T->_cri_section );
		T->_thread_status = TH_STATUS_REQ_QUIT;
	//LeaveCriticalSection( &T->_cri_section );
	while ( T->_thread_status != TH_STATUS_DID_QUIT )
	{
		Sleep( 10 );
	}
}
*/

//==================================================================
//=
//==================================================================
Compak::~Compak()
{
	if ( _fd >= 0 )
		closesocket( _fd );

	StopListen();

	resetIOActivity();

	_fd = -1;
	_listen_fd = -1;

	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		if ( _thread_status == TH_STATUS_RUN )
		{
			_thread_status = TH_STATUS_REQ_QUIT;
			ReleaseMutex( _io_mutex_h );
		}
		else
		{
			ReleaseMutex( _io_mutex_h );
			return;
		}
	}

	do {
		Sleep( 10 );
		if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
		{
			int state = _thread_status;
			ReleaseMutex( _io_mutex_h );

			if ( state == TH_STATUS_DID_QUIT )
				break;
		}
	} while(true);
}

//==================================================================
void Compak::resetIOActivity()
{
	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		_need_disconnect = false;
		DisposeINPacket();

		int	end_idx = _top_out_idx + _n_outpacks;
		for (int i=_top_out_idx; i < end_idx; ++i)
		{
			CompakOutpack *op = &_outpack_queue[ i & (COMPACK_MAX_OUT_QUEUE-1) ];
			SAFE_FREE( op->_head_and_datap );
		}

		_top_out_idx = 0;
		_n_outpacks = 0;
		_top_pack_done_bytes = 0;

		ReleaseMutex( _io_mutex_h );
	}
	else
	{
		PSYS_ASSERT( 0 );
	}
}

//==================================================================
void Compak::onConnect()
{
	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		_status = CONNECTED;
		resetIOActivity();

		ReleaseMutex( _io_mutex_h );
	}
}

//==================================================================
int Compak::onDisconnect( int printerror )
{
	int	err = LAST_SOCK_ERR;
	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		PSYS_ASSERT( _status != NOT_CONNECTED );

		_status = NOT_CONNECTED;

		resetIOActivity();

		closesocket( _fd );
		_fd = -1;

		ReleaseMutex( _io_mutex_h );
	}

	if ( err == ECONNRESET )
		return COM_ERR_HARD_DISCONNECT;
	else
	if ( err == EINVAL )
		return COM_ERR_INVALID_ADDRESS;
	//ECONNABORTED

	return COM_ERR_GRACEFUL_DISCONNECT;
}

//==================================================================
int Compak::Call( char *ipnamep, int port_number )
{
struct hostent		*hp;
u_long				addr;
struct sockaddr_in	out_sa;

	if ( _fd >= 0 )
	{
		PSYS_ASSERT( _status == CONNECTED );
		return COM_ERR_ALREADY_CONNECTED;
	}

	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		_status = CONNECTING;
		_connection_started_time = psys_timer_get_d();
		ReleaseMutex( _io_mutex_h );
	}

	addr = inet_addr( ipnamep );
	if ( addr == INADDR_NONE )
	{
		if NOT( hp = gethostbyname( ipnamep ) )
		{
			if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
			{
				_status = NOT_CONNECTED;
				ReleaseMutex( _io_mutex_h );
			}
			return COM_ERR_INVALID_ADDRESS;
		}
		addr = *(long *)hp->h_addr;
	}

	memset( &out_sa, 0, sizeof(out_sa) );
	memcpy( &out_sa.sin_addr, &addr, sizeof(long) );
	out_sa.sin_family = AF_INET;
	out_sa.sin_port = htons( port_number );

	if ( (_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		PRINT_SOCK_ERR;

		if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
		{
			_status = NOT_CONNECTED;
			ReleaseMutex( _io_mutex_h );
		}
		return -1;
	}
	/*
	if ( bind( _io_fd, (struct sockaddr *)&_out_sa, sizeof(_out_sa) ) < 0 )
	{
		PRINT_SOCK_ERR;
		closesocket( _io_fd );
		_io_fd = -1;
		return RDCON_ERROR;
	}
	*/

	if ( setup_nodelay_noblock( _fd ) )
	{
		PRINT_SOCK_ERR;
		onDisconnect( false );
		return -1;
	}

	//-----------------------------------
	if ( connect( _fd, (struct sockaddr *)&out_sa, sizeof(out_sa)) < 0 )
	{
	int	err = LAST_SOCK_ERR;

		if ( err )
		{
			if ( err != EISCONN && err != EWOULDBLOCK && err != EALREADY )
				return onDisconnect( 1 );
		}
	}

	return COM_ERR_NONE;
}

/*
if ( err == EINVAL )
{
	printf( "EINVAL !!! %i\n", cp->_einval_cnt );
	if ( ++cp->_einval_cnt < 100 )
	{
		Sleep(5);
		return COM_ERR_NONE;
	}
	else
		return onDisconnect( 0 );
}
else
	return onDisconnect( 1 );
*/

//==================================================================
void Compak::StopListen()
{
	if ( _listen_fd >= 0 )
	{
		closesocket( _listen_fd );
		_listen_fd = -1;
	}
}

//==================================================================
int Compak::StartListen( int port_number )
{
struct sockaddr_in	in_sa;
u_long				blockflg;

	StopListen();
	_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if ( _listen_fd < 0 )
		return COM_ERR_GENERIC;

	memset( &in_sa, 0, sizeof(in_sa) );
	in_sa.sin_family = AF_INET;
	in_sa.sin_addr.s_addr = htonl( INADDR_ANY );
	in_sa.sin_port        = htons( port_number );
	if ( bind( _listen_fd, (struct sockaddr *)&in_sa, sizeof(in_sa) ) == -1 ||
		 listen( _listen_fd, SOMAXCONN )                              == -1 )
	{
		StopListen();
		return COM_ERR_GENERIC;
	}

	blockflg = 1;
	if ( ioctlsocket( _listen_fd, FIONBIO, &blockflg ) < 0 )
	{
		StopListen();
		return COM_ERR_GENERIC;
	}

	return COM_ERR_NONE;
}

//==================================================================
void Compak::Disconnect()
{
	if ( _fd >= 0 )
	{
		onDisconnect( false );
	}
}


//==================================================================
static int err_send( int fd, const u_char *datap, int size, int *sent_lenp )
{
	*sent_lenp = send( fd, (const char *)datap, size, 0 );
	if ( *sent_lenp <= 0 )
	{
	int	err;
		switch ( err = LAST_SOCK_ERR )
		{
		case EWOULDBLOCK:
			if ( *sent_lenp == 0 )
				return COM_ERR_GRACEFUL_DISCONNECT;

			return COM_ERR_NONE;

		case ECONNRESET:
			return COM_ERR_HARD_DISCONNECT;

		default:
			return COM_ERR_HARD_DISCONNECT;
		}
	}

	return COM_ERR_NONE;
}

//==================================================================
int Compak::handleSend()
{
int		sent_len, err;

	if NOT( _n_outpacks )
		return COM_ERR_NONE;

	int	end_idx = _top_out_idx + _n_outpacks;
	for (int i=_top_out_idx; i < end_idx; ++i)
	{
		CompakOutpack	*op = &_outpack_queue[ i & (COMPACK_MAX_OUT_QUEUE-1) ];

		err = err_send( _fd, (u_char *)op->_head_and_datap + _top_pack_done_bytes,
						op->_total_bytes - _top_pack_done_bytes,
						&sent_len );
		if ( err )
			return err;

		if ( sent_len <= 0 )		// couldn't send ? would block !
			return COM_ERR_NONE;

		_top_pack_done_bytes += sent_len;

		if ( _top_pack_done_bytes >= op->_total_bytes )
		{
			_top_pack_done_bytes = 0;

			SAFE_FREE( op->_head_and_datap );
			op->_total_bytes = 0;

			_top_out_idx += 1;
			_n_outpacks -= 1;

			PSYS_ASSERT( _n_outpacks >= 0 );

			// 'i' is incremented automatically
		}
		else
			break;	// if not done, then return
	}
	
	return COM_ERR_NONE;
}


//==================================================================
static int err_recv( int fd, u_char *datap, int size, int *recvd_lenp )
{
	*recvd_lenp = recv( fd, (char *)datap, size, 0 );
	if ( *recvd_lenp <= 0 )
	{
		switch ( LAST_SOCK_ERR )
		{
		case EWOULDBLOCK:
			if ( *recvd_lenp == 0 )
				return COM_ERR_GRACEFUL_DISCONNECT;

			return COM_ERR_NONE;

		case ECONNRESET:
			return COM_ERR_HARD_DISCONNECT;

		default:
			return COM_ERR_HARD_DISCONNECT;
		}
	}

	return COM_ERR_NONE;
}

//==================================================================
int Compak::handleReceive()
{
int				recvd_len, err;
CompakInpack	*ip;

//	while (true)
	{
		ip = &_inpack;
		while ( ip->_head_done_bytes < sizeof(packet_header_t) )
		{
			err = err_recv( _fd, (u_char *)&ip->_tmp_head + ip->_head_done_bytes,
							sizeof(packet_header_t) - ip->_head_done_bytes,
							&recvd_len );
			if ( err )
				return err;

			if ( recvd_len <= 0 )		// couldn't receive ? would block
				return COM_ERR_NONE;

			ip->_head_done_bytes += recvd_len;
			PSYS_ASSERT( ip->_head_done_bytes <= sizeof(packet_header_t) );

			//----- update recv stats
			_stats._total_recv_bytes += recvd_len;
			_stats._total_recv_bytes_window += recvd_len;
		}

		if ( ip->_head_done_bytes >= sizeof(packet_header_t) )
		{
			if ( ip->_tmp_head.size == 0 )
			{
				// if there is no data then the head & data points to _tmp_head
				// ..avoids calling a malloc.. but must make sure that dispose_in_packet()
				// does not really try to free the mem !!
				ip->_is_empty_packet = 1;
				ip->_head_and_datap = &ip->_tmp_head;
			}
			else
			{
				while ( ip->_data_done_bytes < ip->_tmp_head.size )
				{
					if NOT( ip->_head_and_datap )
					{
						if NOT( ip->_head_and_datap = (packet_header_t *)PSYS_MALLOC( sizeof(packet_header_t) + ip->_tmp_head.size ) )
							return COM_ERR_OUT_OF_MEMORY;

						memcpy( ip->_head_and_datap, &ip->_tmp_head, sizeof(packet_header_t) );
					}

					err = err_recv( _fd,
									(u_char *)ip->_head_and_datap +
													sizeof(packet_header_t) +
													ip->_data_done_bytes,
									ip->_tmp_head.size - ip->_data_done_bytes,
									&recvd_len );
					if ( err )
						return err;

					if ( recvd_len <= 0 )		// couldn't receive ? would block
						return COM_ERR_NONE;

					ip->_data_done_bytes += recvd_len;
					PSYS_ASSERT( ip->_data_done_bytes <= ip->_tmp_head.size );

					//----- update recv stats
					_stats._total_recv_bytes += recvd_len;
					_stats._total_recv_bytes_window += recvd_len;
				}

				if ( ip->_head_and_datap->id == _on_pack_pkid && _on_pack_callback )
				{
					_on_pack_callback( ip->GetData(), ip->GetDataSize(), _on_pack_userdatap );
					disposeInPack();
				}
			}
		}
	}

	return COM_ERR_NONE;
}

//==================================================================
DWORD WINAPI Compak::ioThread_s( Compak *mythis )
{
	return mythis->ioThread();
}

//==================================================================
DWORD Compak::ioThread()
{
struct timeval	tv;
fd_set			rdset;
fd_set			wtset;
int				err;

	while (true)
	{
		u_int	sleep_ms = 10;

		if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
		{
			if ( _thread_status == TH_STATUS_REQ_QUIT )
			{
				_thread_status = TH_STATUS_DID_QUIT;
				ReleaseMutex( _io_mutex_h );
				return 0;
			}
			else
			if ( _thread_status == TH_STATUS_RUN )
			{
				if ( _status == CONNECTED )
				{
					FD_ZERO( &rdset );
					FD_ZERO( &wtset );
					FD_SET( _fd, &rdset );

					if ( _n_outpacks )
						FD_SET( _fd, &wtset );

					tv.tv_sec = 0;
					tv.tv_usec = 0;	// 1^6(=1sec) -> 1^6 / 1^4 = 1^2 (1/100th of a second)

					if ( select( 2, &rdset, &wtset, 0, &tv ) != -1 )
					{
						if ( FD_ISSET( _fd, &wtset ) || FD_ISSET( _fd, &rdset ) )
						{
							//if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
							{
								if ( FD_ISSET( _fd, &rdset ) )
								{
									if ( err = handleReceive() )
									{
										_need_disconnect = true;
									}
								}

								if ( FD_ISSET( _fd, &wtset ) )
								{
									if ( err = handleSend() )
									{
										_need_disconnect = true;
									}
								}

							}
						}
					}
				}
				else
				{
					sleep_ms = 100;
				}
			}

			ReleaseMutex( _io_mutex_h );
			Sleep( sleep_ms );
		}
	}
/*
	if ( _thread_status == TH_STATUS_REQ_RECYCLE )
	{
		_thread_status = TH_STATUS_RUN;
		goto recycle;
	}
	else
		_thread_status = TH_STATUS_DID_QUIT;
*/

	return 0;
}

//==================================================================
int Compak::Idle()
{
	struct timeval	tv;
	fd_set			rdset, wtset;
	//int				err;
	double			timed;

	FD_ZERO( &rdset );
	FD_ZERO( &wtset );

	switch ( _status )
	{
	case NOT_CONNECTED:
		PSYS_ASSERT( _fd < 0 );
		if ( _listen_fd >= 0 && _fd < 0 )
		{
			FD_SET( _listen_fd, &rdset );
			tv.tv_sec = 0;
			tv.tv_usec = 0;

			if ( select( 1, &rdset, NULL, 0, &tv ) == -1 )
			{
				StopListen();
				return COM_ERR_GENERIC;
			}

			if ( FD_ISSET( _listen_fd, &rdset ) )
			{
				_fd = accept( _listen_fd, 0, 0 );
				if ( _fd >= 0 )
				{
					if ( setup_nodelay_noblock( _fd ) )
					{
						onDisconnect( 1 );
						return COM_ERR_GENERIC;
					}

					//---- reset the stats on a new connection
					_stats.Reset();

					_connected_as_caller = false;
					onConnect();

					return COM_ERR_CONNECTED;
				}
			}
		}
		break;

	case CONNECTING:
		if ( (psys_timer_get_d() - _connection_started_time) >= 20*1000 )
		{
			onDisconnect( false );
			return COM_ERR_TIMEOUT_CONNECTING;
		}
		else
		{
			FD_SET( _fd, &wtset );
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			if ( select( 1, &rdset, &wtset, 0, &tv ) == -1 )
				return onDisconnect( 1 );

			if ( FD_ISSET( _fd, &wtset ) )
			{					
				//---- reset the stats on a new connection
				_stats.Reset();

				_connected_as_caller = true;
				onConnect();
				return COM_ERR_CONNECTED;
			}
		}
		break;

	case CONNECTED:
		if ( _need_disconnect )
		{
			onDisconnect( 1 );

			_need_disconnect = false;
			return COM_ERR_GENERIC;
		}

		break;
	}

	timed = psys_timer_get_d();
	_stats.UpdateWindow( timed );

	return COM_ERR_NONE;
}

//==================================================================
bool Compak::IsConnected() const
{
	return _status == CONNECTED;
}

//==================================================================
bool Compak::GetInputPack( u_int *out_pack_idp,
							 u_int *out_pack_data_sizep,
							 void *out_datap,
							 u_int out_data_max_size,
							 int n_pack_filter,
							 const u_int *filter_id_array )
{
	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		CompakInpack	*ip = &_inpack;

		if ( ip->_head_and_datap && ip->_tmp_head.size == ip->_data_done_bytes )
		{
			if ( n_pack_filter > 0 )
			{
				for (int i=0; i < n_pack_filter; ++i)
					if ( filter_id_array[i] == _inpack._head_and_datap->id )
						break;
			
				if ( i == n_pack_filter )
					return false;
			}

			PSYS_ASSERT( _inpack.GetDataSize() <= out_data_max_size );

			*out_pack_idp = _inpack._head_and_datap->id;
			*out_pack_data_sizep = _inpack.GetDataSize();

			memcpy( out_datap, _inpack.GetData(), _inpack.GetDataSize() );

			ReleaseMutex( _io_mutex_h );
			return true;
		}
		else
		{
			ReleaseMutex( _io_mutex_h );
			return false;
		}

	}
	else
	{
		PSYS_ASSERT( 0 );
		return false;
	}
}

//==================================================================
void Compak::disposeInPack()
{
	// in case of empty packets, _head_and_datap points to _tmp_head
	if ( _inpack._head_and_datap != &_inpack._tmp_head )
		SAFE_FREE( _inpack._head_and_datap );

	memset( &_inpack, 0, sizeof(CompakInpack) );
}

//==================================================================
void Compak::DisposeINPacket()
{
	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		disposeInPack();
		ReleaseMutex( _io_mutex_h );
	}
	else
	{
		PSYS_ASSERT( 0 );
	}
}

//==================================================================
int Compak::SendPacket( u_short pk_id, const void *datap, int data_size, u_int *send_ticketp )
{
	if ( datap == NULL || data_size == 0 )
	{
		datap = NULL;
		data_size = 0;
	}

	void	*packp = AllocPacket( pk_id, data_size );
	if ERR_NULL( packp )
		return -1;

	if ( datap )
		memcpy( packp, datap, data_size );

	return SendPacket( packp, send_ticketp );
}

//==================================================================
int Compak::SendPacket( void *compak_allocated_datap, u_int *send_ticketp )
{
	//u_char				*p;
	CompakOutpack	*op;

	//PSYS_ASSERT( cp->_n_out_packs < COMPACK_MAX_OUT_QUEUE );

	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		if ( _n_outpacks >= COMPACK_MAX_OUT_QUEUE )
		{
			PSYS_ASSERT( 0 );
			return COM_ERR_GENERIC;
		}

		op = &_outpack_queue[ (_top_out_idx + _n_outpacks) & (COMPACK_MAX_OUT_QUEUE-1) ];
		_n_outpacks += 1;

		packet_header_t *packetp = ((packet_header_t *)compak_allocated_datap - 1);

		op->_ticket_num		= _ticket_cnt;
		op->_total_bytes	= sizeof(packet_header_t) + packetp->size;
		op->_head_and_datap	= (packet_header_t *)packetp;

		_stats._send_bytes_queue += op->_total_bytes;
		
		if ( send_ticketp )
			*send_ticketp = _ticket_cnt;

		ReleaseMutex( _io_mutex_h );
	}
	else
	{
		PSYS_ASSERT( 0 );
	}

	++_ticket_cnt;	
	if ( _ticket_cnt == 0 )	// 0 is reserved for no ticket
		_ticket_cnt = 1;

	return COM_ERR_NONE;
}

//==================================================================
void *Compak::AllocPacket( u_short pk_id, int data_size )
{
	packet_header_t	*p;

	// alloc the memory for the copy of the data buffer to transmit
	if NOT( p = (packet_header_t *)PSYS_MALLOC( sizeof(packet_header_t) + data_size ) )
		return NULL;

	p->id	= pk_id;
	p->pad	= 0;
	p->size	= data_size;

	return (void *)(p + 1);
}

//==================================================================
int Compak::SearchOUTQueue( u_short packet_id )
{
	CompakOutpack	*op;
	int					cnt;

	if ( WaitForSingleObject( _io_mutex_h, INFINITE ) == WAIT_OBJECT_0 )
	{
		cnt = 0;

		int	end_idx = _top_out_idx + _n_outpacks;
		for (int i=_top_out_idx; i < end_idx; ++i)
		{
			op = &_outpack_queue[ i & (COMPACK_MAX_OUT_QUEUE-1) ];
			if ( op->_head_and_datap->id == packet_id )
				++cnt;
		}

		ReleaseMutex( _io_mutex_h );
	}
	else
	{
		PSYS_ASSERT( 0 );
	}

	return cnt;
}

//==================================================================
/*
int compak_is_data_sent( Compak *cp, u_int send_ticket )
{
	if ( cp->_outdata_ticket == send_ticket )
		return 0;

	return 1;
}
*/
