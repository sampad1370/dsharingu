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


//static char				_local_ip[128], _global_ip[128];

//==================================================================
static void handle_ip_list( char **iplist, char loc_ip[128], char glb_ip[128]  )
{
int		i;

	loc_ip[0] = glb_ip[0] = 0;

	for (i=0; iplist[i]; ++i)
	{
		if ( strncmp( iplist[i], "192.168", 7 ) == 0 ||
			 strncmp( iplist[i], "10.0", 4 ) == 0 ||
			 strncmp( iplist[i], "23.", 3 ) == 0 )
			strcpy( loc_ip, iplist[i] );
		else
			strcpy( glb_ip, iplist[i] );

		//sprintf( tmp, "* Found IP \04%c%c%c%s", 0xff, 0xff, 0xff, inet_ntoa(addr_ip) );
		//chat_message_add( tmp, 0x00ff00 );

		KSYS_FREE( iplist[i] );

		if ( loc_ip[0] && glb_ip[0] )
			break;
	}

	if ( glb_ip[0] == 0 ) strcpy( glb_ip, "unknown" );
	if ( loc_ip[0] == 0 ) strcpy( loc_ip, "unknown" );

	cons_line_printf( "* Your current Internet IP Address is %s", glb_ip );

}

//==================================================================
static int find_ips( char **iplist )
{
int				ret, i, len;
char			sockname[256], tmp[512];
HOSTENT			*hp;
struct in_addr	addr_ip;

	ret = gethostname( sockname, 255 );
	if ( ret < 0 )
		return -1;

	hp = gethostbyname( sockname );
	//hp = gethostbyaddr( sockname );

	for (i=0; hp->h_addr_list[i]; ++i)
	{
		addr_ip = *(struct in_addr *)(hp->h_addr_list[i]);

		strcpy( tmp, inet_ntoa(addr_ip) );
		len = strlen(tmp);

		if NOT( iplist[i] = (char *)KSYS_MALLOC( len+1 ) )
			return -1;

		strcpy( iplist[i], tmp );
	}
	iplist[i] = 0;

	return 0;
}

//==================================================================
static bool quantizeBlock( u_char *desp, const u_char *srcp )
{
	int		delta_accum = 0;
	int		cur_r = 0;
	int		cur_g = 0;
	int		cur_b = 0;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			int r = (srcp[0] + 0) / 8;
			int g = (srcp[1] + 0) / 8;
			int b = (srcp[2] + 0) / 8;
			srcp += 3;


			int dr = r - cur_r;
			int dg = g - cur_g;
			int db = b - cur_b;

			if ( (i | j) != 0 )
				delta_accum |= dr | dg | db;

			desp[0] = pack_sign( dr );
			desp[1] = pack_sign( dg );
			desp[2] = pack_sign( db );
			desp += 3;

			cur_r = r;
			cur_g = g;
			cur_b = b;
		}
	}

	return delta_accum == 0;
}

//==================================================================
static void dequantizeBlock( u_char *desp, const u_char *srcp )
{
	int	cur_r = 0;
	int	cur_g = 0;
	int	cur_b = 0;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			int dr = unpack_sign( srcp[0] );
			int dg = unpack_sign( srcp[1] );
			int db = unpack_sign( srcp[2] );
			srcp += 3;

			cur_r += dr;
			cur_g += dg;
			cur_b += db;

			desp[0] = cur_r * 8;
			desp[1] = cur_g * 8;
			desp[2] = cur_b * 8;
			desp += 3;
		}
	}
}

//==================================================================
static bool quantizeQuarterBlock( u_char *desp, const u_char *srcp, int quadrant )
{
	int		delta_accum = 0;
	int		cur_r = 0;
	int		cur_g = 0;
	int		cur_b = 0;

	switch ( quadrant )
	{
	case 1:
		srcp += 1 * 3;
		srcp += 0 * ScreenPackerData::BLOCK_WD * 3;
		break;
	case 2:
		srcp += 1 * 3;
		srcp += 1 * ScreenPackerData::BLOCK_WD * 3;
		break;
	case 3:
		srcp += 0 * 3;
		srcp += 1 * ScreenPackerData::BLOCK_WD * 3;
		break;
	}


	for (int i=0; i < ScreenPackerData::BLOCK_WD; i += 2)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; j += 2)
		{
			int r = (srcp[0] + 0) / 8;
			int g = (srcp[1] + 0) / 8;
			int b = (srcp[2] + 0) / 8;
			srcp += 3*2;


			int dr = r - cur_r;
			int dg = g - cur_g;
			int db = b - cur_b;

			if ( (i | j) != 0 )
				delta_accum |= dr | dg | db;

			desp[0] = pack_sign( dr );
			desp[1] = pack_sign( dg );
			desp[2] = pack_sign( db );
			desp += 3;

			cur_r = r;
			cur_g = g;
			cur_b = b;
		}
		srcp += ScreenPackerData::BLOCK_WD * 3;
	}

	return delta_accum == 0;
}

