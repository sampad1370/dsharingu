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

#include <windows.h>
#include <CommCtrl.h>
#include <direct.h>
#include <gl/glew.h>
#include "data_schema.h"
#include "appbase3.h"
#include "dsinstance.h"
#include "dschannel_manager.h"

//==================================================================
enum {
	BACKGROUND_STATIC,

	HOME_TXT_STATIC,
	MYUSERNAME_TXT_STATIC,
	ACCEPTING_CONS_TXT_STATIC,
	SEL_USERS_CAN_WATCH_TXT_STATIC,
	SEL_USERS_CAN_USE_TXT_STATIC,

	CONNECTIONS_BUTT,

	SETTINGS_BUTT,
	SETTINGS_TXT_STATIC,

	CHECKUPDATE_BUTT,

	WEBSITE_BUTT,
	VERSION_INFO_STATIC,
};
//==================================================================
void DSharinguApp::homeWinCreate()
{
	_home_winp = new Window( "home win", &_main_win,
		this, homeWinEventFilter_s,
		WIN_ANCH_TYPE_FIXED, 0,
		WIN_ANCH_TYPE_PARENT_Y1, _chmanagerp->GetTabsWinHeight(),
		WIN_ANCH_TYPE_PARENT_X2, 0,
		WIN_ANCH_TYPE_PARENT_Y2, 0,
		(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INTFLG_DONT_CLEAR) );

	//win_show( _tabs_winp, true );
	GGET_Manager	&gam = _home_winp->GetGGETManager();

	gam.SetCallback( homeWinGadgetCallback_s, this );

	GGET_StaticText *stextp = gam.AddStaticText( BACKGROUND_STATIC, 0, 0,
												 _home_winp->GetWidth(),
												 _home_winp->GetHeight(),
												 NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	int	BUTT_WD = (int)(FONT_TextHeight() * 12);
	int	BUTT_HE = (int)(FONT_TextHeight() * 1.5f);

	int	OFF_Y = BUTT_HE + 6;

	int	x = 16;
	int	y = 6;

	int	static_off_y = FONT_TextHeight() + 3;

	GGET_StaticText *stxtp;
	stxtp =	gam.AddStaticText( HOME_TXT_STATIC, 0, y, _home_winp->GetWidth(), BUTT_HE,
								"DSharingu - Application's Home" );
	//stxtp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );
	//stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += OFF_Y;

	tstring	str;

	str = tstring( "My Username: " ) + tstring( _settings._username );
	stxtp =	gam.AddStaticText( MYUSERNAME_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, str.c_str() );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	if ( _settings._listen_for_connections )
		str = tstring( "* Accepting connections on port " ) + Stringify( _settings._listen_port );
	else
		str = tstring( "* Not accepting any connections" );

	stxtp =	gam.AddStaticText( ACCEPTING_CONS_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, str.c_str() );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	if ( _settings._nobody_can_watch_my_computer )
		str = tstring( "* Nobody can watch my computer" );
	else
		str = tstring( "* Selected users may watch my computer" );

	stxtp =	gam.AddStaticText( SEL_USERS_CAN_WATCH_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, str.c_str() );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	if ( _settings._nobody_can_use_my_computer || _settings._nobody_can_watch_my_computer )
		str = tstring( "* Nobody can use my computer" );
	else
		str = tstring( "* Selected users may use my computer" );

	stxtp =	gam.AddStaticText( SEL_USERS_CAN_USE_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, str.c_str() );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;
	y += static_off_y;


	gam.AddButton( CONNECTIONS_BUTT,	x, y, BUTT_WD, BUTT_HE, "Call or Manage users..." );

	stxtp =	gam.AddStaticText( -1, x + BUTT_WD + 4, y, 400, BUTT_HE,
								"Call, add, remove or modify users." );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += OFF_Y;


	gam.AddButton( SETTINGS_BUTT,		x, y, BUTT_WD, BUTT_HE, "Settings..." );
	stxtp = gam.AddStaticText( SETTINGS_TXT_STATIC, x + BUTT_WD + 4, y, 400, BUTT_HE, "" );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	homeWinOnChangedSettings();
	y += OFF_Y;

	gam.AddButton( CHECKUPDATE_BUTT,	x, y, BUTT_WD, BUTT_HE, "Check for Updates..." );
	stxtp = gam.AddStaticText( -1, x + BUTT_WD + 4, y, 400, BUTT_HE,
		"Check on-line for updates." );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += OFF_Y;

	//gam.AddButton( WEBSITE_BUTT, 0, 0, BUTT_WD, BUTT_HE, "DSharingu" );
	stxtp = gam.AddStaticText( VERSION_INFO_STATIC, 0, 0, 10, 10,
				"Version "APP_VERSION_STR" ("__DATE__" "__TIME__ ") by Davide Pasca" );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;

	_home_winp->PostResize();
}

//==================================================================
void DSharinguApp::homeWinOnChangedSettings()
{
	GGET_Manager	&gam = _home_winp->GetGGETManager();
	GGET_StaticText	*stxtp = (GGET_StaticText *)gam.FindGadget( SETTINGS_TXT_STATIC );

	if ( _settings._username[0] == 0 || _settings._password.IsEmpty() )
	{
		stxtp->SetText( "Please, choose a Username and Password in the Settings dialog." );
		stxtp->SetTextColor( 0.8f, 0, 0, 1 );
	}
	else
	{
		stxtp->SetText( "Change the application settings." );
		stxtp->SetTextColor( 0, 0, 0, 1 );
	}
}

//==================================================================
void DSharinguApp::homeWinGadgetCallback_s( void *userdatap, int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	((DSharinguApp *)userdatap)->homeWinGadgetCallback( gget_id, itemp, action );
}
//==================================================================
void DSharinguApp::homeWinGadgetCallback( int gget_id, GGET_Item *itemp, GGET_CB_Action action )
{
	GGET_Manager	&gam = itemp->GetManager();

	switch ( gget_id )
	{
	case CONNECTIONS_BUTT:
		PostMessage( _main_win._hwnd, WM_COMMAND, ID_FILE_CONNECTIONS, 0 );
		break;

	case SETTINGS_BUTT:
		PostMessage( _main_win._hwnd, WM_COMMAND, ID_FILE_SETTINGS, 0 );
		break;

	case CHECKUPDATE_BUTT:
		PostMessage( _main_win._hwnd, WM_COMMAND, ID_HELP_CHECKFORUPDATES, 0 );
		break;
	}
}

//==================================================================
int DSharinguApp::homeWinEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSharinguApp	*mythis = (DSharinguApp *)userobjp;
	return mythis->homeWinEventFilter( etype, eventp );
}
//==================================================================
int DSharinguApp::homeWinEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_WINRESIZE:
		if ( _home_winp )
		{
			int	BUTT_WD = (int)(FONT_TextHeight() * 12);
			int	BUTT_HE = (int)(FONT_TextHeight() * 1.5f);

			GGET_Manager	&gam = eventp->winp->GetGGETManager();
			gam.FindGadget( BACKGROUND_STATIC )->SetRect( 0, 0, eventp->winp->GetWidth(), eventp->winp->GetHeight() );
			gam.FindGadget( HOME_TXT_STATIC )->SetSize( eventp->winp->GetWidth(), BUTT_HE );

			//gam.FindGadget( WEBSITE_BUTT )->SetRect( 4, _home_winp->GetHeight()-BUTT_HE,
			//											BUTT_WD, BUTT_HE );

			gam.FindGadget( VERSION_INFO_STATIC )->SetRect( 4+BUTT_WD*0, _home_winp->GetHeight()-BUTT_HE,
															_home_winp->GetWidth() - (4+BUTT_WD), BUTT_HE );
		}
		break;

	case WIN_ETYPE_PAINT:
//		doViewPaint();
		break;
	}

	return 0;
}
