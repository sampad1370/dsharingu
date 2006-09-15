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
bool RemoteMng::checkPort( HWND hwnd )
{
	int	val = GetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT );
	if ( val == 0 )	// blank (sort of)
		return true;

	if ( val < 1 || val > 65535  )
	{
		MessageBox( hwnd, "Invalid port number.\nThe valid range is between 1 to 65535.\nLeave it blank to use default.",
			"Remote Manager Problem", MB_OK | MB_ICONSTOP );

		SetDlgEditForReview( hwnd, IDC_RM_REMOTE_PORT );
		return false;
	}

	return true;
}


//===============================================================
bool RemoteMng::checkName( const char *new_namep, bool check_duplicates, HWND hwnd ) const
{
	if ( new_namep[0] == 0 || stricmp( new_namep, _emptyname_string ) == 0 )
	{
		MessageBox( hwnd, "Please choose a real name.",
						"Remote Manager Problem", MB_OK | MB_ICONSTOP );
		return false;
	}

	if ( check_duplicates )
		for (int i=0; i < _remotes_list.len(); ++i)
		{
			if ( stricmp( _remotes_list[i]->_rm_username, new_namep ) == 0 )
			{
				MessageBox( hwnd, "Name already present, please choose a different one.",
					"Remote Manager Problem", MB_OK | MB_ICONSTOP );
				return false;
			}
		}

	return true;
}

//===============================================================
void RemoteMng::setCurRemoteDef( RemoteDef *remotep, HWND hwnd )
{
	if NOT( remotep )
		return;

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

	_cur_remotep = remotep;
}

//===============================================================
void RemoteMng::setNewEntryRemoteDef( HWND hwnd )
{
	SetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, _emptyname_string );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_PASSWORD, "" );
	SetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, "" );
	SetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT, DEF_PORT_NUMBER );
	CheckDlgButton( hwnd, IDC_RM_SEE_REMOTE_SCREEN, false );
	CheckDlgButton( hwnd, IDC_RM_USE_REMOTE_SCREEN, false );

	_cur_remotep = NULL;
}

//===============================================================
bool RemoteMng::getCurRemoteDef( RemoteDef *remotep, bool is_adding, HWND hwnd )
{
	if NOT( remotep )
		return false;

	if ( !checkPort( hwnd ) )
		return false;	// not OK !!

	GetDlgItemText( hwnd, IDC_RM_REMOTE_NAME, remotep->_rm_username, sizeof(remotep->_rm_username)-1 );
	psys_str_remove_beginend_spaces( remotep->_rm_username );
	if NOT( checkName( remotep->_rm_username, is_adding, hwnd ) )
	{
		SetDlgEditForReview( hwnd, IDC_RM_REMOTE_NAME );
		return false;
	}

	WGUTCheckPWMsg pw_msg = GetDlgEditPasswordState( hwnd, IDC_RM_REMOTE_PASSWORD );
	if ( pw_msg == CHECKPW_MSG_BAD )
		return false;	// not OK !!

	// only grab if it was changed
	if ( pw_msg != CHECKPW_MSG_UNCHANGED )
		GetDlgItemSHA1PW( hwnd, IDC_RM_REMOTE_PASSWORD, &remotep->_rm_password );


	GetDlgItemText( hwnd, IDC_RM_REMOTE_ADDRESS, remotep->_rm_ip_address, sizeof(remotep->_rm_ip_address)-1 );

	remotep->_call_port = GetDlgItemInt( hwnd, IDC_RM_REMOTE_PORT );
	remotep->_see_remote_screen = IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN );
	remotep->_use_remote_screen = IsDlgButtonON( hwnd, IDC_RM_USE_REMOTE_SCREEN );

	return true;
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
void RemoteMng::refreshEnabledStatus( HWND hwnd )
{
	int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
	if ERR_FALSE( idx >= 0 )
		return;
	RemoteDef *remotep = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 );

	DlgEnableItem( hwnd, IDC_RM_ADDTOLIST, remotep == NULL );
	DlgEnableItem( hwnd, IDC_RM_CHANGE, remotep != NULL );
	DlgEnableItem( hwnd, IDC_RM_DELETE_REMOTE, remotep != NULL );
	DlgEnableItem( hwnd, IDC_RM_CONNECT, remotep != NULL );

	DlgEnableItem( hwnd, IDC_RM_USE_REMOTE_SCREEN, IsDlgButtonON( hwnd, IDC_RM_SEE_REMOTE_SCREEN) );

	if ( remotep == _locked_remotep && idx != 0 )
	{
		DlgEnableItem( hwnd, IDC_RM_REMOTE_NAME, FALSE );
		DlgEnableItem( hwnd, IDC_RM_REMOTE_PASSWORD, FALSE );
	}
	else
	{
		DlgEnableItem( hwnd, IDC_RM_REMOTE_NAME, TRUE );
		DlgEnableItem( hwnd, IDC_RM_REMOTE_PASSWORD, TRUE );
	}
}

