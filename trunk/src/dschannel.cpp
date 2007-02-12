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
#include "dschannel_manager.h"
#include "dstask.h"


#define CHNTAG				"* "

static const int	BUTT_WD	= 110;
static const int	BUTT_HE	= 22;

//==================================================================
///
//==================================================================
DSChannel::DSChannel( DSChannelManager *managerp, int accepted_fd ) :
	_managerp(managerp),
	_intersys( &_cpk )
{
	_managerp->AddChannelToList( this, "..." );

	create();

	if PTRAP_FALSE( accepted_fd >= 0 )
	{
		_cpk.SetAcceptedConn( accepted_fd );
		setState( STATE_CONNECTED );
	}
	else
		setState( STATE_IDLE );

}

//==================================================================
DSChannel::DSChannel( DSChannelManager *managerp, RemoteDef *remotep ) :
	_managerp(managerp),
	_intersys( &_cpk )
{
	_managerp->AddChannelToList( this, remotep->_rm_username );

	create();

	changeSessionRemote( remotep );

	remotep->SetUserData( this );

	CallRemote( false );
}

//==================================================================
DSChannel::~DSChannel()
{
	// remove the channel from the remotedef that is currently using it
	if ( _session_remotep )
		_session_remotep->SetUserData( NULL );

	//_managerp->RemoveChannel( this );
}

//==================================================================
int DSChannel::thisMessageBoxRet( LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, UINT uDefVal )
{
	if NOT( _is_calling_silently )
		return MessageBox( ((DSharinguApp *)_managerp->_superp)->_main_win._hwnd,
							lpText, lpCaption, uType );
	else
		return uDefVal;
}

//==================================================================
void DSChannel::thisMessageBox( LPCTSTR lpText, LPCTSTR lpCaption, UINT uType )
{
	if NOT( _is_calling_silently )
		MessageBox( ((DSharinguApp *)_managerp->_superp)->_main_win._hwnd,
					lpText, lpCaption, uType );
}

//==================================================================
void DSChannel::CallRemote( bool call_silent ) throw(...)
{
	_is_calling_silently = call_silent;

	_session_remotep->Lock();

	int	err = _cpk.Call( _session_remotep->_rm_ip_address, _session_remotep->GetCallPortNum() );
	switch ( err )
	{
	case 0:
		setState( STATE_CONNECTING );

		if NOT( _is_calling_silently )
		{
			_connecting_dlg_hwnd =
				CreateDialogParam( (HINSTANCE)WinSys::GetInstance(),
				MAKEINTRESOURCE(IDD_CONNECTING), ((DSharinguApp *)_managerp->_superp)->_main_win._hwnd,
				(DLGPROC)connectingDialogProc_s, (LPARAM)this );
			appbase_add_modeless_dialog( _connecting_dlg_hwnd );
			ShowWindow( _connecting_dlg_hwnd, SW_SHOWNORMAL );
		}
		break;

	case COM_ERR_INVALID_ADDRESS:
		thisMessageBox( "The Internet Address seems to be invalid.\nPlease, review it.",
						"Connection Problem", MB_OK | MB_ICONSTOP );

		if ( _session_remotep )
			_session_remotep->Unlock();
		((DSharinguApp *)_managerp->_superp)->_remote_mng.InvalidAddressOnCall();
		break;

	default:
		thisMessageBox( "Error occurred while trying to call.",
				"Connection Problem", MB_OK | MB_ICONSTOP );

		if ( _session_remotep )
			_session_remotep->Unlock();
		break;
	}
}

