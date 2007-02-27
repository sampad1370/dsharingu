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

#include "screen_haar_compr.h"

//==================================================================
static PFORCEINLINE int right_shift( int val, int cnt )
{
	int	sign = val >> 31;
	int	abs_val = ((val ^ sign) + (1 & sign)) >> cnt;

	return (abs_val ^ sign) + (1 & sign);
}

//==================================================================
static PFORCEINLINE int div2( int val )
{
	return right_shift( val, 1 );
}

//==================================================================
static PFORCEINLINE u_int pack_sign( int val )
{
	int	sign_mask = val >> 31;	
	return ((val ^ sign_mask) << 1) | (sign_mask & 1);
}

//==================================================================
static PFORCEINLINE int unpack_sign( u_int val )
{
	int	sign_mask = (int)((val & 1) ^ 1) - 1;
	return (val >> 1) ^ sign_mask;
}


//==================================================================
#if 0
static PFORCEINLINE void swap( short * &a, short * &b )
{
	short	*t = a;
	a = b;
	b = a;
}
#endif

//==================================================================
typedef short TMPTYPE;

//==================================================================
static PFORCEINLINE void makeAvgAndDiff( const TMPTYPE *srcp, TMPTYPE * const desp, u_int n )
{
	TMPTYPE	* const avgp = desp;
	TMPTYPE	* const difp = desp + n/2;

	//static const int	oosqrt2 = (int)((1 << 10) / 1.414213562f);
	for (int i=0; i < n/2; ++i)
	{
		int	v1 = *srcp++;
		int	v2 = *srcp++;

		//int	avg = right_shift( (v1 + v2) * oosqrt2, 10 );
		int avg = div2( v1 + v2 );

		avgp[i] = avg;
		difp[i] = avg - v1;
	}
}

//==================================================================
static PFORCEINLINE void makeV1AndV2( const TMPTYPE * const srcp, TMPTYPE *desp, u_int n )
{
	const TMPTYPE	* const avgp = srcp;
	const TMPTYPE	* const difp = srcp + n/2;

	for (int i=0; i < n/2; ++i)
	{
		int	avg = avgp[i];
		int dif = difp[i];

		*desp++ = avg - dif;	// v1
		*desp++ = avg + dif;	// v2
	}
}

//==================================================================
template<typename TBLOCK>
static void blockToHaar( const TBLOCK *in_blockp,
						 short out_haarp[ScreenHaarComprPack::BLOCK_DIM][ScreenHaarComprPack::BLOCK_DIM],
						 u_int block_dim,
						 u_int min_dim,
						 u_int shift_bits )
{
	PSYS_ASSERT( min_dim >= 2 );

	TMPTYPE tmp_line1[ScreenHaarComprPack::BLOCK_DIM];
	TMPTYPE tmp_line2[ScreenHaarComprPack::BLOCK_DIM];
	TMPTYPE tmp_haar[ScreenHaarComprPack::BLOCK_DIM][ScreenHaarComprPack::BLOCK_DIM];

#if 0
	for (int y=0; y < block_dim; ++y)
	{
		for (int x=0; x < block_dim; ++x)
		{
			*out_haarp++ = *in_blockp++;
		}
	}
	return;
#endif

	for (int y=0; y < block_dim; ++y)
	{
		int	y_off = y * block_dim;

		for (int i=0; i < block_dim; ++i)
			tmp_line1[i] = in_blockp[ i + y_off ];

		for (int n = block_dim; n >= min_dim; n /= 2)
		{
			for (int i=0; i < n; ++i)
				tmp_line2[i] = tmp_line1[i];

			makeAvgAndDiff( tmp_line2, tmp_line1, n );
		}

		for (int i=0; i < block_dim; ++i)
			tmp_haar[i][y] = tmp_line1[i];
	}

	for (int x=0; x < block_dim; ++x)
	{
		//for (int i=0; i < block_dim; ++i)
		//	tmp_line1[i] = tmp_haar[x][i];
		memcpy( tmp_line1, tmp_haar[x], ScreenHaarComprPack::BLOCK_DIM*sizeof(tmp_line1[0]) );

		for (int n = block_dim; n >= min_dim; n /= 2)
		{
			for (int i=0; i < n; ++i)
				tmp_line2[i] = tmp_line1[i];

			makeAvgAndDiff( tmp_line2, tmp_line1, n );
		}

		for (int i=0; i < block_dim; ++i)
		{
			//if ( x == 0 && i == 0 )
			//	out_haarp[x][i] = pack_sign( tmp_line1[i] );
			//else
				out_haarp[x][i] = pack_sign( right_shift( tmp_line1[i], shift_bits ) );
		}
	}
}

