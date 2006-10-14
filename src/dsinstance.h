//==================================================================
//	Copyright (C) 2006  Davide Pasca
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of\
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

#ifndef DSINSTANCE_H
#define DSINSTANCE_H

#include <windows.h>
#include "psys.h"
#include "pwindow.h"
#include "console.h"
#include "compak.h"
#include "screen_sharing.h"
#include "data_schema.h"
#include "interactsys.h"
#include "settings.h"
#include "remotemng.h"
#include "pnetlib_httpfile.h"
#include "download_update.h"
#include "dschannel.h"

#define CHNTAG				"* "
#define APP_NAME			"DSharingu"
#define APP_VERSION_STR		"0.20a"

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
class DSharinguApp
{
	friend class	DSChannel;
	friend class	DSChannelManager;

public:
	enum 
	{
		STEXT_TOOLBARBASE,
		BUTT_CONNECTIONS = 600,
		BUTT_HANGUP,
		BUTT_SETTINGS,
		BUTT_USEREMOTE,
		BUTT_SHELL,
//		BUTT_HELP,
//		BUTT_QUIT,
	};

//private:
public:

	//==================================================================
	DSChannel			*_cur_chanp;
	DSChannelManager	*_chmanagerp;

	static const int	INPACK_BUFF_SIZE = 1024*1024;
	char				_config_fname[256];
	char				_config_pathname[PSYS_MAX_PATH];

	ComListener			_com_listener;
	ScrShare::Writer	_scrwriter;
	Settings			_settings;
	RemoteMng			_remote_mng;

	u_char				*_inpack_buffp;

	win_t				_main_win;
	win_t				*_home_winp;
	win_t				_dbg_win;

	DownloadUpdate		*_download_updatep;	
	HMENU				_main_menu;

	double				_last_autocall_check_time;

//	IntSysMessageParser	_intsysmsgparser;
public:
	DSharinguApp( const char *config_fnamep );
	~DSharinguApp();

	void			Create( bool start_minimized );
	void			StartListening( int port_listen );
	int				Idle();
	win_t			*GetWindowPtr()
	{
		return &_main_win;
	}

private:
	static void		channelSwitch_s( DSharinguApp *superp, DSChannel *new_sel_chanp, DSChannel *old_sel_chanp );
	void			channelSwitch( DSChannel *new_sel_chanp, DSChannel *old_sel_chanp );
	static void		onChannelDelete_s( DSharinguApp *superp, DSChannel *chanp );
	void			onChannelDelete( DSChannel *chanp );

	void		updateViewMenu( DSChannel *chanp );

	HWND		openModelessDialog( void *mythisp, DLGPROC dlg_proc, LPSTR dlg_namep );

	int			mainEventFilter( win_event_type etype, win_event_t *eventp );
	static int	mainEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );

	int			dbgEventFilter( win_event_type etype, win_event_t *eventp );
	static int	dbgEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	void		dbgDoPaint();

	void		cmd_debug( char *params[], int n_params );
	static void cmd_debug_s( void *userp, char *params[], int n_params );

	static void	handleChangedSettings_s( void *mythis );
	void		handleChangedSettings();

	static void	handleChangedRemoteManager_s( void *mythis, RemoteDef *changed_remotep );
	void		handleChangedRemoteManager( RemoteDef *changed_remotep );

	static void	handleCallRemoteManager_s( void *mythis, RemoteDef *remotep );
	void		handleCallRemoteManager( RemoteDef *remotep );

	bool		_about_is_open;
	static BOOL CALLBACK aboutDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK aboutDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	void		saveConfig();

	void		homeWinCreate();
	static int	homeWinEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp );
	int			homeWinEventFilter( win_event_type etype, win_event_t *eventp );
	static void	homeWinGadgetCallback_s( void *userdatap, int gget_id, GGET_Item *itemp, GGET_CB_Action action );
	void		homeWinGadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action );
	void		homeWinOnChangedSettings();
	
	void		handleAutoCall();
};

#endif