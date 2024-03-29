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
#include "resource.h"
#include "wingui_utils.h"
#include "SHA1.h"
#include "remotemng.h"
#include "appbase3.h"

//==================================================================
static const TCHAR _emptyname_string[] = _T("<Choose a Name>");

//==================================================================
///
//==================================================================
RemoteDef::RemoteDef() :
	_schema(_T("RemoteDef")),
	_is_locked(false),
	_userdatap(NULL)
{
	_rm_username[0] = 0;
	_rm_ip_address[0] = 0;
	_call_port = DEF_PORT_NUMBER;
	_can_watch_my_desk = true;
	_can_use_my_desk = false;
	_see_remote_screen = false;
	_call_automatically = false;

	_schema.AddString(	_T("_rm_username"), _rm_username, _countof(_rm_username) );
	_schema.AddSHA1Hash(_T("_rm_password"), &_rm_password );
	_schema.AddString(	_T("_rm_ip_address"), _rm_ip_address, _countof(_rm_ip_address) );
	_schema.AddInt(		_T("_port"), &_call_port, 1, 65535 );
	_schema.AddBool(	_T("_can_watch_my_desk"), &_can_watch_my_desk );
	_schema.AddBool(	_T("_can_use_my_desk"), &_can_use_my_desk );
	_schema.AddBool(	_T("_see_remote_screen"), &_see_remote_screen );
	_schema.AddBool(	_T("_call_automatically"), &_call_automatically );
}

//==================================================================
///
//==================================================================
RemoteMng::RemoteMng() :
	_hwnd(NULL),
	_cur_remotep(NULL)
{
}

//===============================================================
void RemoteMng::makeNameValid( RemoteDef *remotep ) const
{
	PSYS::TStrRemoveBeginendSpaces( remotep->_rm_username );
	if ( remotep->_rm_username[0] == 0 )
	{
		_tcscpy_s( remotep->_rm_username, _countof(remotep->_rm_username), _T("Unnamed") );
	}
}

//===============================================================
void RemoteMng::setRemoteToForm( RemoteDef *remotep, HWND hwnd )
{
	if NOT( remotep )
		return;

	_cur_remotep = remotep;

	if ( hwnd )
	{
		SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, remotep->_rm_username );
		if ( remotep->_rm_password.IsEmpty() )
			SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, _T("") );
		else
			SetDlgItemUnchangedPassword( hwnd, IDC_RM_REMOTE_PASSWORD );
		SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, remotep->_rm_ip_address );
		SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, remotep->_call_port );

		CheckDlgButton( hwnd, IDC_RM_CAN_WATCH_MY_DESK, remotep->_can_watch_my_desk );
		CheckDlgButton( hwnd, IDC_RM_CAN_USE_MY_DESK, remotep->_can_use_my_desk );
		CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, remotep->_see_remote_screen );
		CheckDlgButton( hwnd, IDC_RM_AUTO_CALL, remotep->_call_automatically );
	}
}

//===============================================================
void RemoteMng::setNewEntryRemoteDef( HWND hwnd )
{
	_cur_remotep = NULL;

	SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, _emptyname_string );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, _T("") );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, _T("") );
	SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, DEF_PORT_NUMBER );
	CheckDlgButton( hwnd, IDC_RM_CAN_WATCH_MY_DESK, true );
	CheckDlgButton( hwnd, IDC_RM_CAN_USE_MY_DESK, false );
	CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, false );
	CheckDlgButton( hwnd, IDC_RM_AUTO_CALL, false );
}

//===============================================================
static void AddDlgListTextAndSelect( HWND hwnd, int id, const TCHAR *strp, DWORD val, bool is_sel )
{
	int i = SendDlgItemMessage( hwnd, id, LB_ADDSTRING, 0, (DWORD)strp );
	SendDlgItemMessage( hwnd, id, LB_SETITEMDATA, i, (DWORD)val );
	if ( is_sel )
		SendDlgItemMessage( hwnd, id, LB_SETCURSEL, i, 0 );
}
//===============================================================
static void DeleteDlgListText( HWND hwnd, int id, const TCHAR *strp, DWORD val, bool is_sel )
{
	int i = SendDlgItemMessage( hwnd, id, LB_ADDSTRING, 0, (DWORD)strp );
	SendDlgItemMessage( hwnd, id, LB_SETITEMDATA, i, (DWORD)val );
}

