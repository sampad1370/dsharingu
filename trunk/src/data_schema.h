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
class ItemBase
{
public:
	safe_ptr<char>	_stridp;

public:
	ItemBase( const char *stridp )
	{
		_stridp	= strcpyalloc( stridp );
	}

	virtual ~ItemBase(){};

	virtual void		LoadFromString( const char *strp ) = 0;
	virtual const char	*StoreToString( char *outstrp ) = 0;

	static char		*strcpyalloc( const char *srcp );
};

//==================================================================
class ItemString : public ItemBase
{
public:
	char	*_datap;
	int		_max_str_size;

public:
	ItemString( const char *stridp, char *datap, int strsize ) :
	  ItemBase(stridp)
	{
		_datap	 = datap;
		_max_str_size = strsize;
	}

	virtual ~ItemString(){};

	virtual void		LoadFromString( const char *strp )
	{
		psys_strcpy( _datap, strp, _max_str_size );

	}
	virtual const char	*StoreToString( char *outstrp )
	{
		return _datap;
	}
};

//==================================================================
class ItemSHA1Hash : public ItemBase
{
public:
	sha1_t	*_datap;
	int		_max_str_size;

public:
	ItemSHA1Hash( const char *stridp, sha1_t *sha1hashp ) :
	  ItemBase(stridp)
	{
		_datap	 = sha1hashp;
	}

	virtual ~ItemSHA1Hash(){};

	virtual void		LoadFromString( const char *strp );
	virtual const char	*StoreToString( char *outstrp );
};

//==================================================================
class ItemInt : public ItemBase
{
public:
	int		*_datap;
	int		_min, _max;

public:
	ItemInt( const char *stridp, int *datap, int minval, int maxval ) :
		ItemBase(stridp),
		_datap(datap),
		_min(minval),
		_max(maxval)
	{
	}

	virtual ~ItemInt(){};

	virtual void		LoadFromString( const char *strp )
	{
		*_datap = atoi( strp );
		PCLAMP( *_datap, _min, _max );
	}
	virtual const char	*StoreToString( char *outstrp )
	{
		sprintf( outstrp, "%i", *_datap );
		return outstrp;
	}
};

//==================================================================
class ItemULong : public ItemBase
{
public:
	u_long		*_datap;
	u_long		_min, _max;

public:
	ItemULong( const char *stridp, u_long *datap, u_long minval, u_long maxval ) :
		ItemBase(stridp),
		_datap(datap),
		_min(minval),
		_max(maxval)
	{
	}

	virtual ~ItemULong(){};

	virtual void		LoadFromString( const char *strp )
	{
		*_datap = atoi( strp );
		PCLAMP( *_datap, _min, _max );
	}
	virtual const char	*StoreToString( char *outstrp )
	{
		sprintf( outstrp, "%lu", *_datap );
		return outstrp;
	}
};

//==================================================================
class ItemFloat : public ItemBase
{
public:
	float		*_datap;
	float		_min, _max;

public:
	ItemFloat( const char *stridp, float *datap, float minval, float maxval ) :
		ItemBase(stridp),
		_datap(datap),
		_min(minval),
		_max(maxval)
	{
	}

	virtual ~ItemFloat(){};

	virtual void		LoadFromString( const char *strp )
	{
		*_datap = atof( strp );
		PCLAMP( *_datap, _min, _max );
	}
	virtual const char	*StoreToString( char *outstrp )
	{
		sprintf( outstrp, "%g", *_datap );
		return outstrp;
	}
};

//==================================================================
class ItemBool : public ItemBase
{
public:
	bool	*_datap;

public:
	ItemBool( const char *stridp, bool *datap ) :
		ItemBase(stridp),
		_datap(datap)
	{
	}

	virtual ~ItemBool(){};

	virtual void		LoadFromString( const char *strp )
	{
		*_datap = (atoi( strp ) ? true : false);
	}
	virtual const char	*StoreToString( char *outstrp )
	{
		sprintf( outstrp, "%i", *_datap ? 1 : 0 );
		return outstrp;
	}
};

//==================================================================
class DataSchema
{
public:
	PArray<ItemBase *>		_itemsp_list;
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
	ItemBase	*item_find_by_strid( const char *stridp );
};

#endif