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

#include <math.h>
#include "screen_packer.h"
#include "lzw_packer.h"
#include "memfile.h"
#include "crc32.h"

//#define DISP_FLAT_BLOCKS
//#define FORCE_ALL_BLOCKS

//==================================================================
using namespace PUtils;

//==================================================================
static const int	MAX_BLK_RGB_SIZE		= ScreenPackerData::BLOCK_WD * ScreenPackerData::BLOCK_HE * 3;
static const int	MAX_BLK_RGB_LINESIZE	= ScreenPackerData::BLOCK_WD * 3;
static const int	MAX_BLK_PIXELS			= ScreenPackerData::BLOCK_WD * ScreenPackerData::BLOCK_HE;

static const int	SUBBLK_DIM = 4;
static const int	MAX_SUBBLK_PIXELS = SUBBLK_DIM * SUBBLK_DIM;
static const int	PAK_PAL_DIM = 4;

static const int	BLK_BITS_Y			= 3;//5;
static const int	BLK_BITS_U			= 0;//5;
static const int	BLK_BITS_V			= 0;//5;

static const int	BLK_BITS_Y_MASK		= (1 << BLK_BITS_Y) - 1;
static const int	BLK_BITS_U_MASK		= (1 << BLK_BITS_U) - 1;
static const int	BLK_BITS_V_MASK		= (1 << BLK_BITS_V) - 1;

//static const int	MAX_BLK_PAK_SIZE = (((MAX_SUBBLK_PIXELS + 7) >> 3)*2 + sizeof(u_short)*PAK_PAL_DIM) * 8 * 8;
static const int	MAX_BLK_PAK_SIZE_Y = (((MAX_BLK_PIXELS * BLK_BITS_Y / 8) + 7) & ~7);
static const int	MAX_BLK_PAK_SIZE_U = (((MAX_BLK_PIXELS * BLK_BITS_U / 8) + 7) & ~7);
static const int	MAX_BLK_PAK_SIZE_V = (((MAX_BLK_PIXELS * BLK_BITS_V / 8) + 7) & ~7);
static const int	MAX_BLK_PAK_OFF_Y = 0;
static const int	MAX_BLK_PAK_OFF_U = MAX_BLK_PAK_OFF_Y + MAX_BLK_PAK_SIZE_Y;
static const int	MAX_BLK_PAK_OFF_V = MAX_BLK_PAK_OFF_U + MAX_BLK_PAK_SIZE_U;

static const int	MAX_BLK_PAK_SIZE = MAX_BLK_PAK_SIZE_Y + MAX_BLK_PAK_SIZE_U + MAX_BLK_PAK_SIZE_V;

static const int	BLK_RGB_PITCH	= ScreenPackerData::BLOCK_WD * 3;


//==================================================================
struct ScreenPackerNetMessage
{
	int	w;
	int	h;

	int	from_block_idx;
	int	n_blocks;

	//u_char	blocks_use_bitmap[ n_blocks + 7 >> 3 ];
	//u_char	blocks_datap[ sizeof(ScreenPackerNetMessage) - msg_size ];
};

//==================================================================
void SPAKMM::Reset()
{
	_cur_frame = 0;
	_error = POK;

	for (int i=0; i < _data._blocks_pack_work.len(); ++i)
		_data._blocks_pack_work[i].Reset();
}
//==================================================================
bool SPAKMM::SetScreenSize( int w, int h )
{
	if ( w != _data._w || h != _data._h )
	{
		_data._w = w;
		_data._h = h;

		_max_blocks = ScreenPackerData::CalcMaxBlocks( w, h );

		try {
			_data._blocks_use_bitmap.resize( (_max_blocks + 7) / 8 );
			_data._blocks_pack_work.resize( _max_blocks );
			_data._blkdata_rgb.resize( _max_blocks * MAX_BLK_RGB_SIZE );
			_data._blkdata_yuv.resize( _max_blocks * MAX_BLK_RGB_SIZE );
		} catch (...) {
			_error = PERROR;
			return false;
		}
	}

	return true;
}

