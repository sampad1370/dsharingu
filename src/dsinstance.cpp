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
//==
//==
//==
//==================================================================

#include <windows.h>
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

#define CHNTAG	"*> "
#define APP_NAME			"DSharingu"
#define APP_VERSION_STR		"0.7a"

#define WINDOW_TITLE		APP_NAME" " APP_VERSION_STR " by Davide Pasca 2006 ("__DATE__" "__TIME__ ")"


//===============================================================
BOOL CALLBACK DSChannel::aboutDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );

	DSChannel *mythis = (DSChannel *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->aboutDialogProc( hwnd, umsg, wparam, lparam );
}

//===============================================================
BOOL CALLBACK DSChannel::aboutDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch( umsg )
    {
    case WM_INITDIALOG:
		break; 

    case WM_COMMAND:
	    switch(LOWORD(wparam))
		{
		case IDC_HOMEPAGE:
			ShellExecute( hwnd, "open", "http://kazzuya.com/dsharingu",
						  NULL, NULL, SW_SHOWNORMAL );
			break;

		case IDC_MANUAL:
			ShellExecute( hwnd, "open", "manual\\index.html",
						  NULL, NULL, SW_SHOWNORMAL );
			break;

		case IDOK:
		case IDCANCEL:                 
			DestroyWindow( hwnd );
			break;
	    }
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;
	case WM_DESTROY:
		_about_is_open = false;
		appbase_rem_modeless_dialog( hwnd );
		break;

    default:
		return 0;
    }
		
    return 1;
}


//==================================================================
//==
//==================================================================
DSChannel::DSChannel( const char *config_fnamep ) :
	_intersys( &_cpk ),
	_inpack_buffp(NULL),
	_is_transmitting(false),
	_is_connected(false),
	_im_caller(false),
	_about_is_open(false),
	_connecting_dlg_hwnd(NULL)
{
	_state = STATE_IDLE;
	_view_fitwindow = false;//true;

	psys_strcpy( _config_fname, config_fnamep, sizeof(_config_fname) );

	_intersys.Activate( false );
	_disp_off_x = 0;
	_disp_off_y = 0;

	_frame_since_transmission = 0;
}

//===============================================================
DSChannel::~DSChannel()
{
	SAFE_FREE( _inpack_buffp );

//	_intsysmsgparser.StopThread();
}

//===============================================================
void DSChannel::setInteractiveMode( bool onoff )
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	if ( _is_connected && onoff )
		_intersys.RestartFeed();

	_intersys.Activate( onoff );
	if ( onoff )
		gmp->SetGadgetText( BUTT_USEREMOTE, "[O] Use Remote" );
	else
		gmp->SetGadgetText( BUTT_USEREMOTE, "[ ] Use Remote" );
}

//==================================================================
bool DSChannel::getInteractiveMode()
{
	return _intersys.IsActive();
}

//==================================================================
void DSChannel::onConnect( bool is_connected_as_caller )
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	if ( _connecting_dlg_hwnd )
	{
		CloseWindow( _connecting_dlg_hwnd );
		_connecting_dlg_hwnd = NULL;
	}

	_remote_mng.CloseDialog();

	if ( is_connected_as_caller )
	{
		_console.cons_line_printf( CHNTAG"Establishing connection..." );
	}
	else
	{
		_console.cons_line_printf( CHNTAG"Incoming connection..." );
	}

	//gmp->EnableGadget( BUTT_CONNECTION, false );
	gmp->EnableGadget( BUTT_HANGUP, true );
	//gmp->SetGadgetText( BUTT_CONNECTION, "[O] Connected" );

	_is_connected = true;
	_is_transmitting = false;
	_remote_wants_view = false;
	_remote_wants_share = false;
	_remote_allows_view = false;
	_remote_allows_share = false;

	if ( is_connected_as_caller )
	{
		_session_remotep = _remote_mng.GetCurRemote();

		HandShakeMsg	msg( PROTOCOL_VERSION,
							_settings._username,
							_session_remotep->_rm_username,
							_session_remotep->_rm_password._data );

		if ERR_ERROR( _cpk.SendPacket( HANDSHAKE_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
	else
	{
		// will wait for handshake to findout the name
		_session_remotep = NULL;
		_remote_mng.UnlockRemote();
	}
}

//==================================================================
void DSChannel::onDisconnect()
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	if ( _connecting_dlg_hwnd )
	{
		CloseWindow( _connecting_dlg_hwnd );
		_connecting_dlg_hwnd = NULL;
	}

	_scrwriter.StopGrabbing();

	_console.cons_line_printf( CHNTAG"Disconnected (onDisconnect())." );
	_cpk.Disconnect();
	//gmp->EnableGadget( BUTT_CONNECTION, true );
	gmp->EnableGadget( BUTT_HANGUP, false );
	//gmp->SetGadgetText( BUTT_CONNECTION, "[ ] Connections..." );

	_is_connected = false;
	_session_remotep = NULL;
	_remote_mng.UnlockRemote();


	setInteractiveMode( false );
}

//==================================================================
void DSChannel::setState( State state )
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	switch ( _new_state = _state = state )
	{
	case STATE_CONNECTING:
		//gmp->EnableGadget( BUTT_CONNECTION, false );
		gmp->EnableGadget( BUTT_HANGUP, true );
		//gmp->SetGadgetText( BUTT_CONNECTION, "[ ] Calling..." );
		break;

	case STATE_CONNECTED:
		onConnect( _cpk.IsConnectedAsCaller() );
		break;

	case STATE_DISCONNECTED:
		onDisconnect();
		_state = STATE_IDLE;
		break;

	case STATE_QUIT:
		break;
	}
}

