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
//==
//==
//==
//==
//==================================================================

#ifndef DATASCHEMA_H
#define DATASCHEMA_H

#include <stdio.h>
#include "psys.h"

//==================================================================
class sha1_t
{
public:
	u_char	_data[20];

	sha1_t()
	{
		srand( psys_timer_get_ul64() );
		for (int i=0; i < 20; ++i)
			_data[i] = rand();
	}

	static bool AreEqual( const u_char a[20], const u_char b[20] )
	{
		for (int i=0; i < 20; ++i)
			if ( a[i] != b[i] )
				return false;

		return true;
	}
};

//==================================================================
class dlgmk_item_c
{
	enum {
		DTYPE_STRING,
		DTYPE_SHA1HASH,
		DTYPE_INT,
		DTYPE_ULONG,
		DTYPE_FLOAT,
		DTYPE_BOOLEAN,
	};
private:
	static const int	NUM_STRSIZE = 32;

	static char		*strcpyalloc( const char *srcp );

public:
	//---------------------------------------------------------------------------
	void init( const char *stridp )
	{
		memset( this, 0, sizeof(*this) );
		_stridp	 = strcpyalloc( stridp );
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, sha1_t *sha1hashp )
	{
		init( stridp );
		_dtype	 = DTYPE_SHA1HASH;
		_datap	 = sha1hashp;
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, char *datap, int strsize )
	{
		init( stridp );
		_dtype	 = DTYPE_STRING;
		_datap	 = datap;
		_max_str_size = strsize;
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, int *datap, int i_min, int i_max )
	{
		init( stridp );
		_dtype	 = DTYPE_INT;
		_datap	 = datap;
		_i_min	 = i_min;
		_i_max	 = i_max;
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, u_long *datap, u_long ul_min, u_long ul_max )
	{
		init( stridp );
		_dtype	 = DTYPE_ULONG;
		_datap	 = datap;
		_ul_min	 = ul_min;
		_ul_max	 = ul_max;
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, float *datap, float f_min, float f_max )
	{
		init( stridp );
		_dtype	 = DTYPE_FLOAT;
		_datap	 = datap;
		_f_min	 = f_min;
		_f_max	 = f_max;
	}
	//---------------------------------------------------------------------------
	dlgmk_item_c( const char *stridp, bool *datap )
	{
		init( stridp );
		_dtype	 = DTYPE_BOOLEAN;
		_datap	 = datap;
	}
	//---------------------------------------------------------------------------
	~dlgmk_item_c()
	{
		if ( _titlep )
		{
			delete _titlep;
			_titlep = NULL;
		}
		if ( _stridp )
		{
			delete _stridp;
			_stridp = NULL;
		}
	}
	//---------------------------------------------------------------------------
	void		LoadFromString( const char *strp );
	const char	*StoreToString( char *outstrp );

	//---------------------------------------------------------------------------
	char			*_titlep;
	char			*_stridp;
	void			*_datap;

	int				_dtype;
	int				_max_str_size;
	int				_i_min, _i_max;
	u_long			_ul_min, _ul_max;
	float			_f_min, _f_max;

	u_int			store_fix_data[4];	// up to 128 bit
};

//==================================================================
class DataSchema
{
public:
	PArray<dlgmk_item_c *>	_itemsp_list;

	const char				*_stridp;

	//---------------------------------------------------------------------------
	DataSchema( const char *stridp );
	~DataSchema();

	//---------------------------------------------------------------------------
	void	AddString( const char *stridp, char *datap, int strsize );
	void	AddSHA1Hash( const char *stridp, sha1_t *sha1hashp );
	void	AddInt( const char *stridp, int *datap, int i_min, int i_max );
	void	AddULong( const char *stridp, u_long *datap, u_long ul_min, u_long ul_max );
	void	AddFloat( const char *stridp, float *datap, float f_min, float f_max );
	void	AddBool( const char *stridp, bool *datap );

	PError	SaveData( FILE *fp );
	int		LoadData( FILE *fp );

private:
	dlgmk_item_c	*addItem( dlgmk_item_c *itemp );

	dlgmk_item_c	*item_find_by_strid( const char *stridp );
};

#endif