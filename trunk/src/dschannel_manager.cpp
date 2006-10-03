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

#include <windows.h>
#include <CommCtrl.h>
#include <direct.h>
#include <gl/glew.h>
#include "dsinstance.h"
#include "console.h"
#include "dsharingu_protocol.h"
#include "gfxutils.h"
#include "debugout.h"
#include "resource.h"
#include "SHA1.h"
#include "data_schema.h"
#include "appbase3.h"

//==================================================================
#define CHNTAG				"* "

//==================================================================
enum {
	TAB_HOME = 1,
	TAB_CH0 = 2,
	TAB_CHMAX = TAB_CH0 + 32,
};

//==================================================================
ChannelManager::ChannelManager( win_t *parent_winp, void *userdatap/*,
							    void (*onChannelSwitchCB)( void *userdatap, DSChannel *chanp )*/ ) :
	_parent_winp(parent_winp),
	_userdatap(userdatap),
	//_onChannelSwitchCB(onChannelSwitchCB),
	_tabs_winp(NULL),
	_n_channels(0),
	_cur_chanp(NULL)
{
	_tabs_winp = new win_t( "tabs", parent_winp, this, NULL,
							WIN_ANCH_TYPE_PARENT_X1, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 0,
							WIN_ANCH_TYPE_PARENT_X2, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 22,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR),
							0 );

	win_show( _tabs_winp, true );

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	gam.SetCallback( gadgetCallback_s, this );

	GGET_StaticText *stextp = gam.AddStaticText( DSharinguApp::STEXT_TOOLBARBASE, 0, 0, _tabs_winp->w, _tabs_winp->h, NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	float	x = 4;
	float	y = 3;
	float	w = 70;
	float	h = 19;
	float	y_margin = 4;
	float	x_margin = 4;

	gam.AddButton( TAB_HOME, x, y, w, h, "Home" );	x += w;// + x_margin;
	gam.AddButton( TAB_CH0, x, y, w, h, "Davide" );	x += x_margin;

	gam.SetToggled( TAB_HOME, true );
}

//==================================================================
void ChannelManager::gadgetCallback_s( int gget_id, GGET_Item *itemp, void *userdatap )
{
	((ChannelManager *)userdatap)->gadgetCallback( gget_id, itemp );
}
//==================================================================
void ChannelManager::gadgetCallback( int gget_id, GGET_Item *itemp )
{
	GGET_Manager	&gam = itemp->GetManager();

	for (int i=0; i < _n_channels; ++i)
		gam.SetToggled( TAB_CH0 + i, false );


	if ( gget_id == TAB_HOME || (gget_id >= TAB_CH0 && gget_id < TAB_CHMAX) )
	{
		gam.SetToggled( TAB_HOME, false );
		for (int i=0; i < _n_channels; ++i)
			gam.SetToggled( TAB_CH0 + i, false );

		gam.SetToggled( gget_id, true );

		if ( gget_id == TAB_HOME )
			_cur_chanp = NULL;
		else
			_cur_chanp = _channelsp[ gget_id - TAB_CH0 ];
	}
}

//==================================================================
DSChannel *ChannelManager::NewChannel( RemoteDef *remotep )
{
	if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
	{
		DSChannel	*chanp = new DSChannel( _userdatap, remotep );
		_channelsp[ _n_channels++ ] = chanp;
		_cur_chanp = chanp;
//		if ( _onChannelSwitchCB )
//			_onChannelSwitchCB( _userdatap, chanp );
		return chanp;
	}
	else
		return NULL;
}

//==================================================================
DSChannel *ChannelManager::NewChannel( int accepted_fd )
{
	if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
	{
		DSChannel	*chanp = new DSChannel( _userdatap, accepted_fd );
		_channelsp[ _n_channels++ ] = chanp;
		c = chanp;
//		if ( _onChannelSwitchCB )
//			_onChannelSwitchCB( _userdatap, chanp );
		return chanp;
	}
	else
		return NULL;
}

//==================================================================
void ChannelManager::Idle()
{
	for (int i=0; i < _n_channels; ++i)
	{
		DSChannel	*chanp = _channelsp[i];

		chanp->Idle();
	}
}

//==================================================================
void ChannelManager::Quit()
{
	for (int i=0; i < _n_channels; ++i)
	{
		DSChannel	*chanp = _channelsp[i];

		chanp->Quit();
	}
}
