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
//==
//==
//==
//==================================================================

#ifndef DSINSTANCE_H
#define DSINSTANCE_H

#include <windows.h>
#include "psys.h"
#include "kwindow.h"
#include "console.h"
#include "compak.h"
#include "screen_sharing.h"
#include "data_schema.h"
#include "interactsys.h"
#include "settings.h"

/*
//==================================================================
class IntSysMessageParser : public ThreadBase
{
public:
	static const int	MSG_CPK_INPACK = WM_USER;
private:
	Compak	*_cpkp;
public:
	struct Message
	{
		u_int	_pack_id;
		int		_wd;
		int		_he;
		void	*datap;
	};

	IntSysMessageParser() :
	  _cpkp(NULL)
	{
	}
	
	void	SetCompak( Compak *cpkp )
	{
		_cpkp = cpkp;
	}

	virtual void TakeMsg( UINT msg, WPARAM wParam, LPARAM lParam );
};
*/

//==================================================================
class DSChannel
{
public:
	enum State
	{
		STATE_NULL,
		STATE_IDLE,
		STATE_CONNECTING,
		STATE_CONNECTED,
		STATE_DISCONNECTED,
		STATE_QUIT
	};

	enum 
	{
		STEXT_TOOLBARBASE,
		BUTT_CALL = 600,
		BUTT_HANGUP,
		BUTT_SETTINGS,
		BUTT_USEREMOTE,
		BUTT_SHELL,
		BUTT_HELP,
		BUTT_QUIT,
	};

private:
	static const int	INPACK_BUFF_SIZE = 1024*1024;

	State				_state;
	State				_new_state;
	Compak			_cpk;
	ScrShare::Writer	_scrwriter;
	ScrShare::Reader	_scrreader;
	Settings			_settings;
	console_t			_console;
	int					_flow_cnt;
	bool				_is_connected;
	bool				_is_transmitting;
	bool				_remote_gives_view;
	bool				_remote_gives_share;
	bool				_remote_wants_view;
	bool				_remote_wants_share;
	bool				_view_fitwindow;
	bool				_do_quit_flag;
	bool				_do_save_config;
	bool				_settings_open_for_call;
	char				_destination_ip_name[128];

	u_char				*_inpack_buffp;

	static cons_cmd_def_t	_cmd_defs[3];

	win_t				_main_win;
	win_t				_tool_win;
	win_t				_view_win;
	win_t				_dbg_win;

	InteractiveSystem	_intersys;

	int					_disp_off_x;
	int					_disp_off_y;
	int					_disp_curs_x;
	int					_disp_curs_y;

	u_int				_frame_since_transmission;

//	IntSysMessageParser	_intsysmsgparser;

private:
	void		onConnect();
	void		onDisconnect();
	void		setInteractiveMode( bool onoff );
	bool		getInteractiveMode();
	void		setState( State state );
	void		processInputPacket( u_int pack_id, const u_char *datap, u_int data_size );
	void		ensureDisconnect( const char *messagep, bool is_error=0 );

	int			mainEventFilter( win_event_type etype, win_event_t *eventp );
	static int	mainEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	int			viewEventFilter( win_event_type etype, win_event_t *eventp );
	static int	viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	int			toolEventFilter( win_event_type etype, win_event_t *eventp );
	static int	toolEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	void		drawDispOffArrows();
	void		doPaint();

	int			dbgEventFilter( win_event_type etype, win_event_t *eventp );
	static int	dbgEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	void		dbgDoPaint();

	void		cmd_connect( char *params[], int n_params );
	static void cmd_connect_s( void *userp, char *params[], int n_params );
	void		cmd_debug( char *params[], int n_params );
	static void cmd_debug_s( void *userp, char *params[], int n_params );

	void		console_line_func( const char *txtp, int is_cmd );
	static void console_line_func_s( void *userp, const char *txtp, int is_cmd );

	void		gadgetCallback( int gget_id, GGET_Item *itemp );
	static void	gadgetCallback_s( int gget_id, GGET_Item *itemp, void *userdatap );
	void		rebuildButtons();
	void		reshapeButtons();

	void		handleAutoScroll();

	void		handleConnectedFlow();

	static void	handleChangedSettings_s( void *mythis );
	void		handleChangedSettings();

public:
			DSChannel();
			~DSChannel();

	void	Create( bool do_send_desk, bool do_save_config );
	void	StartListening( int port_listen );
	State	Idle();
	win_t	*GetWindowPtr()
	{
		return &_main_win;
	}
};

#endif