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
//#define CHNTAG				"* "

//==================================================================
enum {
	BACKGROUND_STATIC = 1,
	TAB_CH0 = 2,
	TAB_CHMAX = TAB_CH0 + 32,
};

//==================================================================
static const int	TAB_BASE_X = 4;
static const int	TAB_BASE_Y = 2;
static const int	TAB_BASE_WD = 140;
static const int	TAB_BASE_PAD_X = 0;
static const int	TAB_BASE_HE = 22;
static const int	TAB_BASE_STRIDE_X = TAB_BASE_WD + TAB_BASE_PAD_X;

//==================================================================
DSChannelManager::DSChannelManager( Window *parent_winp, DSharinguApp *superp,
									OnChannelSwitchCBType onChannelSwitchCB,
									OnChannelDeleteCBType onChannelDeleteCBType ) :
	_parent_winp(parent_winp),
	_superp(superp),
	_onChannelSwitchCB(onChannelSwitchCB),
	_onChannelDeleteCBType(onChannelDeleteCBType),
	_tabs_winp(NULL),
	_n_channels(0),
	_cur_chanp(NULL)
{
	_tabs_winp = new Window( _T("tabs"), parent_winp, this,
							eventFilter_s,
							WIN_ANCH_TYPE_PARENT_X1, 0,
							WIN_ANCH_TYPE_PARENT_Y1, 0,
							WIN_ANCH_TYPE_PARENT_X2, 0,
							WIN_ANCH_TYPE_PARENT_Y1, TAB_BASE_HE+TAB_BASE_Y+4,
							(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR),
							0 );

	_tabs_winp->Show( true );

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	gam.SetCallback( gadgetCallback_s, this );

	GGET_StaticText *stextp = gam.AddStaticText( BACKGROUND_STATIC, 0, 0,
				_tabs_winp->GetWidth(),
				_tabs_winp->GetHeight(), NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	addTab( 0, _T("Home") );

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
void DSChannelManager::addTab( int idx, const TCHAR *namep )
{
	float	x = TAB_BASE_X;
	float	y = TAB_BASE_Y;

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	gam.AddTab( TAB_CH0 + idx, x + TAB_BASE_STRIDE_X * idx, y, TAB_BASE_WD, TAB_BASE_HE, namep );
	
	if ( idx > 0 )
		gam.FindGadget( TAB_CH0 + idx )->SetIcon( GGET_Item::STD_ICO_OFF );

	toggleOne( gam, TAB_CH0 + idx );
}

//==================================================================
void DSChannelManager::gadgetCallback_s( void *superp, int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	((DSChannelManager *)superp)->gadgetCallback( gget_id, itemp, action );
}
//==================================================================
void DSChannelManager::gadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	GGET_Manager	&gam = itemp->GetManager();

	if ( gget_id >= TAB_CH0 && gget_id < TAB_CHMAX )
	{
		if ( action == GGET_CB_ACTION_CLICK )
		{
			toggleOne( gam, gget_id );

			DSChannel	*chanp = _channelsp[ gget_id - TAB_CH0 ];
			if ( _onChannelSwitchCB )
				_onChannelSwitchCB( _superp, chanp, _cur_chanp );

			_cur_chanp = chanp;
		}
		else
		if ( action == GGET_CB_ACTION_CLOSETAB )
		{
			DSChannel	*chanp = _channelsp[ gget_id - TAB_CH0 ];
			if ( chanp )
				RemoveChannel( chanp );
		}
	}
}

//==================================================================
DSChannel *DSChannelManager::RecycleOrNewChannel( RemoteDef *remotep, bool call_silent )
{
	DSChannel	*chanp = FindChannelByRemote( remotep );

	if ( chanp )
	{
		chanp->Recycle();
		chanp->CallRemote( call_silent );
		return chanp;
	}
	else
	{
		if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
		{
			DSChannel	*chanp = new DSChannel( this, remotep );
			//_channelsp[ _n_channels ] = chanp;
			//addTab( _n_channels, remotep->_rm_username );
			//++_n_channels;

			if ( _onChannelSwitchCB )
				_onChannelSwitchCB( _superp, chanp, _cur_chanp );

			_cur_chanp = chanp;

			return chanp;
		}
		else
			return NULL;
	}
}

//==================================================================
void DSChannelManager::AddChannelToList( DSChannel *chanp, const TCHAR *namep )
{
	_channelsp[ _n_channels ] = chanp;
	addTab( _n_channels, namep );
	++_n_channels;
}

//==================================================================
u_int DSChannelManager::GetTabsWinHeight() const
{
	return TAB_BASE_HE+TAB_BASE_Y+4;//_tabs_winp->GetHeight();
}

//==================================================================
DSChannel *DSChannelManager::NewChannel( int accepted_fd )
{
	if PTRAP_FALSE( _n_channels < MAX_CHANNELS )
	{
		DSChannel	*chanp = new DSChannel( this, accepted_fd );

		if ( _onChannelSwitchCB )
			_onChannelSwitchCB( _superp, chanp, _cur_chanp );

		_cur_chanp = chanp;

		return chanp;
	}
	else
		return NULL;
}

//==================================================================
DSChannel *DSChannelManager::FindChannelByRemote( const RemoteDef *remotep )
{
	for (int i=0; i < _n_channels; ++i)
	{
		if ( _channelsp[i] && _channelsp[i]->_session_remotep == remotep )
		{
			return _channelsp[i];
		}
	}

	return NULL;
}

//==================================================================
int DSChannelManager::findChannelIndex( DSChannel *chanp )
{
	for (int i=0; i < _n_channels; ++i)
	{
		if ( _channelsp[i] == chanp )
			return i;
	}

	throw "FindChannelIndex() Failed !";

	return NULL;
}

//==================================================================
void DSChannelManager::SetChannelName( DSChannel *chanp, const TCHAR *namep )
{
	GGET_Manager	&gam = _tabs_winp->GetGGETManager();
	gam.SetGadgetText( TAB_CH0 + findChannelIndex( chanp ), namep );
}

//==================================================================
void DSChannelManager::SetChannelTabIcon( DSChannel *chanp, GGET_Item::StdIcon std_icon )
{
	GGET_Manager	&gam = _tabs_winp->GetGGETManager();
	gam.FindGadget( TAB_CH0 + findChannelIndex( chanp ) )->SetIcon( std_icon );
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

//==================================================================
void DSChannelManager::RemoveChannel( DSChannel *chanp )
{
	DSChannel	*new_sel_chanp = NULL;

	GGET_Manager	&gam = _tabs_winp->GetGGETManager();

	float	x = TAB_BASE_X;
	float	y = TAB_BASE_Y;

	for (int i=0; i < _n_channels; ++i)
	{
		if ( _channelsp[i] == chanp )
		{
			gam.RemoveItem( TAB_CH0 + i );

			for (int j=i+1; j < _n_channels; ++j)
			{
				gam.ChangeGadgetID( TAB_CH0 + j, TAB_CH0 + j-1 );
				gam.FindGadget( TAB_CH0 + (j-1) )->SetPos( x + TAB_BASE_STRIDE_X * (j-1), y );
				_channelsp[j-1] = _channelsp[j];
			}
			_n_channels -= 1;

			if ( i < _n_channels )
			{
				new_sel_chanp = _channelsp[i];
				toggleOne( gam, TAB_CH0 + i );
			}
			else
			if ( (i-1) >= 0 )
			{
				new_sel_chanp = _channelsp[i-1];
				toggleOne( gam, TAB_CH0 + (i-1) );
			}
			else
			{
				new_sel_chanp = NULL;
				toggleOne( gam, TAB_CH0 + 0 );
			}

			break;
		}
	}

	if ( _cur_chanp == chanp )
		_cur_chanp = new_sel_chanp;

	if ( _onChannelSwitchCB )
		_onChannelSwitchCB( _superp, new_sel_chanp, NULL );

	SAFE_DELETE( chanp );

//	if ( _onChannelDeleteCBType )
//		_onChannelDeleteCBType( _superp, chanp );
}
