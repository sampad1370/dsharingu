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
//= Creation: Davide Pasca 2005
//=
//=
//=
//=
//==================================================================

#include <stdio.h>
#include "screen_grabber.h"

//==================================================================
static char *error_to_string( HRESULT err )
{
    switch( err )
	{
        case DD_OK:	return "No error.";

        case DDERR_ALREADYINITIALIZED: return "DDERR_ALREADYINITIALIZED\nThis object is already initialized.\0";
        case DDERR_BLTFASTCANTCLIP: return "DDERR_BLTFASTCANTCLIP\nReturn if a clipper object is attached to the source surface passed into a BltFast call.\0";
        case DDERR_CANNOTATTACHSURFACE: return "DDERR_CANNOTATTACHSURFACE\nThis surface can not be attached to the requested surface.\0";
        case DDERR_CANNOTDETACHSURFACE: return "DDERR_CANNOTDETACHSURFACE\nThis surface can not be detached from the requested surface.\0";
        case DDERR_CANTCREATEDC: return "DDERR_CANTCREATEDC\nWindows can not create any more DCs.\0";
        case DDERR_CANTDUPLICATE: return "DDERR_CANTDUPLICATE\nCan't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
        case DDERR_CLIPPERISUSINGHWND: return "DDERR_CLIPPERISUSINGHWND\nAn attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
        case DDERR_COLORKEYNOTSET: return "DDERR_COLORKEYNOTSET\nNo src color key specified for this operation.\0";
        case DDERR_CURRENTLYNOTAVAIL: return "DDERR_CURRENTLYNOTAVAIL\nSupport is currently not available.\0";
        case DDERR_DIRECTDRAWALREADYCREATED: return "DDERR_DIRECTDRAWALREADYCREATED\nA DirectDraw object representing this driver has already been created for this process.\0";
        case DDERR_EXCEPTION: return "DDERR_EXCEPTION\nAn exception was encountered while performing the requested operation.\0";
        case DDERR_EXCLUSIVEMODEALREADYSET: return "DDERR_EXCLUSIVEMODEALREADYSET\nAn attempt was made to set the cooperative level when it was already set to exclusive.\0";
        case DDERR_GENERIC: return "DDERR_GENERIC\nGeneric failure.\0";
        case DDERR_HEIGHTALIGN: return "DDERR_HEIGHTALIGN\nHeight of rectangle provided is not a multiple of reqd alignment.\0";
        case DDERR_HWNDALREADYSET: return "DDERR_HWNDALREADYSET\nThe CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
        case DDERR_HWNDSUBCLASSED: return "DDERR_HWNDSUBCLASSED\nHWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
        case DDERR_IMPLICITLYCREATED: return "DDERR_IMPLICITLYCREATED\nThis surface can not be restored because it is an implicitly created surface.\0";
        case DDERR_INCOMPATIBLEPRIMARY: return "DDERR_INCOMPATIBLEPRIMARY\nUnable to match primary surface creation request with existing primary surface.\0";
        case DDERR_INVALIDCAPS: return "DDERR_INVALIDCAPS\nOne or more of the caps bits passed to the callback are incorrect.\0";
        case DDERR_INVALIDCLIPLIST: return "DDERR_INVALIDCLIPLIST\nDirectDraw does not support the provided cliplist.\0";
        case DDERR_INVALIDDIRECTDRAWGUID: return "DDERR_INVALIDDIRECTDRAWGUID\nThe GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
        case DDERR_INVALIDMODE: return "DDERR_INVALIDMODE\nDirectDraw does not support the requested mode.\0";
        case DDERR_INVALIDOBJECT: return "DDERR_INVALIDOBJECT\nDirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
        case DDERR_INVALIDPARAMS: return "DDERR_INVALIDPARAMS\nOne or more of the parameters passed to the function are incorrect.\0";
        case DDERR_INVALIDPIXELFORMAT: return "DDERR_INVALIDPIXELFORMAT\nThe pixel format was invalid as specified.\0";
        case DDERR_INVALIDPOSITION: return "DDERR_INVALIDPOSITION\nReturned when the position of the overlay on the destination is no longer legal for that destination.\0";
        case DDERR_INVALIDRECT: return "DDERR_INVALIDRECT\nRectangle provided was invalid.\0";
        case DDERR_LOCKEDSURFACES: return "DDERR_LOCKEDSURFACES\nOperation could not be carried out because one or more surfaces are locked.\0";
        case DDERR_NO3D: return "DDERR_NO3D\nThere is no 3D present.\0";
        case DDERR_NOALPHAHW: return "DDERR_NOALPHAHW\nOperation could not be carried out because there is no alpha accleration hardware present or available.\0";
        case DDERR_NOBLTHW: return "DDERR_NOBLTHW\nNo blitter hardware present.\0";
        case DDERR_NOCLIPLIST: return "DDERR_NOCLIPLIST\nNo cliplist available.\0";
        case DDERR_NOCLIPPERATTACHED: return "DDERR_NOCLIPPERATTACHED\nNo clipper object attached to surface object.\0";
        case DDERR_NOCOLORCONVHW: return "DDERR_NOCOLORCONVHW\nOperation could not be carried out because there is no color conversion hardware present or available.\0";
        case DDERR_NOCOLORKEY: return "DDERR_NOCOLORKEY\nSurface doesn't currently have a color key\0";
        case DDERR_NOCOLORKEYHW: return "DDERR_NOCOLORKEYHW\nOperation could not be carried out because there is no hardware support of the destination color key.\0";
        case DDERR_NOCOOPERATIVELEVELSET: return "DDERR_NOCOOPERATIVELEVELSET\nCreate function called without DirectDraw object method SetCooperativeLevel being called.\0";
        case DDERR_NODC: return "DDERR_NODC\nNo DC was ever created for this surface.\0";
        case DDERR_NODDROPSHW: return "DDERR_NODDROPSHW\nNo DirectDraw ROP hardware.\0";
        case DDERR_NODIRECTDRAWHW: return "DDERR_NODIRECTDRAWHW\nA hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
        case DDERR_NOEMULATION: return "DDERR_NOEMULATION\nSoftware emulation not available.\0";
        case DDERR_NOEXCLUSIVEMODE: return "DDERR_NOEXCLUSIVEMODE\nOperation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
        case DDERR_NOFLIPHW: return "DDERR_NOFLIPHW\nFlipping visible surfaces is not supported.\0";
        case DDERR_NOGDI: return "DDERR_NOGDI\nThere is no GDI present.\0";
        case DDERR_NOHWND: return "DDERR_NOHWND\nClipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
        case DDERR_NOMIRRORHW: return "DDERR_NOMIRRORHW\nOperation could not be carried out because there is no hardware present or available.\0";
        case DDERR_NOOVERLAYDEST: return "DDERR_NOOVERLAYDEST\nReturned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
        case DDERR_NOOVERLAYHW: return "DDERR_NOOVERLAYHW\nOperation could not be carried out because there is no overlay hardware present or available.\0";
        case DDERR_NOPALETTEATTACHED: return "DDERR_NOPALETTEATTACHED\nNo palette object attached to this surface.\0";
        case DDERR_NOPALETTEHW: return "DDERR_NOPALETTEHW\nNo hardware support for 16 or 256 color palettes.\0";
        case DDERR_NORASTEROPHW: return "DDERR_NORASTEROPHW\nOperation could not be carried out because there is no appropriate raster op hardware present or available.\0";
        case DDERR_NOROTATIONHW: return "DDERR_NOROTATIONHW\nOperation could not be carried out because there is no rotation hardware present or available.\0";
        case DDERR_NOSTRETCHHW: return "DDERR_NOSTRETCHHW\nOperation could not be carried out because there is no hardware support for stretching.\0";
        case DDERR_NOT4BITCOLOR: return "DDERR_NOT4BITCOLOR\nDirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
        case DDERR_NOT4BITCOLORINDEX: return "DDERR_NOT4BITCOLORINDEX\nDirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
        case DDERR_NOT8BITCOLOR: return "DDERR_NOT8BITCOLOR\nDirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
        case DDERR_NOTAOVERLAYSURFACE: return "DDERR_NOTAOVERLAYSURFACE\nReturned when an overlay member is called for a non-overlay surface.\0";
        case DDERR_NOTEXTUREHW: return "DDERR_NOTEXTUREHW\nOperation could not be carried out because there is no texture mapping hardware present or available.\0";
        case DDERR_NOTFLIPPABLE: return "DDERR_NOTFLIPPABLE\nAn attempt has been made to flip a surface that is not flippable.\0";
        case DDERR_NOTFOUND: return "DDERR_NOTFOUND\nRequested item was not found.\0";
        case DDERR_NOTLOCKED: return "DDERR_NOTLOCKED\nSurface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
        case DDERR_NOTPALETTIZED: return "DDERR_NOTPALETTIZED\nThe surface being used is not a palette-based surface.\0";
        case DDERR_NOVSYNCHW: return "DDERR_NOVSYNCHW\nOperation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
        case DDERR_NOZBUFFERHW: return "DDERR_NOZBUFFERHW\nOperation could not be carried out because there is no hardware support for zbuffer blitting.\0";
        case DDERR_NOZOVERLAYHW: return "DDERR_NOZOVERLAYHW\nOverlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
        case DDERR_OUTOFCAPS: return "DDERR_OUTOFCAPS\nThe hardware needed for the requested operation has already been allocated.\0";
        case DDERR_OUTOFMEMORY: return "DDERR_OUTOFMEMORY\nDirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OUTOFVIDEOMEMORY: return "DDERR_OUTOFVIDEOMEMORY\nDirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OVERLAYCANTCLIP: return "DDERR_OVERLAYCANTCLIP\nThe hardware does not support clipped overlays.\0";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE\nCan only have ony color key active at one time for overlays.\0";
        case DDERR_OVERLAYNOTVISIBLE: return "DDERR_OVERLAYNOTVISIBLE\nReturned when GetOverlayPosition is called on a hidden overlay.\0";
        case DDERR_PALETTEBUSY: return "DDERR_PALETTEBUSY\nAccess to this palette is being refused because the palette is already locked by another thread.\0";
        case DDERR_PRIMARYSURFACEALREADYEXISTS: return "DDERR_PRIMARYSURFACEALREADYEXISTS\nThis process already has created a primary surface.\0";
        case DDERR_REGIONTOOSMALL: return "DDERR_REGIONTOOSMALL\nRegion passed to Clipper::GetClipList is too small.\0";
        case DDERR_SURFACEALREADYATTACHED: return "DDERR_SURFACEALREADYATTACHED\nThis surface is already attached to the surface it is being attached to.\0";
        case DDERR_SURFACEALREADYDEPENDENT: return "DDERR_SURFACEALREADYDEPENDENT\nThis surface is already a dependency of the surface it is being made a dependency of.\0";
        case DDERR_SURFACEBUSY: return "DDERR_SURFACEBUSY\nAccess to this surface is being refused because the surface is already locked by another thread.\0";
        case DDERR_SURFACEISOBSCURED: return "DDERR_SURFACEISOBSCURED\nAccess to surface refused because the surface is obscured.\0";
        case DDERR_SURFACELOST: return "DDERR_SURFACELOST\nAccess to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
        case DDERR_SURFACENOTATTACHED: return "DDERR_SURFACENOTATTACHED\nThe requested surface is not attached.\0";
        case DDERR_TOOBIGHEIGHT: return "DDERR_TOOBIGHEIGHT\nHeight requested by DirectDraw is too large.\0";
        case DDERR_TOOBIGSIZE: return "DDERR_TOOBIGSIZE\nSize requested by DirectDraw is too large, but the individual height and width are OK.\0";
        case DDERR_TOOBIGWIDTH: return "DDERR_TOOBIGWIDTH\nWidth requested by DirectDraw is too large.\0";
        case DDERR_UNSUPPORTED: return "DDERR_UNSUPPORTED\nAction not supported.\0";
        case DDERR_UNSUPPORTEDFORMAT: return "DDERR_UNSUPPORTEDFORMAT\nFOURCC format requested is unsupported by DirectDraw.\0";
        case DDERR_UNSUPPORTEDMASK: return "DDERR_UNSUPPORTEDMASK\nBitmask in the pixel format requested is unsupported by DirectDraw.\0";
        case DDERR_VERTICALBLANKINPROGRESS: return "DDERR_VERTICALBLANKINPROGRESS\nVertical blank is in progress.\0";
        case DDERR_WASSTILLDRAWING: return "DDERR_WASSTILLDRAWING\nInforms DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
        case DDERR_WRONGMODE: return "DDERR_WRONGMODE\nThis surface can not be restored because it was created in a different mode.\0";
        case DDERR_XALIGN: return "DDERR_XALIGN\nRectangle provided was not horizontally aligned on required boundary.\0";
        default:
            return "Unrecognized error value.";
    }
}