//==================================================================
static void dequantizeQuarterBlock( u_char *desp, const u_char *srcp, int quadrant )
{
	int		cur_r = 0;
	int		cur_g = 0;
	int		cur_b = 0;

	if ( quadrant == 0 )
	{
		for (int i=0; i < ScreenPackerData::BLOCK_HE; i += 2)
		{
			for (int j=0; j < ScreenPackerData::BLOCK_WD; j += 2)
			{
				int dr = unpack_sign( srcp[0] );
				int dg = unpack_sign( srcp[1] );
				int db = unpack_sign( srcp[2] );
				srcp += 3;

				cur_r += dr;
				cur_g += dg;
				cur_b += db;

				u_char *des2p = desp + ScreenPackerData::BLOCK_WD * 3;

				desp[0] = desp[3] = des2p[0] = des2p[3] = cur_r * 8;
				desp[1] = desp[4] = des2p[1] = des2p[4] = cur_g * 8;
				desp[2] = desp[5] = des2p[2] = des2p[5] = cur_b * 8;
/*
				desp[0] = desp[3] = des2p[0] = des2p[3] = 255;
				desp[1] = desp[4] = des2p[1] = des2p[4] = 0;
				desp[2] = desp[5] = des2p[2] = des2p[5] = 0;
*/
				desp += 3*2;
			}
			desp += ScreenPackerData::BLOCK_WD * 3;
		}
	}
	else
	{
		switch ( quadrant )
		{
		case 1:
			desp += 1 * 3;
			desp += 0 * ScreenPackerData::BLOCK_WD*3;
			break;
		case 2:
			desp += 1 * 3;
			desp += 1 * ScreenPackerData::BLOCK_WD*3;
			break;
		case 3:
			desp += 0 * 3;
			desp += 1 * ScreenPackerData::BLOCK_WD*3;
			break;
		}

		for (int i=0; i < ScreenPackerData::BLOCK_HE; i += 2)
		{
			for (int j=0; j < ScreenPackerData::BLOCK_WD; j += 2)
			{
				int dr = unpack_sign( srcp[0] );
				int dg = unpack_sign( srcp[1] );
				int db = unpack_sign( srcp[2] );
				srcp += 3;

				cur_r += dr;
				cur_g += dg;
				cur_b += db;
/*
				switch ( quadrant )
				{
				case 1:
					desp[0] = 0;
					desp[1] = 255;
					desp[2] = 0;
					break;
				case 2:
					desp[0] = 0;
					desp[1] = 0;
					desp[2] = 255;
					break;
				case 3:
					desp[0] = 255;
					desp[1] = 255;
					desp[2] = 255;
					break;
				}
*/
				desp[0] = cur_r * 8;
				desp[1] = cur_g * 8;
				desp[2] = cur_b * 8;

				desp += 3*2;
			}
			desp += ScreenPackerData::BLOCK_WD * 3;
		}
	}
}

//==================================================================
static inline void pixelRGB2YC( u_char *desp_y, u_char *desp_c, int r, int g, int b )
{
	int	y = r > g ? r : g;
		y = y > b ? y : b;

	float	ooy = 3.0f / y;

	r = float2int( r * ooy );
	g = float2int( g * ooy );
	b = float2int( b * ooy );

	*desp_y = y >> 2;
	*desp_c = (r << 4) | (g << 2) | b;
}

//==================================================================
static inline void pixelYC2RGB( u_char *desp, int y, int c )
{
	int r = (c >> 4) & 3;
	int g = (c >> 2) & 3;
	int b = (c >> 0) & 3;

	// add 3 if not 0
	y = (y << 2);
	y += 3 & ~((y-1) >> 31);

	desp[0] = ((r+1) * y) >> 2;
	desp[1] = ((g+1) * y) >> 2;
	desp[2] = ((b+1) * y) >> 2;
}