//==================================================================
void DSChannel::saveConfig()
{
	FILE *fp = fopen( _config_fname, "wt" );

	PSYS_ASSERT( fp != NULL );
	if ( fp )
	{
		_settings._schema.SaveData( fp );
		_remote_mng.SaveList( fp );
		fclose( fp );
	}
}

//==================================================================
static void cut_spaces( char *strp )
{
char	*s, *d;

	s = strp;
	d = strp;
	while ( *s )
	{
		if ( *s != ' ' )
			*d++ = *s;

		++s;
	}
	*d++ = 0;
}

//==================================================================
void DSChannel::cmd_connect( char *params[], int n_params )
{
	psys_strcpy( _destination_ip_name, params[0], sizeof(_destination_ip_name) );

	cut_spaces( _destination_ip_name );

	switch ( _cpk.Call( _destination_ip_name, DEF_PORT_NUMBER ) )
	{
	case COM_ERR_NONE:
		//state_set( STATE_CALLING );
		_console.cons_line_printf( "* Calling '%s'...", _destination_ip_name );
		return;

	case COM_ERR_INVALID_ADDRESS:
		//state_set( STATE_CALLING );
		_console.cons_line_printf( "* Calling an Invalid Address" );
		break;

	case COM_ERR_ALREADY_CONNECTED:
		_console.cons_line_printf( "* Already Connected !" );
		break;

	case COM_ERR_GENERIC:
		_console.cons_line_printf( "* Cannot Connect" );
		break;

	default:
		_console.cons_line_printf( "* Cannot Connect (unknown error)" );
		break;
	}
}

//==================================================================
void DSChannel::cmd_connect_s( void *userp, char *params[], int n_params )
{
	((DSChannel *)userp)->cmd_connect( params, n_params );
}

//==================================================================
void DSChannel::cmd_debug( char *params[], int n_params )
{
	if ( win_is_showing( &_dbg_win ) )
		win_show( &_dbg_win, 0 );
	else
		win_show( &_dbg_win, 1 );
}

//==================================================================
void DSChannel::cmd_debug_s( void *userp, char *params[], int n_params )
{
	((DSChannel *)userp)->cmd_debug( params, n_params );
}

//==================================================================
cons_cmd_def_t	DSChannel::_cmd_defs[] =
{
"/connect",		DSChannel::cmd_connect_s			,"/connect <IP>    : connect",
"/debug",		DSChannel::cmd_debug_s				,"/debug           : connect",
0
};

//==================================================================
void DSChannel::console_line_func( const char *txtp, int is_cmd )
{
	if ( _is_connected )
	{
	int		err;

		if ( txtp[0] )
		{
			err = _cpk.SendPacket( TEXT_MSG_PKID, txtp, (strlen( txtp )+1), NULL );

			PSYS_ASSERT( err == 0 );
		}
	}
}
//==================================================================
void DSChannel::console_line_func_s( void *userp, const char *txtp, int is_cmd )
{
	((DSChannel *)userp)->console_line_func( txtp, is_cmd );
}

//==================================================================
void DSChannel::Create( bool do_send_desk )
{
	FILE	*fp = fopen( _config_fname, "rt" );

	if ( fp )
	{
		DataSchema::LoaderItem	loader_items[2] =
		{
			"Settings", Settings::SchemaLoaderProc_s, &_settings,
			"RemoteDef", RemoteMng::RemoteDefLoaderProc_s, &_remote_mng,
		};
		DataSchema::LoadSchemas_s( fp, loader_items, 2 );

		//_settings._schema.LoadData( fp );
		//_remote_mng.LoadList( fp );
		fclose( fp );
	}


	_inpack_buffp = (u_char *)PSYS_MALLOC( INPACK_BUFF_SIZE );

	_state = STATE_DISCONNECTED;
	//setState( STATE_DISCONNECTED );

	win_init_quick( &_main_win, WINDOW_TITLE, NULL,
					this, mainEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_THIS_X1, 640, WIN_ANCH_TYPE_THIS_Y1, 512,
					(win_init_flags)(0*WIN_INIT_FLG_OPENGL | 0*WIN_INIT_FLG_CLIENTEDGE) );

	win_init_quick( &_tool_win, "tool win", &_main_win,
					this, toolEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_PARENT_X2, 0, WIN_ANCH_TYPE_PARENT_Y1, 30,
					(win_init_flags)(WIN_INIT_FLG_OPENGL) );

	win_init_quick( &_view_win, "view win", &_main_win,
					this, viewEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_PARENT_Y1, 30,
					WIN_ANCH_TYPE_PARENT_X2, 0,
					WIN_ANCH_TYPE_PARENT_Y2, -160,
					(win_init_flags)(WIN_INIT_FLG_OPENGL | 0*WIN_INIT_FLG_HSCROLL | 0*WIN_INIT_FLG_VSCROLL) );

	win_init_quick( &_dbg_win, APP_NAME" Debug Window", NULL,
					this, dbgEventFilter_s,
					WIN_ANCH_TYPE_FIXED, 0, WIN_ANCH_TYPE_FIXED, 0,
					WIN_ANCH_TYPE_THIS_X1, 400, WIN_ANCH_TYPE_THIS_Y1, 256,
					(win_init_flags)(WIN_INIT_FLG_OPENGL | WIN_INIT_FLG_INVISIBLE | WIN_INIT_FLG_CLIENTEDGE) );
#ifdef _DEBUG
	win_show( &_dbg_win, 1 );
#endif

	//win_mswin_event_callback_set( &_main_win, windows_event_filter, NULL );

	//-----------------------------------------------
	_console.cons_init( &_main_win, (void *)this );
	_console.cons_line_cb_set( console_line_func_s );
	_console.cons_line_printf( "%s -- %s, %s", APP_NAME" "APP_VERSION_STR, __DATE__, __TIME__ );
	_console.cons_line_printf( "by Davide Pasca - http://kazzuya.com/dsharingu" );
	_console.cons_line_printf( "This software is a TEST release of a work in progress !!!" );
	_console.cons_line_printf( "Type /help for help" );

	_console.cons_cmd_add_defs( _cmd_defs );

	_console.cons_show( 1 );

/*
	_intsysmsgparser.SetCompak( &_cpk );
	_intsysmsgparser.StartThread();
*/

	// we need to initialize this after loading the settings
	_intersys.ActivateExternalInput( _settings._share_my_screen && _settings._show_my_screen );
	_cpk.SetOnPackCallback( REMOCON_ARRAY_PKID, InteractiveSystem::OnPackCallback_s, &_intersys );

	if ( _settings._listen_for_connections )
		StartListening( _settings._listen_port );

	setState( STATE_IDLE );
}