//==================================================================
#if 0
//==================================================================
static _inline long float2int( float d )
{
	double dtemp = (((65536.0 * 65536.0 * 16) + (65536.0*.5)) * 65536.0) + d;
	return (*(long *)&dtemp) - 0x80000000;
}
#else
//==================================================================
static _inline int float2int( float in_val )
{
	int	a;
	int	*int_pointer = &a;

	__asm  fld  in_val
	__asm  mov  edx,int_pointer
	__asm  fistp dword ptr [edx];

	return a;
}
#endif

//==================================================================
static inline int max3( int a, int b, int c )
{
	int t = a > b ? a : b;
	return t > c ? t : c;
}

//==================================================================
static inline int min3( int a, int b, int c )
{
	int t = a < b ? a : b;
	return t < c ? t : c;
}

//==================================================================
static void unpackBitstream( const u_char *srcp, int src_size, int cnt, int bitsize, int *destp )
{
	Memfile memf( srcp, src_size );
	for (int i=0; i < cnt; ++i)
		*destp++ = memf.ReadBits( bitsize );
}
//==================================================================
static void unpackBitstream( const u_char *srcp, int src_size, int cnt, int bitsize, u_char *destp )
{
	Memfile memf( srcp, src_size );
	for (int i=0; i < cnt; ++i)
		*destp++ = memf.ReadBits( bitsize );
}

//==================================================================
static void fillFlatBlock( u_char *desp, u_char src_r, u_char src_g, u_char src_b )
{
	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
#ifdef DISP_FLAT_BLOCKS
			desp[0] = 255;
			desp[1] = 0;
			desp[2] = 255;
#else
			desp[0] = src_r;
			desp[1] = src_g;
			desp[2] = src_b;
#endif
			desp += 3;
		}
	}
}

//==================================================================
void ScreenPacker::BeginPack()
{
	_data._blkdata_head_file.SeekFromStart(0);
	_block_cnt = 0;
	_error = POK;

	_cur_frame += 1;
	if ( _cur_frame == 0 )
		_cur_frame = 1;	// 0 is a special case for never sent ! (useful ? maybe)

	_data._blkdata_bits_file.SeekFromStart(0);
	_lzwpacker.Reset( &_data._blkdata_bits_file );
}

//==================================================================
void ScreenPacker::EndPack()
{
	_lzwpacker.EndData();
	_data._blkdata_bits_file.WriteAlignByte();

	_data._blkdata_head_file.WriteAlignByte();

	u_int size_head = _data._blkdata_head_file.GetCurPos();
	u_int size_bits = _data._blkdata_bits_file.GetCurPos();

	PSYS_DEBUG_PRINTF( "total size = %i (heads: %i  bits: %i)\n", size_head + size_bits, size_head, size_bits );
}

//==================================================================
struct PalEntry
{
	u_short	rgb;
	u_short	pad;
	int		cnt;

	PalEntry()
	{
		rgb = 0;
		pad = 0;
		cnt = 0;
	}
};

//==================================================================
static void swap( int &a, int &b )
{
	int	t = a;
	a = b;
	b = t;
}
//==================================================================
static void swap( u_short &a, u_short &b )
{
	u_short	t = a;
	a = b;
	b = t;
}

//==================================================================
//==================================================================
class Palette
{
public:
	int			_n;
	PalEntry	_entries[16];
public:
	Palette()
	{
		_n = 0;
	}

	//==================================================================
	//bool IsSameColor()
	//{
	//}

	u_short GetEntryRGB( int i ) const
	{
		PSYS_ASSERT( i >= 0 && i < 16 );
		return _entries[i].rgb;
	}

	//==================================================================
	int FindAddColor( int r, int g, int b )
	{
		u_short rgb = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);

		for (int i=0; i < _n; ++i)
		{
			if ( rgb == _entries[i].rgb )
			{
				_entries[ i ].cnt += 1;
				return i;
			}
		}

		PSYS_ASSERT( _n < 16 );
		_entries[ _n ].rgb = rgb;
		_entries[ _n ].cnt = 1;

