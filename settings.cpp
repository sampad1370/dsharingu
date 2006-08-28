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
#include "settings.h"
#include "SHA1.h"
#include "dsharingu_protocol.h"
#include "resource.h"
/*
//==================================================================
enum
{
	GAY_BTN    =   wxID_HIGHEST + 1,

	CONNECT_IP_COMBO,

	STANDARD_PORT_RADIO,
	CUSTOM_PORT_RADIO,
	CUSTOM_PORT_EDIT,

	SHOW_MY_SCREEN_CHECK,
	SHARE_MY_SCREEN_CHECK,

	PASSWORD1_EDIT,
	PASSWORD2_EDIT,
	/*
	CUSTOM_PORT_EDIT,

	PASSWORD1_EDIT,
	PASSWORD2_EDIT,

	SEE_REMOTE_SCREEN_CHECK,
	REMOTE_PASSWORD_EDIT,
	REMOTE_PASSWORD_EDIT,
* /
};
*/
//==================================================================
static void DlgEnableItem( HWND hwnd, int id, BOOL onoff )
{
	HWND iw = GetDlgItem( hwnd, id );
	EnableWindow( iw, onoff );
}

//==================================================================
static void SetDlgItemInt( HWND hwnd, int id, int val )
{
	char	buff[64];
	sprintf( buff, "%i", val );
	SetDlgItemText( hwnd, id, buff );
}

//==================================================================
static int GetDlgItemInt( HWND hwnd, int id )
{
	char	buff[64];
	GetDlgItemText( hwnd, id, buff, sizeof(buff)-1 );
	return atoi( buff );
}

//==================================================================
static bool IsDlgButtonON( HWND hwnd, u_int item_id )
{
	return IsDlgButtonChecked( hwnd, item_id ) ? true : false;
}

//==================================================================
//==
//==================================================================
Settings::Settings() :
_schema("Settings")
{
	_is_open = false;

	_call_ip[0] = 0;

	_use_custom_port_call = false;
	_call_port = DEF_PORT_NUMBER;

	_use_custom_port_listen = false;
	_listen_for_connections = true;
	_listen_port = DEF_PORT_NUMBER;

	_show_my_screen = true;
	_share_my_screen = true;
	_see_remote_screen = true;

	_schema.AddString(	"_call_ip", _call_ip, sizeof(_call_ip) );

	_schema.AddBool(	"_use_custom_port_call", &_use_custom_port_call );
	_schema.AddInt(		"_call_port", &_call_port, 1, 65535 );

	_schema.AddBool(	"_listen_for_connections", &_listen_for_connections );
	_schema.AddBool(	"_use_custom_port_listen", &_use_custom_port_listen );
	_schema.AddInt(		"_listen_port", &_listen_port, 1, 65535 );

	_schema.AddBool(	"_show_my_screen", &_show_my_screen );
	_schema.AddBool(	"_share_my_screen", &_share_my_screen );
	_schema.AddSHA1Hash("_local_pw", &_local_pw );
	_schema.AddBool(	"_see_remote_screen", &_see_remote_screen );
	_schema.AddSHA1Hash("_remote_pw", &_remote_pw );
}

//===============================================================
BOOL CALLBACK Settings::DialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
	{
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );
	}

	Settings *mythis = (Settings *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->DialogProc( hwnd, umsg, wparam, lparam );
}