//==================================================================
void DSChannel::gadgetCallback( int gget_id, GGET_Item *itemp )
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	switch ( gget_id )
	{
	case BUTT_CONNECTION:
		_remote_mng.OpenDialog( &_main_win,
								handleChangedRemoteManager_s,
								handleCallRemoteManager_s,
								this );
		/*
		if ( _settings._call_ip[0] )
		{
			if NOT( _cpk.Call( _settings._call_ip, _settings.GetCallPortNum() ) )
			{
				setState( STATE_CONNECTING );
			}
			else
			{
				//tool_barp->ToggleTool( BUTT_CONNECTION, false );
				gmp->EnableGadget( BUTT_CONNECTION, false );
			}
		}
		else
		{
			//tool_barp->ToggleTool( BUTT_CONNECTION, false );
			gmp->EnableGadget( BUTT_CONNECTION, true );
			MessageBox( _main_win.hwnd, "Please select a calling destination.", "Missing Destination", MB_OK | MB_ICONWARNING );
			_settings_open_for_call = true;
			_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );
		}
		*/
		break;

	case BUTT_HANGUP:
		ensureDisconnect( "Successfully disconnected." );
		break;

	case BUTT_SETTINGS:
		_settings_open_for_call = false;
		_settings.OpenDialog( &_main_win, handleChangedSettings_s, this );
		break;

	case BUTT_QUIT:
		setState( STATE_QUIT );
		break;

	case BUTT_USEREMOTE:
		setInteractiveMode( !getInteractiveMode() );
		break;

	case BUTT_SHELL:
		if ( _console.cons_is_showing() )
		{
			win_anchor_y2_offset_set( &_view_win, 0 );
			_console.cons_show( 0 );
		}
		else
		{
			win_anchor_y2_offset_set( &_view_win, -160 );
			_console.cons_show( 1 );
		}
		break;

	case BUTT_HELP:
		if NOT( _about_is_open )
		{
			HWND hwnd = CreateDialogParam( (HINSTANCE)win_system_getinstance(),
											MAKEINTRESOURCE(IDD_ABOUT), _main_win.hwnd,
											(DLGPROC)aboutDialogProc_s, (LPARAM)this );
			appbase_add_modeless_dialog( hwnd );
			ShowWindow( hwnd, SW_SHOWNORMAL );
			_about_is_open = true;
		}
		break;
	}
}

//==================================================================
void DSChannel::gadgetCallback_s( int gget_id, GGET_Item *itemp, void *userdatap )
{
	((DSChannel *)userdatap)->gadgetCallback( gget_id, itemp );
}

//==================================================================
void DSChannel::rebuildButtons()
{
	float	x = 4;
	float	y = 3;
	float	w = 90;
	float	h = 22;
	float	y_margin = 4;
	float	x_margin = 4;

	GGET_Manager	*gmp = &_tool_win._gget_manager;

	gmp->SetCallback( gadgetCallback_s, this );
	
	GGET_StaticText *stextp = gmp->AddStaticText( STEXT_TOOLBARBASE, 0, 0, _tool_win.w, _tool_win.h, NULL );
	if ( stextp )
		stextp->SetFillType( GGET_StaticText::FILL_TYPE_HTOOLBAR );

	gmp->AddButton( BUTT_CONNECTION, x, y, 98, h, "Connections..." );		x += 98 + x_margin;
	gmp->AddButton( BUTT_HANGUP, x, y, 85, h, "Hangup" );			x += 85 + x_margin;
	gmp->EnableGadget( BUTT_HANGUP, false );
	x += x_margin;
	x += x_margin;
	gmp->AddButton( BUTT_SETTINGS, x, y, 98, h, "Settings..." );	x += 98 + x_margin;
	x += x_margin;
	gmp->AddButton( BUTT_QUIT, x, y, 60, h, "Quit" );				x += 60 + x_margin;

	x += x_margin;
	x += x_margin;
	x += x_margin;
	x += x_margin;
	gmp->AddButton( BUTT_USEREMOTE, x, y, 98, h, "[ ] Use Remote" );	x += 98 + x_margin;
	x += x_margin;
	gmp->AddButton( BUTT_SHELL, x, y, 60, h, "Shell" );			x += 60 + x_margin;
	gmp->AddButton( BUTT_HELP, x, y, 60, h, "About" );			x += 60 + x_margin;

	setInteractiveMode( getInteractiveMode() );
}

