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

#ifndef DSHARINGU_PROTOCOL_H
#define DSHARINGU_PROTOCOL_H

//==================================================================
#define DEF_PORT_NUMBER		51112
#define DEF_PORT_NUMBER_STR	"51112"
#define PROTOCOL_VERSION	2

enum {
	TEXT_MSG_PKID = 1,
	DESK_IMG_PKID,

	HANDSHAKE_PKID,
	ACCEPTING_DESK_PKID,
	BAD_PASSWORD_PKID,

	REMOCON_ARRAY_PKID,
};

//==================================================================
struct HandShakeMsg
{
	u_int	_protocol_version;

	HandShakeMsg( int protocol_version ) :
		_protocol_version(protocol_version)
	{
	}
};

//==================================================================
struct SettingMsg
{
	bool	_show_my_screen;
	bool	_share_my_screen;
	bool	_see_remote_screen;
	u_char	_remote_access_pw[20];

	SettingMsg( bool give_local_view, bool give_local_share, bool want_remote_view, const u_char remote_access_pw[20] ) :
		_show_my_screen(give_local_view),
		_share_my_screen(give_local_share),
		_see_remote_screen(want_remote_view)
	{
		memcpy( _remote_access_pw, remote_access_pw, 20 );
	}
};

//==================================================================
//==================================================================
//==================================================================
struct RemoConMsg
{
	static const int	TYPE_MOUSEMOVE	 = 1;
	static const int	TYPE_MOUSEBUTTON = 2;
	static const int	TYPE_VKEY		 = 3;

	static const int	BUTT_ID_LEFT	= 0;
	static const int	BUTT_ID_RIGHT	= 1;
	static const int	BUTT_ID_MID		= 2;

	u_char	_type;

	union {
		struct {
			u_short	_pos_x;
			u_short	_pos_y;
		} mmove;

		struct {
			u_char	_butt_id  : 4;
			u_char	_butt_updown : 4;
			u_short	_pos_x;
			u_short	_pos_y;
		} mbutt;

		struct {
			u_char	_key_updown;
			u_short	_vkey;
		} key;
	};

	u_int	_time_delta;

	void SetMouseMove( u_short pos_x, u_short pos_y, u_int time_delta )
	{
		_type = TYPE_MOUSEMOVE;
		mmove._pos_x = pos_x;
		mmove._pos_y = pos_y;

		_time_delta = time_delta;
	}

	void SetMouseButt( u_char butt_id, bool butt_updown, u_short pos_x, u_short pos_y, u_int time_delta )
	{
		_type = TYPE_MOUSEBUTTON;
		mbutt._butt_id = butt_id;
		mbutt._butt_updown = butt_updown ? 1 : 0;
		mbutt._pos_x = pos_x;
		mbutt._pos_y = pos_y;

		_time_delta = time_delta;
	}

	void SetVKey( u_short vkey, bool key_updown, u_int time_delta )
	{
		_type = TYPE_VKEY;
		key._vkey = vkey;
		key._key_updown = key_updown ? 1 : 0;

		_time_delta = time_delta;
	}
};

#endif