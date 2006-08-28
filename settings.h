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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <windows.h>
#include "kwindow.h"
#include "psys.h"
#include "data_schema.h"

//==================================================================
class Settings
{
	bool	_is_open;
public:
	char	_call_ip[128];

	bool	_use_custom_port_call;
	int		_call_port;

	bool	_listen_for_connections;
	bool	_use_custom_port_listen;
	int		_listen_port;

	bool	_show_my_screen;
	bool	_share_my_screen;
	sha1_t	_local_pw;

	bool	_see_remote_screen;
	sha1_t	_remote_pw;

	bool	_changed;

	DataSchema	_schema;

	Settings();
	~Settings();
	void	OpenDialog( win_t *parent_winp, void (*onChangedSettingsCB)( void *userdatap ), void *cb_userdatap );
	void	SaveConfig( FILE *fp );
	int		GetCallPortNum() const;
	
	bool	ListenForConnections() const;
	int		GetListenPortNum() const;

private:
	void	(*_onChangedSettingsCB)( void *userdatap );
	void	*_cb_userdatap;
	static BOOL CALLBACK Settings::DialogProc_s( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	BOOL CALLBACK Settings::DialogProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	bool	checkPasswords( HWND hwnd );
	bool	checkPort( HWND hwnd );
	void	enableListenGroup( HWND hwnd );
};

#endif