		return _n++;
	}

	//==================================================================
	u_int	entriesDiff( int a, int b ) const
	{
		u_int	a_rgb = _entries[a].rgb;
		u_int	b_rgb = _entries[b].rgb;
		int	dr =  (a_rgb >> 10) - (b_rgb >> 10);
		int	dg = ((a_rgb >> 5) & 31) - ((b_rgb >> 5) & 15);
		int	db = ((a_rgb >> 0) & 31) - ((b_rgb >> 5) & 31);

		return dr*dr + dg*dg + db*db;
	}

	//==================================================================
	void swapEntries( int a, int b )
	{
		swap( _entries[a].rgb, _entries[b].rgb );
		swap( _entries[a].cnt, _entries[b].cnt );
	}

	//==================================================================
	void sortByUsage( int remap[16] )
	{
		for (int i=0; i < _n; ++i)
		{
			int	max_cnt = _entries[ i ].cnt;
			int	max_idx = i;

			for (int j=i+1; j < _n; ++j)
			{
				if ( _entries[j].cnt > max_cnt )
				{
					max_cnt = _entries[j].cnt;
					max_idx = j;
				}
			}

			swapEntries( i, max_idx );
			swap( remap[i], remap[max_idx] );
		}
	}

	//==================================================================
	void RemoveColors( int remap[16], int n_keep )
	{
		int	inv_sort_map[16];

		for (int i=0; i < 16; ++i)
			inv_sort_map[i] = i;

		if ( n_keep >= _n )
		{
			for (int i=0; i < n_keep; ++i)
				remap[i] = i;
			return;
		}

		sortByUsage( inv_sort_map );

		int	move_map[16];
		for (int i=0; i < _n; ++i)
			move_map[i] = i;

		for (int i=n_keep; i < _n; ++i)
		{
			int		best = 0;
			u_int	best_val = entriesDiff( i, 0 );
			for (int j=1; j < n_keep; ++j)
			{
				u_int val = entriesDiff( i, j );
				if ( val < best_val )
				{
					best = j;
					best_val = val;
				}
			}

			move_map[i] = best;
		}

		for (int i=0; i < _n; ++i)
			remap[ inv_sort_map[ i ] ] = move_map[ i ];
	}
};

//==================================================================
static void packSubblk( u_short des_cols[PAK_PAL_DIM],
					    u_int &des_mod, const u_char *src_rgbp, int pitch )
{
	Palette	palette;
	u_char	map[4][4];

	const u_char	*src_rgbp2 = src_rgbp;

	for (int y=0; y < 4; ++y)
	{
		const u_char	*src_rgbp3 = src_rgbp2;
		src_rgbp2 += pitch;

		for (int x=0; x < 4; ++x, src_rgbp3 += 3)
		{
			map[y][x] = palette.FindAddColor( src_rgbp3[0], src_rgbp3[1], src_rgbp3[2] );
		}
	}

	int	palidx_remap[16];
	palette.RemoveColors( palidx_remap, PAK_PAL_DIM );

	des_mod = 0;
	for (int i=0; i < 16; ++i)
	{
		int	sidx = ((u_char *)map)[i];
		int	didx = palidx_remap[ sidx ];

		PSYS_ASSERT( sidx >= 0 && sidx < 16 );
		PSYS_ASSERT( didx >= 0 && didx < PAK_PAL_DIM );

		des_mod |= didx << (i*2);
	}

	for (int i=0; i < PAK_PAL_DIM; ++i)
		des_cols[i] = palette.GetEntryRGB( i );
}

