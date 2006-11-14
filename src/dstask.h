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
//==================================================================

#ifndef DSTASK_H
#define DSTASK_H

#include <windows.h>
#include <gl/glew.h>
#include "console.h"
#include "gfxutils.h"
#include "gxy.h"

//==================================================================
///
//==================================================================
class DSTask
{
	friend class DSTaskManager;

public:
	enum ViewState
	{
		VS_ICONIZED,
		VS_FITVIEW,
		VS_EXACTVIEW,
	};

private:
	char		_name[64];
	u_int		_butt_id;

	DSTaskManager *_managerp;
	ViewState	_view_state;
	GXY::Rect	_rect;

public:
	DSTask( DSTaskManager *managerp, const char *task_namep, u_int task_butt_id ) :
		_managerp(managerp),
		_view_state(VS_ICONIZED),
		_rect(0,0,100,100)
	{
		psys_strcpy( _name, task_namep, sizeof(_name) );
		_butt_id = task_butt_id;
	}

	ViewState GetViewState() const
	{
		return _view_state;
	}

	void SetViewState( ViewState view_state );

	u_int	GetButtonID() const
	{
		return _butt_id;
	}
};

//==================================================================
///
//==================================================================
class DSTaskManager
{
	friend class DSTask;

	Window				*_winp;
	PArray<DSTask *>	_tasks;
	void				*_cb_userdatap;
	bool				_is_showing;
	void				(*_dstaskCallBack)( void *cb_userdatap, DSTask *taskp, DSTask::ViewState view_state );

public:
	DSTaskManager(	Window *winp,
					void *cb_userdatap,
					void (*dstaskCallBack)( void *cb_userdatap, DSTask *taskp, DSTask::ViewState view_state ) ) :
		_winp(winp),
		_cb_userdatap(cb_userdatap),
		_is_showing(false),
		_dstaskCallBack(dstaskCallBack)
	{
	}

	~DSTaskManager()
	{
	}

	DSTask	*FindByButtID( u_int butt_id );
	void	AddTask( const char *task_namep, u_int task_butt_id, DSTask::ViewState init_view_state );
	void	OnWinResize();
	bool	OnGadget( int gget_id, GGET_Item *itemp, GGET_CB_Action action );
	void	Paint();
	void	Show( bool onoff );

private:
	void	updateViewState( DSTask *taskp );
};

#endif
