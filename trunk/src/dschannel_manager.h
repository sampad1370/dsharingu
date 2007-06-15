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

#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include "pwindow.h"
#include "dsinstance.h"
#include "dschannel.h"

//==================================================================
///
//==================================================================
class DSChannelManager
{
	friend class DSChannel;

public:
	typedef void		(*OnChannelSwitchCBType)( DSharinguApp *superp, DSChannel *new_sel_chanp, DSChannel *old_sel_chanp );
	typedef void		(*OnChannelDeleteCBType)( DSharinguApp *superp, DSChannel *chanp );

public:
	static const int		MAX_CHANNELS = 16;
	DSChannel				*_channelsp[ MAX_CHANNELS ];
	int						_n_channels;
	DSChannel				*_cur_chanp;

	DSharinguApp			*_superp;
private:
	OnChannelSwitchCBType	_onChannelSwitchCB;
	OnChannelDeleteCBType	_onChannelDeleteCBType;
	Window					*_parent_winp;
	Window					*_tabs_winp;

public:
	//==================================================================
	DSChannelManager( Window *parent_winp, DSharinguApp *superp,
					  OnChannelSwitchCBType onChannelSwitchCB,
					  OnChannelDeleteCBType onChannelDeleteCBType );

	DSChannel	*RecycleOrNewChannel( RemoteDef *remotep, bool call_silent );
	DSChannel	*NewChannel( int accepted_fd );
	DSChannel	*FindChannelByRemote( const RemoteDef *remotep );
	DSChannel	*GetCurChannel()
	{
		return	_cur_chanp;
	}
	void		SetChannelName( DSChannel *chanp, const TCHAR *namep );
	void		SetChannelTabIcon( DSChannel *chanp, GGET_Item::StdIcon std_icon );

	void		Idle();
	void		Quit();

	void		RemoveChannel( DSChannel *chanp );

	void		AddChannelToList( DSChannel *chanp, const TCHAR *namep );

	u_int		GetTabsWinHeight() const;

private:
	static void	gadgetCallback_s( void *superp, int gget_id, GGET_Item *itemp, GGET_CB_Action action );
	void		gadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action  );

	void		addTab( int idx, const TCHAR *namep );
	void		toggleOne( GGET_Manager &gam, int gget_id );
	
	static int	eventFilter_s( void *userobjp, WindowEvent *eventp );
	int			eventFilter( WindowEvent *eventp );
	
	int			findChannelIndex( DSChannel *chanp );
};

#endif