//===============================================================
bool Settings::checkPasswords( HWND hwnd )
{
	char	buff1[128];
	char	buff2[128];
	char	rem_buff[128];

	// if we aren't showing or sharing the screen, then passwords are a non-issue
	if NOT( IsDlgButtonON( hwnd, IDC_SHOW_MY_SCREEN_CHECK ) || IsDlgButtonON( hwnd, IDC_SHARE_MY_SCREEN_CHECK ) )
		return true;

	GetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, buff1, sizeof(buff1) );
	GetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, buff2, sizeof(buff2) );
	GetDlgItemText( hwnd, IDC_REMOTE_PASSWORD_EDIT, rem_buff, sizeof(rem_buff) );

	if ( strcmp(buff1, buff2) )
	{
		MessageBox( hwnd, "Passwords don't match !\nPlease make sure that you correctly type the password twice.",
			"Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}
	else
		if ( strlen(buff1) > 32 )
		{
			MessageBox( hwnd, "Password too long !", "Settings Problem", MB_OK | MB_ICONSTOP );

			return false;
		}
		else
			if ( strlen(rem_buff) > 32 )
			{
				MessageBox( hwnd, "Remote's password too long !", "Settings Problem", MB_OK | MB_ICONSTOP );

				return false;
			}

			return true;
}

//===============================================================
bool Settings::checkPort( HWND hwnd )
{
	if NOT( IsDlgButtonON( hwnd, IDC_CUSTOM_PORT_RADIO ) )
		return true;

	int	val = GetDlgItemInt( hwnd, IDC_CUSTOM_PORT_EDIT );
	if ( val < 1 || val > 65535  )
	{
		MessageBox( hwnd, "Invalid port number.\nThe valid range is between 1 to 65535.",
			"Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}

	return true;
}

//===============================================================
static bool GetDlgItemSHA1PW( HWND hDlg, int nIDDlgItem, sha1_t *sha1hashp )
{
	char	tmp[64];
	GetDlgItemText( hDlg, nIDDlgItem, tmp, sizeof(tmp)-1 );

	if ( strcmp( tmp, "emptyempty" ) == 0 )
		return false;

	CSHA1	sha1;

	sha1.Reset();
	sha1.Update( (UINT_8 *)tmp, strlen(tmp) );
	sha1.Final();
	sha1.GetHash( sha1hashp->_data );

	return true;
}

//===============================================================
void Settings::enableListenGroup( HWND hwnd )
{
	bool listen_for_conn = IsDlgButtonON( hwnd, IDC_LISTEN_CONNECTIONS_CHECK );
	bool use_custom = IsDlgButtonON( hwnd, IDC_CUSTOM_PORT_LISTEN_RADIO );

	DlgEnableItem( hwnd, IDC_STANDARD_PORT_LISTEN_RADIO, listen_for_conn );
	DlgEnableItem( hwnd, IDC_CUSTOM_PORT_LISTEN_RADIO, listen_for_conn );
	DlgEnableItem( hwnd, IDC_CUSTOM_PORT_LISTEN_EDIT, use_custom && listen_for_conn );
}

//===============================================================
static void handleCallRadios( HWND hwnd, u_int clicked_id )
{
	bool	is_standard = (clicked_id == IDC_STANDARD_PORT_CALL_RADIO);

	CheckDlgButton( hwnd, IDC_STANDARD_PORT_CALL_RADIO, is_standard );
	CheckDlgButton( hwnd, IDC_CUSTOM_PORT_CALL_RADIO, !is_standard );
	DlgEnableItem( hwnd, IDC_CUSTOM_PORT_CALL_EDIT, !is_standard );
}

//===============================================================
static void handleListenRadios( HWND hwnd, u_int clicked_id )
{
	bool	is_standard = (clicked_id == IDC_STANDARD_PORT_LISTEN_RADIO);

	CheckDlgButton( hwnd, IDC_STANDARD_PORT_LISTEN_RADIO, is_standard );
	CheckDlgButton( hwnd, IDC_CUSTOM_PORT_LISTEN_RADIO, !is_standard );
	DlgEnableItem( hwnd, IDC_CUSTOM_PORT_LISTEN_EDIT, !is_standard );
}

//===============================================================
BOOL CALLBACK Settings::DialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		SetDlgItemText( hwnd, IDC_CONNECT_IP_COMBO, _call_ip );

		CheckDlgButton( hwnd, IDC_CUSTOM_PORT_CALL_RADIO, _use_custom_port_call );
		CheckDlgButton( hwnd, IDC_STANDARD_PORT_CALL_RADIO, !_use_custom_port_call );
		DlgEnableItem( hwnd, IDC_CUSTOM_PORT_CALL_EDIT, _use_custom_port_call );
		SetDlgItemInt( hwnd, IDC_CUSTOM_PORT_CALL_EDIT, _call_port );


		CheckDlgButton( hwnd, IDC_LISTEN_CONNECTIONS_CHECK, _listen_for_connections );
		CheckDlgButton( hwnd, IDC_CUSTOM_PORT_LISTEN_RADIO, _use_custom_port_listen );
		CheckDlgButton( hwnd, IDC_STANDARD_PORT_LISTEN_RADIO, !_use_custom_port_listen );
		DlgEnableItem( hwnd, IDC_CUSTOM_PORT_LISTEN_EDIT, _use_custom_port_listen );
		SetDlgItemInt( hwnd, IDC_CUSTOM_PORT_LISTEN_EDIT, _listen_port );
		enableListenGroup( hwnd );


		CheckDlgButton( hwnd, IDC_SHOW_MY_SCREEN_CHECK, _show_my_screen );
		CheckDlgButton( hwnd, IDC_SHARE_MY_SCREEN_CHECK, _share_my_screen );
		DlgEnableItem( hwnd, IDC_SHARE_MY_SCREEN_CHECK, _show_my_screen );
		DlgEnableItem( hwnd, IDC_PASSWORD1_EDIT, _show_my_screen );
		DlgEnableItem( hwnd, IDC_PASSWORD2_EDIT, _show_my_screen );
		SetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, "emptyempty" );
		SetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, "emptyempty" );

		CheckDlgButton( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK, _see_remote_screen );
		DlgEnableItem( hwnd, IDC_REMOTE_PASSWORD_EDIT, _see_remote_screen );
		SetDlgItemText( hwnd, IDC_REMOTE_PASSWORD_EDIT, "emptyempty" );
		break; 

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDC_SHOW_MY_SCREEN_CHECK:
		case IDC_SHARE_MY_SCREEN_CHECK:
			{
				bool onoff = IsDlgButtonON( hwnd, IDC_SHOW_MY_SCREEN_CHECK );

				DlgEnableItem( hwnd, IDC_SHARE_MY_SCREEN_CHECK, onoff );
				DlgEnableItem( hwnd, IDC_PASSWORD1_EDIT, onoff );
				DlgEnableItem( hwnd, IDC_PASSWORD2_EDIT, onoff );
			}
			break;

		case IDC_SEE_REMOTE_SCREEN_CHECK:
			{
				bool onoff = IsDlgButtonON( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK );

				DlgEnableItem( hwnd, IDC_REMOTE_PASSWORD_EDIT, onoff );
			}
			break;

		case IDC_STANDARD_PORT_CALL_RADIO:
		case IDC_CUSTOM_PORT_CALL_RADIO:
			handleCallRadios( hwnd, LOWORD(wparam) );
			break;

		case IDC_LISTEN_CONNECTIONS_CHECK:
			enableListenGroup( hwnd );
			break;
		case IDC_STANDARD_PORT_LISTEN_RADIO:
		case IDC_CUSTOM_PORT_LISTEN_RADIO:
			handleListenRadios( hwnd, LOWORD(wparam) );
			break;

		case IDOK:
		case IDCANCEL:
			if ( LOWORD(wparam) == IDOK )
			{
				if ( !checkPasswords( hwnd ) || !checkPort( hwnd ) )
					return 1;	// not OK !!

				GetDlgItemText( hwnd, IDC_CONNECT_IP_COMBO, _call_ip, sizeof(_call_ip)-1 );

				_use_custom_port_call = IsDlgButtonON( hwnd, IDC_CUSTOM_PORT_CALL_RADIO );
				_call_port = GetDlgItemInt( hwnd, IDC_CUSTOM_PORT_CALL_EDIT );

				_listen_for_connections = IsDlgButtonON( hwnd, IDC_LISTEN_CONNECTIONS_CHECK );
				_use_custom_port_listen = IsDlgButtonON( hwnd, IDC_CUSTOM_PORT_LISTEN_RADIO );
				_listen_port = GetDlgItemInt( hwnd, IDC_CUSTOM_PORT_LISTEN_EDIT );

				_show_my_screen = IsDlgButtonON( hwnd, IDC_SHOW_MY_SCREEN_CHECK );
				_share_my_screen = IsDlgButtonON( hwnd, IDC_SHARE_MY_SCREEN_CHECK );
				if ( _show_my_screen || _share_my_screen )
				{
					GetDlgItemSHA1PW( hwnd, IDC_PASSWORD1_EDIT, &_local_pw );
				}

				_see_remote_screen = IsDlgButtonON( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK );
				if ( _see_remote_screen )
				{
					GetDlgItemSHA1PW( hwnd, IDC_REMOTE_PASSWORD_EDIT, &_remote_pw );
				}

				if ( _onChangedSettingsCB )
					_onChangedSettingsCB( _cb_userdatap );
			}

			PostMessage(hwnd, WM_CLOSE, 0, 0);
			_is_open = false;
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		_is_open = false;
		break;

	default:
		return 0;
	}

	return 1;
}

