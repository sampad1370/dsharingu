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

#ifndef COMPAK_H
#define COMPAK_H

#include <windows.h>
#include "psys.h"
#include "com_basedef.h"
#include "packet_def.h"

//==================================================================
#define COMPACK_MAX_OUT_QUEUE	512	// must be power of 2 !!!

//==================================================================
struct CompakInpack
{
	u_int				_ticket_num;
	bool				_locked;
	bool				_is_empty_packet;
	int					_head_done_bytes;
	int					_data_done_bytes;
	packet_header_t		_tmp_head;
	packet_header_t		*_head_and_datap;

	CompakInpack() :
		_ticket_num(0),
		_locked(false),
		_is_empty_packet(false),
		_head_done_bytes(0),
		_data_done_bytes(0),
		_head_and_datap(NULL)
	{
	}

	void *GetData()
	{
		return (u_char *)(_head_and_datap + 1);
	}
	u_int GetDataSize()
	{
		return _head_and_datap->size;
	}
};

//==================================================================
struct CompakOutpack
{
	u_int				_ticket_num;
	packet_header_t		*_head_and_datap;
	u_int				_total_bytes;
};

//==================================================================
struct CompakStats
{
	u_long		_total_send_bytes;
	u_long		_total_recv_bytes;
	double		_start_timed;

	u_long		_send_bytes_queue;
	u_long		_recv_bytes_queue;

	u_long		_total_send_bytes_window;
	u_long		_total_recv_bytes_window;
	double		_start_timed_window;
	u_long		_window_size;

	float		_send_bytes_per_sec_window;
	float		_recv_bytes_per_sec_window;
	
	void Reset();
	void UpdateWindow( double timed );
};

//==================================================================
typedef void	(*CompackOnPackCb)( const void *datap, u_int data_size, void *userdatap );

//==================================================================
class Compak
{
	int					_status;
	int					_fd;
	int					_listen_fd;

	u_int				_top_pack_done_bytes;
	CompakOutpack		_outpack_queue[COMPACK_MAX_OUT_QUEUE];
	int					_n_outpacks;
	int					_top_out_idx;

	CompakInpack		_inpack;

	CompakStats			_stats;

	void				*_io_mutex_h;
	void				*_hservice;
	volatile int		_thread_status;

	bool				_need_disconnect;

	u_int				_on_pack_pkid;
	void				*_on_pack_userdatap;
	CompackOnPackCb		_on_pack_callback;

public:
	Compak();
	~Compak();

	int		Call( char *ipnamep, int port_number );
	void	Disconnect();
	int		StartListen( int port_number );
	void	StopListen();
	int		Idle();

	bool	GetInputPack( u_int *out_pack_idp,
						  u_int *out_pack_data_sizep,
						  void *out_datap,
						  u_int out_data_max_size,
						  int n_pack_filter=0,
						  const u_int *filter_id_array=NULL );
	void	DisposeINPacket();

	void	*AllocPacket( u_short pk_id, int data_size );
	int		SendPacket( void *compak_allocated_datap, u_int *send_ticketp=0 );
	int		SendPacket( u_short pk_id, const void *datap=NULL, int data_size=0, u_int *send_ticketp=NULL );
	int		SearchOUTQueue( u_short packet_id );

	void				SetOnPackCallback( u_int pk_id, CompackOnPackCb callback, void *cb_userdatap )
						{
							_on_pack_pkid = pk_id;
							_on_pack_callback = callback;
							_on_pack_userdatap = cb_userdatap;
						}

	const CompakStats	&GetStats() const { return _stats;	}

private:
	int		handleSend();
	int		handleReceive();
	void	resetIOActivity();
	void	onConnect();
	int		onDisconnect( int printerror );
	void	disposeInPack();

	static DWORD WINAPI ioThread_s( Compak *mythis );
	DWORD				ioThread();
};

#endif