//==================================================================
void DSChannel::reshapeButtons()
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	gmp->FindGadget( STEXT_TOOLBARBASE )->SetRect( 0, 0, _tool_win.w, _tool_win.h );
}

//==================================================================
static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
		glLoadIdentity();

	glMatrixMode( GL_MODELVIEW );
}

//==================================================================
static void drawRect( float x1, float y1, float w, float h )
{
	float	x2 = x1 + w;
	float	y2 = y1 + h;

	glVertex2f( x1, y1 );
	glVertex2f( x2, y1 );
	glVertex2f( x2, y2 );
	glVertex2f( x1, y2 );
}

//==================================================================
const static int FRAME_SIZE = 8;
const static int ARROW_SIZE = 32;

//==================================================================
static void drawFrame( float w, float h )
{
	glBegin( GL_QUADS );

		drawRect( 0, 0,  w, FRAME_SIZE );
		drawRect( 0, h-FRAME_SIZE,  w, FRAME_SIZE );

		drawRect( 0, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );
		drawRect( w-FRAME_SIZE, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );

	glEnd();
}

//==================================================================
#define DIR_LEFT	1
#define DIR_RIGHT	2
#define DIR_TOP		4
#define DIR_BOTTOM	8

//==================================================================
static void isPointerInArrows( float w, float h, u_int dirs )
{
}

//==================================================================
static void getArrowVerts( float verts[3][2], float w, float h, u_int dir )
{
	float x1, x2, y1, y2, xh, yh;

	if ( dir & (DIR_LEFT|DIR_RIGHT) )	// L/R
	{
		if ( dir == DIR_LEFT )
		{
			x1 = FRAME_SIZE + FRAME_SIZE/2;
			xh = x1 + ARROW_SIZE/2;
		}
		else
		{
			x1 = w - (FRAME_SIZE + FRAME_SIZE/2);
			xh = x1 - ARROW_SIZE/2;
		}

		yh = h / 2;
		y1 = yh - ARROW_SIZE/2;
		y2 = yh + ARROW_SIZE/2;

		verts[0][0]	= x1;
		verts[0][1] = yh;
		verts[1][0]	= xh;
		verts[1][1] = y1;
		verts[2][0]	= xh;
		verts[2][1] = y2;
	}
	else
	if ( dir & (DIR_TOP|DIR_BOTTOM) )	// U/D
	{
		if ( dir == DIR_TOP )
		{
			y1 = FRAME_SIZE + FRAME_SIZE/2;
			yh = y1 + ARROW_SIZE/2;
		}
		else
		{
			y1 = h - (FRAME_SIZE + FRAME_SIZE/2);
			yh = y1 - ARROW_SIZE/2;
		}

		xh = w / 2;
		x1 = xh - ARROW_SIZE/2;
		x2 = xh + ARROW_SIZE/2;

		verts[0][0]	= xh;
		verts[0][1] = y1;
		verts[1][0]	= x2;
		verts[1][1] = yh;
		verts[2][0]	= x1;
		verts[2][1] = yh;
	}
}

//==================================================================
static void drawArrows_verts( float w, float h, u_int dirs, bool lines_loop )
{
	for (int i=0; i < 4; ++i)
	{
		if ( dirs & (1<<i) )
		{
			float	verts[3][2];

			getArrowVerts( verts, w, h, 1 << i );
			if ( lines_loop )
			{
				glVertex2fv( verts[2] );
				glVertex2fv( verts[0] );
				for (int j=1; j < 3; ++j)
				{
					glVertex2fv( verts[j-1] );
					glVertex2fv( verts[j  ] );
				}
			}
			else
			{
				for (int j=0; j < 3; ++j)
					glVertex2fv( verts[j] );
			}
		}
	}
}

//==================================================================
static void drawArrows( float w, float h, u_int dirs )
{
	glColor4f( 1, .1f, .1f, .4f );
	glBegin( GL_TRIANGLES );
	drawArrows_verts( w, h, dirs, false );
	glEnd();
	
	glColor4f( 0, 0, 0, 1 );
	glBegin( GL_LINES );
	drawArrows_verts( w, h, dirs, true );
	glEnd();
}


//==================================================================
#define CURS_SIZE	16

//==================================================================
static void drawCursor( float px, float py )
{
	float	verts[3][2] = { px, py,
							px + CURS_SIZE*1.0f, py + CURS_SIZE*0.5f,
							px + CURS_SIZE*0.5f, py + CURS_SIZE*1.0f };

	glColor4f( 1, 1, 1, 1 );
	glBegin( GL_TRIANGLES );
		for (int i=0; i < 3; ++i)
			glVertex2fv( verts[i] );
	glEnd();

	glColor4f( 0, 0, 0, 1 );
	glBegin( GL_LINES );
		for (int i=0; i < 3; ++i)
		{
			glVertex2fv( verts[i] );
			glVertex2fv( verts[(i+1)%3] );
		}
	glEnd();
}


//==================================================================
void DSChannel::drawDispOffArrows()
{
	u_int	dirs = 0;

	if ( _disp_off_x < 0 )
		dirs |= DIR_LEFT;

	if ( _disp_off_y < 0 )
		dirs |= DIR_TOP;

	if ( _disp_off_x+_scrreader.GetWidth() >= _view_win.w )
		dirs |= DIR_RIGHT;

	if ( _disp_off_y+_scrreader.GetHeight() >= _view_win.h )
		dirs |= DIR_BOTTOM;

	glColor4f( 1.0f, 0.2f, 0.2f, 0.6f );
	drawArrows( _view_win.w, _view_win.h, dirs );
}

