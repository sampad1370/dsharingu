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

#include <gl/glew.h>
#include "screen_sharing.h"
#include "memfile.h"

//==================================================================
using namespace PUtils;

//==================================================================
//= W R I T E R
//==================================================================
ScrShare::Writer::Writer() :
	_is_grabbing(false),
	_last_grab_time(0),
	_last_w(0),
	_last_h(0),
	_tex_per_x(0),
	_tex_per_y(0)

{
}

//==================================================================
bool ScrShare::Writer::StartGrabbing( HWND hwnd )
{
	if ERR_FALSE( _grabber.StartGrabbing( hwnd ) )
		return false;

	_packer.Reset();

	_is_grabbing = true;

	return true;
}

//==================================================================
void ScrShare::Writer::StopGrabbing()
{
	_is_grabbing = false;
}

//==================================================================
static void convert_32_to_24( const u_char *srcp, int spitch, u_char *desp, int dpitch, int dw, int dh )
{
	for (; dh; --dh)
	{
		const u_char *srcp2 = srcp;
			  u_char *desp2 = desp;

		for (u_char	*dendp = desp + dw * 3; desp2 < dendp; desp2 += 3)
		{
			desp2[0] = srcp2[2];
			desp2[1] = srcp2[1];
			desp2[2] = srcp2[0];
			srcp2 += 4;
		}

		srcp += spitch;
		desp += dpitch;
	}
}
//==================================================================
static void convert_24_to_24( const u_char *srcp, int spitch, u_char *desp, int dpitch, int dw, int dh )
{
	for (; dh; --dh)
	{
		const u_char *srcp2 = srcp;
			  u_char *desp2 = desp;

		for (u_char	*dendp = desp + dw * 3; desp2 < dendp; desp2 += 3)
		{
			desp2[0] = srcp2[2];
			desp2[1] = srcp2[1];
			desp2[2] = srcp2[0];
			srcp2 += 3;
		}

		srcp += spitch;
		desp += dpitch;
	}
}
//==================================================================
static void convert_565_to_24( const u_char *srcp, int spitch, u_char *desp, int dpitch, int dw, int dh )
{
	for (; dh; --dh)
	{
		const u_char *srcp2 = srcp;
			  u_char *desp2 = desp;

		for (u_char	*dendp = desp + dw * 3; desp2 < dendp; desp2 += 3)
		{
			u_int	src_565 = ((u_short *)srcp2)[0];

			desp2[0] = (src_565 >> 11-3) & 0xff;
			desp2[1] = (src_565 >> 5 -2) & 0xff;
			desp2[2] = (src_565 <<    3) & 0xff;
			srcp2 += sizeof(u_short);
		}

		srcp += spitch;
		desp += dpitch;
	}
}
//==================================================================
static void convert_555_to_24( const u_char *srcp, int spitch, u_char *desp, int dpitch, int dw, int dh )
{
	for (; dh; --dh)
	{
		const u_char *srcp2 = srcp;
			  u_char *desp2 = desp;

		for (u_char	*dendp = desp + dw * 3; desp2 < dendp; desp2 += 3)
		{
			u_int	src_565 = ((u_short *)srcp2)[0];

			desp2[0] = (src_565 >> 11-3) & 0xff;
			desp2[1] = (src_565 >> 5 -2) & 0xff;
			desp2[2] = (src_565 <<    3) & 0xff;
			srcp2 += sizeof(u_short);
		}

		srcp += spitch;
		desp += dpitch;
	}
}

//==================================================================
bool ScrShare::Writer::captureAndPack( const DDSURFACEDESC2 &desc )
{
	if ERR_FALSE( desc.ddpfPixelFormat.dwRGBBitCount == 16 ||
				desc.ddpfPixelFormat.dwRGBBitCount == 24 ||
				desc.ddpfPixelFormat.dwRGBBitCount == 32 )
		return false;

	_packer.SetScreenSize( desc.dwWidth, desc.dwHeight );

	int	blk_idx = 0;
	int spitch	= desc.lPitch;
	int	sbypp	= (desc.ddpfPixelFormat.dwRGBBitCount + 7) / 8;

	void (*convert_x_to_24)( const u_char *srcp, int spitch, u_char *desp, int dpitch, int w, int h ) = 0;

	switch ( desc.ddpfPixelFormat.dwRGBBitCount )
	{
	case 16:	if ( desc.ddpfPixelFormat.dwGBitMask == 63<<5 )
					convert_x_to_24 = convert_565_to_24;
				else
					convert_x_to_24 = convert_555_to_24;
				break;

	case 24:	convert_x_to_24 = convert_24_to_24;		break;
	case 32:	convert_x_to_24 = convert_32_to_24;		break;
	default:
		throw;
		break;
	}

	u_char	temp_dest[ ScrShare::BLOCK_WD * 3 * ScrShare::BLOCK_HE ];

	int	height = desc.dwHeight;
	int	width = desc.dwWidth;

	for (int y=0; y < height; y += ScrShare::BLOCK_HE)
	{
		int	block_h = ScrShare::BLOCK_HE;
		if ( y + block_h >= height )
			block_h = height - y;

		for (int x=0; x < width; x += ScrShare::BLOCK_WD, ++blk_idx)
		{
		const u_char	*srcp;

			int	block_w = ScrShare::BLOCK_WD;
			if ( x + block_w >= width )
				block_w = width - x;

			srcp = (u_char *)desc.lpSurface + y * spitch + x * sbypp;

			convert_x_to_24( srcp, spitch, temp_dest, ScrShare::BLOCK_WD*3, block_w, block_h );

			_packer.AddBlock( temp_dest );
		}
	}
	if ERR_ERROR( _packer.GetError() )	return false;

	return true;
}

