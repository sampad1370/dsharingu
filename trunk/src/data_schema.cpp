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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "data_schema.h"

//===================================================================
static int isblanko( char c )
{
	return (c == ' ' || c == '\t' || c == '\f' || c == '\r' || c == '\n');
}

//===================================================================
static int iseol( char c )
{
	return (c == '\f' || c == '\r' || c == '\n');
}

//===================================================================
// not safe
static void get_name_and_value( char *line, char *name, char *value )
{
	int		strsize;

	name[0] = value[0] = 0;

	strsize = 0;
	for (; *line && isblanko( *line ); ++line);

	if ( *line == '"' )	// in case of a string defined by a quotation (has spaces inside)
	{	// proceed to look for the closing quotation
		++line;
		for (; *line && *line != '"'; ++line)
			*name++ = *line;
		++line;
	}
	else
	{	// otherwise proceed to look for the whitespace
		for (; *line && !isblanko( *line ); ++line)
			*name++ = *line;
	}

	*name = 0;

	for (; *line && isblanko( *line ); ++line);

	for (; *line && !iseol( *line ); ++line)
		*value++ = *line;

	*value = 0;
}

//==================================================================
static char	_item_name[256];
static char	_item_value[1024];

//==================================================================
int config_parse_next( FILE *fp, char **namepp, char **valuepp )
{
char	*retstr;
char	buff[4096];

	while ( 1 )
	{
		retstr = fgets( buff, sizeof(buff), fp );
		if NOT( retstr )
		{
			if ( feof(fp) )
			{
				*namepp = NULL;
				*valuepp = NULL;
				return 0;
			}
			else
			{
				*namepp = NULL;
				*valuepp = NULL;

				return -1;
			}
		}
		else
		if ( buff[0] != '#' )
		{
			// not safe
			get_name_and_value( buff, _item_name, _item_value );
			*namepp = _item_name;
			*valuepp = _item_value;
			return 0;
		}
	}

	return 0;
}

//==================================================================
char *dlgmk_item_c::strcpyalloc( const char *srcp )
{
char	*d;

	int	len = strlen( srcp );
	d = new	char [ len + 1 ];
	strcpy( d, srcp );
	return d;
}

//==================================================================
static int get_nibble( char ch )
{
	if ( ch >= '0' && ch <= '9' )
		return (int)ch - '0';
	
	if ( ch >= 'a' && ch <= 'f' )
		return (int)ch - 'a' + 10;

	if ( ch >= 'A' && ch <= 'F' )
		return (int)ch - 'A' + 10;

	PSYS_ASSERT( 0 );
	return 255;
}

//==================================================================
static u_char get_hexbyte( const char *strp )
{
	return (get_nibble( strp[0] ) << 4) |
		   (get_nibble( strp[1] ) << 0);
}

//==================================================================
static PError SHA1_from_hexstring( u_char sha1hash[20], const char *strp )
{
	int	len = strlen(strp);
	if ( len != 20*2 )
		return PERROR;

	for (int i=0; i < 20; ++i)
	{
		sha1hash[i] = get_hexbyte( &strp[i*2] );
	}

	return POK;
}

//==================================================================
static void hexstring_from_SHA1( char *strp, const u_char sha1hash[20] )
{
	for (int i=0; i < 20; ++i)
	{
		sprintf( strp+i*2, "%02x", sha1hash[i] );
	}
	strp[20*2] = 0;
}

//==================================================================
void dlgmk_item_c::LoadFromString( const char *strp )
{
	switch ( _dtype )
	{
	case DTYPE_STRING:
		psys_strcpy( (char *)_datap, strp, _max_str_size );
		break;

	case DTYPE_SHA1HASH:
		SHA1_from_hexstring( (u_char *)_datap, strp );
		break;

	case DTYPE_INT:
		*(int *)_datap = atoi( strp );
		PCLAMP( *(int *)_datap, _i_min, _i_max );
		break;

	case DTYPE_ULONG:
		*(u_long *)_datap = atoi( strp );
		PCLAMP( *(u_long *)_datap, _ul_min, _ul_max );
		break;

	case DTYPE_FLOAT:
		*(float *)_datap = atof( strp );
		PCLAMP( *(float *)_datap, _f_min, _f_max );
		break;

	case DTYPE_BOOLEAN:
		*(bool *)_datap = (atoi( strp ) ? true : false);
		break;
	}
}
//==================================================================
const char *dlgmk_item_c::StoreToString( char *outstrp )
{
	switch ( _dtype )
	{
	case DTYPE_STRING:		return (const char *)_datap;							break;
	case DTYPE_SHA1HASH:	hexstring_from_SHA1( outstrp, (const u_char *)_datap );	break;
	case DTYPE_INT:			sprintf( outstrp, "%i", *(int *)_datap );				break;
	case DTYPE_ULONG:		sprintf( outstrp, "%lu", *(u_long *)_datap );			break;
	case DTYPE_FLOAT:		sprintf( outstrp, "%g", *(float *)_datap );				break;
	case DTYPE_BOOLEAN:		sprintf( outstrp, "%i", *(bool *)_datap ? 1 : 0 );		break;
	}

	return outstrp;
}