//==================================================================
static void unpackSubblk( u_char *des_rgbp, int pitch, u_short src_cols[PAK_PAL_DIM], u_int src_mod )
{
	u_char	cols[PAK_PAL_DIM][3];
	
	for (int i=0; i < PAK_PAL_DIM; ++i)
	{
		u_int	r, g, b;
		
		r = ((src_cols[i] >> 10) & 31) << 3; r += 7 & ~((r-1) >> 31);
		g = ((src_cols[i] >>  5) & 31) << 3; g += 7 & ~((g-1) >> 31);
		b = ((src_cols[i] >>  0) & 31) << 3; b += 7 & ~((b-1) >> 31);

		cols[i][0] = r;
		cols[i][1] = g;
		cols[i][2] = b;
	}

	for (int y=4; y > 0; --y)
	{
		for (int x=4; x > 0; --x)
		{
			u_char	*colp = cols[ src_mod & 3 ];
			src_mod >>= 1*2;

			des_rgbp[0] = colp[0];
			des_rgbp[1] = colp[1];
			des_rgbp[2] = colp[2];
			des_rgbp += 3;
		}
		des_rgbp += pitch - 3*4;
	}
}
/*
//==================================================================
static void blockRGB_to_PAK( u_char *pak_blockp, const u_char *src_rgbp )
{
	Memfile	memf( pak_blockp, MAX_BLK_PAK_SIZE );

	const u_char	*src_rgbp2 = src_rgbp;

	for (int y=0; y < 8; ++y)
	{
		for (int x=0; x < 8; ++x, src_rgbp2 += 3*4)
		{
			u_short cols_rgb[PAK_PAL_DIM];
			u_int	mod_bits;

			packSubblk( cols_rgb, mod_bits, src_rgbp2, ScreenPackerData::BLOCK_WD*3 );

			for (int i=0; i < PAK_PAL_DIM; ++i)
				memf.WriteUShort( cols_rgb[i] );

			memf.WriteUInt( mod_bits );
		}
		src_rgbp2 += MAX_BLK_RGB_LINESIZE * (4-1);
	}
}

//==================================================================
static void blockPAK_to_RGB( u_char *des_rgbp, const u_char *src_pakp )
{
	Memfile	memf( src_pakp, MAX_BLK_PAK_SIZE );

	u_char	*des_rgbp2 = des_rgbp;

	for (int y=0; y < 8; ++y)
	{
		for (int x=0; x < 8; ++x, des_rgbp2 += 3*4)
		{
			u_short cols_rgb[PAK_PAL_DIM];
			u_int	mod_bits;

			for (int i=0; i < PAK_PAL_DIM; ++i)
				cols_rgb[i] = memf.ReadUShort();

			mod_bits = memf.ReadUInt();

			unpackSubblk( des_rgbp2, ScreenPackerData::BLOCK_WD*3, cols_rgb, mod_bits );
		}
		des_rgbp2 += MAX_BLK_RGB_LINESIZE * (4-1);
	}
}
*/
/*
//==================================================================
static inline int packSign( int t )
{
	return ( t < 0 ) ? ((-t << 1) | 1) : (t << 1);
}
//==================================================================
static inline int unpackSign( int t )
{
	return ( t & 1 ) ? (-(t >> 1)) : (t >> 1);
}
*/
/*
//==================================================================
static inline int upkBits( int val, int bitsize, int idx )
{
	u_int	mask = (1 << bitsize) - 1;
	return ((val >> (bitsize*idx)) & mask) << (8 - bitsize);
}

//==================================================================
static inline int upkBitsSign( int val, int bitsize, int idx )
{
	u_int	mask = (1 << bitsize) - 1;
	return (int)(((val >> (bitsize*idx)) & mask) << (32 - bitsize)) >> (32 - 8);
}
// 01234567012345670123456701234567
//                               aa
//                               aa

//==================================================================
static inline int pkBits( int val, int bitsize, int idx )
{
	u_int	mask = (1 << bitsize) - 1;
	return ((val >> (8 - bitsize)) & mask) << (bitsize*idx);
}


//==================================================================
//==================================================================
static u_int rshift_packsign( int val, int cnt, u_int val_mask )
{
	u_int	sign_mask = val_mask + 1;
	u_int	sign_fill = (int)val >> 31;

	if ( val < -((1 << cnt) - 1) )
		val = 0;
	else
		val >>= cnt;

	return (val & val_mask) | (sign_fill & sign_mask);
}

//==================================================================
static u_int rshift_pack( int val, int cnt, u_int val_mask )
{
	val >>= cnt;
	return val & val_mask;
}

//==================================================================
static int unpacksign_lshift( u_int val, int cnt, u_int val_mask )
{
	u_int	sign_mask = val_mask + 1;
//	u_int	sign_fill = (int)val >> 31;

	if ( val & sign_mask )
		return -(int)(val & ~sign_mask) << cnt;
	else
		return val << cnt;
}
*/

//==================================================================
static __inline u_int pack_sign8( int t )
{
//	if ( t < 0 )
//		return -t | 0x80;
//	else
//		return t;

	return t & 255;
}

//==================================================================
static __inline int unpack_sign8( u_int t )
{
//	if ( t & 0x80 )
//	{
//		if ( t == 0x80 )
//			return -128;
//		else
//			return -(int)(t & ~0x80);
//	}
//	else
//		return t;

	return (int)((int)t << 24) >> 24;
}

