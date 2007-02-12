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
static inline int right_shift( int val, int cnt )
{
	int	sign = val >> 31;
	int	abs_val = ((val ^ sign) + (1 & sign)) >> cnt;

	return (abs_val ^ sign) + (1 & sign);
}

//==================================================================
static inline int div2( int val )
{
	return right_shift( val, 1 );
}

//==================================================================
static inline void swap( short * &a, short * &b )
{
	short	*t = a;
	a = b;
	b = a;
}

//==================================================================
static void makeAvgAndDiff( const short *srcp, short *desp, u_int n )
{
	short	*avgp = desp;
	short	*difp = desp + n/2;

	for (int i=n/2; i; --i)
	{
		int	v1 = *srcp++;
		int	v2 = *srcp++;

		int	avg = div2( v1 + v2 );

		*avgp++ = avg;
		*difp++ = avg - v1;
	}
}

//==================================================================
static void makeV1AndV2( const short *srcp, short *desp, u_int n )
{
	const short	*avgp = srcp;
	const short	*difp = srcp + n/2;

	for (int i=n/2; i; --i)
	{
		int	avg = *avgp++;
		int dif = *difp++;

		*desp++ = avg - dif;	// v1
		*desp++ = avg + dif;	// v2
	}
}

//==================================================================
static void blockToHaar( const u_char *in_blockp,
						 short *out_haarp,
						 short *tmp_line1,
						 short *tmp_line2,
						 u_int block_dim,
						 u_int min_dim )
{
	PSYS_ASSERT( min_dim >= 2 );
	short *srcp = tmp_line1;
	short *desp = tmp_line2;

	for (int y=0; y < block_dim; ++y)
	{
		const u_char *in_blockp_line = in_blockp + y * block_dim;

		for (int i=0; i < block_dim; ++i)
			srcp[i] = in_blockp_line[i];

		for (int n = block_dim; n > min_dim; n /= 2)
		{
			makeAvgAndDiff( srcp, desp, n );
			swap( srcp, desp );
		}

		for (int i=0; i < block_dim; ++i)
			out_haarp[i] = desp[i];
	}

	for (int x=0; x < block_dim; ++x)
	{
		for (int i=0, ii=0; i < block_dim; ++i, ii += block_dim)
			srcp[i] = out_haarp[ii+x];

		for (int n = block_dim; n; n /= 2)
		{
			makeAvgAndDiff( srcp, desp, n );
			swap( srcp, desp );
		}

		for (int i=0, ii=0; i < block_dim; ++i, ii += block_dim)
			out_haarp[ii+x] = desp[i];
	}
}

//==================================================================
static void haarToBlock( const short *in_haarp,
						 u_char *out_blockp,
						 short *tmp_line1,
						 short *tmp_line2,
						 short *tmp_haar,
						 u_int block_dim,
						 u_int min_dim )
{
	PSYS_ASSERT( min_dim >= 2 );
	short *srcp = tmp_line1;
	short *desp = tmp_line2;

	for (int x=0; x < block_dim; ++x)
	{
		for (int i=0, ii=0; i < block_dim; ++i, ii += block_dim)
			srcp[i] = in_haarp[ii+x];

		for (int n = min_dim; n < block_dim; n *= 2)
		{
			makeV1AndV2( srcp, desp, n );
			swap( srcp, desp );
		}

		for (int i=0, ii=0; i < block_dim; ++i, ii += block_dim)
			tmp_haar[ii+x] = desp[i];
	}

	for (int y=0; y < block_dim; ++y)
	{
		for (int i=0; i < block_dim; ++i)
			srcp[i] = tmp_haar[i];

		for (int n = min_dim; n < block_dim; n *= 2)
		{
			makeV1AndV2( srcp, desp, n );
			swap( srcp, desp );
		}

		for (int i=0; i < block_dim; ++i)
		{
			int clamped_des = desp[i];
			PCLAMP( clamped_des, 0, 255 );
			out_blockp[i] = clamped_des;
		}
	}

}

//==================================================================
void ScreenHaarComprPack::PackData( const u_char *in_blockp, u_char * &out_datap, u_int &out_data_len )
{
	short	tmp_line1[BLOCK_DIM];
	short	tmp_line2[BLOCK_DIM];

	blockToHaar( in_blockp, (short *)_out_data, tmp_line1, tmp_line2, BLOCK_DIM, 4 );

	out_datap = _out_data;
	//out_data_len = ;
}

//==================================================================
void ScreenHaarComprUnpack::UnpackData( const u_char *in_datap, u_int in_data_len, u_char *out_blockp )
{
	short	tmp_line1[BLOCK_DIM];
	short	tmp_line2[BLOCK_DIM];
	short	tmp_haar[BLOCK_DIM*BLOCK_DIM];

	haarToBlock( (const short *)in_datap, out_blockp, tmp_line1, tmp_line2, tmp_haar, BLOCK_DIM, 4 );

	//out_datap = _out_data;
	//out_data_len = ;
}
