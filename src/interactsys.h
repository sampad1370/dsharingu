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
//==================================================================

#ifndef INTERACTIVE_SYSTEM_H
#define INTERACTIVE_SYSTEM_H

#include "psys.h"
#include "pwindow.h"
#include "compak.h"
#include "dsharingu_protocol.h"

//==================================================================
class InteractiveSystem
{
	Compak				*_cpkp;
	bool				_is_active;
	bool				_is_input_active;
	u_int				_last_feed_time;
	u_int				_last_send_time;

	PArray<RemoConMsg>	_remocon_queue;

	win_t				_test_win;
	bool				_test_win_created;

public:
	InteractiveSystem( Compak *cpkp ) :
		_cpkp(cpkp),
		_is_active(false),
		_test_win_created(false)
	{
		RestartFeed();
	}

	~InteractiveSystem()
	{
	}

	void FeedMessage( u_int message, u_int lparam, u_int wparam,
					  int disp_off_x, 
					  int disp_off_y,
					  float scale_x,
					  float scale_y );

	static bool ProcessMessage_s( u_int pack_id, const void *datap, int disp_wd, int disp_he );
	static void OnPackCallback_s( const void *datap, u_int data_size, void *userdatap );
	void Idle();

	void RestartFeed()
	{
		_last_feed_time = 0;
		_last_send_time = 0;
		_remocon_queue.clear();
	}

	void Activate( bool onoff )
	{
		_is_active = onoff;
		RestartFeed();
	}

	bool IsActive() const
	{
		return _is_active;
	}

	void ActivateExternalInput( bool onoff )
	{
		_is_input_active = onoff;
	}

private:
};

#endif