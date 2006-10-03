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

#define CHNTAG				"* "

//==================================================================
///
//==================================================================
DSChannel::DSChannel( DSharinguApp *superp, int accepted_fd ) :
	_intersys( &_cpk )
{
	create( superp );

	if PTRAP_FALSE( accepted_fd >= 0 )
	{
		_cpk.SetAcceptedConn( accepted_fd );
		setState( STATE_CONNECTED );
	}
	else
		setState( STATE_IDLE );

}

//==================================================================
DSChannel::DSChannel( DSharinguApp *superp, RemoteDef *remotep ) :
	_intersys( &_cpk )
{
	create( superp );

	_session_remotep = remotep;

	remotep->SetUserData( this );
	remotep->Lock();

	int	err = _cpk.Call( remotep->_rm_ip_address, remotep->GetCallPortNum() );
	switch ( err )
	{
	case 0:
		setState( STATE_CONNECTING );
		_connecting_dlg_hwnd =
			CreateDialogParam( (HINSTANCE)win_system_getinstance(),
			MAKEINTRESOURCE(IDD_CONNECTING), _superp->_main_win.hwnd,
			(DLGPROC)connectingDialogProc_s, (LPARAM)this );
		appbase_add_modeless_dialog( _connecting_dlg_hwnd );
		ShowWindow( _connecting_dlg_hwnd, SW_SHOWNORMAL );
		break;

	case COM_ERR_INVALID_ADDRESS:
		MessageBox( _superp->_main_win.hwnd, "The Internet Address seems to be invalid.\nPlease, review it.",
									"Connection Problem", MB_OK | MB_ICONSTOP );

		_session_remotep->Unlock();
		_superp->_remote_mng.InvalidAddressOnCall();
		break;

	default:
		MessageBox( _superp->_main_win.hwnd, "Error occurred while trying to call.",
									"Connection Problem", MB_OK | MB_ICONSTOP );

		_session_remotep->Unlock();
		break;
	}
}

//==================================================================
void DSChannel::create( DSharinguApp *superp )
{
	_superp = superp;
	_state = STATE_IDLE;
	_session_remotep = NULL;
	_view_fitwindow = true;
	_view_scale_x = 1;
	_view_scale_y = 1;
	_disp_off_x = 0;
	_disp_off_y = 0;
	_frame_since_transmission = 0;
	_intersys =  &_cpk;
	_is_transmitting = false;
	_is_connected = false;
	_connecting_dlg_hwnd = NULL;

	_intersys.Activate( false );
/*
	_tool_winp = new win_t( "tool win", &_superp->_main_win,
							this, toolEventFilter_s,
							WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
							WIN_ANCH_TYPE_PARENT_X2, 0, WIN_ANCH_TYPE_PARENT_Y1, 30,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR) );
*/
	_view_winp = new win_t( "view win", &_superp->_main_win,
							this, viewEventFilter_s,
							WIN_ANCH_TYPE_FIXED, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 22,
							WIN_ANCH_TYPE_PARENT_X2, 0,
							WIN_ANCH_TYPE_PARENT_Y2, -160,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR | 0*WIN_INIT_FLG_HSCROLL | 0*WIN_INIT_FLG_VSCROLL) );

	//-----------------------------------------------
	_console.cons_init( &_superp->_main_win, (void *)this );
	_console.cons_line_cb_set( console_line_func_s );
	_console.cons_line_printf( "%s -- %s, %s", APP_NAME" "APP_VERSION_STR, __DATE__, __TIME__ );
	_console.cons_line_printf( "by Davide Pasca - http://kazzuya.com/dsharingu" );
	_console.cons_line_printf( "This software is a TEST release of a work in progress !!!" );
	_console.cons_line_printf( "Type /help for help" );
	_console.cons_cmd_add_defs( _cmd_defs );
	_console.cons_show( 1 );

	setShellVisibility();
	changeSessionRemote( NULL );

	// we need to initialize this after loading the settings
	_intersys.ActivateExternalInput( _superp->_settings._share_my_screen && _superp->_settings._show_my_screen );
	_cpk.SetOnPackCallback( REMOCON_ARRAY_PKID, InteractiveSystem::OnPackCallback_s, &_intersys );

	updateViewScale();
}

