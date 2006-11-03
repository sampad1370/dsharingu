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
///
///
///
///
//==================================================================

#include <windows.h>
#include <CommCtrl.h>
#include <ShlObj.h>
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
#include "dschannel_manager.h"

#define WINDOW_TITLE		APP_NAME" " APP_VERSION_STR

//===============================================================
BOOL CALLBACK DSharinguApp::aboutDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );

	DSharinguApp *mythis = (DSharinguApp *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->aboutDialogProc( hwnd, umsg, wparam, lparam );
}

//===============================================================
BOOL CALLBACK DSharinguApp::aboutDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch( umsg )
    {
    case WM_INITDIALOG:
		SetDlgItemText( hwnd, IDC_ABOUT_APPNAME, APP_NAME" "APP_VERSION_STR );
		break; 

    case WM_COMMAND:
	    switch(LOWORD(wparam))
		{
		case IDC_HOMEPAGE:
			ShellExecute( hwnd, "open", "http://kazzuya.com/dsharingu",
						  NULL, NULL, SW_SHOWNORMAL );
			break;

		case IDC_MANUAL:
			ShellExecute( hwnd, "open", "manual\\index.html",
						  NULL, NULL, SW_SHOWNORMAL );
			break;

		case IDOK:
		case IDCANCEL:                 
			DestroyWindow( hwnd );
			break;
	    }
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;
	case WM_DESTROY:
		_about_is_open = false;
		appbase_rem_modeless_dialog( hwnd );
		break;

    default:
		return 0;
    }
		
    return 1;
}


//==================================================================
//==
//==================================================================
DSharinguApp::DSharinguApp( const char *config_fnamep ) :
	_cur_chanp(NULL),
	_inpack_buffp(NULL),
	_about_is_open(false),
	_download_updatep(NULL),
	_main_menu(NULL),
	_chmanagerp(NULL),
	_home_winp(NULL),
	_last_autocall_check_time(0.0)
{
	psys_strcpy( _config_fname, config_fnamep, sizeof(_config_fname) );
	_config_pathname[0] = 0;
}

//===============================================================
DSharinguApp::~DSharinguApp()
{
	SAFE_FREE( _inpack_buffp );

//	_intsysmsgparser.StopThread();
}

//==================================================================
void DSharinguApp::updateViewMenu( DSChannel *chanp )
{
	if NOT( chanp )
	{
		EnableMenuItem( _main_menu, ID_VIEW_SHELL,		MF_BYCOMMAND | MF_GRAYED );
		EnableMenuItem( _main_menu, ID_VIEW_FITWINDOW,	MF_BYCOMMAND | MF_GRAYED );
		EnableMenuItem( _main_menu, ID_VIEW_ACTUALSIZE, MF_BYCOMMAND | MF_GRAYED );
	}
	else
	{
		EnableMenuItem( _main_menu, ID_VIEW_SHELL,		MF_BYCOMMAND | MF_ENABLED );
		EnableMenuItem( _main_menu, ID_VIEW_FITWINDOW,	MF_BYCOMMAND | MF_ENABLED );
		EnableMenuItem( _main_menu, ID_VIEW_ACTUALSIZE, MF_BYCOMMAND | MF_ENABLED );

		if ( chanp->_console.cons_is_showing() )
			CheckMenuItem( _main_menu, ID_VIEW_SHELL, MF_BYCOMMAND | MF_CHECKED );
		else
			CheckMenuItem( _main_menu, ID_VIEW_SHELL, MF_BYCOMMAND | MF_UNCHECKED );

		CheckMenuItem( _main_menu, ID_VIEW_FITWINDOW, MF_BYCOMMAND | (chanp->_view_fitwindow ? MF_CHECKED : MF_UNCHECKED) );
		CheckMenuItem( _main_menu, ID_VIEW_ACTUALSIZE, MF_BYCOMMAND | (!chanp->_view_fitwindow ? MF_CHECKED : MF_UNCHECKED) );
	}
}

//==================================================================
void DSharinguApp::saveConfig()
{
	FILE *fp;
	errno_t	err = fopen_s( &fp, _config_pathname, "wt" );

	if NOT( err )
	{
		_settings._schema.SaveData( fp );
		_remote_mng.SaveList( fp );
		fclose( fp );
	}
}

