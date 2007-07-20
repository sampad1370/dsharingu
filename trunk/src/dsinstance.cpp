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

#define WINDOW_TITLE		(APP_NAME _T(" ") APP_VERSION_STR)

using namespace PUtils;

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
		SetDlgItemText( hwnd, IDC_ABOUT_APPNAME, APP_NAME _T(" ") APP_VERSION_STR );
		break; 

    case WM_COMMAND:
	    switch(LOWORD(wparam))
		{
		case IDC_HOMEPAGE:
			ShellExecute( hwnd, _T("open"), _T("http://dsharingu.kazzuya.com"),
						  NULL, NULL, SW_SHOWNORMAL );
			break;

		case IDC_MANUAL:
			ShellExecute( hwnd, _T("open"), _T("manual\\index.html"),
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
DSharinguApp::DSharinguApp( const TCHAR *config_fnamep ) :
	_cur_chanp(NULL),
	_inpack_buffp(NULL),
	_about_is_open(false),
	_download_updatep(NULL),
	_main_menu(NULL),
	_chmanagerp(NULL),
	_home_winp(NULL),
	_last_autocall_check_time(0.0),
	_cur_lang(LANG_EN)
{
	_tcscpy_s( _config_fname, config_fnamep );
	_config_pathname[0] = 0;
}

//===============================================================
DSharinguApp::~DSharinguApp()
{
	saveConfig();

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

		if ( chanp->_console->IsShowing() )
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
	errno_t	err = _tfopen_s( &fp, _config_pathname, _T("wt") );

	if NOT( err )
	{
		_settings._schema.SaveData( fp );
		_remote_mng.SaveList( fp );
		fclose( fp );
	}
}

//==================================================================
static void cut_spaces( TCHAR *strp )
{
	TCHAR	*s = strp;
	TCHAR	*d = strp;
	while ( *s )
	{
		if ( *s != _TXCHAR(' ') )
			*d++ = *s;

		++s;
	}
	*d++ = 0;
}

//==================================================================
void DSharinguApp::cmd_debug( TCHAR *params[], int n_params )
{
	_dbg_win.Show( !_dbg_win.IsShowing() );
}

//==================================================================
void DSharinguApp::cmd_debug_s( void *userp, TCHAR *params[], int n_params )
{
	((DSharinguApp *)userp)->cmd_debug( params, n_params );
}

//==================================================================
static bool doesDirExist( const TCHAR *dirnamep )
{
	TCHAR	buff[ PSYS_MAX_PATH ];
	_tgetcwd( buff, PSYS_MAX_PATH-1 );
	bool exists = (_tchdir( dirnamep) == 0);
	_tchdir( buff );

	return exists;
}

//==================================================================
static ImageBase *LoadImageBaseResource( HINSTANCE hi, LPCTSTR resp )
{
	ImageBase	*imgp;

	HRSRC hrsrc = FindResource( hi, resp, RT_BITMAP );
	DWORD size = SizeofResource( hi, hrsrc );

	HGLOBAL hglob = LoadResource( hi, hrsrc );
	void *resdatap = LockResource( hglob );

	imgp = new ImageBase();
	try
	{
		ImageBase::LoadParams	params;
		params._do_convert_truecolor = true;

		imgp->LoadBMP( Memfile(resdatap,size), &params );
	}
	catch (...)
	{
		UnlockResource( hglob );
	}

	UnlockResource( hglob );

	return imgp;
}

//==================================================================
void DSharinguApp::Create( bool start_minimized )
{
	HBITMAP dude = LoadBitmap( WinSys::GetInstance(), MAKEINTRESOURCE(IDB_ICO_EN) );

	_ico_en_imgp = LoadImageBaseResource( WinSys::GetInstance(), MAKEINTRESOURCE(IDB_ICO_EN) );
	_ico_ja_imgp = LoadImageBaseResource( WinSys::GetInstance(), MAKEINTRESOURCE(IDB_ICO_JA) );
	_ico_it_imgp = LoadImageBaseResource( WinSys::GetInstance(), MAKEINTRESOURCE(IDB_ICO_IT) );

	TCHAR szPath[ PSYS_MAX_PATH ];

	if ( SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath) ) )
	{
		_tcscat_s( szPath, _T("\\DSharingu\\") );

		if NOT( doesDirExist(szPath) )
			_tmkdir( szPath );
	}
	_tcscpy_s( _config_pathname, szPath );
	_tcscat_s( _config_pathname, _config_fname );

	// first try in the application data directory
	FILE	*fp;
	errno_t	err;
	if ( err = _tfopen_s( &fp, _config_pathname, _T("rt") ) )	// if it fails, then try in the current directory
		err = _tfopen_s( &fp, _config_fname, _T("rt") );

	if NOT( err )
	{
		DataSchema::LoaderItem	loader_items[2] =
		{
			_T("Settings"), Settings::SchemaLoaderProc_s, &_settings,
			_T("RemoteDef"), RemoteMng::RemoteDefLoaderProc_s, &_remote_mng,
		};
		DataSchema::LoadSchemas_s( fp, loader_items, 2 );

		//_settings._schema.LoadData( fp );
		//_remote_mng.LoadList( fp );
		fclose( fp );
	}


	_inpack_buffp = (u_char *)PSYS_MALLOC( INPACK_BUFF_SIZE );

	_main_menu = LoadMenu( (HINSTANCE)WinSys::GetInstance(), MAKEINTRESOURCE(IDR_MAINMENU) );

	win_init_quick( &_main_win, WINDOW_TITLE, NULL,
					this, mainEventFilter_s,
					Window::ANCH_TYPE_FIXED, 0,
					Window::ANCH_TYPE_FIXED, 0,
					Window::ANCH_TYPE_THIS_X1, 640,
					Window::ANCH_TYPE_THIS_Y1, 490,
					(win_init_flags)(WIN_INIT_FLG_SYSTEM |
									 WIN_INIT_FLG_INVISIBLE |
									 WIN_INIT_FLG_CLIENTEDGE |
									 WIN_INTFLG_DONT_CLEAR ),
					//(win_init_flags)(0*WIN_INIT_FLG_OPENGL | 0*WIN_INIT_FLG_CLIENTEDGE),
					_main_menu );

	win_init_quick( &_dbg_win, APP_NAME _T(" Debug Window"), NULL,
					this, dbgEventFilter_s,
					Window::ANCH_TYPE_FIXED, 0,
					Window::ANCH_TYPE_FIXED, 0,
					Window::ANCH_TYPE_THIS_X1, 400,
					Window::ANCH_TYPE_THIS_Y1, 256,
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
			_T("To connect and to receive calls, you need to choose a Username and Password in the Settings dialog.\n")
			_T("Do you want to do it now ?"),
			_T("DSharingu - Settings Required"), MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			openSettings();
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
HWND DSharinguApp::openModelessDialog( void *mythisp, DLGPROC dlg_proc, LPTSTR dlg_namep )
{
	HWND	hwnd =
		CreateDialogParam( (HINSTANCE)WinSys::GetInstance(),
							dlg_namep, _main_win._hwnd,
							dlg_proc, (LPARAM)mythisp );

	appbase_add_modeless_dialog( hwnd );
	ShowWindow( hwnd, SW_SHOWNORMAL );

	return hwnd;
}

//==================================================================
int DSharinguApp::mainEventFilter_s( void *userobjp, WindowEvent *eventp )
{
	DSharinguApp	*mythis = (DSharinguApp *)userobjp;
	return mythis->mainEventFilter( eventp );
}
//=====================================================
int DSharinguApp::mainEventFilter( WindowEvent *eventp )
{
	if ( _cur_chanp )
		_cur_chanp->_console->cons_parent_eventfilter( NULL, eventp );

	switch ( eventp->GetType() )
	{
	case WindowEvent::ETYPE_ACTIVATE:
		if ( _cur_chanp )
			_cur_chanp->_console->_win.SetFocus();
		break;

	case WindowEvent::ETYPE_CREATE:
		break;

	case WindowEvent::ETYPE_WINRESIZE:
		if ( _chmanagerp )
		{
			for (int i=1; i < _chmanagerp->_n_channels; ++i)
				_chmanagerp->_channelsp[i]->CheckForVisibility();
		}
		break;

	case WindowEvent::ETYPE_SHOW:
		break;

	case WindowEvent::ETYPE_PAINT:
		break;

	case WindowEvent::ETYPE_COMMAND:
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
				_cur_chanp->DoDisconnect( _T("Successfully disconnected.") );
			break;

		case ID_FILE_SETTINGS:
			openSettings();
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
											APP_VERSION_STR_UTF8,
											"dsharingu.kazzuya.com",
											"/dsharingu_data/update_info2.txt",
											_T("DSharingu - Download Update") );
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

	case WindowEvent::ETYPE_DESTROY:
		_chmanagerp->Quit();
		//setState( STATE_QUIT );
		PostQuitMessage(0);
		break;
	}

	return 0;
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
		chanp->_session_remotep->_can_watch_my_desk = changed_remotep->_can_watch_my_desk;
		chanp->_session_remotep->_can_use_my_desk = changed_remotep->_can_use_my_desk;
		chanp->_session_remotep->_see_remote_screen = changed_remotep->_see_remote_screen;
		chanp->setViewMode();

		chanp->_intersys.ActivateExternalInput(
								channelCanWatch( chanp ) && 
								channelCanUse( chanp ) );

		sendUsageAbility( chanp );

		UsageWishMsg	msg( changed_remotep->_see_remote_screen,
							 chanp->_is_using_remote,
							 chanp->isDeskViewable() );

		NetSendMessage<UsageWishMsg>( chanp->_cpk, msg );
	}
}

