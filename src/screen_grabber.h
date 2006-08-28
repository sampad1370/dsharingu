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
//= Creation: Davide Pasca 2005
//=
//=
//=
//=
//==================================================================

#ifndef SCREEN_GRABBER_H
#define SCREEN_GRABBER_H

#include <ddraw.h>
#include "psys.h"

//==================================================================
class ScreenGrabber
{
	HWND					_hwnd;
	LPDIRECTDRAW7			_ddrawp;
	LPDIRECTDRAWSURFACE7	_primary_surfp;
	LPDIRECTDRAWSURFACE7	_offscreen_surfp;
	int						_wd;
	int						_he;

	//PError					_error;
public:
	ScreenGrabber()
	{
		_hwnd = 0;
		_ddrawp = 0;
		_primary_surfp = 0;
		_offscreen_surfp = 0;
		_wd = 0;
		_he = 0;
		//_error = POK;
	}

	bool	StartGrabbing( HWND hwnd );
	bool	GrabFrame();
	void	LockFrame( DDSURFACEDESC2 *descp );
	void	UnlockFrame();

	//PError	GetError() { return _error; }

private:
	PError RebuildOffscreenSurf( const DDSURFACEDESC2 *prim_descp );
	PError VerifyOrCreateContext();
};

#endif