//==================================================================
bool ScrShare::Writer::processGrabbedFrame()
{
	DDSURFACEDESC2	desc;

	_grabber.LockFrame( &desc );

		_last_w = desc.dwWidth;
		_last_h = desc.dwHeight;

		_packer.BeginPack();

			if ERR_FALSE( captureAndPack( desc ) )
			{
				_grabber.UnlockFrame();
				return false;
			}

		_packer.EndPack();

	_grabber.UnlockFrame();
	return true;
}

//==================================================================
bool ScrShare::Writer::UpdateWriter()
{
double	grab_time;

	if ( _is_grabbing )
	{
		grab_time = psys_timer_get_d();

		// grabbing 2 times per second
		if ( grab_time - _last_grab_time > 1000.0/2 )
		{
			_last_grab_time = grab_time;

			if ( _grabber.GrabFrame() )
				if ( processGrabbedFrame() )
					return true;
		}
	}

	return false;
}

//==================================================================
bool ScrShare::Writer::SendFrame( u_int msg_id, Compak *cpkp )
{
	int	tot_size =	sizeof(int) +
					sizeof(int) +
					sizeof(int) + _packer._data._blocks_use_bitmap.size_bytes() +
					sizeof(int) + _packer._data._blkdata_file.GetCurPos();

	u_char	*dest_packp;

	try {
		dest_packp = (u_char *)cpkp->AllocPacket( msg_id, tot_size );
		if ERR_NULL( dest_packp )	return false;

		Memfile	memfile( dest_packp, tot_size );

		memfile.WriteInt( _packer._data.GetWidth() );
		memfile.WriteInt( _packer._data.GetHeight() );
		memfile.WriteUCharArray( _packer._data.GetUseBitmap() );
		memfile.WriteMemfile( &_packer._data.GetData() );
	} catch (...) {
		throw;
	}
	
	if ERR_ERROR( cpkp->SendPacket( dest_packp ) )	return false;

	return true;
}

//==================================================================
//= R E A D E R
//==================================================================
ScrShare::Reader::Reader() :
	_last_w(0),
	_last_h(0),
	_tex_per_x(0),
	_tex_per_y(0),
	_n_texture_ids(0)

{
	for (int i=0; i < MAX_TEXTURES; ++i)
		_texture_ids[i] = 0;
}

//==================================================================
bool ScrShare::Reader::ParseFrame( const void *datap, u_int data_size )
{
	Memfile	memfile( datap, data_size );

	try {
		int w = memfile.ReadInt();
		int h = memfile.ReadInt();

		_packer.SetScreenSize( w, h );

		memfile.ReadUCharArray( _packer._data.GetUseBitmap() );
		memfile.ReadMemfile( &_packer._data.GetData() );

		if ( allocTextures( w, h ) )
		{
			if ( unpackIntoTextures() )
			{
				return true;
			}
		}

	} catch (...) {
		throw;
	}

	return true;
}


//==================================================================
static void check_glerror( int line, const char *filep )
{
	u_int	err = glGetError();
	if ( err )
	{
		PSYS_DEBUG_PRINTF( "GL ERROR #%i at %i - %s\n", line, filep );
	}
}

//==================================================================
#define CHECK_GLERROR	check_glerror( __LINE__, __FILE__ );

//==================================================================
static void alloc_textures_matrix( int tot_w, int tot_h,
								   int tex_w, int tex_h,
								   u_int *out_idsp, 
								   int &n_alloc_ids,
								   int n_max_ids )
{
	int tex_per_x = (tot_w + tex_w-1) / tex_w;
	int tex_per_y = (tot_h + tex_h-1) / tex_h;

	int	n_required	= tex_per_x * tex_per_y;		PSYS_ASSERT( n_required < n_max_ids );
	int	n_to_alloc	= n_required - n_alloc_ids;

	glGenTextures( n_to_alloc, &out_idsp[ n_alloc_ids ] );

	//void *tmpdata = PSYS_MALLOC( tex_w * tex_h * sizeof(u_char) * 4 );
	//KASSERTERR( tmpdata != NULL );
	for (int i=0; i < n_to_alloc; ++i)
	{
		glBindTexture( GL_TEXTURE_2D, out_idsp[ i + n_alloc_ids ] );
//		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

		glTexImage2D( GL_TEXTURE_2D, 0, 4, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );//tmpdata );
		CHECK_GLERROR;
	}
	//SAFE_FREE( tmpdata );

	n_alloc_ids += n_to_alloc;
}