//==================================================================
void DSChannel::create()
{
	_state = STATE_IDLE;
	_is_calling_silently = false;
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
	_is_using_remote = false;
	_connecting_dlg_hwnd = NULL;
	
	_view_winp = NULL;

	_intersys.Activate( false );
/*
	_tool_winp = new Window( "tool win", &((DSharinguApp *)_managerp->_superp)->_main_win,
							this, toolEventFilter_s,
							WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
							WIN_ANCH_TYPE_PARENT_X2, 0, WIN_ANCH_TYPE_PARENT_Y1, 30,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR) );
*/
	
	_view_winp = new Window( "view win", &((DSharinguApp *)_managerp->_superp)->_main_win,
							this, viewEventFilter_s,
							WIN_ANCH_TYPE_FIXED, 0,
							WIN_ANCH_TYPE_PARENT_Y1, _managerp->GetTabsWinHeight(),
							WIN_ANCH_TYPE_PARENT_X2, 0,
							WIN_ANCH_TYPE_PARENT_Y2, -160,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR | 0*WIN_INIT_FLG_HSCROLL | 0*WIN_INIT_FLG_VSCROLL) );

	//-----------------------------------------------
	_console.cons_init( &((DSharinguApp *)_managerp->_superp)->_main_win, (void *)this );
	_console.cons_line_cb_set( console_line_func_s );
	_console.cons_cmd_add_defs( _cmd_defs );
	_console.cons_show( 1 );

	setShellVisibility();
	//changeSessionRemote( NULL );

	// we need to initialize this after loading the settings
	_intersys.ActivateExternalInput(
							!(((DSharinguApp *)_managerp->_superp)->_settings._nobody_can_use_my_computer ||
							  ((DSharinguApp *)_managerp->_superp)->_settings._nobody_can_watch_my_computer) );
	_cpk.SetOnPackCallback( REMOCON_ARRAY_PKID, InteractiveSystem::OnPackCallback_s, &_intersys );

	updateViewScale();

	_task_managerp = new DSTaskManager( _view_winp, this, taskOnGadgetCB_s );
	_task_managerp->AddTask( "Desk", VIEW_WIN_TASK_DESK_BUTTON, DSTask::VS_FITVIEW );
	_task_managerp->AddTask( "Shell", VIEW_WIN_TASK_SHELL_BUTTON, DSTask::VS_FITVIEW );

	_view_winp->PostResize();
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
		EnableMenuItem( ((DSharinguApp *)_managerp->_superp)->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_ENABLED );
		//gam.SetGadgetText( DSharinguApp::BUTT_CONNECTIONS, "[ ] Calling..." );

		_managerp->SetChannelTabIcon( this, GGET_Item::STD_ICO_OFF );
		break;

	case STATE_CONNECTED:
		onConnect( _cpk.IsConnectedAsCaller() );

		_managerp->SetChannelTabIcon( this, GGET_Item::STD_ICO_ON );
		break;

	// recycle the channel
	case STATE_RECYCLE:
		_managerp->SetChannelTabIcon( this, GGET_Item::STD_ICO_OFF );

		WGUT::SafeDestroyWindow( _connecting_dlg_hwnd );
		((DSharinguApp *)_managerp->_superp)->_scrwriter.StopGrabbing();
		EnableMenuItem( ((DSharinguApp *)_managerp->_superp)->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
		_cpk.Disconnect();
		_is_connected = false;
		_is_transmitting = false;
		setInteractiveMode( false );
		_state = STATE_IDLE;
		break;

	case STATE_DISCONNECT_START:
		WGUT::SafeDestroyWindow( _connecting_dlg_hwnd );
		((DSharinguApp *)_managerp->_superp)->_scrwriter.StopGrabbing();
		EnableMenuItem( ((DSharinguApp *)_managerp->_superp)->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
		_is_connected = false;
		_is_transmitting = false;
		if ( _session_remotep )
			_session_remotep->Unlock();
		//changeSessionRemote( NULL );
		setInteractiveMode( false );
		_state = STATE_DISCONNECTING;
		break;

	case STATE_DISCONNECTING:
		break;

	case STATE_DISCONNECTED:
		{
			_managerp->SetChannelTabIcon( this, GGET_Item::STD_ICO_OFF );

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

	refreshInteractionInterface();
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

	((DSharinguApp *)_managerp->_superp)->updateViewMenu( this );
}

//==================================================================
void DSChannel::onConnect( bool is_connected_as_caller )
{
//	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	WGUT::SafeDestroyWindow( _connecting_dlg_hwnd );

	((DSharinguApp *)_managerp->_superp)->_remote_mng.CloseDialog();

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
	EnableMenuItem( ((DSharinguApp *)_managerp->_superp)->_main_menu, ID_FILE_HANGUP, MF_BYCOMMAND | MF_ENABLED );
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
							((DSharinguApp *)_managerp->_superp)->_settings._username,
							_session_remotep->_rm_username,
							_session_remotep->_rm_password._data );

		if ERR_ERROR( _cpk.SendPacket( HANDSHAKE_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
}

//==================================================================
void DSChannel::DoDisconnect( const char *messagep, bool is_error )
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

		if ( _disp_curs_x >= _view_winp->GetWidth()-32 && _view_winp->GetWidth() < x2 )
		{
			_disp_off_x -= 8;
			if ( _disp_curs_x >= _view_winp->GetWidth()-8 )
				_disp_off_x -= 16;

			win_invalidate( _view_winp );
		}

		if ( _disp_curs_y >= _view_winp->GetHeight()-32 && _view_winp->GetHeight() < y2 )
		{
			_disp_off_y -= 8;
			if ( _disp_curs_y >= _view_winp->GetHeight()-8 )
				_disp_off_y -= 16;
			win_invalidate( _view_winp );
		}
	}
}

//==================================================================
int DSChannel::Idle()
{
	if ( _console.cons_is_showing() )
	{
		//		win_focus( &_console._win, 1 );
	}

	handleAutoScroll();

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
		DoDisconnect( "Lost Connection (hard)." );
		break;

	case COM_ERR_INVALID_ADDRESS:
		DoDisconnect( "Call Failed. Invalid Address." );
		break;

	case COM_ERR_GRACEFUL_DISCONNECT:
		DoDisconnect( "Connection Closed." );
		break;

	case COM_ERR_GENERIC:
		DoDisconnect( "Lost Connection" );
		break;

	case COM_ERR_TIMEOUT_CONNECTING:
		thisMessageBox( "Timed out while trying to connect.\n"
			"Please, make sure that the Internet Address and the Port are correct.",
			"Connection Problem", MB_OK | MB_ICONSTOP );

		DoDisconnect( "Timed out trying to connect." );
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
	if ( _cpk.GetInputPack( &pack_id, &data_size, ((DSharinguApp *)_managerp->_superp)->_inpack_buffp, DSharinguApp::INPACK_BUFF_SIZE ) )
	{
		processInputPacket( pack_id, ((DSharinguApp *)_managerp->_superp)->_inpack_buffp, data_size );
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
					RemoteDef	*remotep;

					remotep = ((DSharinguApp *)_managerp->_superp)->_remote_mng.FindRemoteDef( msg._caller_username );
					if ( remotep && remotep->IsLocked() )
					{
						_console.cons_line_printf( CHNTAG" '%s' is trying to connect again !", msg._caller_username );
						return;
					}

					remotep = ((DSharinguApp *)_managerp->_superp)->_remote_mng.FindOrAddRemoteDefAndSelect( msg._caller_username );
					// assign the remote to this channel
					changeSessionRemote( remotep );
					_managerp->SetChannelName( this, msg._caller_username );

					// we remove the older channel using this remote, because we're already using a different one (this one !)
					DSChannel	*older_chan_using_remotep = _managerp->FindChannelByRemote( remotep );
					if ( older_chan_using_remotep != this )
						_managerp->RemoveChannel( older_chan_using_remotep );

					if ( _stricmp( msg._communicating_username, ((DSharinguApp *)_managerp->_superp)->_settings._username ) )
					{
						_console.cons_line_printf( CHNTAG"PROBLEM: Rejected connection from '%s'. The wrong username was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_USERNAME_PKID );
						DoDisconnect( "Connection Failed." );
						return;
					}

					if NOT( sha1_t::AreEqual( msg._communicating_password, ((DSharinguApp *)_managerp->_superp)->_settings._password._data ) )
					{
						_console.cons_line_printf( CHNTAG"PROBLEM: Rejected connection for '%s'. The wrong password was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_PASSWORD_PKID );
						DoDisconnect( "Connection Failed." );
						return;
					}

					_console.cons_line_printf( CHNTAG"OK ! Successfully connected to '%s'", msg._caller_username );

					PSYS_ASSERT( _session_remotep != NULL );
					_session_remotep->SetUserData( this );
					_session_remotep->Lock();

					_is_transmitting = true;
					_frame_since_transmission = 0;
					_cpk.SendPacket( HS_OK );
				}
				else
				{
					if ( msg._protocol_version > PROTOCOL_VERSION )
					{
						_cpk.SendPacket( HS_NEW_PROTOCOL_PKID );
						DoDisconnect( "Connection Failed." );
						if ( thisMessageBoxRet(
									"Cannot communicate because the other party has a newer version of the program !\n"
									"Do you want to download the latest version ?",
									"Incompatible Versions",
									MB_YESNO | MB_ICONERROR, IDNO ) == IDYES )
						{
							ShellExecute( ((DSharinguApp *)_managerp->_superp)->_main_win._hwnd,
								"open", "http://dsharingu.kazzuya.com", NULL, NULL, SW_SHOWNORMAL );
						}
					}
					else
					{
						_cpk.SendPacket( HS_OLD_PROTOCOL_PKID );
						DoDisconnect( "Connection Failed." );
						thisMessageBox(
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
			DoDisconnect( "Connection Failed." );
			break;

		case HS_BAD_PASSWORD_PKID:
			if ( _session_remotep )
				_console.cons_line_printf( CHNTAG"PROBLEM: Wrong password !",
				_session_remotep->_rm_username );
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
			}
			DoDisconnect( "Connection Failed." );
			break;


		case HS_NEW_PROTOCOL_PKID:
			DoDisconnect( "Connection Failed." );
			thisMessageBox(
				"Cannot communicate because the other party has an older version of the program !\n",
				"Incompatible Versions",
				MB_OK | MB_ICONERROR );
			break;

		case HS_OLD_PROTOCOL_PKID:
			DoDisconnect( "Connection Failed." );
			if ( thisMessageBoxRet(
				"Cannot communicate because the other party has a newer version of the program !\n"
				"Do you want to download the latest version ?",
				"Incompatible Versions",
				MB_YESNO | MB_ICONERROR, IDNO ) == IDYES )
			{
				ShellExecute( ((DSharinguApp *)_managerp->_superp)->_main_win._hwnd,
					"open", "http://dsharingu.kazzuya.com", NULL, NULL, SW_SHOWNORMAL );
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
				DoDisconnect( "Connection Failed." );
			}
			break;
		}
		
	}
	else
	{
		PSYS_ASSERT( _session_remotep != NULL );

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
			refreshInteractionInterface();
			break;
		}
	}
}

//==================================================================
void DSChannel::refreshInteractionInterface()
{
	if NOT( _view_winp )
		return;

	GGET_Manager	&gam = _view_winp->GetGGETManager();

	if NOT( _session_remotep )
	{
		//gam.EnableGadget( VIEW_WIN_VIEW_REMOTE_BUTT, false );
		gam.EnableGadget( VIEW_WIN_USE_REMOTE_BUTT, false );
		gam.FindGadget( VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT )->Show( false );
		gam.FindGadget( VIEW_WIN_USE_REM_NOT_ALLOWED_STXT )->Show( false );
	}
	else
	{
		gam.FindGadget( VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT )->Show( !_remote_allows_view );
		gam.FindGadget( VIEW_WIN_USE_REM_NOT_ALLOWED_STXT )->Show( !_remote_allows_share );
		//gam.EnableGadget( VIEW_WIN_VIEW_REMOTE_BUTT, _remote_allows_view );
		gam.EnableGadget( VIEW_WIN_USE_REMOTE_BUTT, _remote_allows_view && _remote_allows_share );

		gam.FindGadget( VIEW_WIN_USE_REMOTE_BUTT )->SetIcon(
			_is_using_remote ? GGET_Item::STD_ICO_MARK_ON : GGET_Item::STD_ICO_MARK_OFF );

		//gam.FindGadget( VIEW_WIN_VIEW_REMOTE_BUTT )->SetIcon(
		//	_session_remotep->_see_remote_screen ? GGET_Item::STD_ICO_MARK_ON : GGET_Item::STD_ICO_MARK_OFF );

		_task_managerp->FindByButtID( VIEW_WIN_TASK_DESK_BUTTON )->SetViewState(
			_session_remotep->_see_remote_screen ? DSTask::VS_FITVIEW : DSTask::VS_ICONIZED );

		// makes invisible as long as the user is interacting
		gam.FindGadget( VIEW_WIN_USE_REMOTE_BUTT )->Show( !_is_using_remote );
		_task_managerp->Show( !_is_using_remote );
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
									_is_using_remote );

				if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}

			{
				UsageAbilityMsg	msg(!((DSharinguApp *)_managerp->_superp)->_settings._nobody_can_watch_my_computer,
									!((DSharinguApp *)_managerp->_superp)->_settings._nobody_can_use_my_computer );
				if ERR_ERROR( _cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
					return;

			}
		}
	}


	bool	do_show = (_remote_wants_view && !((DSharinguApp *)_managerp->_superp)->_settings._nobody_can_watch_my_computer);

	if ( ((DSharinguApp *)_managerp->_superp)->_scrwriter.IsGrabbing() != do_show )
	{
		if ( do_show )
		{
			((DSharinguApp *)_managerp->_superp)->_scrwriter.StartGrabbing( (HWND)((DSharinguApp *)_managerp->_superp)->_main_win._hwnd );
		}
		else
		{
			((DSharinguApp *)_managerp->_superp)->_scrwriter.StopGrabbing();
		}
	}

	if ( ((DSharinguApp *)_managerp->_superp)->_scrwriter.IsGrabbing() )
	{
		int cnt = _cpk.SearchOUTQueue( DESK_IMG_PKID );
		if ( cnt <= 2 )
		{
			bool has_grabbed = ((DSharinguApp *)_managerp->_superp)->_scrwriter.UpdateWriter();

			if ( has_grabbed )
				((DSharinguApp *)_managerp->_superp)->_scrwriter.SendFrame( DESK_IMG_PKID, &_cpk );
		}
	}

	++_frame_since_transmission;

	if ( (_frame_since_transmission & 7) == 0 )
		((DSharinguApp *)_managerp->_superp)->_dbg_win.Invalidate();
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
	GGET_Manager	&gam = _view_winp->GetGGETManager();

	if ( _is_connected && onoff )
		_intersys.RestartFeed();

	_intersys.Activate( onoff );

	_is_using_remote = onoff;
	//_session_remotep->_use_remote_screen = onoff;

	refreshInteractionInterface();
}