//==================================================================
void DSChannel::doPaint()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	begin_2D( NULL, NULL );
	//win_context_begin_ext( winp, 1, false );

		glPushMatrix();
		// render

		glColor3f( 1, 1, 1 );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glLoadIdentity();
			//glScalef( 0.5f, 0.5f, 1 );

			glPushMatrix();
			glTranslatef( _disp_off_x, _disp_off_y, 0 );
			_scrreader.RenderParsedFrame( _view_fitwindow );
			glPopMatrix();

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			if ( _intersys.IsActive() )
			{
				glColor4f( 1.0f, 0.1f, 0.1f, 0.3f );
			}
			else
			{
				glColor4f( 0.3f, 0.3f, 0.3f, 0.3f );
			}

			drawFrame( _view_win.w, _view_win.h );

			drawDispOffArrows();

			if ( _intersys.IsActive() )
			{
				drawCursor( _disp_curs_x, _disp_curs_y );
			}


			glDisable( GL_BLEND );

		glDisable( GL_CULL_FACE );
		glDisable( GL_LIGHTING );

		glPopMatrix();

	end_2D();
	//win_context_end( winp );
}

//=====================================================
int DSChannel::mainEventFilter( win_event_type etype, win_event_t *eventp )
{
	_console.cons_parent_eventfilter( NULL, etype, eventp );

	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		win_focus( &_console._win, true );
		break;

	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
//		reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		break;

	case WIN_ETYPE_DESTROY:
		setState( STATE_QUIT );
		PostQuitMessage(0);
		break;
	}

	return 0;
}
//==================================================================
int DSChannel::mainEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->mainEventFilter( etype, eventp );
}

//==================================================================
int DSChannel::viewEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		win_focus( &_console._win, true );
		break;

	case WIN_ETYPE_KEYDOWN:
		/*
		if ( eventp->keycode == VK_SHIFT )
		{
			if ( GetKeyState( VK_CONTROL ) & 0x8000 )
			{
				_is_interactive_mode = !_is_interactive_mode;
				//ShowCursor( !_is_interactive_mode );
			}
		}
		*/
		break;

	case WIN_ETYPE_MOUSEMOVE:
		if ( _intersys.IsActive() )
		{
			_intersys.FeedMessage( eventp->ms_message,
								   eventp->ms_lparam,
								   eventp->ms_wparam,
								   _disp_off_x,
								   _disp_off_y );

			_disp_curs_x = eventp->mouse_x;
			_disp_curs_y = eventp->mouse_y;

			win_invalidate( eventp->winp );
		}
		break;

	case WIN_ETYPE_LBUTTONDOWN:
	case WIN_ETYPE_LBUTTONUP:
	case WIN_ETYPE_RBUTTONDOWN:
	case WIN_ETYPE_RBUTTONUP:
		if ( _intersys.IsActive() )
		{
			_intersys.FeedMessage( eventp->ms_message,
								   eventp->ms_lparam,
								   eventp->ms_wparam,
								   _disp_off_x,
								   _disp_off_y );

			_disp_curs_x = eventp->mouse_x;
			_disp_curs_y = eventp->mouse_y;

			win_invalidate( eventp->winp );
		}
		break;

/*
	case WIN_ETYPE_MOUSEMOVEDOUT:
		if ( _is_interactive_mode )
		{
			ShowCursor( FALSE );
		}
		break;
*/
	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
		{
		int	new_w = eventp->win_w;
		int	new_h = eventp->win_h;

			if ( _disp_off_x + _scrreader.GetWidth() < new_w )
			{
				_disp_off_x = new_w - _scrreader.GetWidth();
				if ( _disp_off_x > 0 )
					_disp_off_x = 0;
			}

			if ( _disp_off_y + _scrreader.GetHeight() < new_h )
			{
				_disp_off_y = new_h - _scrreader.GetHeight();
				if ( _disp_off_y > 0 )
					_disp_off_y = 0;
			}
		}
		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		doPaint();
		break;
	}

	return 0;
}
//==================================================================
int DSChannel::viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->viewEventFilter( etype, eventp );
}

//==================================================================
int DSChannel::toolEventFilter( win_event_type etype, win_event_t *eventp )
{
	_console.cons_parent_eventfilter( NULL, etype, eventp );

	switch ( etype )
	{
	case WIN_ETYPE_ACTIVATE:
		win_focus( &_console._win, true );
		break;

	case WIN_ETYPE_CREATE:
		rebuildButtons();
		break;

	case WIN_ETYPE_WINRESIZE:
		reshapeButtons();
		break;
	}

	return 0;
}
//==================================================================
int DSChannel::toolEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->toolEventFilter( etype, eventp );
}


//==================================================================
void DSChannel::dbgDoPaint()
{
	debugout_reset();

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	begin_2D( NULL, NULL );
	//win_context_begin_ext( winp, 1, false );

		glPushMatrix();
		// render

		glColor3f( 1, 1, 1 );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_CULL_FACE );
	
		CompakStats	stats = _cpk.GetStats();

		debugout_printf( "Total OUT: %i B   (%u KB)", stats._total_send_bytes, stats._total_send_bytes / 1024 );
		debugout_printf( "Total IN : %i B   (%u KB)", stats._total_recv_bytes, stats._total_recv_bytes / 1024 );
		debugout_printf( "Speed OUT: %u B/s (%u KB/s)", (u_int)stats._send_bytes_per_sec_window, (u_int)(stats._send_bytes_per_sec_window / 1024) );
		debugout_printf( "Speed IN : %u B/s (%u KB/s)", (u_int)stats._recv_bytes_per_sec_window, (u_int)(stats._recv_bytes_per_sec_window / 1024) );
		debugout_printf( "Queue OUT: %u B/s (%u KB/s)", (u_int)stats._send_bytes_queue, (u_int)(stats._send_bytes_queue / 1024) );

		glLoadIdentity();
		glTranslatef( 0, 0, 0 );
		debugout_render();

		glPopMatrix();

	end_2D();
	//win_context_end( winp );
}

