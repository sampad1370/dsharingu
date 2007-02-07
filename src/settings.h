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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <windows.h>
#include "pwindow.h"
#include "psys.h"
#include "data_schema.h"
#include "wingui_utils.h"

//==================================================================
class Settings
{
	bool		_is_open;
public:
	char		_username[32];
	sha1_t		_password;
	bool		_listen_for_connections;
	int			_listen_port;
	bool		_nobody_can_watch_my_computer;
	bool		_nobody_can_use_my_computer;
	bool		_run_after_login;
	bool		_start_minimized;

	DataSchema	_schema;

	Settings();
	~Settings();
	void	OpenDialog( Window *parent_winp, void (*onChangedSettingsCB)( void *userdatap ), void *cb_userdatap );
	void	SaveConfig( FILE *fp );

	static DataSchema *SchemaLoaderProc_s( FILE *fp, void *userdatap )
	{
		return ((Settings *)userdatap)->SchemaLoaderProc( fp );
	}
	DataSchema *SchemaLoaderProc( FILE *fp )
	{
		return &_schema;
	}
	
	bool	ListenForConnections() const;
	int		GetListenPortNum() const;

private:
	void			(*_onChangedSettingsCB)( void *userdatap );
	void			*_cb_userdatap;

	static BOOL CALLBACK DialogProc_s( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	BOOL CALLBACK	DialogProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	WGUTCheckPWMsg	checkPasswords( HWND hwnd );
	bool			checkPort( HWND hwnd );
	void			enableListenGroup( HWND hwnd );
};

#endif