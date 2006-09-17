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
#include "resource.h"
#include "wingui_utils.h"
#include "SHA1.h"
#include "remotemng.h"
#include "appbase3.h"

//==================================================================
static const char _emptyname_string[] = "<Choose a Name>";

//==================================================================
///
//==================================================================
RemoteDef::RemoteDef() :
	_schema("RemoteDef")
{
	_rm_username[0] = 0;
	_rm_ip_address[0] = 0;
	_call_port = DEF_PORT_NUMBER;
	_see_remote_screen = false;
	_use_remote_screen = false;

	_schema.AddString(	"_rm_username", _rm_username, sizeof(_rm_username) );
	_schema.AddSHA1Hash("_rm_password", &_rm_password );
	_schema.AddString(	"_rm_ip_address", _rm_ip_address, sizeof(_rm_ip_address) );
	_schema.AddInt(		"_port", &_call_port, 1, 65535 );
	_schema.AddBool(	"_see_remote_screen", &_see_remote_screen );
	_schema.AddBool(	"_use_remote_screen", &_use_remote_screen );
}

//==================================================================
//==
//==================================================================
RemoteMng::RemoteMng() :
	_hwnd(NULL),
	_cur_remotep(NULL),
	_locked_remotep(NULL)
{
}

//===============================================================
void RemoteMng::makeNameValid( RemoteDef *remotep ) const
{
	psys_str_remove_beginend_spaces( remotep->_rm_username );
	if ( remotep->_rm_username[0] == 0 )
	{
		psys_strcpy( remotep->_rm_username, "Unnamed", sizeof(remotep->_rm_username) );
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
			SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, "" );
		else
			SetDlgItemUnchangedPassword( hwnd, IDC_RM_REMOTE_PASSWORD );
		SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, remotep->_rm_ip_address );
		SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, remotep->_call_port );
		CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, remotep->_see_remote_screen );
		CheckDlgButton( hwnd, IDC_RM_USE_REMOTE_SCREEN, remotep->_use_remote_screen );
	}
}

//===============================================================
void RemoteMng::setNewEntryRemoteDef( HWND hwnd )
{
	_cur_remotep = NULL;

	SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, _emptyname_string );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, "" );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, "" );
	SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, DEF_PORT_NUMBER );
	CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, false );
	CheckDlgButton( hwnd, IDC_RM_USE_REMOTE_SCREEN, false );
}

//===============================================================
static void AddDlgListTextAndSelect( HWND hwnd, int id, const char *strp, DWORD val, bool is_sel )
{
	int i = SendDlgItemMessage( hwnd, id, LB_ADDSTRING, 0, (DWORD)strp );
	SendDlgItemMessage( hwnd, id, LB_SETITEMDATA, i, (DWORD)val );
	if ( is_sel )
		SendDlgItemMessage( hwnd, id, LB_SETCURSEL, i, 0 );
}
//===============================================================
static void DeleteDlgListText( HWND hwnd, int id, const char *strp, DWORD val, bool is_sel )
{
	int i = SendDlgItemMessage( hwnd, id, LB_ADDSTRING, 0, (DWORD)strp );
	SendDlgItemMessage( hwnd, id, LB_SETITEMDATA, i, (DWORD)val );
}

//===============================================================
void RemoteMng::loadRemoteNameFromForm( RemoteDef *remotep, HWND hwnd )
{
	char	old_name[128];

	psys_strcpy( old_name, remotep->_rm_username, sizeof(old_name) );
	GetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, remotep->_rm_username, sizeof(remotep->_rm_username)-1 );
	makeNameValid( remotep );
	bool is_name_changed = (strcmp( remotep->_rm_username, old_name ) != 0);

	if ( is_name_changed )
	{
		for (int i=0; i < _remotes_list.len(); ++i)
		{
			RemoteDef *remote_ip = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, i, 0 );
			PSYS_ASSERT( remote_ip != NULL );

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

	DlgEnableItem( hwnd, IDC_RM_SEE_REMOTE_SCREEN,	is_not_locked );
	DlgEnableItem( hwnd, IDC_RM_USE_REMOTE_SCREEN,
					IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN ) );
}

//===============================================================
void RemoteMng::onListCommand( HWND hwnd )
{
	int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
	if ( idx < 0 )
		return;

	RemoteDef *remotep = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 );

	PSYS_ASSERT( remotep != NULL );
	setRemoteToForm( remotep, hwnd );
}


