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
#include <gl/glew.h>
#include "settings.h"
#include "SHA1.h"
#include "dsharingu_protocol.h"
#include "dsinstance.h"
#include "resource.h"
#include "wingui_utils.h"
#include "appbase3.h"

//==================================================================
static bool getApplicationInstallDir( const TCHAR *appnamep, char *out_instdir_utf8p, DWORD out_instdir_utf8_maxlen )
{
	out_instdir_utf8p[0] = 0;

	TCHAR	subkey[4096];

	_stprintf_s( subkey, _T("Software\\%s"), appnamep );

	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER,
						subkey,
						0,
						KEY_QUERY_VALUE,
						&hkey) == ERROR_SUCCESS )
	{
		DWORD	dwType;
		DWORD	dwSize = out_instdir_utf8_maxlen - 1;

		bool	success =
			(RegQueryValueEx( hkey, _T(""), NULL, &dwType,
								(LPBYTE)out_instdir_utf8p,
								&out_instdir_utf8_maxlen ) == ERROR_SUCCESS);
		out_instdir_utf8p[out_instdir_utf8_maxlen-1] = 0; // careful with buffer overflow
		RegCloseKey( hkey );

		return success;
	}

	return false;
}

//==================================================================
static bool isApplicationInRegistryRun( const TCHAR *appname_tchp )
{
	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER,
						_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
						0,
						KEY_QUERY_VALUE,
						&hkey) == ERROR_SUCCESS )
	{
		DWORD	dwType;
		char	keyvalue[4096];
		DWORD	dwSize = sizeof(keyvalue);

		bool	success;

		keyvalue[0] = 0;
		success = (RegQueryValueEx( hkey, appname_tchp, NULL, &dwType, (LPBYTE)keyvalue, &dwSize ) == ERROR_SUCCESS);
		keyvalue[_countof(keyvalue)-1] = 0;	// careful with buffer overflow
		RegCloseKey( hkey );

		return success;
	}

	return false;
}

//==================================================================
static bool setApplicationToRegistryRun( const char *appname_utf8p, const TCHAR *appname_tchp )
{
	char	fullpath[PSYS_MAX_PATH];

	strcpy_s( fullpath, "\"" );

	getApplicationInstallDir( appname_tchp, fullpath+1, _countof(fullpath)-1 );
	strcat_s( fullpath, "\\" );
	strcat_s( fullpath, appname_utf8p );
	strcat_s( fullpath, ".exe\" /minimized" );

	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
					   0,
					   KEY_WRITE,
					   &hkey ) == ERROR_SUCCESS)
	{
		if ( RegSetValueEx( hkey, appname_tchp, 0, REG_SZ, (LPBYTE)fullpath, strlen(fullpath) ) == ERROR_SUCCESS )
			return true;
		else
		{
			PASSERT( 0 );
		}
	}

	return false;
}

//==================================================================
static bool removeApplicationFromRegistryRun( const TCHAR *appnamep )
{
	HKEY hkey;

	if ( RegOpenKeyEx(  HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
						0,
						KEY_QUERY_VALUE | KEY_WRITE,
						&hkey ) == ERROR_SUCCESS )

	{
		if ( RegQueryValueEx(hkey, appnamep, NULL, NULL, NULL, NULL) == ERROR_SUCCESS )
		{
			if ( RegDeleteValue( hkey, appnamep ) == ERROR_SUCCESS )
				return true;
			else
			{
				PASSERT( 0 );
				return false;
			}
		}
		else
		{
			// simply no entry
			return false;
		}
	}

	PASSERT( 0 );
	return false;
}