//===============================================================
void RemoteMng::loadRemoteNameFromForm( RemoteDef *remotep, HWND hwnd )
{
	TCHAR	old_name[128];

	_tcscpy_s( old_name, remotep->_rm_username );
	WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_RM_REMOTE_NAME, remotep->_rm_username );
	makeNameValid( remotep );
	bool is_name_changed = (_tcscmp( remotep->_rm_username, old_name ) != 0);

	if ( is_name_changed )
	{
		for (int i=0; i < _remotes_list.len(); ++i)
		{
			RemoteDef *remote_ip = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, i, 0 );
			PASSERT( remote_ip != NULL );

			if ( remote_ip == remotep )
			{
				SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_DELETESTRING, i, 0 );
				break;
			}
		}

		AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, remotep->_rm_username, (DWORD)remotep, true );
	}
}

//===============================================================
void RemoteMng::refreshEnabledStatus( HWND hwnd )
{
	int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
	if ( idx < 0 )
		return;

	RemoteDef *remotep = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 );
	if ERR_NULL( remotep )
		return;

	bool	is_not_locked = ( remotep != _locked_remotep );

	DlgEnableItem( hwnd, IDC_RM_REMOTE_NAME,		is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PASSWORD,	is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_ADDRESS,		is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PORT,		is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_DELETE_REMOTE,		is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_CONNECT,			is_not_locked );


	DlgEnableItem( hwnd, IDC_RM_AUTO_CALL, TRUE );

	DlgEnableItem( hwnd, IDC_RM_CAN_WATCH_MY_DESK,	TRUE );
	DlgEnableItem( hwnd, IDC_RM_CAN_USE_MY_DESK,	IsDlgButtonON( hwnd, IDC_RM_CAN_WATCH_MY_DESK ) );

	DlgEnableItem( hwnd, IDC_RM_SEE_REMOTE_SCREEN,	TRUE );
//	DlgEnableItem( hwnd, IDC_RM_USE_REMOTE_SCREEN,
//					IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN ) );
}

//===============================================================
void RemoteMng::onListCommand( HWND hwnd )
{
	int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
	if ( idx < 0 )
		return;

	RemoteDef *remotep = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 );

	PASSERT( remotep != NULL );
	setRemoteToForm( remotep, hwnd );
}


//===============================================================
void RemoteMng::onNameFocus( HWND hwnd )
{
	TCHAR	buff[512];

	WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_RM_REMOTE_NAME, buff );
	PSYS::TStrRemoveBeginendSpaces( buff );

	if ( _tcsicmp( _emptyname_string, buff ) == 0 )
	{
		SendDlgItemMessage( hwnd, IDC_RM_REMOTE_NAME, EM_SETSEL, 0, -1 );
	}
}

//===============================================================
void RemoteMng::updateRemote( HWND hwnd )
{
	if NOT( _cur_remotep )
		return;

	if ( _onRemoteChange )
		_onRemoteChange( _cb_userdatap, _cur_remotep );

	refreshEnabledStatus( hwnd );
}

//===============================================================
BOOL CALLBACK RemoteMng::DialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
	{
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );
	}
	RemoteMng *mythis = (RemoteMng *)GetWindowLongPtr( hwnd, GWLP_USERDATA );
	return mythis->DialogProc( hwnd, umsg, wparam, lparam );
}

//===============================================================
void RemoteMng::onEmptyList( HWND hwnd )
{
	SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, _T("") );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, _T("") );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, _T("") );
	SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, DEF_PORT_NUMBER );
	
	CheckDlgButton( hwnd, IDC_RM_CAN_WATCH_MY_DESK, false );
	CheckDlgButton( hwnd, IDC_RM_CAN_USE_MY_DESK, false );
	CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, false );
	CheckDlgButton( hwnd, IDC_RM_AUTO_CALL, false );

	DlgEnableItem( hwnd, IDC_RM_REMOTE_NAME, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PASSWORD, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_ADDRESS, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PORT, FALSE );
	DlgEnableItem( hwnd, IDC_RM_CAN_WATCH_MY_DESK, false );
	DlgEnableItem( hwnd, IDC_RM_CAN_USE_MY_DESK, false );
	DlgEnableItem( hwnd, IDC_RM_SEE_REMOTE_SCREEN, FALSE );
	DlgEnableItem( hwnd, IDC_RM_AUTO_CALL, FALSE );
	DlgEnableItem( hwnd, IDC_RM_CONNECT, FALSE );
	DlgEnableItem( hwnd, IDC_RM_DELETE_REMOTE, FALSE );
}