//===============================================================
void DSChannel::setViewMode()
{
	refreshInteractionInterface();
}

//==================================================================
void DSChannel::gadgetCallback_s( void *userdatap, int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	((DSChannel *)userdatap)->gadgetCallback( gget_id, itemp, action );
}
//==================================================================
void DSChannel::gadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	if ( _task_managerp )
		if ( _task_managerp->OnGadget( gget_id, itemp, action ) )
			return;

	GGET_Manager	&gam = _tool_winp->GetGGETManager();

	switch ( gget_id )
	{
	/*
	case VIEW_WIN_VIEW_REMOTE_BUTT:
		if ( _session_remotep )
		{
			setViewMode( !_session_remotep->_see_remote_screen );

			if ( _is_transmitting )
			{
				UsageWishMsg	msg( _session_remotep->_see_remote_screen,
									 _is_using_remote );

				if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}
		}
		break;
	*/

	case VIEW_WIN_USE_REMOTE_BUTT:
		if ( _session_remotep )
		{
			setInteractiveMode( !_is_using_remote );
		}
		break;
	}
}

//==================================================================
void DSChannel::taskOnGadgetCB_s( void *userobjp, DSTask *taskp, DSTask::ViewState view_state )
{
	((DSChannel *)userobjp)->taskOnGadgetCB( taskp, view_state );
}
//==================================================================
void DSChannel::taskOnGadgetCB( DSTask *taskp, DSTask::ViewState view_state )
{
	bool	do_show = (view_state != DSTask::VS_ICONIZED);

	switch( taskp->GetButtonID() )
	{
	case VIEW_WIN_TASK_DESK_BUTTON:
		if ( _session_remotep )
		{
			_session_remotep->_see_remote_screen = do_show;
			setViewMode();

			if ( _is_transmitting )
			{
				UsageWishMsg	msg( _session_remotep->_see_remote_screen,
									 _is_using_remote );

				if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}
		}

		{
			GGET_Manager	&gam = _view_winp->GetGGETManager();
			//gam.FindGadget( VIEW_WIN_VIEW_REMOTE_BUTT )->Show( do_show );
			//_task_managerp->FindByButtID( VIEW_WIN_TASK_DESK_BUTTON )->SetViewState( do_show ? DSTask::VS_FITVIEW : DSTask::VS_ICONIZED );
			gam.FindGadget( VIEW_WIN_USE_REMOTE_BUTT )->Show( do_show );
			gam.FindGadget( VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT )->Show( do_show && !_remote_allows_view );
			gam.FindGadget( VIEW_WIN_USE_REM_NOT_ALLOWED_STXT )->Show( do_show && !_remote_allows_share );
		}
		break;

	case VIEW_WIN_TASK_SHELL_BUTTON:
		win_anchor_y2_offset_set( _view_winp, do_show ? -160 : 0 );
		_console.cons_show( do_show );
		break;
	}
}

