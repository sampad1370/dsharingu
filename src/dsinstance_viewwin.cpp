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
///
//==================================================================

#include <windows.h>
#include <CommCtrl.h>
#include <direct.h>
#include <gl/glew.h>
#include "dsinstance.h"
#include "data_schema.h"
#include "appbase3.h"

//==================================================================
static void drawRect( float x1, float y1, float w, float h )
{
	float	x2 = x1 + w;
	float	y2 = y1 + h;

	glVertex2f( x1, y1 );
	glVertex2f( x2, y1 );
	glVertex2f( x2, y2 );
	glVertex2f( x1, y2 );
}

//==================================================================
const static int FRAME_SIZE = 8;
const static int ARROW_SIZE = 32;

//==================================================================
static void drawFrame( float w, float h )
{
	glBegin( GL_QUADS );

		drawRect( 0, 0,  w, FRAME_SIZE );
		drawRect( 0, h-FRAME_SIZE,  w, FRAME_SIZE );

		drawRect( 0, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );
		drawRect( w-FRAME_SIZE, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );

	glEnd();
}

//==================================================================
#define DIR_LEFT	1
#define DIR_RIGHT	2
#define DIR_TOP		4
#define DIR_BOTTOM	8

//==================================================================
static void isPointerInArrows( float w, float h, u_int dirs )
{
}

//==================================================================
static void getArrowVerts( float verts[3][2], float w, float h, u_int dir )
{
	float x1, x2, y1, y2, xh, yh;

	if ( dir & (DIR_LEFT|DIR_RIGHT) )	// L/R
	{
		if ( dir == DIR_LEFT )
		{
			x1 = FRAME_SIZE + FRAME_SIZE/2;
			xh = x1 + ARROW_SIZE/2;
		}
		else
		{
			x1 = w - (FRAME_SIZE + FRAME_SIZE/2);
			xh = x1 - ARROW_SIZE/2;
		}

		yh = h / 2;
		y1 = yh - ARROW_SIZE/2;
		y2 = yh + ARROW_SIZE/2;

		verts[0][0]	= x1;
		verts[0][1] = yh;
		verts[1][0]	= xh;
		verts[1][1] = y1;
		verts[2][0]	= xh;
		verts[2][1] = y2;
	}
	else
	if ( dir & (DIR_TOP|DIR_BOTTOM) )	// U/D
	{
		if ( dir == DIR_TOP )
		{
			y1 = FRAME_SIZE + FRAME_SIZE/2;
			yh = y1 + ARROW_SIZE/2;
		}
		else
		{
			y1 = h - (FRAME_SIZE + FRAME_SIZE/2);
			yh = y1 - ARROW_SIZE/2;
		}

		xh = w / 2;
		x1 = xh - ARROW_SIZE/2;
		x2 = xh + ARROW_SIZE/2;

		verts[0][0]	= xh;
		verts[0][1] = y1;
		verts[1][0]	= x2;
		verts[1][1] = yh;
		verts[2][0]	= x1;
		verts[2][1] = yh;
	}
}

//==================================================================
static void drawArrows_verts( float w, float h, u_int dirs, bool lines_loop )
{
	for (int i=0; i < 4; ++i)
	{
		if ( dirs & (1<<i) )
		{
			float	verts[3][2];

			getArrowVerts( verts, w, h, 1 << i );
			if ( lines_loop )
			{
				glVertex2fv( verts[2] );
				glVertex2fv( verts[0] );
				for (int j=1; j < 3; ++j)
				{
					glVertex2fv( verts[j-1] );
					glVertex2fv( verts[j  ] );
				}
			}
			else
			{
				for (int j=0; j < 3; ++j)
					glVertex2fv( verts[j] );
			}
		}
	}
}

//==================================================================
static void drawArrows( float w, float h, u_int dirs )
{
	glColor4f( 1, .1f, .1f, .7f );
	glBegin( GL_TRIANGLES );
	drawArrows_verts( w, h, dirs, false );
	glEnd();
	
	glColor4f( 0, 0, 0, 1 );
	glBegin( GL_LINES );
	drawArrows_verts( w, h, dirs, true );
	glEnd();
}


//==================================================================
#define CURS_SIZE	16

//==================================================================
static void drawCursor( float px, float py )
{
	float	verts[3][2] = { px, py,
							px + CURS_SIZE*1.0f, py + CURS_SIZE*0.5f,
							px + CURS_SIZE*0.5f, py + CURS_SIZE*1.0f };

	glColor4f( 1, 1, 1, 1 );
	glBegin( GL_TRIANGLES );
		for (int i=0; i < 3; ++i)
			glVertex2fv( verts[i] );
	glEnd();

	glColor4f( 0, 0, 0, 1 );
	glBegin( GL_LINES );
		for (int i=0; i < 3; ++i)
		{
			glVertex2fv( verts[i] );
			glVertex2fv( verts[(i+1)%3] );
		}
	glEnd();
}


//==================================================================
void DSChannel::drawDispOffArrows()
{
	u_int	dirs = 0;

	if ( _disp_off_x < 0 )
		dirs |= DIR_LEFT;

	if ( _disp_off_y < 0 )
		dirs |= DIR_TOP;

	if ( _disp_off_x+_scrreader.GetWidth() >= _view_win.w )
		dirs |= DIR_RIGHT;

	if ( _disp_off_y+_scrreader.GetHeight() >= _view_win.h )
		dirs |= DIR_BOTTOM;

	glColor4f( 1.0f, 0.2f, 0.2f, 0.6f );
	drawArrows( _view_win.w, _view_win.h, dirs );
}

//==================================================================
void DSChannel::doViewPaint()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	begin_2D( NULL, NULL );
	//win_context_begin_ext( winp, 1, false );

		glPushMatrix();
		// render

		glColor3f( 1, 1, 1 );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glLoadIdentity();
			//glScalef( 0.5f, 0.5f, 1 );

			glPushMatrix();
			
			if NOT( _view_fitwindow )
				glTranslatef( _disp_off_x, _disp_off_y, 0 );

			_scrreader.RenderParsedFrame( _view_scale_x, _view_scale_y );
			glPopMatrix();

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			if ( _intersys.IsActive() )
			{
				glColor4f( 1.0f, 0.1f, 0.1f, 0.3f );
			}
			else
			{
				glColor4f( 0.3f, 0.3f, 0.3f, 0.3f );
			}

			//drawFrame( _view_win.w, _view_win.h );

			if NOT( _view_fitwindow )
				drawDispOffArrows();

			if ( _intersys.IsActive() )
			{
				drawCursor( _disp_curs_x, _disp_curs_y );
			}


			glDisable( GL_BLEND );

		glDisable( GL_CULL_FACE );
		glDisable( GL_LIGHTING );

		glPopMatrix();

	end_2D();
	//win_context_end( winp );
}