//===============================================================
BOOL CALLBACK RemoteMng::DialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		{
			_hwnd = hwnd;

			// special case for when there are no elements
			if NOT( _remotes_list.len() )
			{
				onEmptyList( hwnd );
			}
			else
			{
				if NOT( _cur_remotep )
					_cur_remotep = _remotes_list[0];

				for (int i=0; i < _remotes_list.len(); ++i)
				{
					AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST,
						_remotes_list[i]->_rm_username,
						(DWORD)_remotes_list[i],
						_remotes_list[i] == _cur_remotep );
				}

				if ( _cur_remotep )
					setRemoteToForm( _cur_remotep, hwnd );

				refreshEnabledStatus( hwnd );
			}
		}
		break;

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDC_RM_REMOTE_NAME:
			if ( HIWORD(wparam) == EN_SETFOCUS )
			{
				onNameFocus( hwnd );
			}
			else
			if ( HIWORD(wparam) == EN_CHANGE )
			{
				if ( _cur_remotep )
				{
					loadRemoteNameFromForm( _cur_remotep, hwnd );
					updateRemote( hwnd );
				}
			}
			break;

		case IDC_RM_REMOTE_PASSWORD:
			if ( HIWORD(wparam) == EN_CHANGE )
			{
				if ( _cur_remotep )
				{
					WGUTCheckPWMsg pw_msg = GetDlgEditPasswordState( hwnd, IDC_RM_REMOTE_PASSWORD );
					// only grab if it was changed
					if ( pw_msg == CHECKPW_MSG_GOOD )
						GetDlgItemSHA1PW( hwnd, IDC_RM_REMOTE_PASSWORD, &_cur_remotep->_rm_password );
					updateRemote( hwnd );
				}
			}
			break;

		case IDC_RM_REMOTE_ADDRESS:
			if ( HIWORD(wparam) == EN_CHANGE )
			{
				if ( _cur_remotep )
				{
					WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_RM_REMOTE_ADDRESS, _cur_remotep->_rm_ip_address );
					PSYS::TStrRemoveBeginendSpaces( _cur_remotep->_rm_ip_address );
					updateRemote( hwnd );
				}
			}
			break;

		case IDC_RM_REMOTE_PORT:
			if ( HIWORD(wparam) == EN_CHANGE )
			{
				if ( _cur_remotep )
				{
					_cur_remotep->_call_port = GetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT );
					updateRemote( hwnd );
				}
			}
			break;

		case IDC_RM_CAN_WATCH_MY_DESK:
			if ( _cur_remotep )
			{
				_cur_remotep->_can_watch_my_desk = IsDlgButtonON( hwnd, IDC_RM_CAN_WATCH_MY_DESK );
				updateRemote( hwnd );
			}
			break;
		case IDC_RM_CAN_USE_MY_DESK:
			if ( _cur_remotep )
			{
				_cur_remotep->_can_use_my_desk = IsDlgButtonON( hwnd, IDC_RM_CAN_USE_MY_DESK );
				updateRemote( hwnd );
			}
			break;

		case IDC_RM_SEE_REMOTE_SCREEN:
			if ( _cur_remotep )
			{
				_cur_remotep->_see_remote_screen = IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN );
				updateRemote( hwnd );
			}
			break;

		case IDC_RM_AUTO_CALL:
			if ( _cur_remotep )
			{
				_cur_remotep->_call_automatically = IsDlgButtonON( hwnd, IDC_RM_AUTO_CALL );
				updateRemote( hwnd );
			}
			break;

		case IDC_RM_REMOTES_LIST:
			switch (HIWORD(wparam)) 
			{ 
			case LBN_SELCHANGE:
				onListCommand( hwnd );
				refreshEnabledStatus( hwnd );
				break;
			}
			break;

		case IDC_RM_NEW_REMOTE:
			{
				setNewEntryRemoteDef( hwnd );

				RemoteDef	*remotep = new RemoteDef();
				_remotes_list.push_back( remotep );
				AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, remotep->_rm_username, (DWORD)remotep, true );
				setRemoteToForm( remotep, hwnd );
				refreshEnabledStatus( hwnd );

				SetDlgEditForReview( hwnd, IDC_RM_REMOTE_NAME );
			}
			break;

		case IDC_RM_CONNECT:
			if ( _cur_remotep->_rm_ip_address[0] == 0 )
			{
				MessageBox( hwnd, _T("Internet Address is empty.\nPlease provide an Internet Address for the remote to call."),
					_T("Remote Manager Problem"), MB_OK | MB_ICONSTOP );

				SetDlgEditForReview( hwnd, IDC_RM_REMOTE_ADDRESS );
				return false;
			}
			else
			if ( _cur_remotep->_call_port != 0 &&
				(_cur_remotep->_call_port < 1 || _cur_remotep->_call_port > 65535) )
			{
				MessageBox( _hwnd, _T("Invalid port number.\nThe valid range is between 1 to 65535.\nLeave it blank to use default."),
					_T("Remote Manager Problem"), MB_OK | MB_ICONSTOP );

				SetDlgEditForReview( _hwnd, IDC_RM_REMOTE_PORT );
				return false;
			}
			else
			if ( _cur_remotep->_rm_password.IsEmpty() )
			{
				MessageBox( _hwnd, _T("No password provided !\nUse provide a password (4 characters minimum)."),
					_T("Remote Manager Problem"), MB_OK | MB_ICONSTOP );

				SetDlgEditForReview( _hwnd, IDC_RM_REMOTE_PASSWORD );
			}
			else
			{
				if ( _onCallCB )
					_onCallCB( _cb_userdatap, _cur_remotep );
			}
			break;

		case IDC_RM_DELETE_REMOTE:
			if ( _cur_remotep != _locked_remotep )
			{
				for (int i=0; i < _remotes_list.len(); ++i)
				{
					RemoteDef *remote_ip = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, i, 0 );
					PASSERT( remote_ip != NULL );

					if ( remote_ip == _cur_remotep )
					{
						SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_DELETESTRING, i, 0 );
						SAFE_DELETE( remote_ip );
						for (int j=0; j < _remotes_list.len(); ++j)
						{
							if ( _remotes_list[j] == _cur_remotep )
							{
								_remotes_list.erase( j );
								break;
							}
						}
						
						if ( _remotes_list.len() )
						{
							if ( i >= _remotes_list.len() )
								i = _remotes_list.len()-1;

							SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_SETCURSEL, i, 0 );
							int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
							if ( idx >= 0 )
							{
								setRemoteToForm(
									(RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 ),
									hwnd );
							}
						}
						else
						{
							_cur_remotep = NULL;
							onEmptyList( hwnd );
						}

						break;
					}
				}
			}
			else
			{
				PASSERT( 0 );
			}
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
		appbase_rem_modeless_dialog( hwnd );
		_hwnd = NULL;
		break;

	default:
		return 0;
	}

	return 1;
}

