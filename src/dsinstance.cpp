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


#define WINDOW_TITLE		APP_NAME" " APP_VERSION_STR " by Davide Pasca 2006 ("__DATE__" "__TIME__ ")"


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
	_chmanagerp(NULL)
{
	psys_strcpy( _config_fname, config_fnamep, sizeof(_config_fname) );
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
		EnableMenuItem( _main_menu, ID_VIEW_SHELL, FALSE );
		EnableMenuItem( _main_menu, ID_VIEW_FITWINDOW, FALSE );
		EnableMenuItem( _main_menu, ID_VIEW_ACTUALSIZE, FALSE );
	}
	else
	{
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
	FILE *fp = fopen( _config_fname, "wt" );

	PSYS_ASSERT( fp != NULL );
	if ( fp )
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
	if ( win_is_showing( &_dbg_win ) )
		win_show( &_dbg_win, 0 );
	else
		win_show( &_dbg_win, 1 );
}

//==================================================================
void DSharinguApp::cmd_debug_s( void *userp, char *params[], int n_params )
{
	((DSharinguApp *)userp)->cmd_debug( params, n_params );
}

//==================================================================
void DSharinguApp::Create( bool start_minimized )
{
	FILE	*fp = fopen( _config_fname, "rt" );

	if ( fp )
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
					WIN_ANCH_TYPE_THIS_X1, 640, WIN_ANCH_TYPE_THIS_Y1, 512,
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

#ifdef _DEBUG
	win_show( &_dbg_win, 0 );
#endif

	win_show( &_main_win, true, start_minimized || _settings._start_minimized );

	_chmanagerp = new ChannelManager( &_main_win, this );


	if ( _settings._username[0] == 0 || _settings._password.IsEmpty() )
	{
		if ( MessageBox( _main_win.hwnd,
			"To connect and to receive calls, you need to choose a Username and Password in the Settings dialog.\n"
			"Do you want to do it now ?",
			"DSharingu - Settings Required", MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );		
		}
	}

	if ( _settings._listen_for_connections )
		StartListening( _settings._listen_port );

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
							dlg_namep, _main_win.hwnd,
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
			win_focus( &_cur_chanp->_console._win, true );
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
				_cur_chanp->doDisconnect( "Successfully disconnected." );
			break;

		case ID_FILE_SETTINGS:
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );
			break;

		case ID_FILE_EXIT:
			if ( _cur_chanp )
				_cur_chanp->setState( DSChannel::STATE_QUIT );
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
				_download_updatep = new DownloadUpdate( _main_win.hwnd,
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
				WGUT::OpenModelessDialog( (DLGPROC)aboutDialogProc_s, MAKEINTRESOURCE(IDD_ABOUT), _main_win.hwnd, this );
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
		chanp->setInteractiveMode( changed_remotep->_use_remote_screen );

		UsageWishMsg	msg( changed_remotep->_see_remote_screen,
							 changed_remotep->_use_remote_screen );

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
		if ( MessageBox( _main_win.hwnd,
				"Please choose a Username and Password in the Settings dialog before trying to connect.\n"
				"Do you want to do it now ?",
				"Calling Problem", MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );		
		}
		return;
	}

	switchChannel( _chmanagerp->NewChannel( remotep ) );
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
		DSChannel	*chanp = _chmanagerp->_channelsp[i];

		chanp->_intersys.ActivateExternalInput( _settings._share_my_screen && _settings._show_my_screen );

		if ( chanp->_is_transmitting )
		{
			UsageAbilityMsg	msg(_settings._show_my_screen,
								_settings._share_my_screen );
			if ERR_ERROR( chanp->_cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
				return;
		}
	}
}

//==================================================================
DSChannel::State DSharinguApp::Idle()
{
	if ( _download_updatep )
	{
		if NOT( _download_updatep->Idle() )
			SAFE_DELETE( _download_updatep );
	}

	int	accepted_fd;
	if ( _com_listener.Idle( accepted_fd ) == COM_ERR_CONNECTED )
	{
		switchChannel( _chmanagerp->NewChannel( accepted_fd ) );
		
	}

	_chmanagerp->Idle();

	return DSChannel::STATE_IDLE;
}
