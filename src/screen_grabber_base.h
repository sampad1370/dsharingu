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

#ifndef SCREEN_GRABBER_BASE_H
#define SCREEN_GRABBER_BASE_H

#include <Windows.h>
#include "psys.h"

//==================================================================
class ScreenGrabberBase
{
protected:
	HWND		_hwnd;
	int			_wd;
	int			_he;

public:
	struct FrameInfo
	{
		enum DataFormat
		{
			DF_BGR888,
			DF_BGRA8888,
			DF_XRGB1555,
			DF_RGB565,
		};

		u_int		_w;
		u_int		_h;
		u_int		_depth;
		
		DataFormat	_data_format;

		u_char		*_datap;
		int			_pitch;
	};

public:
	ScreenGrabberBase() :
		_hwnd(NULL),
		_wd(0),
		_he(0)
	{
	}

	virtual bool	StartGrabbing( HWND hwnd ) = NULL;
	virtual bool	GrabFrame() = NULL;
	virtual bool	LockFrame( FrameInfo &finfo ) = NULL;
	virtual void	UnlockFrame() = NULL;
};

#endif
