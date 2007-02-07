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
///
///
///
//==================================================================

#include "dstask.h"

//==================================================================
static void drawRect( const GXY::Rect &rect )
{
	float	w = rect._w;//-0.5f;
	float	h = rect._h;//-0.5f;

}

//==================================================================
///
//==================================================================
void DSTaskManager::Paint()
{
	float	win_w = _winp->GetWidth();
	float	win_h = _winp->GetHeight();

	float	w = 32;
	float	h = 32;

	float	x = 10;
	float	y = win_h - w - 10;

/*
	glDisable( GL_TEXTURE_2D );
	glPushMatrix();
	glLoadIdentity();

	for (int i=0; i < _tasks.len(); ++i)
	{
		glBegin( GL_QUADS );
		glColor4f( 1, 0, 0, 1 ); glVertex2f( x + 0, y + h );
		glColor4f( 1, 0, 0, 1 ); glVertex2f( x + w, y + h );
		glColor4f( 0.5f, 0, 0, 1 ); glVertex2f( x + w, y + 0 );
		glColor4f( 00.5, 0, 0, 1 ); glVertex2f( x + 0, y + 0 );
		glEnd();
		x += w + 4;
	}

	glPopMatrix();
*/
}

//==================================================================
void DSTaskManager::OnWinResize()
{
	float	win_w = _winp->GetWidth();
	float	win_h = _winp->GetHeight();

	float	w = 60;
	float	h = 32;

	float	x = 10;
	float	y = win_h - h - 4;

	GGET_Manager	&gam = _winp->GetGGETManager();

	for (int i=0; i < _tasks.len(); ++i)
	{
		GGET_Item	*ggetp = gam.FindGadget( _tasks[i]->_butt_id );
		ggetp->SetRect( x, y, w, h );
/*
		glBegin( GL_QUADS );
		glColor4f( 1, 0, 0, 1 ); glVertex2f( x + 0, y + h );
		glColor4f( 1, 0, 0, 1 ); glVertex2f( x + w, y + h );
		glColor4f( 0.5f, 0, 0, 1 ); glVertex2f( x + w, y + 0 );
		glColor4f( 00.5, 0, 0, 1 ); glVertex2f( x + 0, y + 0 );
		glEnd();
*/
		x += w + 4;
	}
}

//==================================================================
void DSTaskManager::updateViewState( DSTask *taskp )
{
	GGET_Manager	&gam = _winp->GetGGETManager();
	GGET_Item		*ggetp = gam.FindGadget( taskp->_butt_id );

	switch ( taskp->_view_state )
	{
	case DSTask::VS_FITVIEW:
		ggetp->SetIcon( GGET_Item::STD_ICO_MARK_ON );
		break;

	case DSTask::VS_ICONIZED:
		ggetp->SetIcon( GGET_Item::STD_ICO_MARK_OFF );
		break;
	}
}

//==================================================================
void DSTaskManager::AddTask( const char *task_namep, u_int task_butt_id, DSTask::ViewState init_view_state )
{
	int	i = _tasks.len();

	GGET_Manager	&gam = _winp->GetGGETManager();

	gam.AddButton( task_butt_id, 0, 0, 100, 100, task_namep );

	DSTask	*taskp = new DSTask( this, task_namep, task_butt_id );
	taskp->_view_state = init_view_state;
	_tasks.push_back( taskp );
	updateViewState( taskp );
	_dstaskCallBack( _cb_userdatap, taskp, taskp->GetViewState() );
}

//==================================================================
bool DSTaskManager::OnGadget( int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	for (int i=0; i < _tasks.len(); ++i)
	{
		DSTask	*taskp = _tasks[i];

		if ( taskp->_butt_id == gget_id )
		{
			switch ( taskp->_view_state )
			{
			case DSTask::VS_FITVIEW:
				taskp->_view_state = DSTask::VS_ICONIZED;
				break;

			case DSTask::VS_ICONIZED:
				taskp->_view_state = DSTask::VS_FITVIEW;
				break;
			}

			updateViewState( taskp );

			_dstaskCallBack( _cb_userdatap, taskp, taskp->_view_state );
			return true;
		}
	}

	return false;
}

//==================================================================
DSTask	*DSTaskManager::FindByButtID( u_int butt_id )
{
	for (int i=0; i < _tasks.len(); ++i)
	{
		if ( _tasks[i]->_butt_id == butt_id )
		{
			return _tasks[i];
		}
	}

	return NULL;
}

//==================================================================
void DSTaskManager::Show( bool onoff )
{
	GGET_Manager	&gam = _winp->GetGGETManager();

	_is_showing = onoff;

	for (int i=0; i < _tasks.len(); ++i)
	{
		GGET_Item	*ggetp = gam.FindGadget( _tasks[i]->_butt_id );
		ggetp->Show( onoff );
	}
}

//==================================================================
void DSTask::SetViewState( ViewState view_state )
{
	_view_state = view_state;
	_managerp->updateViewState( this );
}