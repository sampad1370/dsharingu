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

#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include "kwindow.h"
#include "dsinstance.h"
#include "dschannel.h"

//==================================================================
///
//==================================================================
class DSChannelManager
{
public:
	static const int	MAX_CHANNELS = 16;
	Channel				*_channelsp[ MAX_CHANNELS ];
	int					_n_channels;
	Channel				*_cur_chanp;

private:
	void				*_superp;
	void				(*_onChannelSwitchCB)( void *superp, Channel *chanp );
	win_t				*_parent_winp;
	win_t				*_tabs_winp;

public:
	//==================================================================
	DSChannelManager( win_t *parent_winp, void *superp,
					void (*onChannelSwitchCB)( void *superp, Channel *chanp ) );

	Channel	*NewChannel( RemoteDef *remotep );
	Channel	*NewChannel( int accepted_fd );
	Channel	*GetCurChannel()
	{
		return	_cur_chanp;
	}
	void		Idle();
	void		Quit();

private:
	static void	gadgetCallback_s( int gget_id, GGET_Item *itemp, void *userdatap );
	void		gadgetCallback( int gget_id, GGET_Item *itemp );
};

#endif