//==================================================================
int DSChannel::dbgEventFilter( win_event_type etype, win_event_t *eventp )
{
	switch ( etype )
	{
	case WIN_ETYPE_CREATE:
		break;

	case WIN_ETYPE_WINRESIZE:
		//reshape( eventp->win_w, eventp->win_h );
		break;

	case WIN_ETYPE_PAINT:
		dbgDoPaint();
		break;

	case WIN_ETYPE_DESTROY:
		//PostQuitMessage(0);
		break;
	}

	return 0;
}

//==================================================================
int DSChannel::dbgEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->dbgEventFilter( etype, eventp );
}


//==================================================================
void DSChannel::StartListening( int port_listen )
{
	_console.cons_line_printf( CHNTAG"Started !" );
	if ( _cpk.StartListen( port_listen ) )
	{
		//msg_badlisten( DEF_PORT_NUMBER );
		_console.cons_line_printf( CHNTAG"PROBLEM: Port %i is busy. Server cannot accept calls.", port_listen );
	}
	else
	{
		//state_set( STATE_LISTENING );
		//state_set( STATE_ACCEPTING_CONNECTIONS );
		_console.cons_line_printf( CHNTAG"OK: Listening on port %i for incoming calls.", port_listen );
	}
}
/*
//==================================================================
void IntSysMessageParser::TakeMsg( UINT msg, WPARAM wParam, LPARAM lParam )
{
	if ( msg == MSG_CPK_INPACK )
	{
		Message	*messagep = (Message *)lParam;

		InteractiveSystem::ProcessMessage_s( messagep->_pack_id, messagep->datap, messagep->_wd, messagep->_he );

		SAFE_FREE( messagep );
	}
}
*/

//==================================================================
void DSChannel::processInputPacket( u_int pack_id, const u_char *datap, u_int data_size )
{
	if NOT( _is_transmitting )
	{
		// processes packets
		switch ( pack_id )
		{
		case HANDSHAKE_PKID:
			{
			const HandShakeMsg	&msg = *(HandShakeMsg *)datap;

				if ( msg._protocol_version == PROTOCOL_VERSION )
				{
					if ( stricmp( msg._receiver_username, _settings._username ) )
					{
						_console.cons_line_printf( "* PROBLEM: Rejected connection from '%s'. The wrong username was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_USERNAME_PKID );
						ensureDisconnect( "Disconnecting." );
						return;
					}

					if NOT( sha1_t::AreEqual( msg._receiver_password, _settings._password._data ) )
					{
						_console.cons_line_printf( "* PROBLEM: Rejected connection for '%s'. The wrong password was provided.",
													msg._caller_username );
						_cpk.SendPacket( HS_BAD_PASSWORD_PKID );
						ensureDisconnect( "Disconnecting." );
						return;
					}

					_session_remotep = _remote_mng.FindOrAddRemoteDefAndSelect( msg._caller_username );
					_remote_mng.LockRemote( _session_remotep );

					_console.cons_line_printf( "* SUCCESFULLY CONNECTED to caller '%s'",
												msg._caller_username );

					_flow_cnt = 0;
					_is_transmitting = true;
					_frame_since_transmission = 0;
					_cpk.SendPacket( HS_OK );
				}
				else
				{
					if ( msg._protocol_version > PROTOCOL_VERSION )
					{
						_cpk.SendPacket( HS_NEW_PROTOCOL_PKID );
						ensureDisconnect( "Disconnecting." );
						if ( MessageBox( _main_win.hwnd,
									"Cannot communicate because the other party has a newer version of the program !\n"
									"Do you want to download the latest version ?",
									"Incompatible Versions",
									MB_YESNO | MB_ICONERROR ) == IDYES )
						{
							ShellExecute( _main_win.hwnd, "open", "http://kazzuya.com/dsharingu", NULL, NULL, SW_SHOWNORMAL );
						}
					}
					else
					{
						_cpk.SendPacket( HS_OLD_PROTOCOL_PKID );
						ensureDisconnect( "Disconnecting." );
						MessageBox( _main_win.hwnd,
									"Cannot communicate because the other party has an older version of the program !\n",
									"Incompatible Versions",
									MB_OK | MB_ICONERROR );
					}
				}
			}
			break;

		case HS_BAD_USERNAME_PKID:
			if ( _session_remotep )
				_console.cons_line_printf( "* PROBLEM: The provided Internet address doesn't belong to '%s'",
				_session_remotep->_rm_username );
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
			}
			break;

		case HS_BAD_PASSWORD_PKID:
			if ( _session_remotep )
				_console.cons_line_printf( "* PROBLEM: You don't have the right password to connect to '%s'",
				_session_remotep->_rm_username );
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
			}
			break;


		case HS_NEW_PROTOCOL_PKID:
			ensureDisconnect( "Disconnecting." );
			MessageBox( _main_win.hwnd,
				"Cannot communicate because the other party has an older version of the program !\n",
				"Incompatible Versions",
				MB_OK | MB_ICONERROR );
			break;

		case HS_OLD_PROTOCOL_PKID:
			ensureDisconnect( "Disconnecting." );
			if ( MessageBox( _main_win.hwnd,
				"Cannot communicate because the other party has a newer version of the program !\n"
				"Do you want to download the latest version ?",
				"Incompatible Versions",
				MB_YESNO | MB_ICONERROR ) == IDYES )
			{
				ShellExecute( _main_win.hwnd, "open", "http://kazzuya.com/dsharingu", NULL, NULL, SW_SHOWNORMAL );
			}
			break;

		case HS_OK:
			if ( _session_remotep )
			{
				_console.cons_line_printf( "* SUCCESFULLY CONNECTED to responder '%s'",
											_session_remotep->_rm_username );

				_is_transmitting = true;
			}
			else
			{
				PSYS_ASSERT( _session_remotep != NULL );
				ensureDisconnect( "Disconnecting. aaa" );
			}
			break;
		}
		
	}
	else
	{
		// processes packets
		switch ( pack_id )
		{
		case TEXT_MSG_PKID:
			_console.cons_line_printf( "%s> %s", _session_remotep->_rm_username, (const char *)datap );
			break;

		case DESK_IMG_PKID:
			_scrreader.ParseFrame( datap, data_size );
			win_invalidate( &_view_win );
			break;

		case USAGE_WISH_PKID:
			_remote_wants_view  = ((UsageWishMsg *)datap)->_see_remote_screen;
			_remote_wants_share = ((UsageWishMsg *)datap)->_use_remote_screen;
			break;

		case USAGE_ABILITY_PKID:
			_remote_allows_view  = ((UsageAbilityMsg *)datap)->_see_remote_screen;
			_remote_allows_share = ((UsageAbilityMsg *)datap)->_use_remote_screen;
			if ( _remote_allows_share && _remote_allows_view )
			{
				_tool_win._gget_manager.EnableGadget( BUTT_USEREMOTE, true );
			}
			else
			{
				_tool_win._gget_manager.EnableGadget( BUTT_USEREMOTE, false );
				setInteractiveMode( false );
			}
			break;
		}
	}
}

