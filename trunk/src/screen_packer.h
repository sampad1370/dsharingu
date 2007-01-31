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

#ifndef SCREEN_PACKER_H
#define SCREEN_PACKER_H

#include "psys.h"
#include "memfile.h"
#include "lzw_packer.h"
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
	u_char	_complexity	: 2;
	u_char	_sub_type	: 2;
	u_char	_pad		: 4;
};

//==================================================================
struct ScreenPackerData
{
	static const int	BLOCK_WD	= 32;
	static const int	BLOCK_HE	= 32;
	static const int	BLOCK_N_PIX	= BLOCK_WD * BLOCK_HE;

	int					_w, _h;

	PArray<BlockPackWork>		_blocks_pack_work;
	PArray<u_char>				_blocks_use_bitmap;
	PUtils::Memfile				_blkdata_head_file;
	PUtils::Memfile				_blkdata_bits_file;
	PArray<u_char>				_blkdata_rgb;
	PArray<u_char>				_blkdata_yuv;

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
	PUtils::Memfile			&GetDataHead()		{	return _blkdata_head_file;	}
	PUtils::Memfile			&GetDataBits()		{	return _blkdata_bits_file;	}
};

//==================================================================
//==================================================================
class SPAKMM
{
public:
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

	bool	SkipBlock();
private:
};

//==================================================================
///
//==================================================================
class ScreenUnpacker : public SPAKMM
{
	LZWUnpacker			_lzwunpacker;

public:
	//==================================================================
	ScreenUnpacker() :
		SPAKMM()
	{
	}

	void	Reset()
	{
		SPAKMM::Reset();
	}

	void	BeginParse();
	void	EndParse();
	bool	ParseNextBlock( void *out_block_datap, int &blk_px, int &blk_py );
private:
};

//==================================================================
///
//==================================================================
class ScreenPacker : public SPAKMM
{
	LZWPacker		_lzwpacker;

public:
	//==================================================================
	ScreenPacker() :
		SPAKMM()
	{
	}

	void	Reset()
	{
		SPAKMM::Reset();
	}

	void	BeginPack();
	bool	IsBlockChanged( u_int new_checksum ) const;
	bool	IsBlockCompleted() const
	{
		const BlockPackWork	*bpworkp = &_data._blocks_pack_work[ _block_cnt ];
		return bpworkp->_sub_level_sent >= 4;
	}
	bool	AddBlock( const void *block_datap, int size, u_int new_checksum );
	void	ContinueBlock();
	void	EndPack();
private:
};

#endif