//==================================================================
//	Copyright (C) 2006-2007  Davide Pasca
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
#include <stdarg.h>
#include <gl/glew.h>
#include "psys.h"
#include "font.h"
#include "debugout.h"

//==================================================================
#define MAX_STRINGS	64
#define MAX_STRLEN	256

//==================================================================
static TCHAR	_strings[MAX_STRINGS][MAX_STRLEN];
static int		_n_strings;

//==================================================================
void debugout_reset()
{
	_n_strings = 0;
}

//==================================================================
void debugout_printf( const TCHAR *fmtp, ... )
{
va_list	va;

	if ( _n_strings >= MAX_STRINGS )
	{
		PASSERT( 0 );
		return;
	}

	va_start( va, fmtp );
		_vstprintf_s( _strings[ _n_strings ], fmtp, va );
	va_end( va );

	++_n_strings;
}

//==================================================================
void debugout_render()
{
	FONT_DrawBegin();

	int line_he = FONT_TextHeight();

	glColor4f( 0, 0, 0, 1 );
	glPushMatrix();
		glTranslatef( 4+1, line_he+1, 0 );
		for (int i=0; i < _n_strings; ++i)
		{
			FONT_puts( _strings[i] );
			glTranslatef( 0, line_he, 0 );
		}
	glPopMatrix();

	glColor4f( 0, 1, 0, 1 );
	glPushMatrix();
		glTranslatef( 4, line_he, 0 );
		for (int i=0; i < _n_strings; ++i)
		{
			FONT_puts( _strings[i] );
			glTranslatef( 0, line_he, 0 );
		}
	glPopMatrix();

	FONT_DrawEnd();

	_n_strings = 0;
}