//===============================================================
void RemoteMng::onNameFocus( HWND hwnd )
{
	char	buff[512];

	GetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, buff, sizeof(buff)-1 );
	psys_str_remove_beginend_spaces( buff );

	if ( stricmp( _emptyname_string, buff ) == 0 )
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
		_onRemoteChange( _cb_userdatap );

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
	SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, "" );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, "" );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, "" );
	SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, DEF_PORT_NUMBER );
	CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, false );
	CheckDlgButton( hwnd, IDC_RM_USE_REMOTE_SCREEN, false );

	DlgEnableItem( hwnd, IDC_RM_REMOTE_NAME, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PASSWORD, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_ADDRESS, FALSE );
	DlgEnableItem( hwnd, IDC_RM_REMOTE_PORT, FALSE );
	DlgEnableItem( hwnd, IDC_RM_SEE_REMOTE_SCREEN, FALSE );
	DlgEnableItem( hwnd, IDC_RM_USE_REMOTE_SCREEN, FALSE );
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
					GetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, _cur_remotep->_rm_ip_address, sizeof(_cur_remotep->_rm_ip_address)-1 );
					psys_str_remove_beginend_spaces( _cur_remotep->_rm_ip_address );
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

		case IDC_RM_SEE_REMOTE_SCREEN:
			if ( _cur_remotep )
			{
				_cur_remotep->_see_remote_screen = IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN );
				updateRemote( hwnd );
			}
			break;

		case IDC_RM_USE_REMOTE_SCREEN:
			if ( _cur_remotep )
			{
				_cur_remotep->_use_remote_screen = IsDlgButtonON( hwnd, IDC_RM_USE_REMOTE_SCREEN );
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
				_remotes_list.append( remotep );
				AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, remotep->_rm_username, (DWORD)remotep, true );
				setRemoteToForm( remotep, hwnd );
				refreshEnabledStatus( hwnd );

				SetDlgEditForReview( hwnd, IDC_RM_REMOTE_NAME );
			}
			break;

		case IDC_RM_CONNECT:
			if ( _cur_remotep->_rm_ip_address[0] == 0 )
			{
				MessageBox( hwnd, "Internet Address is empty.\nPlease provide an Internet Address for the remote to call.",
					"Remote Manager Problem", MB_OK | MB_ICONSTOP );

				SetDlgEditForReview( hwnd, IDC_RM_REMOTE_ADDRESS );
				return false;
			}
			else
			if ( _cur_remotep->_call_port != 0 &&
				(_cur_remotep->_call_port < 1 || _cur_remotep->_call_port > 65535) )
			{
				MessageBox( _hwnd, "Invalid port number.\nThe valid range is between 1 to 65535.\nLeave it blank to use default.",
					"Remote Manager Problem", MB_OK | MB_ICONSTOP );

				SetDlgEditForReview( _hwnd, IDC_RM_REMOTE_PORT );
				return false;
			}
			else
			if ( _cur_remotep->_rm_password.IsEmpty() )
			{
				MessageBox( _hwnd, "No password provided !\nUse provide a password (4 characters minimum).",
					"Remote Manager Problem", MB_OK | MB_ICONSTOP );

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
					PSYS_ASSERT( remote_ip != NULL );

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
				PSYS_ASSERT( 0 );
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
void RemoteMng::OpenDialog( win_t *parent_winp,
							void (*onChangedSettingsCB)( void *userdatap ),
							void (*onCallCB)( void *userdatap, RemoteDef *remotep ),
							void *cb_userdatap )
{
	if NOT( _hwnd )
	{
		_onRemoteChange = onChangedSettingsCB;
		_onCallCB = onCallCB;
		_cb_userdatap = cb_userdatap;

		HWND hwnd =
			CreateDialogParam( (HINSTANCE)win_system_getinstance(),
				MAKEINTRESOURCE(IDD_REMOTEMNG), parent_winp->hwnd,
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
	_remotes_list.append( remotep );

	return &remotep->_schema;
}

//==================================================================
RemoteDef *RemoteMng::FindOrAddRemoteDefAndSelect( const char *namep )
{
	for (int i=0; i < _remotes_list.len(); ++i)
	{
		if ( !stricmp( _remotes_list[i]->_rm_username, namep ) )
		{
			setRemoteToForm( _remotes_list[i], NULL );
			return _remotes_list[i];
		}
	}

	RemoteDef	*remotep = new RemoteDef();
	psys_strcpy( remotep->_rm_username, namep, sizeof(remotep->_rm_username) );
	_remotes_list.append( remotep );
	setRemoteToForm( remotep, NULL );

	return remotep;
}

//==================================================================
RemoteMng::~RemoteMng()
{
	//	delete _dialogp;
	//	_dialogp = 0;
}
