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

#include "psys.h"
#include "bitstream.h"

//==================================================================
#ifdef _DEBUG
//#define DEBUG_VERIFY_IO
#endif

//==================================================================
static int required_bits( u_int value )
{
	for (int i=0; i < 32; ++i)
	{
		if ( value == 0 )
			return i;

		value >>= 1;
	}

	PASSERT( 0 );
	return 31;
}

//==================================================================
void BitStream_Init( BitStream *T, u_int max_value, void *datap, int data_max_size )
{
	T->max_value		= max_value;
	T->bits_per_value	= required_bits( max_value );
	T->cur_idx			= 0;
	T->datap			= (u_char *)datap;
	T->data_max_size	= data_max_size;

	PASSERT( T->bits_per_value >= 1 );
}

//==================================================================
void BitStream_Init( BitStream *T, u_int max_value, const void *datap, int data_max_size )
{
	BitStream_Init( T, max_value, (void *)datap, data_max_size );
}

//==================================================================
void BitStream_WriteValue( BitStream *T, u_int value )
{
	PASSERT( value <= T->max_value );

/*
value: abc

   (value & 1) | dest_stream| idx
i |			   |			|
---------------|------------|----
0 | c		   | -----c--	| 2
1 | b		   | -----cb-	| 1
2 | a		   | -----cba	| 0
*/
#ifdef DEBUG_VERIFY_IO	// verify that all works well 8)
	u_int	original_value = value;
#endif
	int	idx = T->cur_idx + T->bits_per_value;

	// make sure that we are not trying to index outside the maximum data size
	if NOT( idx / 8 < T->data_max_size )
	{
		PASSERT( 0 );
		return;
	}

	for (int i=T->bits_per_value; i > 0; --i)
	{
		--idx;

		int		idx_mod		= idx & 7;
		u_char	*datap_div	= T->datap + idx / 8;

		*datap_div &= ~(1 << idx_mod);
		*datap_div |= (value & 1) << idx_mod;

		value >>= 1;
	}
	
	T->cur_idx += T->bits_per_value;

#ifdef DEBUG_VERIFY_IO	// verify that all works well 8)
	{
	BitStream	verify_stream = *T;
	
		verify_stream.cur_idx -= verify_stream.bits_per_value;
		u_int read_back_value = BitStream_ReadValue( &verify_stream );

		PASSERT( read_back_value == original_value );
	}
#endif
}

//==================================================================
void BitStream_WriteEnd( BitStream *T )
{
	if NOT( (T->cur_idx+7) / 8 <= T->data_max_size )
	{
		PASSERT( 0 );
		return;
	}

	u_char	*datap_div	= T->datap + T->cur_idx / 8;

	for (int i=T->cur_idx; i & 7; ++i)
		*datap_div &= ~(1 << (i & 7));

	T->cur_idx = (T->cur_idx + 7) & ~7;
}

//==================================================================
u_char	*BitStream_GetEndPtr( BitStream *T )
{
	return T->datap + (T->cur_idx+7) / 8;
}

//==================================================================
u_int BitStream_ReadValue( BitStream *T )
{
/*
value: abc

   value	| stream	| idx
i |			|			|
------------|-----------|----
0 | -------a| -----cba	| 0
1 | ------ab| -----cba	| 1
2 | -----abc| -----cba	| 2
*/
	u_int	out_value = 0;

	int	idx = T->cur_idx;

	// make sure that we are not trying to index outside the maximum data size
	if NOT( (idx + T->bits_per_value-1) / 8 < T->data_max_size )
	{
		PASSERT( 0 );
		return 0;
	}

	for (int i=T->bits_per_value; i > 0; --i)
	{
		int		idx_mod		= idx & 7;
		u_char	*datap_div	= T->datap + idx / 8;

		out_value <<= 1;
		out_value |= (*datap_div >> idx_mod) & 1;

		++idx;
	}
	
	T->cur_idx += T->bits_per_value;

	return out_value;
}
