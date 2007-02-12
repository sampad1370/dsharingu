//==================================================================
//	Copyright (C) 2007  Davide Pasca
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

#ifndef SCREEN_HAAR_COMPR_H
#define SCREEN_HAAR_COMPR_H

#include "psys.h"

//==================================================================
class ScreenHaarComprPack
{
public:
	static const int	BLOCK_DIM = 32;

	u_char		_out_data[BLOCK_DIM*BLOCK_DIM * sizeof(short)];

public:
	ScreenHaarComprPack()
	{
	}

	void PackData( const u_char *in_blockp, u_char * &out_datap, u_int &out_data_len );
};

//==================================================================
class ScreenHaarComprUnpack
{
public:
	static const int	BLOCK_DIM = 32;

	short		_tmp_haar[BLOCK_DIM*BLOCK_DIM * sizeof(short)];
	u_char		_out_data[BLOCK_DIM*BLOCK_DIM * sizeof(short)];

public:
	ScreenHaarComprUnpack()
	{
	}

	void UnpackData( const u_char *in_datap, u_int in_data_len, u_char *out_blockp );
};

#endif