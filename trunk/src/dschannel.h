//==================================================================
//	Copyright (C) 2006-2007  Davide Pasca
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
///
///
///
//==================================================================

#ifndef DSCHANNEL_H
#define DSCHANNEL_H

#include <windows.h>
#include <CommCtrl.h>
#include <direct.h>
#include <gl/glew.h>
#include "dsinstance.h"
#include "console.h"
#include "dsharingu_protocol.h"
#include "gfxutils.h"
#include "debugout.h"
#include "resource.h"
#include "SHA1.h"
#include "data_schema.h"
#include "appbase3.h"
#include "dschannel.h"
#include "dstask.h"

//==================================================================
class DSharinguApp;

//==================================================================
///
//==================================================================
class DSChannel
{
	friend class DSChannelManager;

	//==================================================================
	enum
	{
		//VIEW_WIN_VIEW_REMOTE_BUTT = 1,
		VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT,
		VIEW_WIN_USE_REMOTE_BUTT,
		VIEW_WIN_USE_REM_NOT_ALLOWED_STXT,

		VIEW_WIN_TASK_DESK_BUTTON,
		VIEW_WIN_TASK_SHELL_BUTTON,
	};

public:
	enum State
	{
		STATE_IDLE,
		STATE_CONNECTING,
		STATE_CONNECTED,
		STATE_DISCONNECT_START,
		STATE_DISCONNECTING,
		STATE_DISCONNECTED,
		STATE_RECYCLE,
		STATE_QUIT
	};

public:
	DSChannelManager		*_managerp;
	State					_state;
	bool					_is_calling_silently;
	Compak					_cpk;
	ScrShare::Reader		_scrreader;

	RemoteDef				*_session_remotep;	// $$$ this one may become deleted and then.. boom crash !!!

	bool					_is_connected;
	bool					_is_transmitting;
	bool					_is_using_remote;

	Console				_console;

	bool					_remote_wants_view;
	bool					_remote_wants_share;
	bool					_remote_allows_view;
	bool					_remote_allows_share;

	bool					_view_fitwindow;
	float					_view_scale_x;
	float					_view_scale_y;

	static cons_cmd_def_t	_cmd_defs[3];

	safe_ptr<Window>			_tool_winp;
	safe_ptr<Window>			_view_winp;

	u_int					_frame_since_transmission;

	InteractiveSystem		_intersys;
	int						_disp_off_x;
	int						_disp_off_y;
	int						_disp_curs_x;
	int						_disp_curs_y;

	HWND					_connecting_dlg_hwnd;
	int						_connecting_dlg_timer;

	safe_ptr<DSTaskManager>		_task_managerp;

public:
	DSChannel( DSChannelManager *managerp, int accepted_fd );
	DSChannel( DSChannelManager *managerp, RemoteDef *remotep );
	~DSChannel();

	int			Idle();
	void		Quit()
	{
		setState( STATE_QUIT );
	}

	void		Recycle()
	{
		setState( STATE_RECYCLE );
	}

	void		DoDisconnect( const char *messagep, bool is_error=0 );
	void		Show( bool onoff );

	State		GetState() const
	{
		return _state;
	}

	void		CallRemote( bool call_silent ) throw(...);

//private:
public:
	void		create();
	void		setState( State state );
	void		onConnect( bool is_connected_as_caller );

	void		changeSessionRemote( RemoteDef *new_remotep );

	void		handleAutoScroll();
	void		handleConnectedFlow();

	void		refreshInteractionInterface();
	void		setViewMode();
	void		setInteractiveMode( bool onoff );
	bool		getInteractiveMode();
	void		setShellVisibility( bool do_switch=false );
	void		processInputPacket( u_int pack_id, const u_char *datap, u_int data_size );

	int			viewEventFilter( win_event_type etype, win_event_t *eventp );
	static int	viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	int			toolEventFilter( win_event_type etype, win_event_t *eventp );
	static int	toolEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	void		drawDispOffArrows();
	void		doViewPaint();

	void		viewWinRebuildButtons( Window *winp );
	void		viewWinReshapeButtons( Window *winp );

	void		updateViewScale();

	void		console_line_func( const char *txtp, int is_cmd );
	static void console_line_func_s( void *userp, const char *txtp, int is_cmd );

	static void	gadgetCallback_s( void *userdatap, int gget_id, GGET_Item *itemp, GGET_CB_Action action );
	void		gadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action );

	BOOL CALLBACK connectingDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
	static BOOL CALLBACK connectingDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
	
	void		thisMessageBox( LPCTSTR lpText, LPCTSTR lpCaption, UINT uType );
	int			thisMessageBoxRet( LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, UINT uDefVal );
	
	static void taskOnGadgetCB_s( void *userobjp, DSTask *taskp, DSTask::ViewState view_state );
	void		taskOnGadgetCB( DSTask *taskp, DSTask::ViewState view_state );
};

#endif