//==================================================================
void DSharinguApp::openSettings()
{
	LPCTSTR	resnamep;

	switch ( _cur_lang )
	{
	case LANG_IT: resnamep = MAKEINTRESOURCE(IDD_SETTINGS_IT); break;
	case LANG_JA: resnamep = MAKEINTRESOURCE(IDD_SETTINGS_JA); break;
	case LANG_EN:
	default:
		resnamep = MAKEINTRESOURCE(IDD_SETTINGS);
		break;
	}

	_settings.OpenDialog( &_main_win, resnamep, handleChangedSettings_s, this );
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
				_T("Please choose a Username and Password in the Settings dialog before trying to connect.\n")
				_T("Do you want to do it now ?"),
				_T("Calling Problem"), MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			openSettings();
		}
		return;
	}

	//try
	{
		_chmanagerp->RecycleOrNewChannel( remotep, false );
	}// catch(...) {
	//	PASSERT( 0 );
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
int DSharinguApp::dbgEventFilter( WindowEvent *eventp )
{
	switch ( eventp->GetType() )
	{
	case WindowEvent::ETYPE_CREATE:
		break;

	case WindowEvent::ETYPE_WINRESIZE:
		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WindowEvent::ETYPE_PAINT:
		dbgDoPaint();
		break;

	case WindowEvent::ETYPE_DESTROY:
		//PostQuitMessage(0);
		break;
	}

	return 0;
}

