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
#include "dschannel_manager.h"

//==================================================================
#define CHNTAG				"* "

//==================================================================
enum {
	BACKGROUND_STATIC = 1,
	TAB_CH0 = 2,
	TAB_CHMAX = TAB_CH0 + 32,
};

//==================================================================
DSChannelManager::DSChannelManager( win_t *parent_winp, DSharinguApp *superp,
									OnChannelSwitchCBType onChannelSwitchCB ) :
	_parent_winp(parent_winp),
	_superp(superp),
	_onChannelSwitchCB(onChannelSwitchCB),
	_tabs_winp(NULL),
	_n_channels(0),
	_cur_chanp(NULL)
{
	_tabs_winp = new win_t( "tabs", parent_winp, this,
							eventFilter_s,
							WIN_ANCH_TYPE_PARENT_X1, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 0,
							WIN_ANCH_TYPE_PARENT_X2, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 22,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR),
							0 );

	_tabs_winp->Show( true );

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	gam.SetCallback( gadgetCallback_s, this );

	GGET_StaticText *stextp = gam.AddStaticText( BACKGROUND_STATIC, 0, 0, _tabs_winp->GetWidth(), _tabs_winp->GetHeight(), NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	addTab( 0, "Home" );

	// channel 0 is home.. no real channel !
	_channelsp[ 0 ] = NULL;
	_n_channels = 1;
}

//==================================================================
int DSChannelManager::eventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannelManager	*mythis = (DSChannelManager *)userobjp;
	return mythis->eventFilter( etype, eventp );
}
//==================================================================
int DSChannelManager::eventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_WINRESIZE:
		if ( _tabs_winp )
		{
			GGET_Manager	&gam = eventp->winp->GetGGETManager();
			gam.FindGadget( BACKGROUND_STATIC )->SetSize( eventp->winp->GetWidth(), eventp->winp->GetHeight() );
		}
		break;
	}

	return 0;
}

//==================================================================
void DSChannelManager::toggleOne( GGET_Manager &gam, int gget_id )
{
	for (int i=0; i < _n_channels; ++i)
		gam.SetToggled( TAB_CH0 + i, false );

	gam.SetToggled( gget_id, true );
}

//==================================================================
void DSChannelManager::addTab( int idx, const char *namep )
{
	float	x = 4;
	float	y = 3;
	float	w = 70;
	float	h = 20;

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	gam.AddButton( TAB_CH0 + idx, x + w * idx, y, w, h, namep );
	toggleOne( gam, TAB_CH0 + idx );
}

//==================================================================
void DSChannelManager::gadgetCallback_s( int gget_id, GGET_Item *itemp, void *superp )
{
	((DSChannelManager *)superp)->gadgetCallback( gget_id, itemp );
}
//==================================================================
void DSChannelManager::gadgetCallback( int gget_id, GGET_Item *itemp )
{
	GGET_Manager	&gam = itemp->GetManager();

	if ( gget_id >= TAB_CH0 && gget_id < TAB_CHMAX )
	{
		toggleOne( gam, gget_id );

		DSChannel	*chanp = _channelsp[ gget_id - TAB_CH0 ];
		if ( _onChannelSwitchCB )
			_onChannelSwitchCB( _superp, chanp, _cur_chanp );

		_cur_chanp = chanp;
	}
}

//==================================================================
DSChannel *DSChannelManager::NewChannel( RemoteDef *remotep )
{
	if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
	{
		DSChannel	*chanp = new DSChannel( _superp, remotep );
		_channelsp[ _n_channels ] = chanp;
		addTab( _n_channels, remotep->_rm_username );
		++_n_channels;

		if ( _onChannelSwitchCB )
			_onChannelSwitchCB( _superp, chanp, _cur_chanp );

		_cur_chanp = chanp;

		return chanp;
	}
	else
		return NULL;
}

//==================================================================
DSChannel *DSChannelManager::NewChannel( int accepted_fd )
{
	if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
	{
		DSChannel	*chanp = new DSChannel( _superp, accepted_fd );
		_channelsp[ _n_channels ] = chanp;
		addTab( _n_channels, "...." );
		++_n_channels;

		if ( _onChannelSwitchCB )
			_onChannelSwitchCB( _superp, chanp, _cur_chanp );

		_cur_chanp = chanp;

		return chanp;
	}
	else
		return NULL;
}

//==================================================================
void DSChannelManager::SetChannelName( DSChannel *chanp, const char *namep )
{
	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	for (int i=0; i < _n_channels; ++i)
	{
		if ( _channelsp[i] == chanp )
			gam.SetGadgetText( TAB_CH0 + i, namep );
	}
}

//==================================================================
void DSChannelManager::Idle()
{
	for (int i=0; i < _n_channels; ++i)
	{
		DSChannel	*chanp = _channelsp[i];

		if ( chanp )
			chanp->Idle();
	}
}

//==================================================================
void DSChannelManager::Quit()
{
	for (int i=0; i < _n_channels; ++i)
	{
		DSChannel	*chanp = _channelsp[i];

		if ( chanp )
			chanp->Quit();
	}
}