//==================================================================
static void cut_spaces( char *strp )
{
char	*s, *d;

	s = strp;
	d = strp;
	while ( *s )
	{
		if ( *s != ' ' )
			*d++ = *s;

		++s;
	}
	*d++ = 0;
}

//==================================================================
void DSharinguApp::cmd_debug( char *params[], int n_params )
{
	_dbg_win.Show( !_dbg_win.IsShowing() );
}

//==================================================================
void DSharinguApp::cmd_debug_s( void *userp, char *params[], int n_params )
{
	((DSharinguApp *)userp)->cmd_debug( params, n_params );
}

//==================================================================
static bool doesDirExist( const char *dirnamep )
{
	char	buff[ PSYS_MAX_PATH ];
	_getcwd( buff, PSYS_MAX_PATH-1 );
	bool exists = (_chdir( dirnamep) == 0);
	_chdir( buff );

	return exists;
}

//==================================================================
void DSharinguApp::Create( bool start_minimized )
{
	TCHAR szPath[ PSYS_MAX_PATH ];

	if ( SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath) ) )
	{
		strcat_s( szPath, sizeof(szPath), "\\DSharingu\\" );
		
		if NOT( doesDirExist(szPath) )
			_mkdir( szPath );
	}
	psys_strcpy( _config_pathname, szPath, sizeof(_config_pathname) );
	strcat_s( _config_pathname, sizeof(_config_pathname), _config_fname );
	

	// first try in the application data directory
	FILE	*fp;
	errno_t	err;
	if ( err = fopen_s( &fp, _config_pathname, "rt" ) )	// if it fails, then try in the current directory
		err = fopen_s( &fp, _config_fname, "rt" );

	if NOT( err )
	{
		DataSchema::LoaderItem	loader_items[2] =
		{
			"Settings", Settings::SchemaLoaderProc_s, &_settings,
			"RemoteDef", RemoteMng::RemoteDefLoaderProc_s, &_remote_mng,
		};
		DataSchema::LoadSchemas_s( fp, loader_items, 2 );

		//_settings._schema.LoadData( fp );
		//_remote_mng.LoadList( fp );
		fclose( fp );
	}


	_inpack_buffp = (u_char *)PSYS_MALLOC( INPACK_BUFF_SIZE );

	_main_menu = LoadMenu( (HINSTANCE)win_system_getinstance(), MAKEINTRESOURCE(IDR_MAINMENU) );

	win_init_quick( &_main_win, WINDOW_TITLE, NULL,
					this, mainEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_THIS_X1, 640, WIN_ANCH_TYPE_THIS_Y1, 490,
					(win_init_flags)(WIN_INIT_FLG_SYSTEM |
									 WIN_INIT_FLG_INVISIBLE |
									 WIN_INIT_FLG_CLIENTEDGE |
									 WIN_INTFLG_DONT_CLEAR ),
					//(win_init_flags)(0*WIN_INIT_FLG_OPENGL | 0*WIN_INIT_FLG_CLIENTEDGE),
					_main_menu );

	win_init_quick( &_dbg_win, APP_NAME" Debug Window", NULL,
					this, dbgEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_THIS_X1, 400, WIN_ANCH_TYPE_THIS_Y1, 256,
					(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR | WIN_INIT_FLG_INVISIBLE | WIN_INIT_FLG_CLIENTEDGE) );

	homeWinCreate();

#ifdef _DEBUG
	_dbg_win.Show( false );
#endif

	_main_win.Show( true, start_minimized || _settings._start_minimized );

	_chmanagerp = new DSChannelManager( &_main_win, this, channelSwitch_s, onChannelDelete_s );


	if ( _settings._username[0] == 0 || _settings._password.IsEmpty() )
	{
		if ( MessageBox( _main_win._hwnd,
			"To connect and to receive calls, you need to choose a Username and Password in the Settings dialog.\n"
			"Do you want to do it now ?",
			"DSharingu - Settings Required", MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );		
		}
	}

	handleChangedSettings();

	updateViewMenu( NULL );
}