//==================================================================
bool ScrShare::Reader::allocTextures( int w, int h )
{
	if ( w != _last_w || h != _last_h )
	{
		if ERR_FALSE( w <= ScrShare::MAX_SCREEN_WD && h <= ScrShare::MAX_SCREEN_HE )
			return false;

		alloc_textures_matrix( w, h,
								ScrShare::TEX_WD,
								ScrShare::TEX_HE,
								_texture_ids, 
								_n_texture_ids,
								ScrShare::MAX_TEXTURES );

		_tex_per_x = (w + ScrShare::TEX_WD-1) / ScrShare::TEX_WD;
		_tex_per_y = (h + ScrShare::TEX_HE-1) / ScrShare::TEX_HE;

		_last_w = w;
		_last_h = h;

		_packer.SetScreenSize( _last_w, _last_h );
		if ERR_ERROR( _packer.GetError() )
			return false;
	}

	return true;
}

//==================================================================
bool ScrShare::Reader::unpackIntoTextures()
{
	int	cursel_tex_idx = -1;

	_packer.BeginParse();

	u_char	temp_block[ ScrShare::BLOCK_WD * 3 * ScrShare::BLOCK_HE ];
	int		ix, iy;

	//double	t1 = psys_timer_get_d();

	while ( _packer.ParseNextBlock( temp_block, ix, iy ) )
	{
		int	tex_x_idx = (ix/ScrShare::TEX_WD);
		int	tex_y_idx = (iy/ScrShare::TEX_HE);
		int	tex_idx = tex_x_idx + tex_y_idx * _tex_per_x;

		if ( cursel_tex_idx != tex_idx )
		{
			cursel_tex_idx = tex_idx;
			glBindTexture( GL_TEXTURE_2D, _texture_ids[ cursel_tex_idx ] );
			PSYS_ASSERT( _texture_ids[ cursel_tex_idx ] != 0 );
		}

		int	tex_local_x = ix - tex_x_idx * ScrShare::TEX_WD;
		int	tex_local_y = iy - tex_y_idx * ScrShare::TEX_HE;
		glTexSubImage2D( GL_TEXTURE_2D, 0, tex_local_x, tex_local_y, ScrShare::BLOCK_WD, ScrShare::BLOCK_HE,
							GL_RGB, GL_UNSIGNED_BYTE, temp_block );
	}

	//double	t2 = psys_timer_get_d();

	//PSYS_DEBUG_PRINTF( "%i\n", (int)(t2 - t1) );
	//psys_debug_printf( "%i\n", (int)(t2 - t1) );

	_packer.EndParse();

	if ERR_ERROR( _packer.GetError() )
		return false;

	return true;
}

//==================================================================
void ScrShare::Reader::RenderParsedFrame( bool do_fit_viewport )
{
	float	x, y;
	int		tex_idx;

	// check if there is anything to render
	if ( _last_w <= 0 || _last_h <= 0 )
		return;

	glMatrixMode( GL_TEXTURE );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glEnable( GL_TEXTURE_2D );
	CHECK_GLERROR;

	glPushMatrix();
	//glLoadIdentity();

	if ( do_fit_viewport )
	{
	int	vport[4];

		glGetIntegerv( GL_VIEWPORT, vport );

		PSYS_ASSERT( _last_w > 0 );
		PSYS_ASSERT( _last_h > 0 );

		float	ratio_x = (float)vport[2] / _last_w;
		float	ratio_y = (float)vport[3] / _last_h;

		glScalef( ratio_x, ratio_y, 1 );
	}

	tex_idx = 0;
	y = 0;
	for (int i=0; i < _tex_per_y; ++i, y += ScrShare::TEX_HE)
	{
		x = 0;
		for (int j=0; j < _tex_per_x; ++j, ++tex_idx, x += ScrShare::TEX_WD)
		{
			u_int	tex_id = _texture_ids[ tex_idx ];
			glBindTexture( GL_TEXTURE_2D, tex_id );

			CHECK_GLERROR;
			glBegin( GL_QUADS );
				glColor3f( 1, 1, 1 );	glTexCoord2f( 0, 0 );	glVertex2f( x, y );
				glColor3f( 1, 1, 1 );	glTexCoord2f( 1, 0 );	glVertex2f( x+ScrShare::TEX_WD, y );
				glColor3f( 1, 1, 1 );	glTexCoord2f( 1, 1 );	glVertex2f( x+ScrShare::TEX_WD, y+ScrShare::TEX_HE );
				glColor3f( 1, 1, 1 );	glTexCoord2f( 0, 1 );	glVertex2f( x, y+ScrShare::TEX_HE );
			glEnd();
			CHECK_GLERROR;
		}
	}

	glPopMatrix();

	glDisable( GL_TEXTURE_2D );
}