//===============================================================
void RemoteMng::onListCommand( HWND hwnd )
{
	int	idx = SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETCURSEL, 0, 0 );
	if ERR_FALSE( idx >= 0 )
		return;
	RemoteDef *remotep = (RemoteDef *)SendDlgItemMessage( hwnd, IDC_RM_REMOTES_LIST, LB_GETITEMDATA, idx, 0 );

	if ( !remotep )
	{
		setNewEntryRemoteDef( hwnd );
	}
	else
	{
		setCurRemoteDef( remotep, hwnd );
	}
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
BOOL CALLBACK RemoteMng::DialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		{
			_hwnd = hwnd;

			if ( _remotes_list.len() )
			{
				if NOT( _cur_remotep )
					_cur_remotep = _remotes_list[0];

				AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, " New Entry...", (DWORD)0, false );
				setCurRemoteDef( _cur_remotep, hwnd );	// select first available as current
			}
			else
			{
				AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, " New Entry...", (DWORD)0, true );
				setNewEntryRemoteDef( hwnd );
			}

			for (int i=0; i < _remotes_list.len(); ++i)
			{
				AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST,
										 _remotes_list[i]->_rm_username,
										 (DWORD)_remotes_list[i],
										 _remotes_list[i] == _cur_remotep );
			}

			refreshEnabledStatus( hwnd );
		}
		break;

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDC_RM_REMOTE_NAME:
			if ( HIWORD(wparam) == EN_SETFOCUS )
				onNameFocus( hwnd );
			break;

		case IDC_RM_REMOTE_PASSWORD:
			if ( HIWORD(wparam) == EN_SETFOCUS )
			{
				if NOT( IsDlgEditPasswordChanged( hwnd, IDC_RM_REMOTE_PASSWORD ) )
				{
					SetDlgEditForReview( hwnd, IDC_RM_REMOTE_PASSWORD );
				}
			}

			//if ( HIWORD(wparam) == EN_SETFOCUS )
			//	SendDlgItemMessage( hwnd, IDC_RM_REMOTE_NAME, EM_SETSEL, 0, -1 );
			break;

		case IDC_RM_REMOTES_LIST:
			onListCommand( hwnd );
			refreshEnabledStatus( hwnd );
			break;

		case IDC_RM_SEE_REMOTE_SCREEN:
			refreshEnabledStatus( hwnd );
			break;
		case IDC_RM_USE_REMOTE_SCREEN:
			refreshEnabledStatus( hwnd );
			break;

		case IDC_RM_ADDTOLIST:
			{
				RemoteDef	*remotep = new RemoteDef();
				if NOT( getCurRemoteDef( remotep, true, hwnd ) )
				{
					SAFE_DELETE( remotep );
				}
				else
				{
					_remotes_list.append( remotep );
					AddDlgListTextAndSelect( hwnd, IDC_RM_REMOTES_LIST, remotep->_rm_username, (DWORD)remotep, true );
					setCurRemoteDef( remotep, hwnd );
				}

				refreshEnabledStatus( hwnd );
			}
			break;

		case IDC_RM_CHANGE:
			{
				RemoteDef	*remotep = GetCurRemote();
				PSYS_ASSERT( remotep != NULL );
				if ( remotep )
				{
					getCurRemoteDef( remotep, false, hwnd );

					if ( _onChangedSettingsCB )
						_onChangedSettingsCB( _cb_userdatap );
				}

				refreshEnabledStatus( hwnd );
			}
			break;

		case IDC_RM_CONNECT:
			if ( _onCallCB )
				_onCallCB( _cb_userdatap );
			break;

		case IDOK:
		case IDCANCEL:
			if ( _onChangedSettingsCB )
				_onChangedSettingsCB( _cb_userdatap );

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
							void (*onCallCB)( void *userdatap ),
							void *cb_userdatap )
{
	if NOT( _hwnd )
	{
		_onChangedSettingsCB = onChangedSettingsCB;
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
			setCurRemoteDef( _remotes_list[i], NULL );
			return _remotes_list[i];
		}
	}

	RemoteDef	*remotep = new RemoteDef();
	psys_strcpy( remotep->_rm_username, namep, sizeof(remotep->_rm_username) );
	_remotes_list.append( remotep );
	setCurRemoteDef( remotep, NULL );

	return remotep;
}

//==================================================================
RemoteMng::~RemoteMng()
{
	//	delete _dialogp;
	//	_dialogp = 0;
}