//==================================================================
void DSChannel::ensureDisconnect( const char *messagep, bool is_error )
{
	_cpk.Disconnect();
	setState( STATE_DISCONNECTED );
/*
	if ( is_error )
		MessageBox( _main_win.hwnd, "Disconnected", messagep, MB_OK | MB_ICONERROR );
	else
		MessageBox( _main_win.hwnd, "Disconnected", messagep, MB_OK );
*/
}

// 286

//==================================================================
void DSChannel::handleAutoScroll()
{
	if ( _intersys.IsActive() )
	{
		int	x1 = _disp_off_x;
		int	y1 = _disp_off_y;
		int	x2 = x1 + _scrreader.GetWidth();
		int	y2 = y1 + _scrreader.GetHeight();

		if ( _disp_curs_x < 32 && _disp_off_x < 0 )
		{
			_disp_off_x += 8;
			if ( _disp_curs_x < 8 )
				_disp_off_x += 16;

			if ( _disp_off_x > 0 )
				_disp_off_x = 0;

			win_invalidate( &_view_win );
		}

		if ( _disp_curs_y < 32 && _disp_off_y < 0 )
		{
			_disp_off_y += 8;
			if ( _disp_curs_y < 8 )
				_disp_off_y += 16;

			if ( _disp_off_y > 0 )
				_disp_off_y = 0;

			win_invalidate( &_view_win );
		}

		if ( _disp_curs_x >= _view_win.w-32 && _view_win.w < x2 )
		{
			_disp_off_x -= 8;
			if ( _disp_curs_x >= _view_win.w-8 )
				_disp_off_x -= 16;
			win_invalidate( &_view_win );
		}

		if ( _disp_curs_y >= _view_win.h-32 && _view_win.h < y2 )
		{
			_disp_off_y -= 8;
			if ( _disp_curs_y >= _view_win.h-8 )
				_disp_off_y -= 16;
			win_invalidate( &_view_win );
		}
	}
}