//==================================================================
static inline void pixelRGB2YC2RGB( u_char *des_rgbp, const u_char *src_rgbp )
{
	u_char	tmp_y;
	u_char	tmp_c;

	pixelRGB2YC( &tmp_y, &tmp_c, src_rgbp[0], src_rgbp[1], src_rgbp[2] );
	pixelYC2RGB( des_rgbp, tmp_y, tmp_c );
}

//==================================================================
static inline void block4x4RGB_to_YC( u_char *desp_y,	 int des_y_pitch,
								   u_char *desp_c,
								   const u_char *src_rgbp1, int src_pitch )
{
	const u_char *src_rgbp2 = src_rgbp1 + 3 + 0;
	const u_char *src_rgbp3 = src_rgbp1 + 3 + src_pitch;
	const u_char *src_rgbp4 = src_rgbp1 + 0 + src_pitch;

	int	r1 = src_rgbp1[0];
	int r2 = src_rgbp2[0];
	int r3 = src_rgbp3[0];
	int r4 = src_rgbp4[0];
	int	g1 = src_rgbp1[1];
	int g2 = src_rgbp2[1];
	int g3 = src_rgbp3[1];
	int g4 = src_rgbp4[1];
	int	b1 = src_rgbp1[2];
	int b2 = src_rgbp2[2];
	int b3 = src_rgbp3[2];
	int b4 = src_rgbp4[2];
	
	int	y1 = ((r1 + g1 + b1) * (128/3) + 127) >> 7;//max3( r1, g1, b1 );
	int	y2 = ((r2 + g2 + b2) * (128/3) + 127) >> 7;//max3( r2, g2, b2 );
	int	y3 = ((r3 + g3 + b3) * (128/3) + 127) >> 7;//max3( r3, g3, b3 );
	int	y4 = ((r4 + g4 + b4) * (128/3) + 127) >> 7;//max3( r4, g4, b4 );

	int r = med4( r1, r2, r3, r4 );
	int g = med4( g1, g2, g3, g4 );
	int b = med4( b1, b2, b3, b4 );

	desp_y[0 +			 0] = y1 >> 2;
	desp_y[1 +			 0] = y2 >> 2;
	desp_y[1 + des_y_pitch] = y3 >> 2;
	desp_y[0 + des_y_pitch] = y4 >> 2;

	int max_avg_y = max4( y1, y2, y3, y4 );
	if ( max_avg_y )
	{
		int rt = float2int( 3 * r * (1.0f/255) );
		int gt = float2int( 3 * g * (1.0f/255) );
		int bt = float2int( 3 * b * (1.0f/255) );

		KASSERT( rt <= 3 && gt <= 3 && bt <= 3 );

		*desp_c = (rt << 4) | (gt << 2) | bt;
	}
	else
		*desp_c = 0;
}

//==================================================================
static inline void block4x4YC_to_RGB( u_char *des_rgbp1, int des_pitch,
								   const u_char *srcp_y1, int src_y_pitch,
								   const u_char *srcp_c )
{
	const u_char *srcp_y2 = srcp_y1 + 1 + 0;
	const u_char *srcp_y3 = srcp_y1 + 1 + src_y_pitch;
	const u_char *srcp_y4 = srcp_y1 + 0 + src_y_pitch;

	int	c = *srcp_c;
	int rt = (c >> 4) & 3;
	int gt = (c >> 2) & 3;
	int bt = (c >> 0) & 3;

	// mul by 4 and add 3 if not 0
	int y1 = (int)srcp_y1[0] << 2;	y1 += 3 & ~((y1-1) >> 31);
	int y2 = (int)srcp_y2[0] << 2;	y2 += 3 & ~((y2-1) >> 31);
	int y3 = (int)srcp_y3[0] << 2;	y3 += 3 & ~((y3-1) >> 31);
	int y4 = (int)srcp_y4[0] << 2;	y4 += 3 & ~((y4-1) >> 31);

	int max_avg_y = max4( y1, y2, y3, y4 );

	u_char *des_rgbp2 = des_rgbp1 + 3 + 0;
	u_char *des_rgbp3 = des_rgbp1 + 3 + des_pitch;
	u_char *des_rgbp4 = des_rgbp1 + 0 + des_pitch;

	int r = (rt * 256) / 3;	// 0..3 -> 1..4
	int g = (gt * 256) / 3;	// 0..3 -> 1..4
	int b = (bt * 256) / 3;	// 0..3 -> 1..4
//	r = g = b = 256;
/*
	KASSERT( ((r * y1) >> 7+8) < 255 );
	KASSERT( ((g * y1) >> 7+8) < 255 );
	KASSERT( ((b * y1) >> 7+8) < 255 );
	KASSERT( ((r * y2) >> 7+8) < 255 );
	KASSERT( ((g * y2) >> 7+8) < 255 );
	KASSERT( ((b * y2) >> 7+8) < 255 );
	KASSERT( ((r * y3) >> 7+8) < 255 );
	KASSERT( ((g * y3) >> 7+8) < 255 );
	KASSERT( ((b * y3) >> 7+8) < 255 );
	KASSERT( ((r * y4) >> 7+8) < 255 );
	KASSERT( ((g * y4) >> 7+8) < 255 );
	KASSERT( ((b * y4) >> 7+8) < 255 );
*/

	des_rgbp1[0] = (r * y1) >> 8;
	des_rgbp1[1] = (g * y1) >> 8;
	des_rgbp1[2] = (b * y1) >> 8;
	des_rgbp2[0] = (r * y2) >> 8;
	des_rgbp2[1] = (g * y2) >> 8;
	des_rgbp2[2] = (b * y2) >> 8;
	des_rgbp3[0] = (r * y3) >> 8;
	des_rgbp3[1] = (g * y3) >> 8;
	des_rgbp3[2] = (b * y3) >> 8;
	des_rgbp4[0] = (r * y4) >> 8;
	des_rgbp4[1] = (g * y4) >> 8;
	des_rgbp4[2] = (b * y4) >> 8;
}

