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

static const int	MAX_BLK_PAK_SIZE = (((MAX_SUBBLK_PIXELS + 7) >> 3)*2 + sizeof(u_short)*PAK_PAL_DIM) * 8 * 8;

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

	for (int i=0; i < _data._blocks_pack_work.size(); ++i)
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
		} catch (...) {
			_error = PERROR;
			return false;
		}
	}

	return true;
}

//==================================================================
void SPAKMM::BeginParse()
{
	_data._blkdata_file.SeekFromStart(0);
	_block_cnt = 0;
	_error = POK;
}

//==================================================================
void SPAKMM::EndParse()
{
}

//==================================================================
static __inline int pack_sign( int t )
{
	return ( t < 0 ) ? ((-t << 1) | 1) : (t << 1);
}
//==================================================================
static __inline int unpack_sign( int t )
{
	return ( t & 1 ) ? (-(t >> 1)) : (t >> 1);
}

//==================================================================
static bool blockIsFlat( const u_char *srcp )
{
	u_int	*srcp2   = (u_int *)(srcp);
	u_int	*srcendp = (u_int *)(srcp + MAX_BLK_RGB_SIZE);

	PSYS_ASSERT( ((ScreenPackerData::BLOCK_WD*3) % 4) == 0 );

	u_int	rgbr = *(u_int *)(srcp);
	u_int	gbrg = *(u_int *)(srcp+1);
	u_int	brgb = *(u_int *)(srcp+2);

	while ( srcp2 < srcendp )
	{
		if ( *srcp2++ != rgbr )	return false;
		if ( *srcp2++ != gbrg )	return false;
		if ( *srcp2++ != brgb )	return false;
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
void SPAKMM::BeginPack()
{
	_data._blkdata_file.SeekFromStart(0);
	_block_cnt = 0;
	_error = POK;

	_cur_frame += 1;
	if ( _cur_frame == 0 )
		_cur_frame = 1;	// 0 is a special case for never sent ! (useful ? maybe)
}

//==================================================================
void SPAKMM::EndPack()
{
	_data._blkdata_file.WriteAlignByte();

	u_int size = _data._blkdata_file.GetCurPos();

	PSYS_DEBUG_PRINTF( "total size = %i\n", size );
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
		int	dr =  (a_rgb>>10) - (b_rgb>>10);
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

//==================================================================
bool SPAKMM::AddBlock( const void *block_datap, int size )
{
	if ERR_FALSE( _block_cnt < _max_blocks )
	{
		_error = PERROR;
		return false;
	}

	bool is_flat = blockIsFlat( (const u_char *)block_datap );

	BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];

	u_int new_checksum = crc32( 0, (const u_char *)block_datap, MAX_BLK_RGB_SIZE );
#ifndef FORCE_ALL_BLOCKS
	if ( bpworkp->_checksum == new_checksum )
	{
		if ( bpworkp->_sub_level_sent == 4 )
		{
			SkipBlock();
			return false;
		}
	}
	else
#endif
	{
		bpworkp->_checksum = new_checksum;
		bpworkp->_sub_level_sent = 0;
	}

	BlockPackHead	head;

	head._is_flat = is_flat;
	head._sub_type = 0;
	head._pad = 0;

	if ( head._is_flat )
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

		blockRGB_to_PAK( pak_block, (const u_char *)block_datap );

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

	_data._blocks_use_bitmap[ _block_cnt / 8 ] |= 1 << (_block_cnt & 7);
	++_block_cnt;

	return true;
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
bool SPAKMM::ParseNextBlock( void *out_block_datap, int &blk_px, int &blk_py )
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

			_data._blkdata_file.ReadData( &head, sizeof(head) );

			u_char	*local_destp = &_data._blkdata_rgb[ _block_cnt * MAX_BLK_RGB_SIZE ];

			if ( head._is_flat )
			{
				u_char	r = _data._blkdata_file.ReadUChar();
				u_char	g = _data._blkdata_file.ReadUChar();
				u_char	b = _data._blkdata_file.ReadUChar();
				fillFlatBlock( local_destp, r, g, b );
			}
			else
			{
				u_char	pak_block[ MAX_BLK_PAK_SIZE ];
				Memfile	pak_block_memf( pak_block, MAX_BLK_PAK_SIZE );

				if ERR_FALSE( LZW_UnpackExpand( &_data._blkdata_file, &pak_block_memf ) )
				{
					_error = PERROR;
					return false;
				}
				_data._blkdata_file.ReadAlignByte();

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