//==================================================================
void RemoteMng::OpenDialog( Window *parent_winp,
							void (*onRemoteChangeCB)( void *userdatap, RemoteDef *changed_remotep ),
							void (*onCallCB)( void *userdatap, RemoteDef *remotep ),
							void *cb_userdatap )
{
	if NOT( _hwnd )
	{
		_onRemoteChange = onRemoteChangeCB;
		_onCallCB = onCallCB;
		_cb_userdatap = cb_userdatap;

		HWND hwnd =
			CreateDialogParam( (HINSTANCE)WinSys::GetInstance(),
				MAKEINTRESOURCE(IDD_REMOTEMNG), parent_winp->_hwnd,
				DialogProc_s, (LPARAM)this );
		appbase_add_modeless_dialog( hwnd );
		ShowWindow( hwnd, SW_SHOWNORMAL );
	}

	//_dialogp->DoOpen( parent_winp, "RemoteMng" );
}

//==================================================================
void RemoteMng::InvalidAddressOnCall()
{
	if ( _hwnd )
	{
		SetDlgEditForReview( _hwnd, IDC_RM_REMOTE_ADDRESS );
	}
}

//==================================================================
void RemoteMng::CloseDialog()
{
	DestroyWindow( _hwnd );
	//CloseWindow( _hwnd );
}

//==================================================================
void RemoteMng::SaveList( FILE *fp )
{
	for (int i=0; i < _remotes_list.len(); ++i)
	{
		_remotes_list[i]->_schema.SaveData( fp );
	}
}

//==================================================================
DataSchema *RemoteMng::RemoteDefLoaderProc( FILE *fp )
{
	RemoteDef	*remotep = new RemoteDef();
	remotep->_schema.LoadData( fp );

	if ( remotep->_rm_username[0] == 0 )	// bad username
	{
		SAFE_DELETE( remotep );
		return NULL;
	}
	_remotes_list.push_back( remotep );

	return &remotep->_schema;
}

//==================================================================
RemoteDef *RemoteMng::FindOrAddRemoteDefAndSelect( const TCHAR *namep )
{
	RemoteDef	*remotep = FindRemoteDef( namep );
	if ( remotep )
	{
		setRemoteToForm( remotep, NULL );
		return remotep;
	}

	remotep = new RemoteDef();
	_tcscpy_s( remotep->_rm_username, namep );
	_remotes_list.push_back( remotep );
	setRemoteToForm( remotep, NULL );

	return remotep;
}

//==================================================================
RemoteDef *RemoteMng::FindRemoteDef( const TCHAR *namep )
{
	for (int i=0; i < _remotes_list.len(); ++i)
	{
		if ( !_tcsicmp( _remotes_list[i]->_rm_username, namep ) )
		{
			return _remotes_list[i];
		}
	}

	return NULL;
}


//==================================================================
RemoteMng::~RemoteMng()
{
	//	delete _dialogp;
	//	_dialogp = 0;
}
