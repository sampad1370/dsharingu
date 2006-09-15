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

#ifndef REMOTEMNG_H
#define REMOTEMNG_H

#include <windows.h>
#include "kwindow.h"
#include "psys.h"
#include "dsharingu_protocol.h"
#include "data_schema.h"

//==================================================================
class RemoteDef
{
public:
	char		_rm_username[32];
	sha1_t		_rm_password;
	char		_rm_ip_address[128];
	int			_call_port;
	bool		_see_remote_screen;
	bool		_use_remote_screen;

	DataSchema	_schema;

	RemoteDef();

	int		GetCallPortNum() const
	{
		return _call_port;
	}
};

//==================================================================
class RemoteMng
{
	HWND				_hwnd;
public:
	PArray<RemoteDef *>	_remotes_list;
	RemoteDef			*_cur_remotep;
	RemoteDef			*_locked_remotep;

	RemoteMng();
	~RemoteMng();
	void	OpenDialog( win_t *parent_winp,
						void (*onChangedSettingsCB)( void *userdatap ),
						void (*onCallCB)( void *userdatap ),
						void *cb_userdatap );
	void	CloseDialog();

	void	SaveList( FILE *fp );

	static DataSchema *RemoteDefLoaderProc_s( FILE *fp, void *userdatap )
	{
		return ((RemoteMng *)userdatap)->RemoteDefLoaderProc( fp );
	}
	DataSchema	*RemoteDefLoaderProc( FILE *fp );
	RemoteDef	*FindOrAddRemoteDefAndSelect( const char *namep );

	RemoteDef	*GetCurRemote()
	{
		return _cur_remotep;
	}

	void	LockRemote( RemoteDef *remotep )
	{
		_locked_remotep = remotep;
		if ( _hwnd )
			refreshEnabledStatus( _hwnd );
	}
	void	UnlockRemote()
	{
		_locked_remotep = NULL;
		if ( _hwnd )
			refreshEnabledStatus( _hwnd );
	}

private:
	void	onListCommand( HWND hwnd );
	void	onNameFocus( HWND hwnd );
	void	refreshEnabledStatus( HWND hwnd );
	void	setNewEntryRemoteDef( HWND hwnd );
	void	setCurRemoteDef( RemoteDef *remotep, HWND hwnd );
	bool	getCurRemoteDef( RemoteDef *remotep, bool is_adding, HWND hwnd );
	void	(*_onChangedSettingsCB)( void *userdatap );
	void	(*_onCallCB)( void *userdatap );
	void	*_cb_userdatap;
	static BOOL CALLBACK DialogProc_s( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	BOOL CALLBACK DialogProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );

	bool	checkPort( HWND hwnd );
	bool	checkName( const char *new_namep, bool check_duplicates, HWND hwnd ) const;
	void	enableListenGroup( HWND hwnd );
};

#endif