//==================================================================
DataSchema::DataSchema( const char *stridp )
{
	memset( this, 0, sizeof(*this) );

	_stridp	 = stridp;
}

//==================================================================
DataSchema::~DataSchema()
{
	for (int i=0; i < _itemsp_list.len(); ++i)
		delete _itemsp_list[i];
}

//==================================================================
dlgmk_item_c *DataSchema::addItem( dlgmk_item_c *itemp )
{
	_itemsp_list.append( itemp );

	return itemp;
}

//==================================================================
void DataSchema::AddString( const char *stridp, char *datap, int strsize )
{
	addItem( new dlgmk_item_c( stridp, datap, strsize ) );
}
//==================================================================
void DataSchema::AddSHA1Hash( const char *stridp, sha1_t *sha1hashp )
{
	addItem( new dlgmk_item_c( stridp, sha1hashp ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddInt( const char *stridp, int *datap, int i_min, int i_max )
{
	addItem( new dlgmk_item_c( stridp, datap, i_min, i_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddULong( const char *stridp, u_long *datap, u_long ul_min, u_long ul_max )
{
	addItem( new dlgmk_item_c( stridp, datap, ul_min, ul_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddFloat( const char *stridp, float *datap, float f_min, float f_max )
{
	addItem( new dlgmk_item_c( stridp, datap, f_min, f_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddBool( const char *stridp, bool *datap )
{
	addItem( new dlgmk_item_c( stridp, datap ) );
}

//==================================================================
static int mymax( int a, int b )
{
	return a > b ? a : b;
}
//==================================================================
static int make_blank_safe_string( char *desp, const char *srcp, int dmax )
{
bool	has_blanks;

	has_blanks = false;
	for (int i=0; i < dmax-3; ++i)
	{
		// we copy just in case it doesn't have blanks 8)
		if NOT( desp[i] = srcp[i] )
		{
			if ( has_blanks )
			{
				*desp++ = '"';
				for (int j=0; j < i; ++j)
					*desp++ = *srcp++;
				*desp++ = '"';
				*desp = 0;
			}
			return 0;
		}

		if ( isblanko( srcp[i] ) )
		{
			has_blanks = true;
			// don't break keep going, we need to know the size
		}
	}

	return -1;
}

//==================================================================
PError DataSchema::SaveData( FILE *fp )
{
	if ( fprintf( fp, "@BEGIN %s\n", _stridp ) < 0 )
		return PERROR;

	for (int i=0; i < _itemsp_list.len(); ++i)
	{
	dlgmk_item_c	*itemp = _itemsp_list[i];
	char			buff[256];
	char			buff2[1024];
	
		int err = make_blank_safe_string( buff, itemp->_stridp, sizeof(buff) );
		PSYS_ASSERT( err == 0 );
		if ( err )
			return PERROR;

		if ( fprintf( fp, "%s\t\t%s\n", buff, itemp->StoreToString( buff2 ) ) < 0 )
			return PERROR;
	}

	if ( fprintf( fp, "@END %s\n", _stridp ) < 0 )
		return PERROR;

	return POK;
}

//==================================================================
dlgmk_item_c *DataSchema::item_find_by_strid( const char *stridp )
{
	for (int i=0; i < _itemsp_list.len(); ++i)
	{
		if ( !stricmp( stridp, _itemsp_list[i]->_stridp ) )
			return _itemsp_list[i];
	}

	return NULL;
}

//==================================================================
int DataSchema::LoadData( FILE *fp )
{
char			*namep, *valuep;
dlgmk_item_c	*itemp;

	while NOT( config_parse_next( fp, &namep, &valuep ) )
	{
		if NOT( namep )
			return 0;

		if ( valuep )
		{
			if ( !stricmp( namep, "@begin" ) )
				continue;

			if ( !stricmp( namep, "@end" ) )
				return 0;

			if NOT( itemp = item_find_by_strid( namep ) )
			{
				PSYS_ASSERT( 0 );
			}
			else
			{
				itemp->LoadFromString( valuep );
			}
		}
	}

	return -1;
}
