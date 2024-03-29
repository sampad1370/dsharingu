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
///
//==================================================================

#ifndef DSHARINGU_PROTOCOL_H
#define DSHARINGU_PROTOCOL_H

#include "pnetlib_compak.h"

//==================================================================
#define DEF_PORT_NUMBER		51112
#define DEF_PORT_NUMBER_STR	"51112"
#define PROTOCOL_VERSION	7

//==================================================================
template <class MSG>
int NetSendMessage( Compak &cpk, const MSG &msg, u_int *send_ticketp=NULL )
{
	return cpk.SendPacket( MSG._msg_id, (void *)&msg, sizeof(msg), send_ticketp );
}

//==================================================================
enum {
	TEXT_MSG_PKID = 1,
	DESK_IMG_PKID,

	HandShakeMsg_ID,
	HS_BAD_USERNAME_PKID,
	HS_BAD_PASSWORD_PKID,
	HS_OLD_PROTOCOL_PKID,
	HS_NEW_PROTOCOL_PKID,
	HS_OK,

	UsageWishMsg_ID,
	UsageAbilityMsg_ID,

	REMOCON_ARRAY_PKID,
};

//==================================================================
struct HandShakeMsg
{
	const static u_int	_msg_id = HandShakeMsg_ID;

	u_int	_protocol_version;
	TCHAR	_caller_username[32];
	TCHAR	_communicating_username[32];
	u_char	_communicating_password[20];

	HandShakeMsg(	u_int protocol_version,
					const TCHAR caller_username[32],
					const TCHAR communicating_username[32],
					const u_char communicating_password[20] ) :
		_protocol_version(protocol_version)
	{
		memcpy( _caller_username, caller_username, 32 );
		memcpy( _communicating_username, communicating_username, 32 );
		memcpy( _communicating_password, communicating_password, 20 );
	}
};

//==================================================================
struct UsageWishMsg
{
	const static u_int	_msg_id = UsageWishMsg_ID;

	bool	_see_remote_screen;
	bool	_use_remote_screen;
	bool	_is_watching;

	UsageWishMsg(	bool see_remote_screen,
					bool use_remote_screen,
					bool is_watching ) :
		_see_remote_screen(see_remote_screen),
		_use_remote_screen(use_remote_screen),
		_is_watching(is_watching)
	{
	}
};

//==================================================================
struct UsageAbilityMsg
{
	const static u_int	_msg_id = UsageAbilityMsg_ID;

	bool	_see_remote_screen;
	bool	_use_remote_screen;

	UsageAbilityMsg(	bool see_remote_screen,
						bool use_remote_screen ) :
			_see_remote_screen(see_remote_screen),
			_use_remote_screen(use_remote_screen)
	{
	}
};

//==================================================================
//==================================================================
//==================================================================
struct RemoConMsg
{
	//const static u_int	_msg_id = RemoConMsg_ID;

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

	RemoConMsg(){}

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