//==================================================================
//==================================================================
static inline int max3_and_acc( int a, int b, int c, int &acc_a, int &acc_b, int &acc_c )
{
	acc_a += a;
	acc_b += b;
	acc_c += c;

	int t = a > b ? a : b;
	return t > c ? t : c;
}

//==================================================================
static inline int max4( int a, int b, int c, int d )
{
	int t;

	t = a > b ? a : b;
	t = c > t ? c : t;
	t = d > t ? d : t;

	return t;
}

//==================================================================
static void swapint( int &a, int &b )
{
	int	t = a; a = b; b = t;
}

//==================================================================
static inline int med4( int a, int b, int c, int d )
{
	if ( b > a )	swapint( a, b );
	if ( c > d )	swapint( c, d );
	if ( b > c )	swapint( b, c );
	//if ( a > d )	swapint( a, d );

	return c;
}

//==================================================================
//==================================================================
static void blockRGB2YC( u_char *desp, const u_char *srcp )
{
	u_char	*desp_y = desp;
	u_char	*desp_c = desp + MAX_BLK_Y_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			pixelRGB2YC( desp_y++, desp_c++, srcp[0], srcp[1], srcp[2] );
			srcp += 3;
		}
	}
}

//==================================================================
static void blockYC2RGB( u_char *desp, const u_char *srcp )
{
	const u_char	*srcp_y = srcp;
	const u_char	*srcp_c = srcp + MAX_BLK_Y_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			pixelYC2RGB( desp, *srcp_y++, *srcp_c++ );
			desp += 3;
		}
	}
}

//==================================================================
//==================================================================
static void blockRGB_to_YC41( u_char *desp, const u_char *srcp_rgb )
{
	u_char	*desp_y = desp;
	u_char	*desp_c = desp + MAX_BLK_Y_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD/2; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE/2; ++j)
		{
			block4x4RGB_to_YC( desp_y, BLK_Y_PITCH,
							   desp_c,
							   srcp_rgb, BLK_RGB_PITCH );
			desp_y += 2;
			desp_c += 1;
			srcp_rgb += 3 * 2;
		}
		desp_y += BLK_Y_PITCH * (2-1);
		srcp_rgb += BLK_RGB_PITCH * (2-1);
	}
}

//==================================================================
static void blockYC41_to_RGB( u_char *desp_rgb, const u_char *srcp )
{
	const u_char	*srcp_y = srcp;
	const u_char	*srcp_c = srcp + MAX_BLK_Y_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD/2; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE/2; ++j)
		{
			block4x4YC_to_RGB( desp_rgb, BLK_RGB_PITCH,
							   srcp_y, BLK_Y_PITCH,
							   srcp_c );
			srcp_y += 2;
			srcp_c += 1;
			desp_rgb += 3 * 2;
		}
		srcp_y += BLK_Y_PITCH * (2-1);
		desp_rgb += BLK_RGB_PITCH * (2-1);
	}
}

