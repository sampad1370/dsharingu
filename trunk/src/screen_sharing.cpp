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

#include <gl/glew.h>
#include "screen_sharing.h"
#include "memfile.h"
#include "crc32.h"

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
static void copy_block( const u_char *srcp, int spitch, u_char *desp, int dpitch, int dw, int dh, int bypp )
{
	int	dw_size = dw * bypp;

	for (; dh; --dh)
	{
		const u_char *srcp2 = srcp;
		
		u_char	*desp2 = desp;
		u_char	*dendp = desp + dw_size;

		switch( bypp )
		{
		case 4:
			while ( desp2 < dendp )
			{
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
			}
			break;
		case 3:
			while ( desp2 < dendp )
			{
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
			}
			break;
		case 2:
			while ( desp2 < dendp )
			{
				*desp2++ = *srcp2++;
				*desp2++ = *srcp2++;
			}
			break;
		case 1:
			while ( desp2 < dendp )
			{
				*desp2++ = *srcp2++;
			}
			break;
		}

		srcp += spitch;
		desp += dpitch;
	}
}

//==================================================================
bool ScrShare::Writer::captureAndPack( const ScreenGrabberBase::FrameInfo &finfo )
{
	if ERR_FALSE( finfo._depth == 16 || finfo._depth == 24 || finfo._depth == 32 )
		return false;

	_packer.SetScreenSize( finfo._w, finfo._h );

	int	blk_idx = 0;
	int spitch	= finfo._pitch;
	int	sbypp	= (finfo._depth + 7) / 8;


	void (*convert_x_to_24)( const u_char *srcp, int spitch, u_char *desp, int dpitch, int w, int h ) = 0;

	switch ( finfo._data_format )
	{
	case ScreenGrabberBase::FrameInfo::DF_XRGB1555:	convert_x_to_24 = convert_555_to_24;	break;
	case ScreenGrabberBase::FrameInfo::DF_RGB565:	convert_x_to_24 = convert_565_to_24;	break;
	case ScreenGrabberBase::FrameInfo::DF_BGR888:	convert_x_to_24 = convert_24_to_24;		break;
	case ScreenGrabberBase::FrameInfo::DF_BGRA8888:	convert_x_to_24 = convert_32_to_24;		break;
	default:
		throw "Unknown format !!";
		break;
	}

	int	width = finfo._w;
	int	height = finfo._h;

	int	blk_max_size = ScrShare::BLOCK_WD * ScrShare::BLOCK_HE * sbypp;
	int	tmp_blk_pitch = sbypp * ScrShare::BLOCK_WD;

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

			int	blk_now_size = block_w * block_h * sbypp;

			srcp = (u_char *)finfo._datap + y * spitch + x * sbypp;

			u_char	tmp_blk[ ScrShare::BLOCK_WD * ScrShare::BLOCK_HE * 4 ];
			memset( tmp_blk, 0, ScrShare::BLOCK_WD * ScrShare::BLOCK_HE * 4 );
			copy_block( srcp, spitch, tmp_blk, tmp_blk_pitch, block_w, block_h, sbypp );

			u_int new_checksum = crc32( 0, (const u_char *)tmp_blk, blk_max_size );
			if ( _packer.IsBlockChanged( new_checksum ) )
			{
				u_char	temp_blk24[ ScrShare::BLOCK_WD * 3 * ScrShare::BLOCK_HE ];
				convert_x_to_24( tmp_blk, tmp_blk_pitch, temp_blk24, ScrShare::BLOCK_WD*3, block_w, block_h );
				_packer.AddBlock( temp_blk24, 0, new_checksum );
			}
			else
			if NOT( _packer.IsBlockCompleted() )
			{
				_packer.ContinueBlock();
			}
			else
			{
				_packer.SkipBlock();
			}
		}
	}
	if ERR_ERROR( _packer.GetError() )	return false;

	return true;
}