//==================================================================
///
//==================================================================
Settings::Settings() :
	_schema(_T("Settings"))
{
	_is_open = false;

	_username[0] = 0;
	_listen_for_connections = true;
	_listen_port = DEF_PORT_NUMBER;
	_nobody_can_watch_my_computer = false;
	_nobody_can_use_my_computer = true;
	_run_after_login = isApplicationInRegistryRun( APP_NAME );
	_start_minimized = false;

	_schema.AddString(	_T("_username"), _username, _countof(_username) );
	_schema.AddSHA1Hash( _T("_password"), &_password );
	_schema.AddBool(	_T("_listen_for_connections"), &_listen_for_connections );
	_schema.AddInt(		_T("_listen_port"), &_listen_port, 1, 65535 );
	_schema.AddBool(	_T("_nobody_can_watch_my_computer"), &_nobody_can_watch_my_computer, _T("_forbid_show_my_desktop") );
	_schema.AddBool(	_T("_nobody_can_use_my_computer"), &_nobody_can_use_my_computer, _T("_forbid_share_my_desktop")  );
	_schema.AddBool(	_T("_run_after_login"), &_run_after_login );
	_schema.AddBool(	_T("_start_minimized"), &_start_minimized );
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
WGUTCheckPWMsg Settings::checkPasswords( HWND hwnd )
{
	// if we aren't showing or sharing the screen, then passwords are a non-issue
//	if NOT( IsDlgButtonON( hwnd, IDC_FORBID_SHOW_MY_DESKTOP_CHECK ) || IsDlgButtonON( hwnd, IDC_FORBID_SHARE_MY_DESKTOP_CHECK ) )
//		return CHECKPW_MSG_UNCHANGED;


	TCHAR	buff1[128];
	TCHAR	buff2[128];

	WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_PASSWORD1_EDIT, buff1 );
	WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_PASSWORD2_EDIT, buff2 );

	if ( _tcscmp( buff1, buff2 ) )
	{
		MessageBox( hwnd, _T("Passwords don't match !\nPlease make sure that you correctly type the password twice."),
			_T("Settings Problem"), MB_OK | MB_ICONSTOP );

		SetDlgEditForReview( hwnd, IDC_PASSWORD1_EDIT );

		return CHECKPW_MSG_BAD;
	}

	WGUTCheckPWMsg pw1_msg = GetDlgEditPasswordState( hwnd, IDC_PASSWORD1_EDIT, _T("Settings Problem") );
	switch ( pw1_msg )
	{
	case CHECKPW_MSG_BAD:
		return CHECKPW_MSG_BAD;
		
	case CHECKPW_MSG_EMPTY:
		return CHECKPW_MSG_EMPTY;
	}

	WGUTCheckPWMsg pw2_msg = GetDlgEditPasswordState( hwnd, IDC_PASSWORD2_EDIT, _T("Settings Problem") );
	switch ( pw2_msg )
	{
	case CHECKPW_MSG_BAD:
		return CHECKPW_MSG_BAD;

	case CHECKPW_MSG_EMPTY:
		return CHECKPW_MSG_EMPTY;
	}


	if ( pw2_msg == CHECKPW_MSG_BAD || pw2_msg == CHECKPW_MSG_EMPTY )
		return CHECKPW_MSG_BAD;

	if ( (pw1_msg == CHECKPW_MSG_UNCHANGED && pw2_msg == CHECKPW_MSG_UNCHANGED) ||
		 (pw1_msg == CHECKPW_MSG_EMPTY && pw2_msg == CHECKPW_MSG_EMPTY) )
		 return CHECKPW_MSG_UNCHANGED;


	return CHECKPW_MSG_GOOD;
}

//===============================================================
bool Settings::checkPort( HWND hwnd )
{
	int	val = GetDlgItemInt( hwnd, IDC_ST_LOCAL_PORT );
	if ( val == 0 )	// blank (sort of)
		return true;

	if ( val < 1 || val > 65535  )
	{
		MessageBox( hwnd, _T("Invalid port number.\nThe valid range is between 1 to 65535.\nLeave it blank to use default."),
			_T("Settings Problem"), MB_OK | MB_ICONSTOP );

		SetDlgEditForReview( hwnd, IDC_ST_LOCAL_PORT );
		return false;
	}

	return true;
}

//===============================================================
void Settings::enableListenGroup( HWND hwnd )
{
	bool listen_for_conn = IsDlgButtonON( hwnd, IDC_LISTEN_CONNECTIONS_CHECK );
	DlgEnableItem( hwnd, IDC_ST_LOCAL_PORT, listen_for_conn );
}