//==================================================================
//== 2005/12/25
//==================================================================
static void haar_Transform2D( int *datap, int levels, bool inverse )
{
int	*plane, *line, *nextline;
int	step, step2, x, y, ll_width;
int	a, b, c, d, A, B, C, D;

	ll_width = ScreenPackerData::BLOCK_WD >> levels;
	plane = datap;

	if NOT( inverse )
	{
		for(step=1; step <= ll_width; step<<=1)
		{
			step2 = step + step;
			for(y=0; (y+step)<ScreenPackerData::BLOCK_WD; y += step2)
			{
				line = plane + y*ScreenPackerData::BLOCK_WD;
				nextline = line + step*ScreenPackerData::BLOCK_WD;
				for(x=0; (x+step)<ScreenPackerData::BLOCK_WD; x += step2)
				{
					a = line[x];		b = line[x+step];
					c = nextline[x];	d = nextline[x+step];

					A = (a+b+c+d)>>2;
					B = a-b-c+d;
					C = a+b-c-d;
					D = a-b+c-d;

					line[x] 			= A; // will be coded in next pass
					line[x+step]		= B;
					nextline[x]			= C;
					nextline[x+step]	= D;
				}
			}
		}
	}
	else
	{
		/** go backwards in scale **/
		for(step=ll_width; step >= 1; step >>= 1)
		{
			step2 = step + step;

			for(y=0;(y+step)<ScreenPackerData::BLOCK_WD;y += step2)
			{
				line = plane + y*ScreenPackerData::BLOCK_WD;
				nextline = line + step*ScreenPackerData::BLOCK_WD;

				for(x=0;(x+step)<ScreenPackerData::BLOCK_WD;x += step2)
				{
					A = line[x]; // already decoded.
					B = line[x+step];
					C = nextline[x];
					D = nextline[x+step];

					a = A + ((3 + B + C + D)>>2);
					b = A + ((3 - B + C - D)>>2);
					c = A + ((3 - B - C + D)>>2);
					d = A + ((3 + B - C - D)>>2);

					line[x]			= a;
					line[x+step]	= b;
					nextline[x] 	= c;
					nextline[x+step]= d;
				}
			}
		}
	}
}

//==================================================================
//==================================================================
static inline void pixelRGB_to_PISV( int &out_p, int &out_i, int &out_s, int &out_v,
									int r, int g, int b )
{
	int max = max3( r, g, b );
	int min = min3( r, g, b );

	int	diff	= max - min;

	out_v = max;

	if ( diff == 0 )
	{
		out_p = 0;
		out_i = 0;
		out_s = 0;
	}
	else
	{
		out_s = 255 * diff / max;

		//KASSERT( (255 * diff / max) <= 255 );

		float	oo_diff = (float)255 / diff;
		//float	hf;

		if ( max == r )
		{
			if ( g > b )
			{
				out_p = 0;
				out_i = float2int( (g - b) * oo_diff );
			}
			else
			{
				out_p = 5;
				out_i = float2int( (b - g) * oo_diff );
			}
		}
		else
		if ( max == g )
		{
			if ( r > b )
			{
				out_p = 1;
				out_i = float2int( (r - b) * oo_diff );
			}
			else
			{
				out_p = 2;
				out_i = float2int( (b - r) * oo_diff );
			}
		}
		else
		{
			if ( g > r )
			{
				out_p = 3;
				out_i = float2int( (g - r) * oo_diff );
			}
			else
			{
				out_p = 4;
				out_i = float2int( (r - g) * oo_diff );
			}
		}
	}
}

//==================================================================
//==================================================================
static inline void pixelPISV_to_RGB( u_char *desp_rgb, int p, int i, int s, int v )
{
	if ( v == 0 )
	{
		desp_rgb[0] = 0;
		desp_rgb[1] = 0;
		desp_rgb[2] = 0;
		return;
	}

	if ( s == 0 )
	{
		desp_rgb[0] = v;
		desp_rgb[1] = v;
		desp_rgb[2] = v;
		return;
	}
/*
	//KASSERT( i <= 15 );
	float	ss = s * (1.0f/S_QUANT_MAX);
	float	ii = i * (1.0f/S_QUANT_MAX);

	int x = float2int( v * (1 - ss) );	
	int y = float2int( v * (1 - ss * ii) );
	int z = float2int( v * (1 - ss * (1 - ii) ));
*/

	int x = v * (255   - s)				>> 8;
	int y = v * (65535 - s * i)			>> 16;
	int z = v * (65535 - s * (255 - i))	>> 16;

	int	r, g, b;

	switch ( p )
	{
	case 0:		r = v;	g = z;	b = x;	break;
	case 1:		r = y;	g = v;	b = x;	break;
	case 2:		r = x;	g = v;	b = z;	break;
	case 3:		r = x;	g = y;	b = v;	break;
	case 4:		r = z;	g = x;	b = v;	break;
	default:	r = v;	g = x;	b = y;	break;
	}

	desp_rgb[0] = r;
	desp_rgb[1] = g;
	desp_rgb[2] = b;
}