//==================================================================
int DSharinguApp::dbgEventFilter_s( void *userobjp, WindowEvent *eventp )
{
	DSharinguApp	*mythis = (DSharinguApp *)userobjp;
	return mythis->dbgEventFilter( eventp );
}


//==================================================================
void DSharinguApp::StartListening( int port_listen )
{
	// $$$ _console->cons_line_printf( CHNTAG"Started !" );
	if ( _com_listener.StartListen( port_listen ) )
	{
		//msg_badlisten( DEF_PORT_NUMBER );
		// $$$ _console->cons_line_printf( CHNTAG"PROBLEM: Port %i is busy. Server cannot accept calls.", port_listen );
	}
	else
	{
		//state_set( STATE_LISTENING );
		//state_set( STATE_ACCEPTING_CONNECTIONS );
		// $$$ _console->cons_line_printf( CHNTAG"OK: Listening on port %i for incoming calls.", port_listen );
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
			chanp->_intersys.ActivateExternalInput(
									channelCanWatch( chanp ) && 
									channelCanUse( chanp ) );

			sendUsageAbility( chanp );
		}
	}

	PSYS::tstring	title( WINDOW_TITLE );

	title += _T(" - ");
	
	if ( _settings._username[0] )
	{
		title += _settings._username;
	}
	else
		title += _T("no username !");

	_main_win.SetTitle( title.c_str() );
}

//==================================================================
bool DSharinguApp::channelCanWatch( const DSChannel *chanp )
{
	return !_settings._nobody_can_watch_my_computer && chanp->_session_remotep->_can_watch_my_desk;
}
//==================================================================
bool DSharinguApp::channelCanUse( const DSChannel *chanp )
{
	return !_settings._nobody_can_use_my_computer && chanp->_session_remotep->_can_use_my_desk;
}
//==================================================================
void DSharinguApp::sendUsageAbility( DSChannel *chanp )
{
	if ( chanp->_is_transmitting )
	{
		UsageAbilityMsg	msg( channelCanWatch( chanp ),
							 channelCanUse( chanp ) );

		NetSendMessage<UsageAbilityMsg>( chanp->_cpk, msg );
	}
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
							PASSERT( 0 );
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
						PASSERT( 0 );
					}
				}
			}
		}
	}
}
//==================================================================
bool DSharinguApp::Idle()
{
	bool	quitting = Application::Idle();

	if ( _download_updatep )
	{
		if NOT( _download_updatep->Idle() )
			SAFE_DELETE( _download_updatep );
	}
	else
	{
		double	now_time = PSYS::TimerGetD();
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
				PASSERT( 0 );
			}
		}
	}

	_chmanagerp->Idle();

	return quitting;
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
		PASSERT( 0 );
		_cur_chanp = NULL;
	}

//	SAFE_DELETE( chanp );
}