//===============================================================
BOOL CALLBACK Settings::DialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		SetDlgItemText( hwnd, IDC_ST_USERNAME, _username );

		if ( _password.IsEmpty() )
		{
			SetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, _T("") );
			SetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, _T("") );
		}
		else
		{
			SetDlgItemUnchangedPassword( hwnd, IDC_PASSWORD1_EDIT );
			SetDlgItemUnchangedPassword( hwnd, IDC_PASSWORD2_EDIT );
		}

		CheckDlgButton( hwnd, IDC_LISTEN_CONNECTIONS_CHECK, _listen_for_connections );
		SetDlgItemInt( hwnd, IDC_ST_LOCAL_PORT, _listen_port );
		enableListenGroup( hwnd );

		CheckDlgButton( hwnd, IDC_FORBID_SHOW_MY_DESKTOP_CHECK, _nobody_can_watch_my_computer );
		CheckDlgButton( hwnd, IDC_FORBID_SHARE_MY_DESKTOP_CHECK, _nobody_can_use_my_computer );
		DlgEnableItem( hwnd, IDC_FORBID_SHARE_MY_DESKTOP_CHECK, !_nobody_can_watch_my_computer );

		_run_after_login = isApplicationInRegistryRun( APP_NAME );
		CheckDlgButton( hwnd, IDC_RUN_AFTER_LOGIN, _run_after_login );
		CheckDlgButton( hwnd, IDC_SETTINGS_START_MINIMIZED, _start_minimized );
		break; 

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDC_FORBID_SHOW_MY_DESKTOP_CHECK:
		case IDC_FORBID_SHARE_MY_DESKTOP_CHECK:
			{
				bool onoff = IsDlgButtonON( hwnd, IDC_FORBID_SHOW_MY_DESKTOP_CHECK );
				DlgEnableItem( hwnd, IDC_FORBID_SHARE_MY_DESKTOP_CHECK, !onoff );
			}
			break;

		case IDC_LISTEN_CONNECTIONS_CHECK:
			enableListenGroup( hwnd );
			break;

		case IDC_RUN_AFTER_LOGIN:
			break;

		case IDOK:
		case IDCANCEL:
			if ( LOWORD(wparam) == IDOK )
			{
				WGUT_GETDLGITEMTEXTSAFE( hwnd, IDC_ST_USERNAME, _username );
				if ( _username[0] == 0 )
				{
					MessageBox( hwnd, _T("Username is empty !\nPlease make sure that you choose a username."),
						_T("Settings Problem"), MB_OK | MB_ICONSTOP );

					SetDlgEditForReview( hwnd, IDC_ST_USERNAME );
					return 1;	// not OK !!
				}

				WGUTCheckPWMsg pw_state = checkPasswords( hwnd );

				if ( pw_state != CHECKPW_MSG_GOOD && pw_state != CHECKPW_MSG_UNCHANGED )
					return 1;	// not OK !!

				if ( pw_state == CHECKPW_MSG_GOOD )
					GetDlgItemSHA1PW( hwnd, IDC_PASSWORD1_EDIT, &_password );

				if NOT( checkPort( hwnd ) )
					return 1;	// not OK !!

				_listen_for_connections = IsDlgButtonON( hwnd, IDC_LISTEN_CONNECTIONS_CHECK );
				_listen_port = GetDlgItemInt( hwnd, IDC_ST_LOCAL_PORT );

				_nobody_can_watch_my_computer = IsDlgButtonON( hwnd, IDC_FORBID_SHOW_MY_DESKTOP_CHECK );
				_nobody_can_use_my_computer = IsDlgButtonON( hwnd, IDC_FORBID_SHARE_MY_DESKTOP_CHECK );

				_run_after_login = IsDlgButtonON( hwnd, IDC_RUN_AFTER_LOGIN );
				if ( _run_after_login )
					setApplicationToRegistryRun( APP_NAME_UTF8, APP_NAME );
				else
					removeApplicationFromRegistryRun( APP_NAME );

				_start_minimized = IsDlgButtonON( hwnd, IDC_SETTINGS_START_MINIMIZED );

				// do this last !!!
				if ( _onChangedSettingsCB )
					_onChangedSettingsCB( _cb_userdatap );
			}

			DestroyWindow( hwnd );
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

	case WM_DESTROY:
		_is_open = false;
		appbase_rem_modeless_dialog( hwnd );
		break;

	default:
		return 0;
	}

	return 1;
}

//==================================================================
void Settings::OpenDialog( Window *parent_winp, void (*onChangedSettingsCB)( void *userdatap ), void *cb_userdatap )
{
	if NOT( _is_open )
	{
		_is_open = true;
		_onChangedSettingsCB = onChangedSettingsCB;
		_cb_userdatap = cb_userdatap;

		HWND hwnd =
			CreateDialogParam( (HINSTANCE)WinSys::GetInstance(),
				MAKEINTRESOURCE(IDD_SETTINGS), parent_winp->_hwnd,
				DialogProc_s, (LPARAM)this );
		appbase_add_modeless_dialog( hwnd );
		ShowWindow( hwnd, SW_SHOWNORMAL );
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
int Settings::GetListenPortNum() const
{
	return _listen_port;
}

//==================================================================
bool Settings::ListenForConnections() const
{
	return _listen_for_connections;
}