//==================================================================
static void blockRGB_to_PISV( u_char *desp, const u_char *srcp )
{
	MemFile	stream_p( desp, MAX_BLK_P_SIZE );	desp += MAX_BLK_P_SIZE;
	MemFile	stream_i( desp, MAX_BLK_I_SIZE );	desp += MAX_BLK_I_SIZE;
	MemFile	stream_s( desp, MAX_BLK_S_SIZE );	desp += MAX_BLK_S_SIZE;
	MemFile	stream_v( desp, MAX_BLK_V_SIZE );	desp += MAX_BLK_V_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			int	p_val, i_val, s_val, v_val;

			pixelRGB_to_PISV( p_val, i_val, s_val, v_val,
							  srcp[0], srcp[1], srcp[2] );

			//p_val
			i_val >>= (8 - I_QUANT_BITS);
			s_val >>= (8 - S_QUANT_BITS);
			v_val >>= (8 - V_QUANT_BITS);

			stream_p.WriteBits( p_val, P_QUANT_BITS );
			stream_i.WriteBits( i_val, I_QUANT_BITS );
			stream_s.WriteBits( s_val, S_QUANT_BITS );
			stream_v.WriteBits( v_val, V_QUANT_BITS );

			srcp += 3;
		}
	}

	stream_p.WriteAlignByte();
	stream_i.WriteAlignByte();
	stream_s.WriteAlignByte();
	stream_v.WriteAlignByte();
}

//==================================================================
static void unpackBitstream( const u_char *srcp, int src_size, int cnt, int bitsize, int *destp )
{
	MemFile memf( srcp, src_size );
	for (int i=0; i < cnt; ++i)
		*destp++ = memf.ReadBits( bitsize );
}
//==================================================================
static void unpackBitstream( const u_char *srcp, int src_size, int cnt, int bitsize, u_char *destp )
{
	MemFile memf( srcp, src_size );
	for (int i=0; i < cnt; ++i)
		*destp++ = memf.ReadBits( bitsize );
}

//==================================================================
static void blockPISV_to_RGB( u_char *desp, const u_char *srcp )
{
	u_char	p_vals[ MAX_BLK_PIXELS ], *p_val; p_val = p_vals;
	u_char	i_vals[ MAX_BLK_PIXELS ], *i_val; i_val = i_vals;
	u_char	s_vals[ MAX_BLK_PIXELS ], *s_val; s_val = s_vals;
	int		v_vals[ MAX_BLK_PIXELS ], *v_val; v_val = v_vals;

	unpackBitstream( srcp, MAX_BLK_P_SIZE, MAX_BLK_PIXELS, P_QUANT_BITS, p_vals );	srcp += MAX_BLK_P_SIZE;
	unpackBitstream( srcp, MAX_BLK_I_SIZE, MAX_BLK_PIXELS, I_QUANT_BITS, i_vals );	srcp += MAX_BLK_I_SIZE;
	unpackBitstream( srcp, MAX_BLK_S_SIZE, MAX_BLK_PIXELS, S_QUANT_BITS, s_vals );	srcp += MAX_BLK_S_SIZE;
	unpackBitstream( srcp, MAX_BLK_V_SIZE, MAX_BLK_PIXELS, V_QUANT_BITS, v_vals );	srcp += MAX_BLK_V_SIZE;

	for (int i=0; i < ScreenPackerData::BLOCK_WD; ++i)
	{
		for (int j=0; j < ScreenPackerData::BLOCK_HE; ++j)
		{
			pixelPISV_to_RGB( desp,
							  (int)*p_val++ << (0),
							  (int)*i_val++ << (8 - I_QUANT_BITS),
							  (int)*s_val++ << (8 - S_QUANT_BITS),
							  (int)*v_val++ << (8 - V_QUANT_BITS) );
			desp += 3;
		}
	}
}
