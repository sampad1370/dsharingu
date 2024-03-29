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
#include <commctrl.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <string>
#include "appbase3.h"
#include "wingui_utils.h"
#include "pnetlib_httpfile.h"
#include "download_update.h"
#include "resource.h"

//==================================================================
DownloadUpdate::DownloadUpdate( HWND parent_hwnd,
							    const char *cur_versionp,
								const char *hostnamep,
								const char *update_info_pathp,
								const TCHAR *message_box_titlep ) :
	_cur_versionp(cur_versionp),
	_hostnamep(hostnamep),
	_message_box_titlep(message_box_titlep),
	_alive(true),
	_state(0),
	_httpfilep(NULL),
	_exe_httpfilep(NULL),
	_dlg_hwnd(NULL)
{
	_httpfilep =
		new HTTPFile( _hostnamep, update_info_pathp, 80 );

	_dlg_hwnd =
		WGUT::OpenModelessDialog( (DLGPROC)dialogProc_s,
								  MAKEINTRESOURCE(IDD_DOWNLOADING_UPDATE),
								  parent_hwnd,
								  this );
}

//==================================================================
DownloadUpdate::~DownloadUpdate()
{
	SAFE_DELETE( _httpfilep );
	SAFE_DELETE( _exe_httpfilep );
	WGUT::SafeDestroyWindow( _dlg_hwnd );
}

//==================================================================
static double quantizeVersion( const char *vstrp )
{
	const char *vstrp_end = vstrp + strlen(vstrp);

	char	release_type = 'f';
	double	val = 0;
	double	coeff = 1;

	for (const char *vstrp2 = vstrp_end-1; vstrp2 >= vstrp; --vstrp2)
	{
		if ( *vstrp2 == 'a' )
			release_type = 'a';
		else
		if ( *vstrp2 == 'b' )
			release_type = 'b';
		else
		if ( *vstrp2 == 'f' )
			release_type = 'f';
		else
		if ( *vstrp2 == '.' )
		{
			coeff *= 100.0;
		}
		else
		if ( *vstrp2 >= '0' && *vstrp2 <= '9' )
		{
			coeff *= 10.0;
			val += (double)(*vstrp2-'0') * coeff;
		}
	}

	val += (double)(release_type-'a') * 0.1;

	return val;
}

//==================================================================
bool DownloadUpdate::Idle()
{
	if ( _httpfilep->Idle() < 0 )
	{
		WGUT::SafeDestroyWindow( _dlg_hwnd );
		return false;
	}

	if ( _exe_httpfilep && _exe_httpfilep->Idle() < 0 )
	{
		WGUT::SafeDestroyWindow( _dlg_hwnd );
		return false;
	}

	if ( _state == 0 )
	{
		char instr[ 2048 ];
		if ( _httpfilep->GetINDataStr( instr, _countof(instr) ) )
		{
			_state = 1;
			if ( strlen(instr) )
			{
				char	version[128];
				char	hostname[128];

				sscanf_s( instr, "%s %s %s",
						  version, _countof(version),
						  hostname, _countof(hostname),
						  _download_fname, _countof(_download_fname) );

				double other_version = quantizeVersion( version );
				double this_version = quantizeVersion( _cur_versionp );

				if ( other_version <= this_version )
				{
					MessageBox( _dlg_hwnd,
						_T("You are using the most recent version of this software.\n")
						_T("Please check at another time."),
						_message_box_titlep, MB_OK | MB_ICONINFORMATION );

					WGUT::SafeDestroyWindow( _dlg_hwnd );
					return false;
				}

				_exe_httpfilep = new HTTPFile( hostname, _download_fname, 80 );
			}
		}
	}
	else
	if ( _state == 1 )
	{
		PUtils::Memfile	indata_mf;
		if ( _exe_httpfilep->GetINData( indata_mf ) )
		{
			_state = 2;
			DlgEnableItem( _dlg_hwnd, IDC_DU_INSTALL, TRUE );
			DlgShowItem( _dlg_hwnd, IDC_DU_PLEASEWAIT, FALSE );
			DlgShowItem( _dlg_hwnd, IDC_DU_READYINSTALL, TRUE );

			TCHAR szPath[MAX_PATH];

			if PTRAP_FALSE( SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath)) )
			{
				_exe_desk_path_str = PSYS::tstring( szPath );

				TCHAR *download_fname_tchp = PSYS::ANSIToTCHAR( _download_fname );
				PSYS::tstring	download_fname_only( download_fname_tchp );
				delete [] download_fname_tchp;

				int	pos_fname = download_fname_only.find_last_of( _TXCHAR('/') );
				if ( pos_fname >= 0 )
					download_fname_only = download_fname_only.substr( pos_fname + 1 );

				//int	b = download_fname_only.find_last_of( '*' );

				//int	c = a;

				if ( _exe_desk_path_str.find_last_of( _TXCHAR('\\') ) == _exe_desk_path_str.length()-1 )
				{
					_exe_desk_path_str += download_fname_only;
				}
				else
				{
					_exe_desk_path_str += _TXCHAR('\\');
					_exe_desk_path_str += download_fname_only;
				}

				FILE *fp;
				errno_t	err = _tfopen_s( &fp, _exe_desk_path_str.c_str(), _T("wb") );
				if PTRAP_FALSE( err == 0 )
				{
					bool done = (fwrite( indata_mf.GetData(), indata_mf.GetDataSize(), 1, fp ) > 0);
					fclose( fp );
					if PTRAP_FALSE( done )
					{
						ShellExecute( _dlg_hwnd, _T("open"),
							_exe_desk_path_str.c_str(),
							_T("/S"), NULL, SW_SHOWNORMAL );
						WGUT::SafeDestroyWindow( _dlg_hwnd );
						return false;
					}
				}
			}
		}
	}

	return _alive;
}

//===============================================================
BOOL CALLBACK DownloadUpdate::dialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );

	DownloadUpdate *mythis = (DownloadUpdate *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->dialogProc( hwnd, umsg, wparam, lparam );
}
//===============================================================
BOOL CALLBACK DownloadUpdate::dialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		SendMessage( GetDlgItem( hwnd, IDC_DU_DOWNLOADING_PROGRESS ),
					 PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );
		SetTimer( hwnd, 1, 1000/4, NULL );
		//_connecting_dlg_timer = 0;

		DlgEnableItem( hwnd, IDC_DU_INSTALL, FALSE );
		break;

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDOK:
		case IDCANCEL:
			//doDisconnect( "Connection aborted." );
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_TIMER:
		if ( _exe_httpfilep )
			SendMessage( GetDlgItem( hwnd, IDC_DU_DOWNLOADING_PROGRESS ),
						 PBM_SETPOS,
						 _exe_httpfilep->GetPercentageComplete(), 0 );

		SetTimer( hwnd, 1, 1000/4, NULL );
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

	case WM_DESTROY:
		appbase_rem_modeless_dialog( hwnd );
		_alive = false;
		_dlg_hwnd = NULL;
		break;

	default:
		return 0;
	}

	return 1;
}
