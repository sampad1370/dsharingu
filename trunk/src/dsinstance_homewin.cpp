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

	CALL_ADD_REM_OR_MODIFY_USERS_TXT_STATIC,

	CHECKUPDATE_BUTT,

	WEBSITE_BUTT,
	VERSION_INFO_STATIC,

	LANG_EN_BUTT,
	LANG_IT_BUTT,
	LANG_JA_BUTT,
};

//==================================================================
#define CHANGE_APP_SETTINGS				_T( "Change the application settings." )
#define CALL_ADD_REM_OR_MODIFY_USERS	_T( "Call, add, remove or modify users." )
#define CALL_OR_MANAGE_USERS			_T( "Call or Manage users..." )
#define MY_USERNAME						_T( "My Username: " )
#define ACCEPTING_CONS_ON_PORT			_T( "* Accepting connections on port " )
#define NOBODY_CAN_WATCH_MY_COMP		_T( "* Nobody can watch my computer" )
#define PERMITTED_USR_MAY_WATCH_MY_COMP	_T( "* Permitted users may watch my computer" )

//==================================================================
const TCHAR *DSharinguApp::localStr( const TCHAR *strp ) const
{
static TCHAR	*strs[][3] =
{
	CHANGE_APP_SETTINGS				,_T("Cambia impostazioni applicazione."), _T("�A�v���P�[�V�����ݒ��ύX����B"),
	CALL_ADD_REM_OR_MODIFY_USERS	,_T("Chiama, aggiungi, rimuivi, modifica, utenti."),_T("���[�U���Ăяo���A�Q���A�����A�ύX�B"),
	CALL_OR_MANAGE_USERS			,_T("Chiama o Gestisci utenti..."),_T("���[�U���Ăяo���A�ύX..."),
	MY_USERNAME						,_T("Mio Nome-utente: " ), _T("�����̃��[�U�l�[��: "),
	ACCEPTING_CONS_ON_PORT			,_T("* Connessioni aprte sulla porta "),_T("�ڑ����Ƃ���B�|�[�g�F"),
	NOBODY_CAN_WATCH_MY_COMP		,_T("* Nessuno puo' vedere il mio computer" ),_T("* �N���������̉�ʂ��������Ȃ�" ),
	PERMITTED_USR_MAY_WATCH_MY_COMP	,_T("* Utenti permessi possono vedere il mio comp."),_T("* ���e���ꂽ���[�U����ʂ���������"),

	NULL, NULL, NULL
};

	const TCHAR	*found_strp = NULL;

	for (int i=0; i < 100; ++i)
	{
		if NOT( strs[i][0] )
			break;

		if ( _tcscmp( strs[i][0], strp ) == 0 )
			found_strp = strs[i][_cur_lang];
	}

	if NOT( found_strp )
		found_strp = strp;

	return found_strp;
}

//==================================================================
void DSharinguApp::homeWinOnChangedSettings()
{
	GGET_Manager	&gam = _home_winp->GetGGETManager();
	GGET_StaticText	*stxtp = (GGET_StaticText *)gam.FindGadget( SETTINGS_TXT_STATIC );

	if ( _settings._username[0] == 0 || _settings._password.IsEmpty() )
	{
		stxtp->SetText( _T( "Please, choose a Username and Password in the Settings dialog." ) );
		stxtp->SetTextColor( 0.8f, 0, 0, 1 );
	}
	else
	{
		stxtp->SetText( localStr( CHANGE_APP_SETTINGS ) );
		stxtp->SetTextColor( 0, 0, 0, 1 );
	}

	PSYS::tstring	str;

	str = PSYS::tstring( localStr( MY_USERNAME ) ) + PSYS::tstring( _settings._username );
	gam.FindGadget( MYUSERNAME_TXT_STATIC )->SetText( str.c_str() );

	gam.FindGadget( SETTINGS_TXT_STATIC )->SetText( localStr( CHANGE_APP_SETTINGS ) );
	gam.FindGadget( CALL_ADD_REM_OR_MODIFY_USERS_TXT_STATIC )->SetText( localStr( CALL_ADD_REM_OR_MODIFY_USERS ) );
	gam.FindGadget( CONNECTIONS_BUTT )->SetText( localStr( CALL_OR_MANAGE_USERS ) );

	if ( _settings._listen_for_connections )
		str = PSYS::tstring( localStr(ACCEPTING_CONS_ON_PORT) ) + PSYS::Stringify( _settings._listen_port );
	else
		str = PSYS::tstring( _T( "* Not accepting any connections" ) );

	gam.FindGadget( ACCEPTING_CONS_TXT_STATIC )->SetText( str.c_str() );

	if ( _settings._nobody_can_watch_my_computer )
		str = PSYS::tstring( localStr( NOBODY_CAN_WATCH_MY_COMP ) );
	else
		str = PSYS::tstring( localStr( PERMITTED_USR_MAY_WATCH_MY_COMP ) );

	gam.FindGadget( SEL_USERS_CAN_WATCH_TXT_STATIC )->SetText( str.c_str() );}

