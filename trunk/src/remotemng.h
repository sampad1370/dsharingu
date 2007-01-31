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

#ifndef REMOTEMNG_H
#define REMOTEMNG_H

#include <windows.h>
#include "pwindow.h"
#include "psys.h"
#include "dsharingu_protocol.h"
#include "data_schema.h"

//==================================================================
class RemoteDef
{
	bool		_is_locked;
	void		*_userdatap;

public:
	char		_rm_username[32];
	sha1_t		_rm_password;
	char		_rm_ip_address[128];
	int			_call_port;
	bool		_can_watch_my_desk;
	bool		_can_use_my_desk;
	bool		_see_remote_screen;
	bool		_call_automatically;

	DataSchema	_schema;

	RemoteDef();

	int		GetCallPortNum() const
	{
		return _call_port;
	}

	void	Lock()
	{
		_is_locked = true;
	}

	void	Unlock()
	{
		_is_locked = false;
	}

	bool	IsLocked() const
	{
		return _is_locked;
	}

	void SetUserData( void *userdatap )
	{
		_userdatap = userdatap;
	}

	void *GetUserData()
	{
		return _userdatap;
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
	void	OpenDialog( Window *parent_winp,
						void (*onChangedSettingsCB)( void *userdatap, RemoteDef *changed_remotep ),
						void (*onCallCB)( void *userdatap, RemoteDef *remotep ),
						void *cb_userdatap );
	void	CloseDialog();

	void	SaveList( FILE *fp );

	static DataSchema *RemoteDefLoaderProc_s( FILE *fp, void *userdatap )
	{
		return ((RemoteMng *)userdatap)->RemoteDefLoaderProc( fp );
	}
	DataSchema	*RemoteDefLoaderProc( FILE *fp );
	RemoteDef	*FindRemoteDef( const char *namep );
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
	void	UnlockRemote( RemoteDef *remotep )
	{
		_locked_remotep = NULL;
		if ( _hwnd )
			refreshEnabledStatus( _hwnd );
	}

	void	InvalidAddressOnCall();

private:
	void	onListCommand( HWND hwnd );
	void	onNameFocus( HWND hwnd );
	void	updateRemote( HWND hwnd );
	void	refreshEnabledStatus( HWND hwnd );
	void	setNewEntryRemoteDef( HWND hwnd );
	void	setRemoteToForm( RemoteDef *remotep, HWND hwnd );
	void	loadRemoteFromForm( RemoteDef *remotep, HWND hwnd );
	void	loadRemoteNameFromForm( RemoteDef *remotep, HWND hwnd );
	void	(*_onRemoteChange)( void *userdatap, RemoteDef *changed_remotep );
	void	(*_onCallCB)( void *userdatap, RemoteDef *remotep );
	void	*_cb_userdatap;
	void	onEmptyList( HWND hwnd );
	static BOOL CALLBACK DialogProc_s( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );
	BOOL CALLBACK DialogProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam );

	void	makeNameValid( RemoteDef *remotep ) const;
	void	enableListenGroup( HWND hwnd );
};

#endif