//==================================================================
enum {
	COMPLEXITY_FLAT,
	COMPLEXITY_TEXT,
	COMPLEXITY_IMAGE
};
//==================================================================
static inline void RGBtoYUV( const u_char *src_rgbp, int des_yuv[3] )
{
	int r = src_rgbp[0];
	int g = src_rgbp[1];
	int b = src_rgbp[2];
/*
	des_yuv[0] = (r + 2*g + b) / 4;
	des_yuv[1] = r - g;
	des_yuv[2] = b - g;
*/
	des_yuv[0] = g;
	des_yuv[1] = r - g;
	des_yuv[2] = b - g;

	PSYS_ASSERT( des_yuv[0] >= 0 && des_yuv[0] <= 255 );
	PSYS_ASSERT( des_yuv[1] >= -255 && des_yuv[1] <= 255 );
	PSYS_ASSERT( des_yuv[2] >= -255 && des_yuv[2] <= 255 );
}

//==================================================================
static int rshift_sign( int val, int cnt )
{
	u_int	sign_fill = val >> 31;

	val = ((val ^ sign_fill) >> cnt) ^ sign_fill;

	return val;
}

//==================================================================
static inline void YUVtoRGB( int y, int u, int v, u_char *des_rgbp )
{
	int	g = y - rshift_sign( u + v, 2 );
	int	r = u + g;
	int	b = v + g;
/*
	if ( (u_int)r > 255 )
		r = ~(r >> 31) & 255;

	if ( (u_int)g > 255 )
		g = ~(g >> 31) & 255;

	if ( (u_int)b > 255 )
		b = ~(b >> 31) & 255;
*/
//	r = y + u;//r;
//	g = y;//g;
//	b = y + v;//b;

	if ( (u_int)r > 255 )
		r = ~(r >> 31) & 255;

	if ( (u_int)g > 255 )
		g = ~(g >> 31) & 255;

	if ( (u_int)b > 255 )
		b = ~(b >> 31) & 255;
/*
	PCLAMP( r, 0, 255 );
	PCLAMP( g, 0, 255 );
	PCLAMP( b, 0, 255 );
*/
	des_rgbp[0] = r;
	des_rgbp[1] = g;
	des_rgbp[2] = b;
}

//==================================================================
static int convertBlockToYUV( const u_char *const srcp,
							  u_char out_y[MAX_BLK_PIXELS],
							  u_char out_u[MAX_BLK_PIXELS],
							  u_char out_v[MAX_BLK_PIXELS] )
{
	u_char const	*srcendp = srcp + MAX_BLK_RGB_SIZE;

	{
		u_char	*out_yp = out_y;

		for (const u_char *srcp2 = srcp; srcp2 < srcendp; srcp2 += 3)
		{
			/*
			int		tmp_yuv[3];
			u_char	tmp_rgb[3];

			RGBtoYUV( srcp2, tmp_yuv );
			for (int i=0; i < 3; ++i)
			{
				tmp_yuv[i] = unpack_sign8( (u_char)pack_sign8( tmp_yuv[i] ) );
			}
			YUVtoRGB( tmp_yuv[0], tmp_yuv[1], tmp_yuv[2], tmp_rgb );

			PSYS_ASSERT( tmp_rgb[0] == srcp2[0] );
			PSYS_ASSERT( tmp_rgb[1] == srcp2[1] );
			PSYS_ASSERT( tmp_rgb[2] == srcp2[2] );
*/
			int	r = srcp2[0];
			int	g = srcp2[1];
			int	b = srcp2[2];
			*out_yp++ = (r + 2*g + b) / 4;
		}
	}

	int	complexity = 0;

	{
		u_char	*out_yp = out_y;
		u_char const *out_yp_end = out_y + ScreenPackerData::BLOCK_N_PIX;

		for (int y=ScreenPackerData::BLOCK_HE-1; y > 0; --y)
		{
			for (int x=ScreenPackerData::BLOCK_WD-1; x > 0; --x)
			{
				complexity +=	(0 != ((out_yp[0] ^ out_yp[1]) |
									   (out_yp[0] ^ out_yp[ScreenPackerData::BLOCK_WD])) );

				++out_yp;
			}
			++out_yp;
		}
	}
/*
	for (int i=-128; i < 128; ++i)
	{
		PSYS_ASSERT( unpack_sign8( pack_sign8( i ) ) == i );
	}
*/

	// if flat, no need to calculate U and V
	if ( complexity == 0 )
		return COMPLEXITY_FLAT;

	u_char	*out_up = out_u;
	u_char	*out_vp = out_v;
	for (const u_char *srcp2 = srcp; srcp2 < srcendp; )
	{
		*out_up++ = pack_sign8( rshift_sign( (int)srcp2[0] - (int)srcp2[1], 1 ) );
		*out_vp++ = pack_sign8( rshift_sign( (int)srcp2[2] - (int)srcp2[1], 1 ) );
		srcp2 += 3;
	}

	if ( complexity <= ScreenPackerData::BLOCK_N_PIX/16 )
		return COMPLEXITY_TEXT;
	else
		return COMPLEXITY_IMAGE;
}