//==================================================================
void Settings::OpenDialog( win_t *parent_winp, void (*onChangedSettingsCB)( void *userdatap ), void *cb_userdatap )
{
	if NOT( _is_open )
	{
		_is_open = true;
		_onChangedSettingsCB = onChangedSettingsCB;
		_cb_userdatap = cb_userdatap;

		DialogBoxParam( (HINSTANCE)win_system_getinstance(),
			MAKEINTRESOURCE(IDD_SETTINGS), parent_winp->hwnd,
			DialogProc_s, (LPARAM)this );
	}

	//_dialogp->DoOpen( parent_winp, "Settings" );
}

//==================================================================
void Settings::SaveConfig( FILE *fp )
{
	//	_dialogp->SaveData( fp );
}

//==================================================================
Settings::~Settings()
{
	//	delete _dialogp;
	//	_dialogp = 0;
}

//==================================================================
int Settings::GetCallPortNum() const
{
	if ( _use_custom_port_call )
		return _call_port;
	else
		return DEF_PORT_NUMBER;
}

//==================================================================
int Settings::GetListenPortNum() const
{
	if ( _use_custom_port_listen )
		return _listen_port;
	else
		return DEF_PORT_NUMBER;
}

//==================================================================
bool Settings::ListenForConnections() const
{
	return _listen_for_connections;
}