//==================================================================
void DSChannel::handleChangedSettings_s( void *mythis )
{
	((DSChannel *)mythis)->handleChangedSettings();
}
//==================================================================
void DSChannel::handleChangedSettings()
{
	saveConfig();

	_cpk.StopListen();
	if ( _settings._listen_for_connections )
		StartListening( _settings._listen_port );

	if ( _settings_open_for_call )
	{
		gadgetCallback( BUTT_CONNECTION, NULL );
	}

	_intersys.ActivateExternalInput( _settings._share_my_screen && _settings._show_my_screen );

	if ( _is_transmitting )
	{
		UsageAbilityMsg	msg(_settings._show_my_screen,
							_settings._share_my_screen );
		if ERR_ERROR( _cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
}

//==================================================================
void DSChannel::handleChangedRemoteManager_s( void *mythis )
{
	((DSChannel *)mythis)->handleChangedRemoteManager();
}
//==================================================================
void DSChannel::handleChangedRemoteManager()
{
	saveConfig();

	if ( _is_transmitting )
	{
		UsageWishMsg	msg(
			_session_remotep->_see_remote_screen,
			_session_remotep->_use_remote_screen );

		if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
			return;
	}
}

//==================================================================
void DSChannel::handleCallRemoteManager_s( void *mythis )
{
	((DSChannel *)mythis)->handleCallRemoteManager();
}
//==================================================================
void DSChannel::handleCallRemoteManager()
{
	_session_remotep = _remote_mng.GetCurRemote();
	_remote_mng.LockRemote( _session_remotep );

	if ERR_NULL( _session_remotep )
		return;

	if ( _state >= STATE_CONNECTING && _state <= STATE_CONNECTED )
	{
		if ( MessageBox( _main_win.hwnd,
			"You are already connected.\n"
			"Do you want to terminate the current connection ?",
			"Already Connected",
			MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			ensureDisconnect( "Successfully disconnected." );
		}
		else
			return;
	}

	if NOT( _cpk.Call( _session_remotep->_rm_ip_address, _session_remotep->GetCallPortNum() ) )
	{
		setState( STATE_CONNECTING );

		_connecting_dlg_hwnd =
			CreateDialogParam( (HINSTANCE)win_system_getinstance(),
						MAKEINTRESOURCE(IDD_CONNECTING), _main_win.hwnd,
						(DLGPROC)connectingDialogProc_s, (LPARAM)this );
		
		if ERR_NULL( _connecting_dlg_hwnd )
			return;

		appbase_add_modeless_dialog( _connecting_dlg_hwnd );
		ShowWindow( _connecting_dlg_hwnd, SW_SHOWNORMAL );
	}
}

//===============================================================
BOOL CALLBACK DSChannel::connectingDialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
	{
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );
	}

	DSChannel *mythis = (DSChannel *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->connectingDialogProc( hwnd, umsg, wparam, lparam );
}
//===============================================================
BOOL CALLBACK DSChannel::connectingDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch( umsg )
	{
	case WM_INITDIALOG:
		break; 

	case WM_COMMAND:
		switch( LOWORD(wparam) )
		{
		case IDOK:
		case IDCANCEL:
			ensureDisconnect( "Connection aborted." );
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

	case WM_DESTROY:
		appbase_rem_modeless_dialog( hwnd );
		_connecting_dlg_hwnd = NULL;
		break;

	default:
		return 0;
	}

	return 1;
}

//==================================================================
void DSChannel::handleConnectedFlow()
{
	if NOT( _is_transmitting )
		return;

	if ( _is_transmitting )
	{
		_intersys.Idle();

		if ( _frame_since_transmission == 1 )
		{
			{
				UsageWishMsg	msg(_session_remotep->_see_remote_screen,
					_session_remotep->_use_remote_screen );

				if ERR_ERROR( _cpk.SendPacket( USAGE_WISH_PKID, &msg, sizeof(msg), NULL ) )
					return;
			}

			{
				UsageAbilityMsg	msg(_settings._show_my_screen,
									_settings._share_my_screen );
				if ERR_ERROR( _cpk.SendPacket( USAGE_ABILITY_PKID, &msg, sizeof(msg), NULL ) )
					return;

			}
		}
	}


	bool	do_share = (_remote_wants_view && _settings._show_my_screen);

	if ( _scrwriter.IsGrabbing() != do_share )
	{
		if ( _settings._show_my_screen )
		{
			if NOT( _scrwriter.StartGrabbing( (HWND)_main_win.hwnd ) )
			{
				PSYS_ASSERT( 0 );
				return;
			}
		}
		else
		{
			_scrwriter.StopGrabbing();
		}
	}

	if ( _scrwriter.IsGrabbing() )
	{
		int cnt = _cpk.SearchOUTQueue( DESK_IMG_PKID );
		if ( cnt <= 2 )
		{
			bool has_grabbed = _scrwriter.UpdateWriter();

			if ( has_grabbed )
				_scrwriter.SendFrame( DESK_IMG_PKID, &_cpk );
		}
	}

	++_frame_since_transmission;
}

//==================================================================
DSChannel::State DSChannel::Idle()
{
	if ( _console.cons_is_showing() )
	{
//		win_focus( &_console._win, 1 );
	}

	handleAutoScroll();

	if ( _state == STATE_NULL )
		return _state;

	int	err;

	switch ( err = _cpk.Idle() )
	{
	case COM_ERR_CONNECTED:
		_intersys.Activate( false );
		_disp_off_x = 0;
		_disp_off_y = 0;
		setState( STATE_CONNECTED );
		break;

	case COM_ERR_HARD_DISCONNECT:
		setState( STATE_DISCONNECTED );
		ensureDisconnect( "Connection Lost" );
		break;

	case COM_ERR_INVALID_ADDRESS:
		setState( STATE_DISCONNECTED );
		ensureDisconnect( "Call Failed. No program answering." );
		break;

	case COM_ERR_GRACEFUL_DISCONNECT:
		setState( STATE_DISCONNECTED );
		ensureDisconnect( "Connection Closed" );
		break;

	case COM_ERR_GENERIC:
		setState( STATE_DISCONNECTED );
		ensureDisconnect( "Connection Lost (generic error)" );
		break;

	case COM_ERR_NONE:
		break;

	default:
		//_is_connected = false;
		//_console.cons_line_printf( "* connection err #%i !!", err );
		break;
	}

	u_int	pack_id;
	u_int	data_size;
	if ( _cpk.GetInputPack( &pack_id, &data_size, _inpack_buffp, INPACK_BUFF_SIZE ) )
	{
		processInputPacket( pack_id, _inpack_buffp, data_size );
		_cpk.DisposeINPacket();
	}

	//-------------
	if ( _state == STATE_CONNECTED )
	{
		handleConnectedFlow();
	}

	win_invalidate( &_dbg_win );

	State tmp = _new_state;
	_new_state = STATE_NULL;

	return tmp;
}