//==================================================================
//==================================================================
static void blockYUV_to_PAK( u_char *pak_blockp,
							 const u_char *src_yp,
							 const u_char *src_up,
							 const u_char *src_vp )
{
/*
	u_char *pak_block_yp = pak_blockp + MAX_BLK_PAK_OFF_Y;
	s_char *pak_block_up = (s_char *)pak_blockp + MAX_BLK_PAK_OFF_U;
	s_char *pak_block_vp = (s_char *)pak_blockp + MAX_BLK_PAK_OFF_V;
*/
	Memfile	pack_y_mf( pak_blockp + MAX_BLK_PAK_OFF_Y, MAX_BLK_PAK_SIZE_Y );
	Memfile	pack_u_mf( pak_blockp + MAX_BLK_PAK_OFF_U, MAX_BLK_PAK_SIZE_U );
	Memfile	pack_v_mf( pak_blockp + MAX_BLK_PAK_OFF_V, MAX_BLK_PAK_SIZE_V );

	const u_char *src_yp2 = src_yp;
	const u_char *src_up2 = src_up;
	const u_char *src_vp2 = src_vp;

	const u_char *const src_yp_end = src_yp + MAX_BLK_PIXELS;
	for (; src_yp2 != src_yp_end; )
	{
		//pack_y_mf.WriteBits( (*src_yp2++ >> 8-BLK_BITS_Y) & BLK_BITS_Y_MASK, BLK_BITS_Y );
		//pack_u_mf.WriteBits( (*src_up2++ >> 8-BLK_BITS_U) & BLK_BITS_U_MASK, BLK_BITS_U );
		//pack_v_mf.WriteBits( (*src_vp2++ >> 8-BLK_BITS_V) & BLK_BITS_V_MASK, BLK_BITS_V );

		pack_y_mf.WriteBits( (*src_yp2++ >> 8-BLK_BITS_Y) & BLK_BITS_Y_MASK, BLK_BITS_Y );
		pack_u_mf.WriteBits( (*src_up2++ >> 8-BLK_BITS_U) & BLK_BITS_U_MASK, BLK_BITS_U );
		pack_v_mf.WriteBits( (*src_vp2++ >> 8-BLK_BITS_V) & BLK_BITS_V_MASK, BLK_BITS_V );
	}

	pack_y_mf.WriteAlignByte();
	pack_u_mf.WriteAlignByte();
	pack_v_mf.WriteAlignByte();
}

//==================================================================
static void blockPAK_to_RGB( u_char *des_rgbp, const u_char *src_pakp )
{
	Memfile	pack_y_mf( src_pakp + MAX_BLK_PAK_OFF_Y, MAX_BLK_PAK_SIZE_Y );
	Memfile	pack_u_mf( src_pakp + MAX_BLK_PAK_OFF_U, MAX_BLK_PAK_SIZE_U );
	Memfile	pack_v_mf( src_pakp + MAX_BLK_PAK_OFF_V, MAX_BLK_PAK_SIZE_V );

	u_char const	*des_rgbp_end = des_rgbp + MAX_BLK_RGB_SIZE;
	for (u_char *des_rgbp2 = des_rgbp; des_rgbp != des_rgbp_end; des_rgbp += 3)
	{
		YUVtoRGB( (u_char)(pack_y_mf.ReadBits( BLK_BITS_Y ) << 8-BLK_BITS_Y),
				  unpack_sign8( pack_u_mf.ReadBits( BLK_BITS_U ) << 8-BLK_BITS_U ) << 1,
				  unpack_sign8( pack_v_mf.ReadBits( BLK_BITS_V ) << 8-BLK_BITS_V ) << 1,
				  des_rgbp );
	}
}