//==================================================================
template<typename TBLOCK>
static void haarToBlock( const short *in_haarp,
						 TBLOCK out_blockp[ScreenHaarComprPack::BLOCK_DIM][ScreenHaarComprPack::BLOCK_DIM],
						 u_int block_dim,
						 u_int min_dim,
						 u_int shift_bits,
						 int clamp_min,
						 int clamp_max )
{
	PSYS_ASSERT( min_dim >= 2 );

	TMPTYPE tmp_line1[ScreenHaarComprPack::BLOCK_DIM];
	TMPTYPE tmp_line2[ScreenHaarComprPack::BLOCK_DIM];
	TMPTYPE tmp_haar[ScreenHaarComprPack::BLOCK_DIM][ScreenHaarComprPack::BLOCK_DIM];

#if 0
	for (int y=0; y < block_dim; ++y)
	{
		for (int x=0; x < block_dim; ++x)
		{
			*out_blockp++ = *in_haarp++;
		}
	}
	return;
#endif

	int	expand_val = 0;//(1 << shift_bits) >> 1;

	for (int x=0; x < block_dim; ++x)
	{
		int	x_off = x * block_dim;

		for (int i=0; i < block_dim; ++i)
		{
			/*
			if ( x == 0 && i == 0 )
			{
				tmp_line1[i] = unpack_sign( in_haarp[ i + x_off ] );
			}
			else*/
			{
				int	val = unpack_sign( in_haarp[ i + x_off ] ) << shift_bits;
				tmp_line1[i] = (val < 0 ? val-expand_val : val+expand_val);
			}
		}

		for (int n = min_dim; n <= block_dim; n *= 2)
		{
			for (int i=0; i < n; ++i)
				tmp_line2[i] = tmp_line1[i];
			//memcpy( tmp_line2, tmp_line1, n*sizeof(tmp_line1[0]) );

			makeV1AndV2( tmp_line2, tmp_line1, n );
		}

		for (int i=0; i < block_dim; ++i)
			tmp_haar[i][x] = tmp_line1[i];
	}

	for (int y=0; y < block_dim; ++y)
	{
		for (int i=0; i < block_dim; ++i)
			tmp_line1[i] = tmp_haar[y][i];
		//memcpy( tmp_line1, tmp_haar[y], ScreenHaarComprPack::BLOCK_DIM*sizeof(tmp_line1[0]) );

		for (int n = min_dim; n <= block_dim; n *= 2)
		{
			//for (int i=0; i < n; ++i)
			//	tmp_line2[i] = tmp_line1[i];
			memcpy( tmp_line2, tmp_line1, n*sizeof(tmp_line1[0]) );

			makeV1AndV2( tmp_line2, tmp_line1, n );
		}

		for (int i=0; i < block_dim; ++i)
		{
			int clamped_des = tmp_line1[i];
			PCLAMP( clamped_des, clamp_min, clamp_max );
			out_blockp[y][i] = clamped_des;
		}
	}
}

//==================================================================
static const int MIN_DIM = 2;

//==================================================================
PUtils::Memfile ScreenHaarComprPack::PackData( const u_char *in_blockp, u_int quant_rshift )
{
	blockToHaar<u_char>( in_blockp, (short (*)[BLOCK_DIM])_out_data,
				 BLOCK_DIM, MIN_DIM, quant_rshift );

	return PUtils::Memfile( (const void *)_out_data, BLOCK_DIM*BLOCK_DIM*sizeof(short) );
}

//==================================================================
PUtils::Memfile ScreenHaarComprPack::PackData( const s_char *in_blockp, u_int quant_rshift )
{
	blockToHaar<s_char>( in_blockp, (short (*)[BLOCK_DIM])_out_data,
				 BLOCK_DIM, MIN_DIM, quant_rshift );

	return PUtils::Memfile( (const void *)_out_data, BLOCK_DIM*BLOCK_DIM*sizeof(short) );
}

//==================================================================
PUtils::Memfile ScreenHaarComprPack::PackData( const short *in_blockp, u_int quant_rshift )
{
	blockToHaar<short>( in_blockp, (short (*)[BLOCK_DIM])_out_data,
						BLOCK_DIM, MIN_DIM, quant_rshift );

	return PUtils::Memfile( (const void *)_out_data, BLOCK_DIM*BLOCK_DIM*sizeof(short) );
}

//==================================================================
void ScreenHaarComprUnpack::UnpackData( const u_char *in_datap,
										u_int in_data_len,
										u_char *out_blockp,
										u_int quant_rshift )
{
	haarToBlock<u_char>( (const short *)in_datap,
						 (u_char (*)[BLOCK_DIM])out_blockp,
						 BLOCK_DIM, MIN_DIM, quant_rshift, 0, 255 );
}

//==================================================================
void ScreenHaarComprUnpack::UnpackData( const u_char *in_datap,
										u_int in_data_len,
										s_char *out_blockp,
										u_int quant_rshift )
{
	haarToBlock<s_char>( (const short *)in_datap,
						 (s_char (*)[BLOCK_DIM])out_blockp,
						 BLOCK_DIM, MIN_DIM, quant_rshift, -128, 127 );
}

//==================================================================
void ScreenHaarComprUnpack::UnpackData( const u_char *in_datap,
										u_int in_data_len,
										short *out_blockp,
										u_int quant_rshift )
{
	haarToBlock<short>( (const short *)in_datap,
						(short (*)[BLOCK_DIM])out_blockp,
						BLOCK_DIM, MIN_DIM, quant_rshift, -32768, 32767 );
}
