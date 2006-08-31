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
char *ItemBase::strcpyalloc( const char *srcp )
{
	char	*d;

	int	len = strlen( srcp );
	d = new	char [ len + 1 ];
	strcpy( d, srcp );
	return d;
}

//==================================================================
bool config_parse_next( FILE *fp, char *namep, char *valuep )
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
				return true;
			}
			else
			{
				//throw "cfg file error";
				return true;
			}
		}
		else
		if ( buff[0] != '#' )
		{
			// not safe
			get_name_and_value( buff, namep, valuep );
			return false;
		}
	}

	return false;
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
void ItemSHA1Hash::LoadFromString( const char *strp )
{
	SHA1_from_hexstring( (u_char *)_datap, strp );
}

//==================================================================
const char *ItemSHA1Hash::StoreToString( char *outstrp )
{
	hexstring_from_SHA1( outstrp, (const u_char *)_datap );
	return outstrp;
}

//==================================================================
DataSchema::DataSchema( const char *stridp ) :
	_stridp(stridp)
{
}

//==================================================================
DataSchema::~DataSchema()
{
}

//==================================================================
void DataSchema::AddString( const char *stridp, char *datap, int strsize )
{
	_itemsp_list.append( new ItemString( stridp, datap, strsize ) );
}
//==================================================================
void DataSchema::AddSHA1Hash( const char *stridp, sha1_t *sha1hashp )
{
	_itemsp_list.append( new ItemSHA1Hash( stridp, sha1hashp ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddInt( const char *stridp, int *datap, int i_min, int i_max )
{
	_itemsp_list.append( new ItemInt( stridp, datap, i_min, i_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddULong( const char *stridp, u_long *datap, u_long ul_min, u_long ul_max )
{
	_itemsp_list.append( new ItemULong( stridp, datap, ul_min, ul_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddFloat( const char *stridp, float *datap, float f_min, float f_max )
{
	_itemsp_list.append( new ItemFloat( stridp, datap, f_min, f_max ) );
}
//---------------------------------------------------------------------------
void DataSchema::AddBool( const char *stridp, bool *datap )
{
	_itemsp_list.append( new ItemBool( stridp, datap ) );
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
	ItemBase	*itemp = _itemsp_list[i];
	char		buff[256];
	char		buff2[1024];
	
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
ItemBase *DataSchema::item_find_by_strid( const char *stridp )
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
ItemBase	*itemp;

	char	name[256];
	char	value[1024];

	while NOT( config_parse_next( fp, name, value ) )
	{
		if ( value[0] )
		{
			if ( !stricmp( name, "@begin" ) )
				continue;

			if ( !stricmp( name, "@end" ) )
				return 0;

			if NOT( itemp = item_find_by_strid( name ) )
			{
				PSYS_ASSERT( 0 );
			}
			else
			{
				itemp->LoadFromString( value );
			}
		}
	}

	return -1;
}