//==================================================================
bool ScreenPacker::IsBlockChanged( u_int new_checksum ) const
{
#ifdef FORCE_ALL_BLOCKS
	return true;

#else
	const BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];

	return bpworkp->_checksum != new_checksum;

#endif
}

//==================================================================
bool ScreenPacker::AddBlock( const void *block_datap, int size, u_int new_checksum )
{
	if ERR_FALSE( _block_cnt < _max_blocks )
	{
		_error = PERROR;
		return false;
	}

	BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];

	bpworkp->_checksum = new_checksum;
//	bpworkp->_sub_level_sent = 0;

	BlockPackHead	head;

	u_char	y_block[ MAX_BLK_PIXELS ];
	u_char	u_block[ MAX_BLK_PIXELS ];
	u_char	v_block[ MAX_BLK_PIXELS ];

	head._complexity = convertBlockToYUV( (const u_char *)block_datap, y_block, u_block, v_block );
	head._sub_type = 0;
	head._pad = 0;
	// at complexity 0, we can't rely on having the block converted
	if ( head._complexity == COMPLEXITY_FLAT )
	{
		_data._blkdata_head_file.WriteData( &head, sizeof(head) );

		bpworkp->_sub_level_sent = 4;

		u_char	adapted_rgb[3];

		//pixelRGB2YC2RGB( adapted_rgb, (const u_char *)block_datap );
		adapted_rgb[0] = ((const u_char *)block_datap)[0];
		adapted_rgb[1] = ((const u_char *)block_datap)[1];
		adapted_rgb[2] = ((const u_char *)block_datap)[2];

		_data._blkdata_head_file.WriteUChar( adapted_rgb[0] );
		_data._blkdata_head_file.WriteUChar( adapted_rgb[1] );
		_data._blkdata_head_file.WriteUChar( adapted_rgb[2] );
	}
	else
	{
		// YC BLOCK
		u_char	pak_block[ MAX_BLK_PAK_SIZE ];

		blockYUV_to_PAK( pak_block, y_block, u_block, v_block );

		head._sub_type = 0;
		_data._blkdata_head_file.WriteData( &head, sizeof(head) );

		bpworkp->_sub_level_sent = 4;

		_lzwpacker.PackData( &Memfile( pak_block, MAX_BLK_PAK_SIZE ) );
//		_lzwpacker.EndData();
/*
		if ERR_FALSE( LZW_PackCompress( &Memfile( pak_block, MAX_BLK_PAK_SIZE ), &_data._blkdata_file ) )
		{
			_error = PERROR;
			return false;
		}
*/
//		_data._blkdata_bits_file.WriteAlignByte();
	}

	_data._blocks_use_bitmap[ _block_cnt / 8 ] |= 1 << (_block_cnt & 7);
	++_block_cnt;

	return true;
}

//==================================================================
void ScreenPacker::ContinueBlock()
{
	BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];

	++bpworkp->_sub_level_sent;


	/*
	if ERR_FALSE( _block_cnt < _max_blocks )
	{
		_error = PERROR;
		return false;
	}

	BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];

	bpworkp->_checksum = new_checksum;
//	bpworkp->_sub_level_sent = 0;

	BlockPackHead	head;

	u_char	y_block[ MAX_BLK_PIXELS ];
	s_char	u_block[ MAX_BLK_PIXELS ];
	s_char	v_block[ MAX_BLK_PIXELS ];

	head._complexity = convertBlockToYUV( (const u_char *)block_datap, y_block, u_block, v_block );
	head._sub_type = 0;
	head._pad = 0;
	// at complexity 0, we can't rely on having the block converted
	if ( head._complexity == COMPLEXITY_FLAT )
	{
		_data._blkdata_file.WriteData( &head, sizeof(head) );

		bpworkp->_sub_level_sent = 4;

		u_char	adapted_rgb[3];

		//pixelRGB2YC2RGB( adapted_rgb, (const u_char *)block_datap );
		adapted_rgb[0] = ((const u_char *)block_datap)[0];
		adapted_rgb[1] = ((const u_char *)block_datap)[1];
		adapted_rgb[2] = ((const u_char *)block_datap)[2];

		_data._blkdata_file.WriteUChar( adapted_rgb[0] );
		_data._blkdata_file.WriteUChar( adapted_rgb[1] );
		_data._blkdata_file.WriteUChar( adapted_rgb[2] );
	}
	else
	{
		// YC BLOCK
		u_char	pak_block[ MAX_BLK_PAK_SIZE ];

		blockYUV_to_PAK( pak_block, y_block, u_block, v_block );

		head._sub_type = 0;
		_data._blkdata_file.WriteData( &head, sizeof(head) );

		bpworkp->_sub_level_sent = 4;

		if ERR_FALSE( LZW_PackCompress( &Memfile( pak_block, MAX_BLK_PAK_SIZE ), &_data._blkdata_file ) )
		{
			_error = PERROR;
			return false;
		}

		_data._blkdata_file.WriteAlignByte();
	}

	*/
	_data._blocks_use_bitmap[ _block_cnt / 8 ] |= 1 << (_block_cnt & 7);
	++_block_cnt;
}