//==================================================================
int DSChannel::viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->viewEventFilter( etype, eventp );
}
//==================================================================
int DSChannel::viewEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_CREATE:
		viewWinRebuildButtons( eventp->winp );
		break;

	case WIN_ETYPE_ACTIVATE:
		_console._win.SetFocus();
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

	case WIN_ETYPE_MOUSEMOVEDOUT:
		if ( _intersys.IsActive() )
		{
			setInteractiveMode( false );
		}
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
		viewWinReshapeButtons( eventp->winp );

		if ( _task_managerp )
			_task_managerp->OnWinResize();

		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		doViewPaint();
		break;
	}

	return 0;
}

//==================================================================
void DSChannel::viewWinRebuildButtons( Window *winp )
{
	float	x = 4;
	float	w = 90;
	float	h = 22;
	float	y_margin = 4;
	float	x_margin = 4;

	GGET_Manager	&gam = winp->GetGGETManager();

	gam.SetCallback( gadgetCallback_s, this );

	GGET_Item	*itemp;

//	gam.AddButton( VIEW_WIN_VIEW_REMOTE_BUTT, 0, 0, BUTT_WD, BUTT_HE, "View Remote" );
	itemp = gam.AddStaticText( VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT, 0, 0, BUTT_WD*2, BUTT_HE, "Viewing Not Allowed" );
	itemp->SetTextColor( 0.9f, 0, 0, 1 );
	itemp->_flags |= GGET_FLG_ALIGN_LEFT;
	itemp->Show( false );

	gam.AddButton( VIEW_WIN_USE_REMOTE_BUTT, 0, 0, BUTT_WD, BUTT_HE, "Use Remote" );
	itemp = gam.AddStaticText( VIEW_WIN_USE_REM_NOT_ALLOWED_STXT, 0, 0, BUTT_WD*2, BUTT_HE, "Usage Not Allowed" );
	itemp->SetTextColor( 0.9f, 0, 0, 1 );
	itemp->_flags |= GGET_FLG_ALIGN_LEFT;
	itemp->Show( false );

	setViewMode();
}

