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

#include <stdio.h>
#include "screen_grabber_gdi.h"

//==================================================================
ScreenGrabberGDI::~ScreenGrabberGDI()
{
	if ( _desk_hwnd && _desk_hdc )
	{
		ReleaseDC( _desk_hwnd, _desk_hdc );
		_desk_hdc = NULL;
	}

	if ( _desk_hwnd && _capture_hdc )
	{
		ReleaseDC( _desk_hwnd, _capture_hdc );
		_capture_hdc = NULL;
	}
}

//==================================================================
bool ScreenGrabberGDI::StartGrabbing( HWND hwnd )
{
	_hwnd = hwnd;
	update();

	return true;
}

//==================================================================
bool ScreenGrabberGDI::GrabFrame()
{
	update();

	BitBlt( _capture_hdc, 0, 0, _scr_wd, _scr_he, _desk_hdc, 0, 0, SRCCOPY );

	LPVOID				pBuf = NULL;
	BITMAPINFO			bmpInfo;
	BITMAPFILEHEADER	bmpFileHeader; 

	memset( &bmpInfo, 0, sizeof(BITMAPINFO) );
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits( _desk_hdc, _capture_hbmp, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS );

	if ( bmpInfo.bmiHeader.biSizeImage <= 0 )
	{
		u_int	pitch = bmpInfo.bmiHeader.biWidth * (bmpInfo.bmiHeader.biBitCount+7)/8;
		pitch = (pitch + 3) & ~3;

		bmpInfo.bmiHeader.biSizeImage = pitch * abs(bmpInfo.bmiHeader.biHeight);
	}
/*
	bmpInfo.bmiHeader.biCompression=BI_RGB;

	GetDIBits(hdc,hBitmap,0,bmpInfo.bmiHeader.biHeight,pBuf, &bmpInfo, DIB_RGB_COLORS);
*/
	return true;
}

//==================================================================
bool ScreenGrabberGDI::LockFrame( FrameInfo &out_finfo )
{
	return true;
}

//==================================================================
void ScreenGrabberGDI::UnlockFrame()
{
}

//==================================================================
void ScreenGrabberGDI::update()
{
	int		wd = GetSystemMetrics(SM_CXSCREEN);
	int		he = GetSystemMetrics(SM_CYSCREEN);

	if ( _scr_wd == wd && _scr_he == he )
		return;

	_scr_wd = wd;
	_scr_he = he;

	_desk_hwnd = GetDesktopWindow();

	if ( _desk_hdc )
	{
		ReleaseDC( _desk_hwnd, _desk_hdc );
		_desk_hdc = NULL;
	}
	_desk_hdc = GetDC( _desk_hwnd );

	if ( _capture_hdc )
	{
		ReleaseDC( _desk_hwnd, _capture_hdc );
		_capture_hdc = NULL;
	}
	_capture_hdc = CreateCompatibleDC( _desk_hdc );

	if ( _capture_hbmp )
	{
		DeleteObject( _capture_hbmp );
		_capture_hbmp = NULL;
	}
	_capture_hbmp = CreateCompatibleBitmap( _desk_hdc, _scr_wd, _scr_he );
	SelectObject( _capture_hdc, _capture_hbmp );
}
