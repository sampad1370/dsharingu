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
//= Creation: Davide Pasca 2006
//=
//=
//=
//=
//==================================================================

#ifndef SCREEN_SHARING_H
#define SCREEN_SHARING_H

#include "compak.h"
#include "screen_grabber.h"
#include "screen_packer.h"

//==================================================================
namespace ScrShare
{
const static int	MAX_SCREEN_WD = 1600;
const static int	MAX_SCREEN_HE = 1200;

const static int	TEX_WD		= 256;
const static int	TEX_HE		= 256;

const static int	BLOCK_WD	= 32;
const static int	BLOCK_HE	= 32;

const static int	MAX_TEXTURES	= ((MAX_SCREEN_WD + TEX_WD-1) / TEX_WD) *
										((MAX_SCREEN_HE + TEX_HE-1) / TEX_HE);

const static int	MAX_BLOCKS		= ((MAX_SCREEN_WD + BLOCK_WD-1) / BLOCK_WD) *
										((MAX_SCREEN_HE + BLOCK_HE-1) / BLOCK_HE);

//==================================================================
//= W R I T E R
//==================================================================
class Writer
{
	bool				_is_grabbing;
	double				_last_grab_time;
	ScreenGrabber		_grabber;
	int					_last_w, _last_h;
	int					_tex_per_x;
	int					_tex_per_y;

	ScreenPacker		_packer;

public:
	Writer();
	//~Writer();

	bool	StartGrabbing( HWND hwnd );
	bool	IsGrabbing() const
	{
		return _is_grabbing;
	}
	void	StopGrabbing();
	bool	UpdateWriter();
	bool	SendFrame( u_int msg_id, Compak *cpkp );

	int		GetWidth() const
	{
		return _last_w;
	}
	int		GetHeight() const
	{
		return _last_h;
	}

private:
	bool processGrabbedFrame();
	bool captureAndPack( const DDSURFACEDESC2 &desc );
};

//==================================================================
//= R E A D E R
//==================================================================
class Reader
{
	int					_last_w, _last_h;
	int					_tex_per_x;
	int					_tex_per_y;
	u_int				_texture_ids[ MAX_TEXTURES ];
	int					_n_texture_ids;	
	ScreenUnpacker		_unpacker;

public:
	Reader();
	//~Reader();

	void	RenderParsedFrame( bool do_fit_viewport );
	bool	ParseFrame( const void *datap, u_int data_size );
	int		GetWidth() const
	{
		return _last_w;
	}
	int		GetHeight() const
	{
		return _last_h;
	}

private:
	bool allocTextures( int w, int h );
	bool unpackIntoTextures();
};

};

#endif
