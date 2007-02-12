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
///
//==================================================================

#ifndef SCREEN_GRABBER_GDI_H
#define SCREEN_GRABBER_GDI_H

#include "psys.h"
#include "screen_grabber_base.h"

//==================================================================
class ScreenGrabberGDI : public ScreenGrabberBase
{
	int		_scr_wd;
	int		_scr_he;
	HWND	_desk_hwnd;
	HDC		_desk_hdc;
	HDC		_capture_hdc;
	HBITMAP	_capture_hbmp;

public:
	ScreenGrabberGDI() :
		_scr_wd(0),
		_scr_he(0),
		_desk_hwnd(NULL),
		_desk_hdc(NULL),
		_capture_hdc(NULL),
		_capture_hbmp(NULL)
	{
	}

	~ScreenGrabberGDI();

	virtual bool	StartGrabbing( HWND hwnd );
	virtual bool	GrabFrame();
	virtual bool	LockFrame( FrameInfo &out_finfo );
	virtual	void	UnlockFrame();

private:

	void update();
};

#endif
