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
///
///
///
//==================================================================

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "psys.h"

//==================================================================
struct BitStream
{
	int		bits_per_value;
	u_int	max_value;
	int		cur_idx;
	u_char	*datap;
	int		data_max_size;
};

//==================================================================
void	BitStream_Init( BitStream *T, u_int max_value, void *datap, int data_max_size );
void	BitStream_Init( BitStream *T, u_int max_value, const void *datap, int data_max_size );

void	BitStream_WriteValue( BitStream *T, u_int value );
u_int	BitStream_ReadValue( BitStream *T );

void	BitStream_WriteEnd( BitStream *T );
u_char	*BitStream_GetEndPtr( BitStream *T );

#endif