//==================================================================
static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
		glLoadIdentity();

	glMatrixMode( GL_MODELVIEW );
}

//=====================================================
HWND DSharinguApp::openModelessDialog( void *mythisp, DLGPROC dlg_proc, LPSTR dlg_namep )
{
	HWND	hwnd =
		CreateDialogParam( (HINSTANCE)win_system_getinstance(),
							dlg_namep, _main_win._hwnd,
							dlg_proc, (LPARAM)mythisp );

	appbase_add_modeless_dialog( hwnd );
	ShowWindow( hwnd, SW_SHOWNORMAL );

	return hwnd;
}

//=====================================================
int DSharinguApp::mainEventFilter( win_event_type etype, win_event_t *eventp )
{
	if ( _cur_chanp )
		_cur_chanp->_console.cons_parent_eventfilter( NULL, etype, eventp );

	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		if ( _cur_chanp )
			_cur_chanp->_console._win.SetFocus();
		break;

	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
//		reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		break;

	case WIN_ETYPE_COMMAND:
		switch ( eventp->command )
		{
		case ID_FILE_CONNECTIONS:
			_remote_mng.OpenDialog( &_main_win,
									handleChangedRemoteManager_s,
									handleCallRemoteManager_s,
									this );
			break;

		case ID_FILE_HANGUP:
			if ( _cur_chanp )
				_cur_chanp->DoDisconnect( "Successfully disconnected." );
			break;

		case ID_FILE_SETTINGS:
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );
			break;

		case ID_FILE_EXIT:
			PostMessage( _main_win._hwnd, WM_CLOSE, 0, 0 );
			break;

		case ID_VIEW_FITWINDOW:
			if ( _cur_chanp )
			{
				_cur_chanp->_view_fitwindow = true;
				_cur_chanp->updateViewScale();
				updateViewMenu( _cur_chanp );
				_cur_chanp->_view_winp->Invalidate();
			}
			break;

		case ID_VIEW_ACTUALSIZE:
			if ( _cur_chanp )
			{
				_cur_chanp->_view_fitwindow = false;
				_cur_chanp->updateViewScale();
				updateViewMenu( _cur_chanp );
				_cur_chanp->_view_winp->Invalidate();
			}
			break;

		case ID_VIEW_SHELL:
			if ( _cur_chanp )
			{
				_cur_chanp->setShellVisibility( true );
			}
			break;


		case ID_HELP_CHECKFORUPDATES:
			if NOT( _download_updatep )
			{
				_download_updatep = new DownloadUpdate( _main_win._hwnd,
											APP_VERSION_STR,
											"kazzuya.com",
											"/dsharingu_data/update_info.txt",
											"/dsharingu_data/",
											"DSharingu - Download Update" );
			}
			break;

		case ID_HELP_ABOUT:
			if NOT( _about_is_open )
			{
				WGUT::OpenModelessDialog( (DLGPROC)aboutDialogProc_s, MAKEINTRESOURCE(IDD_ABOUT), _main_win._hwnd, this );
				_about_is_open = true;
			}
			break;
		}
		break;

	case WIN_ETYPE_DESTROY:
		_chmanagerp->Quit();
		//setState( STATE_QUIT );
		PostQuitMessage(0);
		break;
	}

	return 0;
}
//==================================================================
int DSharinguApp::mainEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSharinguApp	*mythis = (DSharinguApp *)userobjp;
	return mythis->mainEventFilter( etype, eventp );
}