//==================================================================
bool ScrShare::Writer::processGrabbedFrame()
{
	ScreenGrabberBase::FrameInfo	finfo;

	_grabber.LockFrame( finfo );

		_last_w = finfo._w;
		_last_h = finfo._h;

		_packer.BeginPack();

			if ERR_FALSE( captureAndPack( finfo ) )
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
		grab_time = PSYS::TimerGetD();

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
					sizeof(int) + _packer._data._blkdata_head_file.GetCurPos() +
					sizeof(int) + _packer._data._blkdata_bits_file.GetCurPos();

	u_char	*dest_packp;

	try {
		dest_packp = (u_char *)cpkp->AllocPacket( msg_id, tot_size );

		Memfile	memfile( dest_packp, tot_size );

		memfile.WriteInt( _packer._data.GetWidth() );
		memfile.WriteInt( _packer._data.GetHeight() );
		memfile.WriteUCharArray( _packer._data.GetUseBitmap() );
		memfile.WriteMemfile( &_packer._data.GetDataHead() );
		memfile.WriteMemfile( &_packer._data.GetDataBits() );
	} catch (...) {
		throw "Sad !";
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

		_unpacker.SetScreenSize( w, h );

		memfile.ReadUCharArray( _unpacker._data.GetUseBitmap() );
		memfile.ReadMemfile( &_unpacker._data.GetDataHead() );
		memfile.ReadMemfile( &_unpacker._data.GetDataBits() );

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
static void check_glerror( int line, const TCHAR *filep )
{
	u_int	err = glGetError();
	if ( err )
	{
		PDEBUG_PRINTF( _T("GL ERROR #%i at %i - %s\n"), line, filep );
	}
}

//==================================================================
#define CHECK_GLERROR	check_glerror( __LINE__, _T(__FILE__) );

//==================================================================
static void alloc_textures_matrix( int tot_w, int tot_h,
								   int tex_w, int tex_h,
								   u_int *out_idsp, 
								   int &n_alloc_ids,
								   int n_max_ids )
{
	int tex_per_x = (tot_w + tex_w-1) / tex_w;
	int tex_per_y = (tot_h + tex_h-1) / tex_h;

	int	n_required	= tex_per_x * tex_per_y;		PASSERT( n_required <= n_max_ids );
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

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

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

		_unpacker.SetScreenSize( _last_w, _last_h );
		if ERR_ERROR( _unpacker.GetError() )
			return false;
	}

	return true;
}

//==================================================================
bool ScrShare::Reader::unpackIntoTextures()
{
	int	cursel_tex_idx = -1;

	_unpacker.BeginParse();

	u_char	temp_block[ ScrShare::BLOCK_WD * 3 * ScrShare::BLOCK_HE ];
	int		ix, iy;

	//double	t1 = PSYS::TimerGetD();

	while ( _unpacker.ParseNextBlock( temp_block, ix, iy ) )
	{
		int	tex_x_idx = (ix/ScrShare::TEX_WD);
		int	tex_y_idx = (iy/ScrShare::TEX_HE);
		int	tex_idx = tex_x_idx + tex_y_idx * _tex_per_x;

		if ( cursel_tex_idx != tex_idx )
		{
			cursel_tex_idx = tex_idx;
			glBindTexture( GL_TEXTURE_2D, _texture_ids[ cursel_tex_idx ] );
			PASSERT( _texture_ids[ cursel_tex_idx ] != 0 );
		}

		int	tex_local_x = ix - tex_x_idx * ScrShare::TEX_WD;
		int	tex_local_y = iy - tex_y_idx * ScrShare::TEX_HE;
		glTexSubImage2D( GL_TEXTURE_2D, 0, tex_local_x, tex_local_y, ScrShare::BLOCK_WD, ScrShare::BLOCK_HE,
							GL_RGB, GL_UNSIGNED_BYTE, temp_block );
	}

	//double	t2 = PSYS::TimerGetD();

	//PDEBUG_PRINTF( "%i\n", (int)(t2 - t1) );
	//DebugPrintF( "%i\n", (int)(t2 - t1) );

	_unpacker.EndParse();

	if ERR_ERROR( _unpacker.GetError() )
		return false;

	return true;
}

//==================================================================
static void drawTextureRect( float x, float y, float w, float h, bool is_linear )
{
	float	s1, s2;
	float	t1, t2;

	float	half_tex_w = 0.5f / w;
	float	half_tex_h = 0.5f / w;

	if ( is_linear )
	{
		s1 = half_tex_w;
		s2 = 1 - half_tex_w;

		t1 = half_tex_h;
		t2 = 1 - half_tex_h;
	}
	else
	{
		s1 = 0;
		t1 = 0;
		s2 = 1;
		t2 = 1;
	}

	glBegin( GL_QUADS );
		glColor3f( 1, 1, 1 );	glTexCoord2f( s1, t1 );	glVertex2f( x + 0, y + 0 );
		glColor3f( 1, 1, 1 );	glTexCoord2f( s2, t1 );	glVertex2f( x + w, y + 0 );
		glColor3f( 1, 1, 1 );	glTexCoord2f( s2, t2 );	glVertex2f( x + w, y + h );
		glColor3f( 1, 1, 1 );	glTexCoord2f( s1, t2 );	glVertex2f( x + 0, y + h );
	glEnd();
}

//==================================================================
void ScrShare::Reader::RenderParsedFrame( float scale_x, float scale_y )
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

	glScalef( scale_x, scale_y, 1 );

	tex_idx = 0;
	y = 0;
	for (int i=0; i < _tex_per_y; ++i, y += ScrShare::TEX_HE)
	{
		x = 0;
		for (int j=0; j < _tex_per_x; ++j, ++tex_idx, x += ScrShare::TEX_WD)
		{
			u_int	tex_id = _texture_ids[ tex_idx ];
			glBindTexture( GL_TEXTURE_2D, tex_id );

			bool	is_linear;

			if ( scale_x != 1.0f || scale_y != 1.0f )
			{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				is_linear = true;
			}
			else
			{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				is_linear = false;
			}

			drawTextureRect( x, y, ScrShare::TEX_WD, ScrShare::TEX_HE, is_linear );
		}
	}

	glPopMatrix();

	glDisable( GL_TEXTURE_2D );
}
