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
static bool GetApplicationInstallDir( const char *appnamep, char *out_instdirp, int cnt_out_instdir )
{
	out_instdirp[0] = 0;

	TCHAR	buff[4096];

	sprintf_s( buff, _countof(buff), "Software\\%s", appnamep );

	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER,
						buff,
						0,
						KEY_QUERY_VALUE,
						&hkey) == ERROR_SUCCESS )
	{
		DWORD	dwType;
		DWORD	dwSize = sizeof(buff)-1;

		bool	yesno = (RegQueryValueEx(hkey, "", NULL, &dwType, (LPBYTE)buff, &dwSize ) == ERROR_SUCCESS);
		RegCloseKey( hkey );

		strcpy_s( out_instdirp, cnt_out_instdir, buff );

		return yesno;
	}

	return false;
}


//==================================================================
static bool IsApplicationInRegistryRun( const char *appnamep )
{
	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER,
						"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
						0,
						KEY_QUERY_VALUE,
						&hkey) == ERROR_SUCCESS )
	{
		DWORD	dwType;
		char	buff[4096];
		DWORD	dwSize = sizeof(buff)-1;

		bool	yesno;

		buff[0] = 0;
		yesno = (RegQueryValueEx( hkey, appnamep, NULL, &dwType, (LPBYTE)buff, &dwSize ) == ERROR_SUCCESS);
		RegCloseKey( hkey );

		return yesno;
	}

	return false;
}

//==================================================================
static bool SetApplicationToRegistryRun( const char *appnamep )
{
	char	fullpath[PSYS_MAX_PATH];

	strcpy_s( fullpath, _countof(fullpath), "\"" );

	GetApplicationInstallDir( appnamep, fullpath+1, _countof(fullpath)-1 );
	strcat_s( fullpath, _countof(fullpath), "\\" );
	strcat_s( fullpath, _countof(fullpath), appnamep );
	strcat_s( fullpath, _countof(fullpath), ".exe\" /minimized" );

	HKEY hkey;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
					   0,
					   KEY_WRITE,
					   &hkey ) == ERROR_SUCCESS)
	{
		if ( RegSetValueEx( hkey, appnamep, 0,REG_SZ, (LPBYTE)fullpath, strlen(fullpath) ) == ERROR_SUCCESS )
			return true;
		else
		{
			PSYS_ASSERT( 0 );
		}
	}

	return false;
}

//==================================================================
static bool RemoveApplicationFromRegistryRun( const char *appnamep )
{
	HKEY hkey;

	if ( RegOpenKeyEx(  HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
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
				PSYS_ASSERT( 0 );
				return false;
			}
		}
		else
		{
			// simply no entry
			return false;
		}
	}

	PSYS_ASSERT( 0 );
	return false;
}

//==================================================================
///
//==================================================================
Settings::Settings() :
	_schema("Settings")
{
	_is_open = false;

	_username[0] = 0;
	_listen_for_connections = true;
	_listen_port = DEF_PORT_NUMBER;
	_nobody_can_watch_my_computer = false;
	_nobody_can_use_my_computer = true;
	_run_after_login = IsApplicationInRegistryRun( APP_NAME );
	_start_minimized = false;

	_schema.AddString(	"_username", _username, sizeof(_username) );
	_schema.AddSHA1Hash( "_password", &_password );
	_schema.AddBool(	"_listen_for_connections", &_listen_for_connections );
	_schema.AddInt(		"_listen_port", &_listen_port, 1, 65535 );
	_schema.AddBool(	"_nobody_can_watch_my_computer", &_nobody_can_watch_my_computer, "_forbid_show_my_desktop" );
	_schema.AddBool(	"_nobody_can_use_my_computer", &_nobody_can_use_my_computer, "_forbid_share_my_desktop"  );
	_schema.AddBool(	"_run_after_login", &_run_after_login );
	_schema.AddBool(	"_start_minimized", &_start_minimized );
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


	char	buff1[128];
	char	buff2[128];

	GetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, buff1, sizeof(buff1)-1 );
	GetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, buff2, sizeof(buff2)-1 );

	if ( strcmp(buff1, buff2) )
	{
		MessageBox( hwnd, "Passwords don't match !\nPlease make sure that you correctly type the password twice.",
			"Settings Problem", MB_OK | MB_ICONSTOP );

		SetDlgEditForReview( hwnd, IDC_PASSWORD1_EDIT );

		return CHECKPW_MSG_BAD;
	}

	WGUTCheckPWMsg pw1_msg = GetDlgEditPasswordState( hwnd, IDC_PASSWORD1_EDIT, "Settings Problem" );
	switch ( pw1_msg )
	{
	case CHECKPW_MSG_BAD:
		return CHECKPW_MSG_BAD;
		
	case CHECKPW_MSG_EMPTY:
		return CHECKPW_MSG_EMPTY;
	}

	WGUTCheckPWMsg pw2_msg = GetDlgEditPasswordState( hwnd, IDC_PASSWORD2_EDIT, "Settings Problem" );
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
		MessageBox( hwnd, "Invalid port number.\nThe valid range is between 1 to 65535.\nLeave it blank to use default.",
			"Settings Problem", MB_OK | MB_ICONSTOP );

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
			SetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, "" );
			SetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, "" );
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

		_run_after_login = IsApplicationInRegistryRun( APP_NAME );
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
				GetDlgItemText( hwnd, IDC_ST_USERNAME, _username, sizeof(_username)-1 );
				if ( _username[0] == 0 )
				{
					MessageBox( hwnd, "Username is empty !\nPlease make sure that you choose a username.",
						"Settings Problem", MB_OK | MB_ICONSTOP );

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
					SetApplicationToRegistryRun( APP_NAME );
				else
					RemoveApplicationFromRegistryRun( APP_NAME );

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
