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
//= Creation: Davide Pasca 2002
//=
//=
//=
//=
//==================================================================

#ifndef COM_BASEDEF_H
#define COM_BASEDEF_H

//==================================================================
enum
{
	COM_ERR_GENERIC = -1,
	COM_ERR_INVALID_ADDRESS = -2,
	COM_ERR_HARD_DISCONNECT = -3,
	COM_ERR_OUT_OF_MEMORY = -4,
	COM_ERR_NONE = 0,
	COM_ERR_SEND_QUEUE_FULL = 1,
	COM_ERR_GRACEFUL_DISCONNECT,
	COM_ERR_CONNECTED,
	COM_ERR_TIMEOUT_CONNECTING,
	COM_ERR_ALREADY_CONNECTED
};

#endif

