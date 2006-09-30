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

#ifndef DOWNLOADUPDATE_H
#define DOWNLOADUPDATE_H

#include <windows.h>
#include "psys.h"
#include "pnetlib_httpfile.h"

//==================================================================
///
//==================================================================
class DownloadUpdate
{
public:
	const char			*_cur_versionp;
	const char			*_hostnamep;
	const char			*_base_exe_pathp;
	const char			*_message_box_titlep;

	bool				_alive;
	int					_state;
	HTTPFile			*_httpfilep;
	HTTPFile			*_exe_httpfilep;
	HWND				_dlg_hwnd;
	char				_donwload_fname[128];
	std::string			_exe_desk_path_str;

	DownloadUpdate( HWND parent_hwnd,
					const char *cur_versionp,
					const char *hostnamep,
					const char *update_info_pathp,
					const char *base_exe_pathp,
					const char *message_box_titlep );
	~DownloadUpdate();

	bool Idle();

	static BOOL CALLBACK dialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
	BOOL CALLBACK dialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
};

#endif