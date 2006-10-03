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

#ifndef DSCHANNEL_MANAGER_H
#define DSCHANNEL_MANAGER_H

#include "kwindow.h"
#include "dsinstance.h"
#include "dschannel.h"

//==================================================================
///
//==================================================================
class ChannelManager
{
public:
	static const int	MAX_CHANNELS = 16;
	DSChannel			*_channelsp[ MAX_CHANNELS ];
	int					_n_channels;
	DSChannel			*_cur_chanp;

private:
	void				*_userdatap;
	void				(*_onChannelSwitchCB)( void *userdatap, DSChannel *chanp );
	win_t				*_parent_winp;
	win_t				*_tabs_winp;

public:
	//==================================================================
	ChannelManager( win_t *parent_winp, void *userdatap/*,
					void (*onChannelSwitchCB)( void *userdatap, DSChannel *chanp )*/ );

	DSChannel	*NewChannel( RemoteDef *remotep );
	DSChannel	*NewChannel( int accepted_fd );
	DSChannel	*GetCurChannel()
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