//==================================================================
void DSChannel::setState( State state )
{
//	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	switch ( _state = state )
	{
	case STATE_CONNECTING:
		//gam.EnableGadget( DSharinguApp::BUTT_CONNECTIONS, false );
//		gam.EnableGadget( DSharinguApp::BUTT_HANGUP, true );
		EnableMenuItem( _superp->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_ENABLED );
		//gam.SetGadgetText( DSharinguApp::BUTT_CONNECTIONS, "[ ] Calling..." );
		break;

	case STATE_CONNECTED:
		onConnect( _cpk.IsConnectedAsCaller() );
		break;

	case STATE_DISCONNECT_START:
		{
//			GGET_Manager	&gam = _tool_winp->GetGGETManager();

			WGUT::SafeDestroyWindow( _connecting_dlg_hwnd );

			_superp->_scrwriter.StopGrabbing();

//			gam.EnableGadget( DSharinguApp::BUTT_CONNECTIONS, false );
//			gam.EnableGadget( DSharinguApp::BUTT_HANGUP, false );
			EnableMenuItem( _superp->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
			//gam.SetGadgetText( DSharinguApp::BUTT_CONNECTIONS, "[ ] Connections..." );

			_is_connected = false;
			_is_transmitting = false;
			changeSessionRemote( NULL );
			if ( _session_remotep )
				_session_remotep->Unlock();

			setInteractiveMode( false );

			_state = STATE_DISCONNECTING;
		}
		break;

	case STATE_DISCONNECTING:
		break;

	case STATE_DISCONNECTED:
		{
//			GGET_Manager	&gam = _tool_winp->GetGGETManager();
//			gam.EnableGadget( DSharinguApp::BUTT_CONNECTIONS, true );
			_state = STATE_IDLE;
		}
		break;

	case STATE_QUIT:
		break;
	}
}

//==================================================================
void DSChannel::changeSessionRemote( RemoteDef *new_remotep )
{
	_session_remotep = new_remotep;
//	if ( _session_remotep )
//		_tool_winp->GetGGETManager().EnableGadget( DSharinguApp::BUTT_USEREMOTE, true );
//	else
//		_tool_winp->GetGGETManager().EnableGadget( DSharinguApp::BUTT_USEREMOTE, false );
}

//==================================================================
cons_cmd_def_t	DSChannel::_cmd_defs[] =
{
"/debug",		DSharinguApp::cmd_debug_s		,"/debug           : debug",
0
};

//==================================================================
void DSChannel::console_line_func_s( void *userp, const char *txtp, int is_cmd )
{
	((DSChannel *)userp)->console_line_func( txtp, is_cmd );
}

//==================================================================
void DSChannel::console_line_func( const char *txtp, int is_cmd )
{
	if ( _is_connected )
	{
		if ( txtp[0] )
		{
			int err = _cpk.SendPacket( TEXT_MSG_PKID, txtp, (strlen( txtp )+1), NULL );

			PSYS_ASSERT( err == 0 );
		}
	}
}

//==================================================================
bool DSChannel::getInteractiveMode()
{
	return _intersys.IsActive();
}

//==================================================================
void DSChannel::setShellVisibility( bool do_switch )
{
//	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	if ( do_switch )
	{
		_console.cons_show( !_console.cons_is_showing() );
	}

	if ( _console.cons_is_showing() )
	{
		win_anchor_y2_offset_set( _view_winp, -160 );
		_console.cons_show( 1 );
//		gam.SetGadgetText( DSharinguApp::BUTT_SHELL, "[O] Shell" );
	}
	else
	{
		win_anchor_y2_offset_set( _view_winp, 0 );
		_console.cons_show( 0 );
//		gam.SetGadgetText( DSharinguApp::BUTT_SHELL, "[ ] Shell" );
	}

	_superp->updateViewMenu( this );
}

//==================================================================
void DSChannel::onConnect( bool is_connected_as_caller )
{
//	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	WGUT::SafeDestroyWindow( _connecting_dlg_hwnd );

	_superp->_remote_mng.CloseDialog();

	if ( is_connected_as_caller )
	{
		_console.cons_line_printf( CHNTAG"Calling..." );
	}
	else
	{
		_console.cons_line_printf( CHNTAG"Incoming connection..." );
	}

	//gam.EnableGadget( DSharinguApp::BUTT_CONNECTIONS, false );
//	gam.EnableGadget( DSharinguApp::BUTT_HANGUP, true );
	EnableMenuItem( _superp->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_ENABLED );
	//gam.SetGadgetText( DSharinguApp::BUTT_CONNECTIONS, "[O] Connected" );

	_is_connected = true;
	_is_transmitting = false;
	_remote_wants_view = false;
	_remote_wants_share = false;
	_remote_allows_view = false;
	_remote_allows_share = false;
	_frame_since_transmission = 0;

	if ( is_connected_as_caller )
	{
		PSYS_ASSERT( _session_remotep != NULL );

		HandShakeMsg	msg( PROTOCOL_VERSION,
							_superp->_settings._username,
							_session_remotep->_rm_username,
							_session_remotep->_rm_password._data );

		if ERR_ERROR( _cpk.SendPacket( HANDSHAKE_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
}

//==================================================================
void DSChannel::doDisconnect( const char *messagep, bool is_error )
{
	if ( messagep )
		_console.cons_line_printf( CHNTAG"%s", messagep );

	setState( STATE_DISCONNECT_START );
}

//==================================================================
void DSChannel::handleAutoScroll()
{
	if ( _view_fitwindow )
	{

	}
	else
	//if ( _intersys.IsActive() )
	if ( _view_winp->IsMousePointerInside() )
	{
		int	x1 = _disp_off_x;
		int	y1 = _disp_off_y;
		int	x2 = x1 + _scrreader.GetWidth();
		int	y2 = y1 + _scrreader.GetHeight();

		if ( _disp_curs_x < 32 && _disp_off_x < 0 )
		{
			_disp_off_x += 8;
			if ( _disp_curs_x < 8 )
				_disp_off_x += 16;

			if ( _disp_off_x > 0 )
				_disp_off_x = 0;

			win_invalidate( _view_winp );
		}

		if ( _disp_curs_y < 32 && _disp_off_y < 0 )
		{
			_disp_off_y += 8;
			if ( _disp_curs_y < 8 )
				_disp_off_y += 16;

			if ( _disp_off_y > 0 )
				_disp_off_y = 0;

			win_invalidate( _view_winp );
		}

		if ( _disp_curs_x >= _view_winp->w-32 && _view_winp->w < x2 )
		{
			_disp_off_x -= 8;
			if ( _disp_curs_x >= _view_winp->w-8 )
				_disp_off_x -= 16;

			win_invalidate( _view_winp );
		}

		if ( _disp_curs_y >= _view_winp->h-32 && _view_winp->h < y2 )
		{
			_disp_off_y -= 8;
			if ( _disp_curs_y >= _view_winp->h-8 )
				_disp_off_y -= 16;
			win_invalidate( _view_winp );
		}
	}
}

//==================================================================
DSChannel::State DSChannel::Idle()
{
	if ( _console.cons_is_showing() )
	{
		//		win_focus( &_console._win, 1 );
	}

	handleAutoScroll();

	if ( _state == STATE_NULL )
		return _state;

	int	err;

	switch ( err = _cpk.Idle() )
	{
	case COM_ERR_CONNECTED:
		_intersys.Activate( false );
		_disp_off_x = 0;
		_disp_off_y = 0;
		setState( STATE_CONNECTED );
		break;

	case COM_ERR_HARD_DISCONNECT:
		doDisconnect( "Connection Lost." );
		break;

	case COM_ERR_INVALID_ADDRESS:
		doDisconnect( "Call Failed. Invalid Address." );
		break;

	case COM_ERR_GRACEFUL_DISCONNECT:
		doDisconnect( "Connection Closed." );
		break;

	case COM_ERR_GENERIC:
		doDisconnect( "Connection Lost (generic error)" );
		break;

	case COM_ERR_TIMEOUT_CONNECTING:
		MessageBox( _superp->_main_win.hwnd, "Timed out while trying to connect.\n"
			"Please, make sure that the Internet Address and the Port are correct.",
			"Connection Problem", MB_OK | MB_ICONSTOP );

		doDisconnect( "Timed out trying to connect." );
		break;

	case COM_ERR_NONE:
		break;

	default:
		//_is_connected = false;
		//_console.cons_line_printf( CHNTAG"connection err #%i !!", err );
		break;
	}

	u_int	pack_id;
	u_int	data_size;
	if ( _cpk.GetInputPack( &pack_id, &data_size, _superp->_inpack_buffp, DSharinguApp::INPACK_BUFF_SIZE ) )
	{
		processInputPacket( pack_id, _superp->_inpack_buffp, data_size );
		_cpk.DisposeINPacket();
	}

	//-------------
	switch ( _state )
	{
	case STATE_CONNECTED:
		handleConnectedFlow();
		break;

	case STATE_DISCONNECTING:
		if ( _cpk.IsConnected() )
		{
			if ( _cpk.GetOUTQueueCnt() <= 0 )
			{
				_cpk.Disconnect();
				setState( STATE_DISCONNECTED );
			}
		}
		else
			setState( STATE_DISCONNECTED );
		break;
	}

	return _state;
}

//==================================================================
void DSChannel::processInputPacket( u_int pack_id, const u_char *datap, u_int data_size )
{
	if NOT( _is_transmitting )
	{
		// processes packets
		switch ( pack_id )
		{
		case HANDSHAKE_PKID:
			{
			const HandShakeMsg	&msg = *(HandShakeMsg *)datap;

				if ( msg._protocol_version == PROTOCOL_VERSION )
				{
					if ( stricmp( msg._communicating_username, _superp->_settings._username ) )
					{
						_console.cons_line_printf( CHNTAG"PROBLEM: Rejected connection from '%s'. The wrong username was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_USERNAME_PKID );
						doDisconnect( "Connection Failed." );
						return;
					}

					if NOT( sha1_t::AreEqual( msg._communicating_password, _superp->_settings._password._data ) )
					{
						_console.cons_line_printf( CHNTAG"PROBLEM: Rejected connection for '%s'. The wrong password was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_PASSWORD_PKID );
						doDisconnect( "Connection Failed." );
						return;
					}

					changeSessionRemote( _superp->_remote_mng.FindOrAddRemoteDefAndSelect( msg._caller_username ) );
					_session_remotep->Lock();

					_console.cons_line_printf( CHNTAG"OK ! Successfully connected to '%s'", msg._caller_username );

					_is_transmitting = true;
					_frame_since_transmission = 0;
					_cpk.SendPacket( HS_OK );
				}
				else
				{
					if ( msg._protocol_version > PROTOCOL_VERSION )
					{
						_cpk.SendPacket( HS_NEW_PROTOCOL_PKID );
						doDisconnect( "Connection Failed." );
						if ( MessageBox( _superp->_main_win.hwnd,
									"Cannot communicate because the other party has a newer version of the program !\n"
									"Do you want to download the latest version ?",
									"Incompatible Versions",
									MB_YESNO | MB_ICONERROR ) == IDYES )
						{
							ShellExecute( _superp->_main_win.hwnd, "open", "http://kazzuya.com/dsharingu", NULL, NULL, SW_SHOWNORMAL );
						}
					}
					else
					{
						_cpk.SendPacket( HS_OLD_PROTOCOL_PKID );
						doDisconnect( "Connection Failed." );
						MessageBox( _superp->_main_win.hwnd,
									"Cannot communicate because the other party has an older version of the program !\n",
									"Incompatible Versions",
									MB_OK | MB_ICONERROR );
					}
				}
			}
			break;

		case HS_BAD_USERNAME_PKID:
			if ( _session_remotep )
				_console.cons_line_printf( CHNTAG"PROBLEM: Wrong username !", _session_remotep->_rm_username );
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
			}
			doDisconnect( "Connection Failed." );
			break;

		case HS_BAD_PASSWORD_PKID:
			if ( _session_remotep )
				_console.cons_line_printf( CHNTAG"PROBLEM: Wrong password !",
				_session_remotep->_rm_username );
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
			}
			doDisconnect( "Connection Failed." );
			break;


		case HS_NEW_PROTOCOL_PKID:
			doDisconnect( "Connection Failed." );
			MessageBox( _superp->_main_win.hwnd,
				"Cannot communicate because the other party has an older version of the program !\n",
				"Incompatible Versions",
				MB_OK | MB_ICONERROR );
			break;

		case HS_OLD_PROTOCOL_PKID:
			doDisconnect( "Connection Failed." );
			if ( MessageBox( _superp->_main_win.hwnd,
				"Cannot communicate because the other party has a newer version of the program !\n"
				"Do you want to download the latest version ?",
				"Incompatible Versions",
				MB_YESNO | MB_ICONERROR ) == IDYES )
			{
				ShellExecute( _superp->_main_win.hwnd, "open", "http://kazzuya.com/dsharingu", NULL, NULL, SW_SHOWNORMAL );
			}
			break;

		case HS_OK:
			if ( _session_remotep )
			{
				_console.cons_line_printf( CHNTAG"OK ! Succesfully connected to '%s'",
											_session_remotep->_rm_username );

				_is_transmitting = true;
			}
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
				doDisconnect( "Connection Failed." );
			}
			break;
		}
		
	}
	else
	{
		// processes packets
		switch ( pack_id )
		{
		case TEXT_MSG_PKID:
			_console.cons_line_printf( "%s> %s", _session_remotep->_rm_username, (const char *)datap );
			break;

		case DESK_IMG_PKID:
			_scrreader.ParseFrame( datap, data_size );
			updateViewScale();
			win_invalidate( _view_winp );
			break;

		case USAGE_WISH_PKID:
			_remote_wants_view  = ((UsageWishMsg *)datap)->_see_remote_screen;
			_remote_wants_share = ((UsageWishMsg *)datap)->_use_remote_screen;
			break;

		case USAGE_ABILITY_PKID:
			_remote_allows_view  = ((UsageAbilityMsg *)datap)->_see_remote_screen;
			_remote_allows_share = ((UsageAbilityMsg *)datap)->_use_remote_screen;
			PSYS_ASSERT( _session_remotep != NULL );
			if ( _remote_allows_share && _remote_allows_view &&
				 _session_remotep &&
				 _session_remotep->_see_remote_screen )
			{
//				_tool_winp->GetGGETManager().EnableGadget( DSharinguApp::BUTT_USEREMOTE, true );
			}
			else
			{
//				_tool_winp->GetGGETManager().EnableGadget( DSharinguApp::BUTT_USEREMOTE, false );
				setInteractiveMode( false );
			}
			break;
		}
	}
}

//==================================================================
void DSChannel::handleConnectedFlow()
{
	if NOT( _is_transmitting )
		return;

	if ( _is_transmitting )
	{
		_intersys.Idle();

		if ( _frame_since_transmission == 1 )
		{
			{
				UsageWishMsg	msg(_session_remotep->_see_remote_screen,
					_session_remotep->_use_remote_screen );

				if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}

			{
				UsageAbilityMsg	msg(_superp->_settings._show_my_screen,
					_superp->_settings._share_my_screen );
				if ERR_ERROR( _cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
					return;

			}
		}
	}


	bool	do_show = (_remote_wants_view && _superp->_settings._show_my_screen);

	if ( _superp->_scrwriter.IsGrabbing() != do_show )
	{
		if ( do_show )
		{
			_superp->_scrwriter.StartGrabbing( (HWND)_superp->_main_win.hwnd );
		}
		else
		{
			_superp->_scrwriter.StopGrabbing();
		}
	}

	if ( _superp->_scrwriter.IsGrabbing() )
	{
		int cnt = _cpk.SearchOUTQueue( DESK_IMG_PKID );
		if ( cnt <= 2 )
		{
			bool has_grabbed = _superp->_scrwriter.UpdateWriter();

			if ( has_grabbed )
				_superp->_scrwriter.SendFrame( DESK_IMG_PKID, &_cpk );
		}
	}

	++_frame_since_transmission;

	if ( (_frame_since_transmission & 7) == 0 )
		_superp->_dbg_win.Invalidate();
}

//==================================================================
void DSChannel::updateViewScale()
{
	if ( _view_fitwindow && _scrreader.GetWidth() > 0 && _scrreader.GetHeight() > 0 )
	{
		_view_scale_x = (float)_view_winp->GetWidth() / _scrreader.GetWidth();
		_view_scale_y = (float)_view_winp->GetHeight() / _scrreader.GetHeight();
	}
	else
	{
		_view_scale_x = 1;
		_view_scale_y = 1;
	}
}

//===============================================================
void DSChannel::setInteractiveMode( bool onoff )
{
//	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	if ( _is_connected && onoff )
		_intersys.RestartFeed();

	_intersys.Activate( onoff );
//	if ( onoff )
//		gam.SetGadgetText( DSharinguApp::BUTT_USEREMOTE, "[O] Use Remote" );
//	else
//		gam.SetGadgetText( DSharinguApp::BUTT_USEREMOTE, "[ ] Use Remote" );
}

//==================================================================
void DSChannel::gadgetCallback_s( int gget_id, GGET_Item *itemp, void *userdatap )
{
	((DSChannel *)userdatap)->gadgetCallback( gget_id, itemp );
}
//==================================================================
void DSChannel::gadgetCallback( int gget_id, GGET_Item *itemp )
{
	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	switch ( gget_id )
	{
	case DSharinguApp::BUTT_CONNECTIONS:
		_superp->_remote_mng.OpenDialog( &_superp->_main_win,
								DSharinguApp::handleChangedRemoteManager_s,
								DSharinguApp::handleCallRemoteManager_s,
								_superp );
		break;

	case DSharinguApp::BUTT_HANGUP:
		doDisconnect( "Successfully disconnected." );
		break;

//	case DSharinguApp::BUTT_superp->_settings:
//		_superp->_settings.OpenDialog( &_superp->_main_win, handleChangedSettings_s, this );
//		break;
/*
	case DSharinguApp::BUTT_QUIT:
		setState( STATE_QUIT );
		break;
*/
	case DSharinguApp::BUTT_USEREMOTE:
		if ( _session_remotep )
		{
			_session_remotep->_use_remote_screen = !_session_remotep->_use_remote_screen;
			setInteractiveMode( _session_remotep->_use_remote_screen );
		}
		break;

	case DSharinguApp::BUTT_SHELL:
		setShellVisibility( true );
		break;
	}
}

//==================================================================
int DSChannel::viewEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		win_focus( &_console._win, true );
		break;

	case WIN_ETYPE_KEYDOWN:
		/*
		if ( eventp->keycode == VK_SHIFT )
		{
		if ( GetKeyState( VK_CONTROL ) & 0x8000 )
		{
		_is_interactive_mode = !_is_interactive_mode;
		//ShowCursor( !_is_interactive_mode );
		}
		}
		*/
		break;

	case WIN_ETYPE_MOUSEMOVE:
		if ( _intersys.IsActive() )
		{
			_intersys.FeedMessage( eventp->ms_message,
				eventp->ms_lparam,
				eventp->ms_wparam,
				_disp_off_x,
				_disp_off_y,
				_view_scale_x,
				_view_scale_y );

			_disp_curs_x = eventp->mouse_x;
			_disp_curs_y = eventp->mouse_y;

			win_invalidate( eventp->winp );
		}
		else
		{
			_disp_curs_x = eventp->mouse_x;
			_disp_curs_y = eventp->mouse_y;
		}
		break;

	case WIN_ETYPE_LBUTTONDOWN:
	case WIN_ETYPE_LBUTTONUP:
	case WIN_ETYPE_RBUTTONDOWN:
	case WIN_ETYPE_RBUTTONUP:
		if ( _intersys.IsActive() )
		{
			_intersys.FeedMessage( eventp->ms_message,
				eventp->ms_lparam,
				eventp->ms_wparam,
				_disp_off_x,
				_disp_off_y,
				_view_scale_x,
				_view_scale_y );

			_disp_curs_x = eventp->mouse_x;
			_disp_curs_y = eventp->mouse_y;

			win_invalidate( eventp->winp );
		}
		break;

		/*
		case WIN_ETYPE_MOUSEMOVEDOUT:
		if ( _is_interactive_mode )
		{
		ShowCursor( FALSE );
		}
		break;
		*/
	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
		{
			int	new_w = eventp->win_w;
			int	new_h = eventp->win_h;

			if ( _disp_off_x + _scrreader.GetWidth() < new_w )
			{
				_disp_off_x = new_w - _scrreader.GetWidth();
				if ( _disp_off_x > 0 )
					_disp_off_x = 0;
			}

			if ( _disp_off_y + _scrreader.GetHeight() < new_h )
			{
				_disp_off_y = new_h - _scrreader.GetHeight();
				if ( _disp_off_y > 0 )
					_disp_off_y = 0;
			}
		}
		updateViewScale();
		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		doViewPaint();
		break;
	}

	return 0;
}
//==================================================================
int DSChannel::viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->viewEventFilter( etype, eventp );
}
/*
//==================================================================
int DSChannel::toolEventFilter( win_event_type etype, win_event_t *eventp )
{
	_console.cons_parent_eventfilter( NULL, etype, eventp );

	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		win_focus( &_console._win, true );
		break;

	case WIN_ETYPE_CREATE:
		_tool_winp = eventp->winp;
		rebuildButtons( eventp->winp );
		break;

	case WIN_ETYPE_WINRESIZE:
		reshapeButtons( eventp->winp );
		break;
	}

	return 0;
}
//==================================================================
int DSChannel::toolEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->toolEventFilter( etype, eventp );
}
*/
//==================================================================
void DSChannel::rebuildButtons( win_t *winp )
{
	float	x = 4;
	float	y = 3;
	float	w = 90;
	float	h = 22;
	float	y_margin = 4;
	float	x_margin = 4;

	GGET_Manager	&gam = winp->GetGGETManager();

	gam.SetCallback( gadgetCallback_s, this );
	
	GGET_StaticText *stextp = gam.AddStaticText( DSharinguApp::STEXT_TOOLBARBASE, 0, 0, winp->w, winp->h, NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	gam.AddButton( DSharinguApp::BUTT_CONNECTIONS, x, y, 98, h, "Connections..." );		x += 98 + x_margin;
	gam.AddButton( DSharinguApp::BUTT_HANGUP, x, y, 85, h, "Hang-up" );			x += 85 + x_margin;
	gam.EnableGadget( DSharinguApp::BUTT_HANGUP, false );
	EnableMenuItem( _superp->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
//	x += x_margin;
//	x += x_margin;
//	gam.AddButton( DSharinguApp::BUTT_SETTINGS, x, y, 98, h, "Settings..." );	x += 98 + x_margin;
//	x += x_margin;
//	gam.AddButton( DSharinguApp::BUTT_QUIT, x, y, 60, h, "Quit" );				x += 60 + x_margin;

	x += x_margin;
	x += x_margin;
	x += x_margin;
	x += x_margin;
	gam.AddButton( DSharinguApp::BUTT_USEREMOTE, x, y, 98, h, "[ ] Use Remote" );	x += 98 + x_margin;
	x += x_margin;
	gam.AddButton( DSharinguApp::BUTT_SHELL, x, y, 60, h, "[ ] Shell" );			x += 60 + x_margin;
//	gam.AddButton( DSharinguApp::BUTT_HELP, x, y, 60, h, "About" );			x += 60 + x_margin;

	setInteractiveMode( getInteractiveMode() );
}

//==================================================================
void DSChannel::reshapeButtons( win_t *winp )
{
	GGET_Manager	&gam = winp->GetGGETManager();

	gam.FindGadget( DSharinguApp::STEXT_TOOLBARBASE )->SetRect( 0, 0, winp->w, winp->h );
}

//===============================================================
BOOL CALLBACK DSChannel::connectingDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
	{
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );
	}

	DSChannel *mythis = (DSChannel *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->connectingDialogProc( hwnd, umsg, wparam, lparam );
}
//===============================================================
BOOL CALLBACK DSChannel::connectingDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		SendMessage( GetDlgItem( hwnd, IDC_CONNECTING_REMOTE_PROGRESS ),
					 PBM_SETRANGE, 0, MAKELPARAM( 0, 20*4 ) );
		SetTimer( hwnd, 1, 1000/4, NULL );
		_connecting_dlg_timer = 0;
		break;

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDOK:
		case IDCANCEL:
			doDisconnect( "Connection aborted." );
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_TIMER:
		SendMessage( GetDlgItem( hwnd, IDC_CONNECTING_REMOTE_PROGRESS ), PBM_SETPOS, ++_connecting_dlg_timer, 0 );
		SetTimer( hwnd, 1, 1000/4, NULL );
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

	case WM_DESTROY:
		appbase_rem_modeless_dialog( hwnd );
		_connecting_dlg_hwnd = NULL;
		break;

	default:
		return 0;
	}

	return 1;
}