//==================================================================
static void DD_ErrorReport( HRESULT err, char *filep, int line )
{
char	buff[512];

	if ( err )
	{
		sprintf_s( buff, _countof(buff), "DirectDraw Error: %s - %i - %s", error_to_string( err ), line, filep );
		PSYS_DEBUG_PRINTF( "%s\n", buff );
		//MessageBox( NULL, buff, "RD DirectDraw Error", MB_OK | MB_ICONERROR );
	}
}

//==================================================================
#define DD_ERRORREPORT(_ERR_) DD_ErrorReport( _ERR_, __FILE__, __LINE__ )

//==================================================================
#define DDERR_BEGIN(_EXPR_) {	HRESULT err = (_EXPR_); \
								if ( err ) \
								{	\
									DD_ErrorReport( err, __FILE__, __LINE__ );

#define DDERR_END			} }
								

//==================================================================
bool ScreenGrabber::StartGrabbing( HWND hwnd )
{
	_hwnd = hwnd;
	if ERR_ERROR( verifyOrCreateContext() )
		return false;

	return true;
}

//==================================================================
bool ScreenGrabber::GrabFrame()
{
	if ERR_FALSE( _primary_surfp != NULL && _offscreen_surfp != NULL )
		return false;

	DDERR_BEGIN( _offscreen_surfp->BltFast( 0, 0, _primary_surfp, 0, 0x00000000 ) )
		return false;
	DDERR_END;

	return true;
}

//==================================================================
void ScreenGrabber::LockFrame( DDSURFACEDESC2 *descp )
{
	if ERR_NULL( _offscreen_surfp )
		return;

	descp->dwSize = sizeof(*descp);

	DDERR_BEGIN( _offscreen_surfp->Lock( NULL, descp, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL ) );
		if ERR_ERROR( PERROR )
			return;
	DDERR_END;
}

//==================================================================
void ScreenGrabber::UnlockFrame()
{
	if ( _offscreen_surfp )
	{
		_offscreen_surfp->Unlock( NULL );
	}
}

//==================================================================
PError ScreenGrabber::rebuildOffscreenSurf( const DDSURFACEDESC2 *prim_descp )
{
	if ( _offscreen_surfp )
	{
		_offscreen_surfp->Release();
		_offscreen_surfp = 0;
	}

	DDSURFACEDESC2	off_desc = { 0 };

	off_desc.dwSize							= sizeof(off_desc);
	off_desc.dwFlags						= DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	off_desc.dwWidth						= prim_descp->dwWidth;
	off_desc.dwHeight						= prim_descp->dwHeight;
	off_desc.ddsCaps.dwCaps					= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	off_desc.ddpfPixelFormat.dwFlags		= DDPF_RGB;
	off_desc.ddpfPixelFormat.dwRGBBitCount	= prim_descp->ddpfPixelFormat.dwRGBBitCount;
	off_desc.ddpfPixelFormat.dwRBitMask		= prim_descp->ddpfPixelFormat.dwRBitMask;
	off_desc.ddpfPixelFormat.dwGBitMask		= prim_descp->ddpfPixelFormat.dwGBitMask;
	off_desc.ddpfPixelFormat.dwBBitMask		= prim_descp->ddpfPixelFormat.dwBBitMask;
  
	DDERR_BEGIN( _ddrawp->CreateSurface( &off_desc, &_offscreen_surfp, NULL ) )
		return PERROR;
	DDERR_END;

	return POK;
}

//==================================================================
PError ScreenGrabber::verifyOrCreateContext()
{
	if NOT( _ddrawp )
	{
		DDERR_BEGIN( DirectDrawCreateEx( NULL, (LPVOID *)&_ddrawp, IID_IDirectDraw7, NULL ) )
			return PERROR;
		DDERR_END;

		DDERR_BEGIN( _ddrawp->SetCooperativeLevel( _hwnd, DDSCL_NORMAL ) )
			return PERROR;
		DDERR_END;
	}

	if NOT( _primary_surfp )
	{
		DDSURFACEDESC2   ddsd = { 0 };

		ddsd.dwSize = sizeof( ddsd );
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		DDERR_BEGIN( _ddrawp->CreateSurface( &ddsd, &_primary_surfp, NULL ) )
			return PERROR;
		DDERR_END;

		/*
		if ISERROR( err = _ddrawp->GetGDISurface( &_primary_surfp ) )
		{
			DD_ERRORREPORT( err );
			//PSYS_ASSERT( 0 );
			return PERROR;
		}
		*/
		/*
		HWND hwnd = GetDesktopWindow();
		HDC	hdc = GetDC( hwnd );

		if ISERROR( err = _ddrawp->GetSurfaceFromDC( hdc, &_primary_surfp ) )
		{
			DD_ERRORREPORT( err );
			ReleaseDC( hwnd, hdc );
			//PSYS_ASSERT( 0 );
			return PERROR;
		}
		ReleaseDC( hwnd, hdc );
		*/
	}

	DDSURFACEDESC2	prim_desc = { 0 };
	prim_desc.dwSize = sizeof(prim_desc);
	DDERR_BEGIN( _primary_surfp->GetSurfaceDesc( &prim_desc ) )
		return PERROR;
	DDERR_END;

	if NOT( _offscreen_surfp )
	{
		if ( rebuildOffscreenSurf( &prim_desc ) != POK )
		{
			PSYS_ASSERT( 0 );
			return PERROR;
		}
	}

	DDSURFACEDESC2	off_desc = {0};
	off_desc.dwSize = sizeof(off_desc);
	DDERR_BEGIN( _offscreen_surfp->GetSurfaceDesc( &off_desc ) )
		return PERROR;
	DDERR_END;

	if ( off_desc.dwWidth					 != prim_desc.dwWidth ||
		 off_desc.dwHeight					 != prim_desc.dwHeight ||
		 off_desc.ddpfPixelFormat.dwRBitMask != prim_desc.ddpfPixelFormat.dwRBitMask ||
		 off_desc.ddpfPixelFormat.dwGBitMask != prim_desc.ddpfPixelFormat.dwGBitMask ||
		 off_desc.ddpfPixelFormat.dwBBitMask != prim_desc.ddpfPixelFormat.dwBBitMask )
	{
		if ( rebuildOffscreenSurf( &prim_desc ) != POK )
		{
			PSYS_ASSERT( 0 );
			return PERROR;
		}
	}

	return POK;
}