//==================================================================
//==================================================================
void DSharinguApp::handleChangedRemoteManager_s( void *mythis, RemoteDef *changed_remotep )
{
	((DSharinguApp *)mythis)->handleChangedRemoteManager( changed_remotep );
}
//==================================================================
void DSharinguApp::handleChangedRemoteManager( RemoteDef *changed_remotep )
{
	saveConfig();

	DSChannel	*chanp = (DSChannel *)changed_remotep->GetUserData();

	if ( chanp && chanp->_is_transmitting )
	{
		chanp->_session_remotep->_see_remote_screen = changed_remotep->_see_remote_screen;
		chanp->setViewMode();

		UsageWishMsg	msg( changed_remotep->_see_remote_screen,
							 chanp->_is_using_remote );

		if ERR_ERROR( chanp->_cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
}

//==================================================================
void DSharinguApp::handleCallRemoteManager_s( void *mythis, RemoteDef *remotep )
{
	((DSharinguApp *)mythis)->handleCallRemoteManager( remotep );
}
//==================================================================
void DSharinguApp::handleCallRemoteManager( RemoteDef *remotep )
{
	if ( _settings._username[0] == 0 || _settings._password.IsEmpty() )
	{
		if ( MessageBox( _main_win._hwnd,
				"Please choose a Username and Password in the Settings dialog before trying to connect.\n"
				"Do you want to do it now ?",
				"Calling Problem", MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );		
		}
		return;
	}

	//try
	{
		_chmanagerp->RecycleOrNewChannel( remotep, false );
	}// catch(...) {
	//	PSYS_ASSERT( 0 );
	//}
}

//==================================================================
void DSharinguApp::dbgDoPaint()
{
	debugout_reset();

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	begin_2D( NULL, NULL );
	//win_context_begin_ext( winp, 1, false );

		glPushMatrix();
		// render

		glColor3f( 1, 1, 1 );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_CULL_FACE );
/*
		CompakStats	stats = _cpk.GetStats();

		debugout_printf( "Total OUT: %i B   (%u KB)", stats._total_send_bytes, stats._total_send_bytes / 1024 );
		debugout_printf( "Total IN : %i B   (%u KB)", stats._total_recv_bytes, stats._total_recv_bytes / 1024 );
		debugout_printf( "Speed OUT: %u B/s (%u KB/s)", (u_int)stats._send_bytes_per_sec_window, (u_int)(stats._send_bytes_per_sec_window / 1024) );
		debugout_printf( "Speed IN : %u B/s (%u KB/s)", (u_int)stats._recv_bytes_per_sec_window, (u_int)(stats._recv_bytes_per_sec_window / 1024) );
		debugout_printf( "Queue OUT: %u B/s (%u KB/s)", (u_int)stats._send_bytes_queue, (u_int)(stats._send_bytes_queue / 1024) );
*/
		glLoadIdentity();
		glTranslatef( 0, 0, 0 );
		debugout_render();

		glPopMatrix();

	end_2D();
	//win_context_end( winp );
}

//==================================================================
int DSharinguApp::dbgEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		dbgDoPaint();
		break;

	case WIN_ETYPE_DESTROY:
		//PostQuitMessage(0);
		break;
	}

	return 0;
}

//==================================================================
int DSharinguApp::dbgEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSharinguApp	*mythis = (DSharinguApp *)userobjp;
	return mythis->dbgEventFilter( etype, eventp );
}


//==================================================================
void DSharinguApp::StartListening( int port_listen )
{
	// $$$ _console.cons_line_printf( CHNTAG"Started !" );
	if ( _com_listener.StartListen( port_listen ) )
	{
		//msg_badlisten( DEF_PORT_NUMBER );
		// $$$ _console.cons_line_printf( CHNTAG"PROBLEM: Port %i is busy. Server cannot accept calls.", port_listen );
	}
	else
	{
		//state_set( STATE_LISTENING );
		//state_set( STATE_ACCEPTING_CONNECTIONS );
		// $$$ _console.cons_line_printf( CHNTAG"OK: Listening on port %i for incoming calls.", port_listen );
	}
}
/*
//==================================================================
void IntSysMessageParser::TakeMsg( UINT msg, WPARAM wParam, LPARAM lParam )
{
	if ( msg == MSG_CPK_INPACK )
	{
		Message	*messagep = (Message *)lParam;

		InteractiveSystem::ProcessMessage_s( messagep->_pack_id, messagep->datap, messagep->_wd, messagep->_he );

		SAFE_FREE( messagep );
	}
}
*/