//==================================================================
void DSChannel::viewWinReshapeButtons( Window *winp )
{
	GGET_Manager	&gam = winp->GetGGETManager();

//	GGET_Item	*itemp = gam.FindGadget( VIEW_WIN_USE_REMOTE_BUTT );
	//buttp->SetPos( eventp->winp->GetWidth() - BUTT_WD - 12, 20 );
//	buttp->SetPos( 12, 20 );

	float	y = 8;

	gam.FindGadget( VIEW_WIN_USE_REMOTE_BUTT )->SetPos( 20, y );
	gam.FindGadget( VIEW_WIN_USE_REM_NOT_ALLOWED_STXT )->SetPos( 20 + BUTT_WD + 4, y );
	y += BUTT_HE + 4;

//	gam.FindGadget( VIEW_WIN_VIEW_REMOTE_BUTT )->SetPos( 20, y );
	gam.FindGadget( VIEW_WIN_VIEW_REM_NOT_ALLOWED_STXT )->SetPos( 20 + BUTT_WD + 4, y );
//	y += BUTT_HE + 4;


	//gam.FindGadget( DSharinguApp::STEXT_TOOLBARBASE )->SetRect( 0, 0, winp->GetWidth(), winp->GetHeight() );
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
			DoDisconnect( "Connection aborted." );
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

//===============================================================
void DSChannel::Show( bool onoff )
{
	_view_winp->Show( onoff );
	_console.cons_show( onoff );
}