//==================================================================
void DSharinguApp::homeWinCreateLangButts( GGET_Manager &gam, int y )
{
	GGET_Button	*buttp;
	{
	buttp = gam.AddButton( LANG_EN_BUTT, 256, y, 52, 28, _T("Eng") );	buttp->SetIcon( _ico_en_imgp );
	buttp = gam.AddButton( LANG_IT_BUTT, 256, y, 52, 28, _T("Ita") );	buttp->SetIcon( _ico_en_imgp );
	buttp = gam.AddButton( LANG_JA_BUTT, 256, y, 52, 28, _T("���{��") );	buttp->SetIcon( _ico_en_imgp );
	}
}

//==================================================================
void DSharinguApp::homeWinOnResizeLangButts( GGET_Manager &gam, Window *winp )
{
	float	x, y;

	y = winp->GetHeight();
	x = winp->GetWidth();

	GGET_Button	*buttp;

	buttp = (GGET_Button *)gam.FindGadget( LANG_JA_BUTT );
	x -= buttp->GetWidth() + 4;
	y -= buttp->GetHeight() + 4;
	buttp->SetPos( x, y );

	buttp = (GGET_Button *)gam.FindGadget( LANG_IT_BUTT );
	x -= buttp->GetWidth() + 4;
	buttp->SetPos( x, y );

	buttp = (GGET_Button *)gam.FindGadget( LANG_EN_BUTT );
	x -= buttp->GetWidth() + 4;
	buttp->SetPos( x, y );
}

//==================================================================
void DSharinguApp::homeWinCreate()
{
	_home_winp = new Window( _T("home win"), &_main_win,
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
								_T( "DSharingu - Application's Home" ) );
	//stxtp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );
	//stxtp->_flags |= GGET_FLG_ALIGN_LEFT;

	homeWinCreateLangButts( gam, y );

	y += OFF_Y;

	PSYS::tstring	str;

	stxtp =	gam.AddStaticText( MYUSERNAME_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, NULL );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	stxtp =	gam.AddStaticText( ACCEPTING_CONS_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, NULL );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	stxtp =	gam.AddStaticText( SEL_USERS_CAN_WATCH_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, NULL );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;

	if ( _settings._nobody_can_use_my_computer || _settings._nobody_can_watch_my_computer )
		str = PSYS::tstring( _T( "* Nobody can use my computer" ) );
	else
		str = PSYS::tstring( _T( "* Selected users may use my computer" ) );

	stxtp =	gam.AddStaticText( SEL_USERS_CAN_USE_TXT_STATIC, x, y, _home_winp->GetWidth(), 0, str.c_str() );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += static_off_y;
	y += static_off_y;


	gam.AddButton( CONNECTIONS_BUTT,	x, y, BUTT_WD, BUTT_HE, localStr( CALL_OR_MANAGE_USERS ) );

	stxtp =	gam.AddStaticText( CALL_ADD_REM_OR_MODIFY_USERS_TXT_STATIC, x + BUTT_WD + 4, y, 400, BUTT_HE,
								CALL_ADD_REM_OR_MODIFY_USERS );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += OFF_Y;


	gam.AddButton( SETTINGS_BUTT,		x, y, BUTT_WD, BUTT_HE, _T( "Settings..." ) );
	stxtp = gam.AddStaticText( SETTINGS_TXT_STATIC, x + BUTT_WD + 4, y, 400, BUTT_HE, _T( "" ) );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	homeWinOnChangedSettings();
	y += OFF_Y;

	gam.AddButton( CHECKUPDATE_BUTT,	x, y, BUTT_WD, BUTT_HE, _T( "Check for Updates..." ) );
	stxtp = gam.AddStaticText( -1, x + BUTT_WD + 4, y, 400, BUTT_HE, _T("Check on-line for updates.") );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;
	y += OFF_Y;

	//gam.AddButton( WEBSITE_BUTT, 0, 0, BUTT_WD, BUTT_HE, "DSharingu" );
	stxtp = gam.AddStaticText( VERSION_INFO_STATIC, 0, 0, 10, 10,
				_T("Version ") APP_VERSION_STR _T(" (") _T(__DATE__) _T(" ") _T(__TIME__) _T(") by Davide Pasca") );
	stxtp->_flags |= GGET_FLG_ALIGN_LEFT;

	_home_winp->PostResize();
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

	case LANG_EN_BUTT:	_cur_lang = LANG_EN; homeWinOnChangedSettings();	break;
	case LANG_IT_BUTT:	_cur_lang = LANG_IT; homeWinOnChangedSettings();	break;
	case LANG_JA_BUTT:	_cur_lang = LANG_JA; homeWinOnChangedSettings();	break;
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

			homeWinOnResizeLangButts( gam, eventp->winp );
		}
		break;

	case WIN_ETYPE_PAINT:
//		doViewPaint();
		break;
	}

	return 0;
}