//==================================================================
void DSharinguApp::handleChangedSettings_s( void *mythis )
{
	((DSharinguApp *)mythis)->handleChangedSettings();
}
//==================================================================
void DSharinguApp::handleChangedSettings()
{
	saveConfig();

	homeWinOnChangedSettings();

	if ( _settings._listen_for_connections )
	{	// only restart listening if the port has changed
		if ( _com_listener.GetListenPort() != _settings._listen_port )
		{
			_com_listener.StopListen();
			StartListening( _settings._listen_port );
		}
	}
	else
		_com_listener.StopListen();

	for (int i=0; i < _chmanagerp->_n_channels; ++i)
	{
		DSChannel	*chanp = (DSChannel *)_chmanagerp->_channelsp[i];

		if ( chanp )
		{
			chanp->_intersys.ActivateExternalInput( _settings._share_my_desktop && _settings._show_my_desktop );

			if ( chanp->_is_transmitting )
			{
				UsageAbilityMsg	msg(_settings._show_my_desktop,
					_settings._share_my_desktop );
				if ERR_ERROR( chanp->_cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}
		}
	}

	std::string	title( WINDOW_TITLE );

	title += " - ";
	
	if ( _settings._username[0] )
	{
		title += _settings._username;
	}
	else
		title += "no username !";

	_main_win.SetTitle( title.c_str() );
}

//==================================================================
void DSharinguApp::handleAutoCall()
{
	for (int i=0; i < _remote_mng._remotes_list.len(); ++i)
	{
		RemoteDef	*remotep = _remote_mng._remotes_list[i];
		if ( remotep->_call_automatically )
		{
			bool	is_being_handled = false;
			for (int j=0; j < _chmanagerp->_n_channels; ++j)
			{
				DSChannel	*chanp = _chmanagerp->_channelsp[j];
				if ( chanp && chanp->_session_remotep == remotep )
				{
					if ( chanp->GetState() == DSChannel::STATE_IDLE )
					{
						try
						{
							chanp->CallRemote( true );
						} catch(...) {
							PSYS_ASSERT( 0 );
						}
					}
					is_being_handled = true;
				}
			}

			if NOT( is_being_handled )
			{
				if ( _settings._username[0] != 0 && !_settings._password.IsEmpty() )
				{
					try
					{
						_chmanagerp->RecycleOrNewChannel( remotep, true );
					} catch(...) {
						PSYS_ASSERT( 0 );
					}
				}
			}
		}
	}
}
//==================================================================
int DSharinguApp::Idle()
{
	if ( _download_updatep )
	{
		if NOT( _download_updatep->Idle() )
			SAFE_DELETE( _download_updatep );
	}
	else
	{
		double	now_time = psys_timer_get_d();
		if ( now_time - _last_autocall_check_time >= 1000.0 )
		{
			_last_autocall_check_time = now_time;
			handleAutoCall();
		}

		int	accepted_fd;
		if ( _com_listener.Idle( accepted_fd ) == COM_ERR_CONNECTED )
		{
			try
			{
				_chmanagerp->NewChannel( accepted_fd );
			} catch(...) {
				PSYS_ASSERT( 0 );
			}
		}
	}

	_chmanagerp->Idle();

	return DSChannel::STATE_IDLE;
}

//==================================================================
void DSharinguApp::channelSwitch_s( DSharinguApp *superp, DSChannel *new_sel_chanp, DSChannel *old_sel_chanp )
{
	((DSharinguApp *)superp)->channelSwitch( new_sel_chanp, old_sel_chanp );
}
//==================================================================
void DSharinguApp::channelSwitch( DSChannel *new_sel_chanp, DSChannel *old_sel_chanp )
{
	if ( old_sel_chanp )
		old_sel_chanp->Show( false );
	else
		_home_winp->Show( false );

	if ( new_sel_chanp )
		new_sel_chanp->Show( true );
	else
		_home_winp->Show( true );

	_cur_chanp = (DSChannel *)new_sel_chanp;
	updateViewMenu( (DSChannel *)new_sel_chanp );
}

//==================================================================
void DSharinguApp::onChannelDelete_s( DSharinguApp *superp, DSChannel *chanp )
{
	((DSharinguApp *)superp)->onChannelDelete( chanp );
}
//==================================================================
void DSharinguApp::onChannelDelete( DSChannel *chanp )
{
	if ( _cur_chanp == chanp )
	{
		PSYS_ASSERT( 0 );
		_cur_chanp = NULL;
	}

//	SAFE_DELETE( chanp );
}
