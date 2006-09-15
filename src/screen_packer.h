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

#ifndef SCREEN_PACKER_H
#define SCREEN_PACKER_H

#include "psys.h"
#include "memfile.h"
#include <vector>

//==================================================================
struct BlockPackWork
{
	u_int	_work_flags;
	u_int	_checksum;
	int		_frame_sent;
	u_char	_sub_level_sent;

	BlockPackWork()
	{
		Reset();
	}

	void Reset()
	{
		_work_flags		= 0;
		_checksum		= 0;
		_frame_sent		= 0;
		_sub_level_sent	= 0;
	}
};

//==================================================================
enum BLOCKPACKHEAD_FLAGS
{
	BLKPKHEAD_FLG_FLAT		= 1,
	BLKPKHEAD_FLG_SUBMASK	= 1,
};
//==================================================================
struct BlockPackHead
{
	u_char	_is_flat	: 1;
	u_char	_sub_type	: 2;
	u_char	_pad		: 5;
};

//==================================================================
struct ScreenPackerData
{
	static const int	BLOCK_WD	= 32;
	static const int	BLOCK_HE	= 32;

	int					_w, _h;

	std::vector<BlockPackWork>	_blocks_pack_work;
	PArray<u_char>				_blocks_use_bitmap;
	PUtils::Memfile				_blkdata_file;
	PArray<u_char>				_blkdata_rgb;

	//==================================================================
	static int CalcMaxBlocks( int w, int h )
	{
		return ((w + BLOCK_WD-1) / BLOCK_WD) * ((h + BLOCK_HE-1) / BLOCK_HE);
	}

	//==================================================================
	int GetBlocksPerRow()
	{
		return ((_w + BLOCK_WD-1) / BLOCK_WD);
	}

	//==================================================================
	int						GetWidth()		const {	return _w;	}
	int						GetHeight()		const {	return _h;	}
	PArray<u_char>			&GetUseBitmap()		{	return _blocks_use_bitmap;	}
	PUtils::Memfile			&GetData()			{	return _blkdata_file;	}
};

inline int					SPAKD_GetWidth( const ScreenPackerData *T )		{	return T->_w; }
inline int					SPAKD_GetHeight( const ScreenPackerData *T )	{	return T->_h; }
inline const PArray<u_char>	&SPAKD_GetUseBitmap( const ScreenPackerData *T ){	return T->_blocks_use_bitmap; }
//inline const PArray<u_char>	&SPAKD_GetData( const ScreenPackerData *T )		{	return T->_blocks_data; }

//==================================================================
//==================================================================
struct SPAKMM
{
	ScreenPackerData	_data;

	int					_max_blocks;
	int					_block_cnt;

	int					_cur_frame;

	PError				_error;
	PError				GetError() { PError tmp = _error; _error = POK; return tmp; }

	//==================================================================
	SPAKMM()
	{
		_data._w = 0;
		_data._h = 0;
		_block_cnt = 0;
		_cur_frame = 0;
		_max_blocks = 0;
		_error = POK;
	}

	void	Reset();

	bool	SetScreenSize( int w, int h );
	void	BeginPack();
	bool	AddBlock( const void *block_datap, int size=0 );
	void	EndPack();

	void	BeginParse();
	void	EndParse();
	bool	ParseNextBlock( void *out_block_datap, int &blk_px, int &blk_py );

private:
	bool	SkipBlock();
};

#endif