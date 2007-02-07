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
//==================================================================

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <winuser.h>
#include "interactsys.h"
#include "memfile.h"

//==================================================================
using namespace PUtils;

//==================================================================
void InteractiveSystem::FeedMessage( u_int message, u_int lparam, u_int wparam,
									 int disp_off_x, 
									 int disp_off_y,
									 float scale_x,
									 float scale_y )
{
	PSYS_ASSERT( _is_active );
	PSYS_ASSERT( scale_x > 0 );
	PSYS_ASSERT( scale_y > 0 );

	u_int	now_time = GetTickCount();

	int	diff_time;
	
	if ( _last_feed_time == 0 )
		diff_time = 0;
	else
		diff_time = now_time - _last_feed_time;

	_last_feed_time = now_time;

	RemoConMsg	remconmsg;

	switch ( message )
	{
	case WM_MOUSEMOVE:
		{
		/*
		PSYS_DEBUG_PRINTF( "%g %g\n", 
			(LOWORD(lparam) - disp_off_x) / scale_x,
			(HIWORD(lparam) - disp_off_y) / scale_y );
		*/

		remconmsg.SetMouseMove( (LOWORD(lparam) - disp_off_x) / scale_x,
							    (HIWORD(lparam) - disp_off_y) / scale_y,
							    diff_time );

		_remocon_queue.push_back( remconmsg );
		}
		break;

	case WM_LBUTTONUP:	
	case WM_LBUTTONDOWN:
	case WM_RBUTTONUP:	
	case WM_RBUTTONDOWN:
	case WM_MBUTTONUP:	
	case WM_MBUTTONDOWN:
		{
			int		butt_id;
			bool	butt_updown;

			switch ( message )
			{
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
				butt_id = RemoConMsg::BUTT_ID_LEFT;
				break;
			case WM_RBUTTONUP:
			case WM_RBUTTONDOWN:
				butt_id = RemoConMsg::BUTT_ID_RIGHT;
				break;
			case WM_MBUTTONUP:
			case WM_MBUTTONDOWN:
				butt_id = RemoConMsg::BUTT_ID_MID;
				break;
			}

			switch ( message )
			{
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
				butt_updown = 0;
				break;
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				butt_updown = 1;
				break;
			}


			remconmsg.SetMouseButt( butt_id,
								butt_updown,
								(LOWORD(lparam) - disp_off_x) / scale_x,
								(HIWORD(lparam) - disp_off_y) / scale_y,
								diff_time );

			_remocon_queue.push_back( remconmsg );
		}
		break;
	}
}

//==================================================================
void InteractiveSystem::Idle()
{
	u_int	now_time = GetTickCount();

	if ( now_time - _last_send_time >= 1000/8 && _remocon_queue.len() )
	{
		PSYS_ASSERT( _remocon_queue.len() > 0 && _remocon_queue.len() < 256 );

		RemoConMsg	buff[256+1];
		Memfile	memf( buff, sizeof(buff) );

		memf.WriteInt( _remocon_queue.len() );
		memf.WriteData( &_remocon_queue[0], sizeof(_remocon_queue[0]) * _remocon_queue.len() );

		_cpkp->SendPacket( REMOCON_ARRAY_PKID, (void *)memf._datap, memf.GetCurPos(), NULL );

		_remocon_queue.clear();

		_last_send_time = now_time;
	}
}
/*
//==================================================================
void InteractiveSystem::onMousePos( int px, int py )
{
	if NOT( _test_win_created )
	{
		PSYS_ZEROMEM( &_test_win );
		win_init_quick( &_test_win, "pointer !", NULL, NULL, NULL,
						WIN_ANCH_TYPE_FIXED, px,
						WIN_ANCH_TYPE_FIXED, py,
						WIN_ANCH_TYPE_THIS_X1, 16,
						WIN_ANCH_TYPE_THIS_Y1, 16,
						(win_init_flags)0 );

		_test_win_created = true;
	}

	win_move( &_test_win, px, py );
}
*/
/*
	void onMousePos( INPUT *out_inputp, const RemoConMsg &message );
	void onMouseButton( INPUT *out_inputp, const RemoConMsg &message );
*/
//==================================================================
void onMousePos( INPUT *out_inputp, const RemoConMsg &message, int disp_wd, int disp_he )
{
	PSYS_ASSERT( message._type == RemoConMsg::TYPE_MOUSEMOVE );

	out_inputp->type = INPUT_MOUSE;

	out_inputp->mi.dx = message.mmove._pos_x * 65535 / disp_wd;
	out_inputp->mi.dy = message.mmove._pos_y * 65535 / disp_he;
	out_inputp->mi.time = 0;
	out_inputp->mi.mouseData = 0;
	out_inputp->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	out_inputp->mi.dwExtraInfo = NULL;
}

//==================================================================
void onMouseButton( INPUT *out_inputp, const RemoConMsg &message, int disp_wd, int disp_he )
{
	PSYS_ASSERT( message._type == RemoConMsg::TYPE_MOUSEBUTTON );

	out_inputp->type = INPUT_MOUSE;

	out_inputp->mi.dx = message.mbutt._pos_x * 65535 / disp_wd;
	out_inputp->mi.dy = message.mbutt._pos_y * 65535 / disp_he;
	out_inputp->mi.time = 0;
	out_inputp->mi.mouseData = 0;
	out_inputp->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	out_inputp->mi.dwExtraInfo = NULL;

	switch ( message.mbutt._butt_id )
	{
	case RemoConMsg::BUTT_ID_LEFT:
		if ( message.mbutt._butt_updown == 0 )
			out_inputp->mi.dwFlags |= MOUSEEVENTF_LEFTUP;
		else
			out_inputp->mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
		break;
	case RemoConMsg::BUTT_ID_RIGHT:
		if ( message.mbutt._butt_updown == 0 )
			out_inputp->mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
		else
			out_inputp->mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
		break;
	case RemoConMsg::BUTT_ID_MID:
		if ( message.mbutt._butt_updown == 0 )
			out_inputp->mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
		else
			out_inputp->mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
		break;
	}
}

//==================================================================
bool InteractiveSystem::ProcessMessage_s( u_int pack_id, const void *datap, int disp_wd, int disp_he )
{
	switch ( pack_id )
	{
	case REMOCON_ARRAY_PKID:
		{
		INPUT				inputs[256];
		int					n_inputs;
		const RemoConMsg	*msg_arrayp;

			n_inputs = *(int *)datap;
			msg_arrayp = (RemoConMsg *)((int *)datap+1);

			if ( n_inputs > 256 )
			{
				PSYS_ASSERT( 0 );
				return true;
			}

			for (int i=0; i < n_inputs; ++i)
			{
				switch ( msg_arrayp[i]._type )
				{
				case RemoConMsg::TYPE_MOUSEBUTTON:
					onMouseButton( &inputs[i], msg_arrayp[i], disp_wd, disp_he );
					break;

				case RemoConMsg::TYPE_MOUSEMOVE:
					onMousePos( &inputs[i], msg_arrayp[i], disp_wd, disp_he );
					break;
				}
			}

			SendInput( n_inputs, inputs, sizeof(inputs[0]) );
		}
		return true;
	}

	return false;
}

//==================================================================
void InteractiveSystem::OnPackCallback_s( const void *datap, u_int data_size, void *userdatap )
{
	if ( ((InteractiveSystem *)userdatap)->_is_input_active )
		ProcessMessage_s( REMOCON_ARRAY_PKID, datap, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) );
}