//==================================================================
bool SPAKMM::SkipBlock()
{
	if ERR_FALSE( _block_cnt < _max_blocks )
	{
		_error = PERROR;
		return false;
	}

	_data._blocks_use_bitmap[ _block_cnt / 8 ] &= ~(1 << (_block_cnt & 7));
	++_block_cnt;

	return true;
}

//==================================================================
///
//==================================================================
void ScreenUnpacker::BeginParse()
{
	_data._blkdata_head_file.SeekFromStart(0);
	_data._blkdata_bits_file.SeekFromStart(0);
	_block_cnt = 0;
	_error = POK;

	_lzwunpacker.Reset( &_data._blkdata_bits_file );
}

//==================================================================
void ScreenUnpacker::EndParse()
{
}

//==================================================================
bool ScreenUnpacker::ParseNextBlock( void *out_block_datap, int &blk_px, int &blk_py )
{
	if ( _block_cnt >= _max_blocks )
		return false;	// end
/*
	if ERR_FALSE( _block_cnt < _max_blocks )
	{
		_error = PERROR;
		return false;
	}
*/
	for ( ; _block_cnt < _max_blocks; ++_block_cnt )
	{
		if ( _data._blocks_use_bitmap[ _block_cnt / 8 ] & (1 << (_block_cnt & 7)) )
		{
			BlockPackHead	head;

			_data._blkdata_head_file.ReadData( &head, sizeof(head) );

			u_char	*local_destp = &_data._blkdata_rgb[ _block_cnt * MAX_BLK_RGB_SIZE ];

			if ( head._complexity == COMPLEXITY_FLAT )
			{
				u_char	r = _data._blkdata_head_file.ReadUChar();
				u_char	g = _data._blkdata_head_file.ReadUChar();
				u_char	b = _data._blkdata_head_file.ReadUChar();
				fillFlatBlock( local_destp, r, g, b );
			}
			else
			{
				u_char	pak_block[ MAX_BLK_PAK_SIZE ];
				Memfile	pak_block_memf( pak_block, MAX_BLK_PAK_SIZE );

				_lzwunpacker.UnpackData( &pak_block_memf, MAX_BLK_PAK_SIZE );
//				_lzwunpacker.SeekEnd();
//				PSYS_ASSERT( _lzwunpacker.IsCompleted() );
				/*
				if ERR_FALSE( LZW_UnpackExpand( &_data._blkdata_file, &pak_block_memf ) )
				{
					_error = PERROR;
					return false;
				}
				*/
//				_data._blkdata_bits_file.ReadAlignByte();

				blockPAK_to_RGB( (u_char *)local_destp, pak_block );
			}

			memcpy( out_block_datap, local_destp, MAX_BLK_RGB_SIZE );

			blk_py = (_block_cnt / _data.GetBlocksPerRow()) * ScreenPackerData::BLOCK_WD;
			blk_px = (_block_cnt % _data.GetBlocksPerRow()) * ScreenPackerData::BLOCK_HE;

			++_block_cnt;
			return true;
		}
	}

	// end
	blk_py = 0;
	blk_px = 0;
	return false;
}
