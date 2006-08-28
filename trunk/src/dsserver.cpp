//==================================================================
//==
//==
//==
//==================================================================

#include <windows.h>
#include <gl/glew.h>
#include "dsserver.h"
#include "console.h"
#include "dsharingu_protocol.h"
#include "gfxutils.h"
#include "debugout.h"
#include "resource.h"
#include "SHA1.h"
#include "data_schema.h"

#define CHNTAG	"*> "
#define APP_NAME			"DSharingu"
#define APP_VERSION_STR		"0.3a"

//==================================================================
static void DlgEnableItem( HWND hwnd, int id, BOOL onoff )
{
	HWND iw = GetDlgItem( hwnd, id );
	EnableWindow( iw, onoff );
}

//==================================================================
static void SetDlgItemInt( HWND hwnd, int id, int val )
{
	char	buff[64];
	sprintf( buff, "%i", val );
	SetDlgItemText( hwnd, id, buff );
}

//==================================================================
static int GetDlgItemInt( HWND hwnd, int id )
{
	char	buff[64];
	GetDlgItemText( hwnd, id, buff, sizeof(buff)-1 );
	return atoi( buff );
}


//==================================================================
//==
//==================================================================
Settings::Settings() :
	_schema("Settings")
{
	_is_open = false;

	_call_ip[0] = 0;
	_use_custom_port = false;
	_call_port = DEF_PORT_NUMBER;
	_show_my_screen = true;
	_share_my_screen = true;
	_see_remote_screen = true;

	_schema.AddString(	"_call_ip", _call_ip, sizeof(_call_ip) );
	_schema.AddBool(	"_use_custom_port", &_use_custom_port );
	_schema.AddInt(		"_call_port", &_call_port, 1, 65535 );
	_schema.AddBool(	"_show_my_screen", &_show_my_screen );
	_schema.AddBool(	"_share_my_screen", &_share_my_screen );
	_schema.AddSHA1Hash("_local_pw", &_local_pw );
	_schema.AddBool(	"_see_remote_screen", &_see_remote_screen );
	_schema.AddSHA1Hash("_remote_pw", &_remote_pw );
}

//===============================================================
BOOL CALLBACK Settings::DialogProc_s(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if ( umsg == WM_INITDIALOG )
	{
		SetWindowLongPtr( hwnd, GWLP_USERDATA, lparam );
	}

	Settings *mythis = (Settings *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

	return mythis->DialogProc( hwnd, umsg, wparam, lparam );
}

//===============================================================
bool Settings::checkPasswords( HWND hwnd )
{
	char	buff1[128];
	char	buff2[128];
	char	rem_buff[128];

	// if we aren't sharing the screen, then passwords are a non-issue
	if NOT( IsDlgButtonChecked( hwnd, IDC_SHARE_MY_SCREEN_CHECK ) )
		return true;

	GetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, buff1, sizeof(buff1) );
	GetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, buff2, sizeof(buff2) );
	GetDlgItemText( hwnd, IDC_REMOTE_PASSWORD_EDIT, rem_buff, sizeof(rem_buff) );

	if ( strcmp(buff1, buff2) )
	{
		MessageBox( hwnd, "Passwords don't match !\nPlease make sure that you correctly type the password twice.",
					"Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}
	else
	if ( strlen(buff1) > 32 )
	{
		MessageBox( hwnd, "Password too long !", "Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}
	else
	if ( strlen(rem_buff) > 32 )
	{
		MessageBox( hwnd, "Remote's password too long !", "Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}

	return true;
}

//===============================================================
bool Settings::checkPort( HWND hwnd )
{
	if NOT( IsDlgButtonChecked( hwnd, IDC_CUSTOM_PORT_RADIO ) )
		return true;

	int	val = GetDlgItemInt( hwnd, IDC_CUSTOM_PORT_EDIT );
	if ( val < 1 || val > 65535  )
	{
		MessageBox( hwnd, "Invalid port number.\nThe valid range is between 1 to 65535.",
					"Settings Problem", MB_OK | MB_ICONSTOP );

		return false;
	}

	return true;
}

//===============================================================
static bool GetDlgItemSHA1PW( HWND hDlg, int nIDDlgItem, sha1_t *sha1hashp )
{
	char	tmp[64];
	GetDlgItemText( hDlg, nIDDlgItem, tmp, sizeof(tmp)-1 );

	if ( strcmp( tmp, "emptyempty" ) == 0 )
		return false;

	CSHA1	sha1;

	sha1.Reset();
	sha1.Update( (UINT_8 *)tmp, strlen(tmp) );
	sha1.Final();
	sha1.GetHash( sha1hashp->_data );

	return true;
}

//===============================================================
BOOL CALLBACK Settings::DialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch( umsg )
    {
    case WM_INITDIALOG:
		SetDlgItemText( hwnd, IDC_CONNECT_IP_COMBO, _call_ip );
		
		CheckDlgButton( hwnd, IDC_CUSTOM_PORT_RADIO, _use_custom_port );
		CheckDlgButton( hwnd, IDC_STANDARD_PORT_RADIO, !_use_custom_port );
			DlgEnableItem( hwnd, IDC_CUSTOM_PORT_EDIT, _use_custom_port );
			SetDlgItemInt( hwnd, IDC_CUSTOM_PORT_EDIT, _call_port );

		CheckDlgButton( hwnd, IDC_SHOW_MY_SCREEN_CHECK, _show_my_screen );
		CheckDlgButton( hwnd, IDC_SHARE_MY_SCREEN_CHECK, _share_my_screen );
			DlgEnableItem( hwnd, IDC_PASSWORD1_EDIT, _show_my_screen || _share_my_screen );
			DlgEnableItem( hwnd, IDC_PASSWORD2_EDIT, _show_my_screen || _share_my_screen );
			SetDlgItemText( hwnd, IDC_PASSWORD1_EDIT, "emptyempty" );
			SetDlgItemText( hwnd, IDC_PASSWORD2_EDIT, "emptyempty" );

		CheckDlgButton( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK, _see_remote_screen );
			DlgEnableItem( hwnd, IDC_REMOTE_PASSWORD_EDIT, _see_remote_screen );
			SetDlgItemText( hwnd, IDC_REMOTE_PASSWORD_EDIT, "emptyempty" );
		break; 

    case WM_COMMAND:
	    switch(LOWORD(wparam))
		{
		case IDC_SHOW_MY_SCREEN_CHECK:
		case IDC_SHARE_MY_SCREEN_CHECK:
			{
			BOOL	onoff = IsDlgButtonChecked( hwnd, IDC_SHOW_MY_SCREEN_CHECK ) ||
							IsDlgButtonChecked( hwnd, IDC_SHARE_MY_SCREEN_CHECK );

				DlgEnableItem( hwnd, IDC_PASSWORD1_EDIT, onoff );
				DlgEnableItem( hwnd, IDC_PASSWORD2_EDIT, onoff );
			}
			break;

		case IDC_SEE_REMOTE_SCREEN_CHECK:
			{
			BOOL	onoff = IsDlgButtonChecked( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK );

				DlgEnableItem( hwnd, IDC_REMOTE_PASSWORD_EDIT, onoff );
			}
			break;

		case IDC_STANDARD_PORT_RADIO:
			if ( IsDlgButtonChecked( hwnd, IDC_STANDARD_PORT_RADIO ) )
			{
				CheckDlgButton( hwnd, IDC_CUSTOM_PORT_RADIO, false );
				DlgEnableItem( hwnd, IDC_CUSTOM_PORT_EDIT, false );
			}
			break;

		case IDC_CUSTOM_PORT_RADIO:
			if ( IsDlgButtonChecked( hwnd, IDC_CUSTOM_PORT_RADIO ) )
			{
				CheckDlgButton( hwnd, IDC_STANDARD_PORT_RADIO, false );
				DlgEnableItem( hwnd, IDC_CUSTOM_PORT_EDIT, true );
			}
			break;

		case IDOK:
		case IDCANCEL:
			if ( LOWORD(wparam) == IDOK )
			{
				if ( !checkPasswords( hwnd ) || !checkPort( hwnd ) )
					return 1;	// not OK !!

				GetDlgItemText( hwnd, IDC_CONNECT_IP_COMBO, _call_ip, sizeof(_call_ip)-1 );

				_use_custom_port = IsDlgButtonChecked( hwnd, IDC_CUSTOM_PORT_RADIO ) ? true : false;
					_call_port = GetDlgItemInt( hwnd, IDC_CUSTOM_PORT_EDIT );

				_show_my_screen = IsDlgButtonChecked( hwnd, IDC_SHOW_MY_SCREEN_CHECK ) ? true : false;

				_share_my_screen = IsDlgButtonChecked( hwnd, IDC_SHARE_MY_SCREEN_CHECK ) ? true : false;
				if ( _share_my_screen )
				{
					GetDlgItemSHA1PW( hwnd, IDC_PASSWORD1_EDIT, &_local_pw );
				}

				_see_remote_screen = IsDlgButtonChecked( hwnd, IDC_SEE_REMOTE_SCREEN_CHECK ) ? true : false;
				if ( _see_remote_screen )
				{
					GetDlgItemSHA1PW( hwnd, IDC_REMOTE_PASSWORD_EDIT, &_remote_pw );
				}

				_changed = true;
			}

			PostMessage(hwnd, WM_CLOSE, 0, 0);
			_is_open = false;
			break;
	    }
		break;

    case WM_CLOSE:
		EndDialog(hwnd, 0);
		_is_open = false;
		break;

    default:
		return 0;
    }
		
    return 1;
}

//==================================================================
void Settings::OpenDialog( win_t *parent_winp )
{
	if NOT( _is_open )
	{
		_is_open = true;

		DialogBoxParam( (HINSTANCE)win_system_getinstance(),
				MAKEINTRESOURCE(IDD_SETTINGS), parent_winp->hwnd,
				DialogProc_s, (LPARAM)this );
	}

	//_dialogp->DoOpen( parent_winp, "Settings" );
}

//==================================================================
void Settings::SaveConfig( FILE *fp )
{
//	_dialogp->SaveData( fp );
}

//==================================================================
Settings::~Settings()
{
//	delete _dialogp;
//	_dialogp = 0;
}

//==================================================================
int Settings::GetPortNum() const
{
	if ( _use_custom_port )
		return _call_port;
	else
		return DEF_PORT_NUMBER;
}

/*
//==================================================================
//==================================================================
class AboutDialog
{
	Dialog	*_dialogp;
public:

	AboutDialog() :
	  _dialogp(NULL)
	{
	}
	
	void OpenUI( win_t *parent_winp )
	{
		_dialogp->DoOpen( parent_winp, "About" );
	}
	
	~AboutDialog()
	{
		delete _dialogp;
		_dialogp = 0;
	}
};
*/

//===============================================================
static long FAR PASCAL about_dlgproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch( umsg )
    {
    case WM_INITDIALOG:
		SetTimer( hwnd, 0, 1000/20, NULL );
		break; 

	case WM_TIMER:
//		rd_idle_everything();
		SetTimer( hwnd, 0, 1000/20, NULL );
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
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
	    }
		break;

    case WM_CLOSE:
		EndDialog(hwnd, 0);
		break;

    default:
		return 0;
    }
		
    return 1;
}

//==================================================================
//==
//==================================================================
DSChannel::DSChannel() :
	_intersys( &_cpk ),
	_inpack_buffp(NULL)
{
	_state = STATE_IDLE;
	_is_connected = false;
	_view_fitwindow = false;//true;

	_intersys.Activate( false );
	_disp_off_x = 0;
	_disp_off_y = 0;

	_frame_since_transmission = 0;
};

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
void DSChannel::onConnect()
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	_console.cons_line_printf( CHNTAG"Accepted an incoming connection !" );
	gmp->EnableGadget( BUTT_CALL, false );
	gmp->EnableGadget( BUTT_HANGUP, true );
	gmp->SetGadgetText( BUTT_CALL, "[O] Connected" );
	
	_is_connected = true;
	_is_transmitting = false;
	_remote_gives_view = false;
	_remote_gives_share = false;
	_remote_wants_view = false;
	_remote_wants_share = false;

	HandShakeMsg	msg( PROTOCOL_VERSION );
	if ERR_ERROR( compak_send_packet( &_cpk, HANDSHAKE_PKID, &msg, sizeof(msg), NULL ) )
		return;
}

//==================================================================
void DSChannel::setState( State state )
{
	GGET_Manager	*gmp = &_tool_win._gget_manager;

	switch ( _new_state = _state = state )
	{
	case STATE_CONNECTING:
		gmp->EnableGadget( BUTT_CALL, false );
		gmp->EnableGadget( BUTT_HANGUP, true );
		gmp->SetGadgetText( BUTT_CALL, "[ ] Calling..." );
		break;

	case STATE_CONNECTED:
		onConnect();
		break;

	case STATE_DISCONNECTED:
		_sshare.StopSharing();

		_console.cons_line_printf( CHNTAG"Disconnected." );
		_cpk.Disconnect();
		gmp->EnableGadget( BUTT_CALL, true );
		gmp->EnableGadget( BUTT_HANGUP, false );
		gmp->SetGadgetText( BUTT_CALL, "[ ] Call..." );
		_is_connected = false;
		break;

	case STATE_QUIT:
		if ( _do_save_config )
		{
		FILE	*fp;

			fp = fopen( "dsharingu.cfg", "wt" );
			KASSERT( fp != NULL );
			if ( fp )
			{
				_settings._schema.SaveData( fp );
				fclose( fp );
			}
		}

	}
}

//===============================================================================
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

//===============================================================================
void DSChannel::cmd_connect( char *params[], int n_params )
{
	ksys_strcpy( _destination_ip_name, params[0], sizeof(_destination_ip_name) );

	cut_spaces( _destination_ip_name );

	switch ( compak_call( &_cpk, _destination_ip_name, _settings.GetPortNum() ) )
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

//===============================================================================
void DSChannel::cmd_connect_s( void *userp, char *params[], int n_params )
{
	((DSChannel *)userp)->cmd_connect( params, n_params );
}

//===============================================================================
void DSChannel::cmd_debug( char *params[], int n_params )
{
	if ( win_is_showing( &_dbg_win ) )
		win_show( &_dbg_win, 0 );
	else
		win_show( &_dbg_win, 1 );
}

//===============================================================================
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

//===============================================================================
void DSChannel::console_line_func( const char *txtp, int is_cmd )
{
	if ( _is_connected )
	{
	int		err;

		if ( txtp[0] )
		{
			err = compak_send_packet( &_cpk, TEXT_MSG_PKID, txtp, (strlen( txtp )+1), NULL );

			KASSERT( err == 0 );
		}
	}
}
//===============================================================================
void DSChannel::console_line_func_s( void *userp, const char *txtp, int is_cmd )
{
	((DSChannel *)userp)->console_line_func( txtp, is_cmd );
}

//===============================================================================
void DSChannel::Create( bool do_send_desk, bool do_save_config )
{
//	if ( _do_save_config )
	{
	FILE	*fp;

		fp = fopen( "dsharingu.cfg", "rt" );
		//KASSERT( fp != NULL );
		if ( fp )
		{
			_settings._schema.LoadData( fp );
			fclose( fp );
		}
	}



	_inpack_buffp = (u_char *)KSYS_MALLOC( INPACK_BUFF_SIZE );

	compak_init( &_cpk );

	_state = STATE_DISCONNECTED;
	//setState( STATE_DISCONNECTED );

	win_init_quick( &_main_win, APP_NAME" " APP_VERSION_STR " by Davide Pasca 2005 ("__DATE__" "__TIME__ ")", NULL,
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

//	_settings._show_my_screen = do_send_desk;
//	_settings._share_my_screen = do_send_desk;

	_do_save_config = do_save_config;
/*
	_intsysmsgparser.SetCompak( &_cpk );
	_intsysmsgparser.StartThread();
*/

	_cpk.SetOnPackCallback( REMOCON_ARRAY_PKID, InteractiveSystem::OnPackCallback_s, NULL );
}

//==================================================================
void DSChannel::gadgetCallback( int gget_id, GGET_Item *itemp )
{
	switch ( gget_id )
	{
	case BUTT_CALL:
		if ( _settings._call_ip[0] )
		{
			if NOT( compak_call( &_cpk, _settings._call_ip, _settings.GetPortNum() ) )
			{
				setState( STATE_CONNECTING );
			}
		}
		else
		{
			MessageBox( _main_win.hwnd, "Please select a calling destination and try again.", "Missing Destination", MB_OK | MB_ICONWARNING );
			_settings.OpenDialog( &_main_win );
		}
		break;

	case BUTT_HANGUP:
		ensureDisconnect( "Succesfully disconnected." );
		break;

	case BUTT_SETTINGS:
		_settings.OpenDialog( &_main_win );
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
		{
		static int	about_open;
			
			if ( about_open )
				return;

			about_open = 1;
			DialogBox( (HINSTANCE)win_system_getinstance(),
					MAKEINTRESOURCE(IDD_ABOUT), _main_win.hwnd,
					(DLGPROC)about_dlgproc);
			about_open = 0;

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

	gmp->AddButton( BUTT_CALL, x, y, 98, h, "[ ] Call..." );		x += 98 + x_margin;
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

//===============================================================================
static void reshape( int w, int h )
{
	glViewport( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
		glLoadIdentity();

	glMatrixMode( GL_MODELVIEW );
}

//===============================================================================
static void drawRect( float x1, float y1, float w, float h )
{
	float	x2 = x1 + w;
	float	y2 = y1 + h;

	glVertex2f( x1, y1 );
	glVertex2f( x2, y1 );
	glVertex2f( x2, y2 );
	glVertex2f( x1, y2 );
}

//===============================================================================
const static int FRAME_SIZE = 8;
const static int ARROW_SIZE = 32;

//===============================================================================
static void drawFrame( float w, float h )
{
	glBegin( GL_QUADS );

		drawRect( 0, 0,  w, FRAME_SIZE );
		drawRect( 0, h-FRAME_SIZE,  w, FRAME_SIZE );

		drawRect( 0, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );
		drawRect( w-FRAME_SIZE, FRAME_SIZE, FRAME_SIZE, h-FRAME_SIZE*2 );

	glEnd();
}

//===============================================================================
#define DIR_LEFT	1
#define DIR_RIGHT	2
#define DIR_TOP		4
#define DIR_BOTTOM	8

//===============================================================================
static void isPointerInArrows( float w, float h, u_int dirs )
{
}

//===============================================================================
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

//===============================================================================
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

//===============================================================================
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


//===============================================================================
#define CURS_SIZE	16

//===============================================================================
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


//===============================================================================
void DSChannel::drawDispOffArrows()
{
	u_int	dirs = 0;

	if ( _disp_off_x < 0 )
		dirs |= DIR_LEFT;

	if ( _disp_off_y < 0 )
		dirs |= DIR_TOP;

	if ( _disp_off_x+_sshare.GetWidth() >= _view_win.w )
		dirs |= DIR_RIGHT;

	if ( _disp_off_y+_sshare.GetHeight() >= _view_win.h )
		dirs |= DIR_BOTTOM;

	glColor4f( 1.0f, 0.2f, 0.2f, 0.6f );
	drawArrows( _view_win.w, _view_win.h, dirs );
}

//===============================================================================
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
			//_sshare.RenderGrabbedFrame( _view_fitwindow );
			_sshare.RenderParsedFrame( _view_fitwindow );
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
//===============================================================================
int DSChannel::mainEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->mainEventFilter( etype, eventp );
}

//===============================================================================
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

			if ( _disp_off_x + _sshare.GetWidth() < new_w )
			{
				_disp_off_x = new_w - _sshare.GetWidth();
				if ( _disp_off_x > 0 )
					_disp_off_x = 0;
			}

			if ( _disp_off_y + _sshare.GetHeight() < new_h )
			{
				_disp_off_y = new_h - _sshare.GetHeight();
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
//===============================================================================
int DSChannel::viewEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->viewEventFilter( etype, eventp );
}

//===============================================================================
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
//===============================================================================
int DSChannel::toolEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->toolEventFilter( etype, eventp );
}


//===============================================================================
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

		debugout_printf( "Total OUT: %i B   (%u KB)", _cpk._stats._total_send_bytes, _cpk._stats._total_send_bytes / 1024 );
		debugout_printf( "Total IN : %i B   (%u KB)", _cpk._stats._total_recv_bytes, _cpk._stats._total_recv_bytes / 1024 );
		debugout_printf( "Speed OUT: %u B/s (%u KB/s)", (u_int)_cpk._stats._send_bytes_per_sec_window, (u_int)(_cpk._stats._send_bytes_per_sec_window / 1024) );
		debugout_printf( "Speed IN : %u B/s (%u KB/s)", (u_int)_cpk._stats._recv_bytes_per_sec_window, (u_int)(_cpk._stats._recv_bytes_per_sec_window / 1024) );
		debugout_printf( "Queue OUT: %u B/s (%u KB/s)", (u_int)_cpk._stats._send_bytes_queue, (u_int)(_cpk._stats._send_bytes_queue / 1024) );

		glLoadIdentity();
		glTranslatef( 0, 0, 0 );
		debugout_render();

		glPopMatrix();

	end_2D();
	//win_context_end( winp );
}

//===============================================================================
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

//===============================================================================
int DSChannel::dbgEventFilter_s( void *userobjp, win_event_type etype, win_event_t *eventp )
{
	DSChannel	*mythis = (DSChannel *)userobjp;
	return mythis->dbgEventFilter( etype, eventp );
}


//==================================================================
void DSChannel::StartListening( int port_listen )
{
	setState( STATE_IDLE );

	_console.cons_line_printf( CHNTAG"Started !" );
	if ( compak_listen_start( &_cpk, port_listen ) )
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
					_flow_cnt = 0;
					_is_transmitting = true;
					_frame_since_transmission = 0;
				}
				else
				{
					if ( msg._protocol_version > PROTOCOL_VERSION )
					{
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
						if ( MessageBox( _main_win.hwnd,
									"Cannot communicate because the other party has an older version of the program !\n"
									"Do you want to download the latest version ? (the other party should do the same)",
									"Incompatible Versions",
									MB_YESNO | MB_ICONERROR ) == IDYES )
						{
							ShellExecute( _main_win.hwnd, "open", "http://kazzuya.com/dsharingu", NULL, NULL, SW_SHOWNORMAL );
						}
					}
				}
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
			_console.cons_line_printf( "MSG> %s", (const char *)datap );
			break;

		case DESK_IMG_PKID:
			_sshare.ParseFrame( datap, data_size );
			win_invalidate( &_view_win );
			break;

		case ACCEPTING_DESK_PKID:
			_remote_gives_view = ((SettingMsg *)datap)->_show_my_screen;
			_remote_gives_share = ((SettingMsg *)datap)->_share_my_screen;

			_remote_wants_view = ((SettingMsg *)datap)->_see_remote_screen;
			if ( _remote_wants_view && (_settings._show_my_screen || _settings._share_my_screen) )
			{
				if NOT( sha1_t::AreEqual( ((SettingMsg *)datap)->_remote_access_pw, _settings._local_pw._data ) )
				{
					_console.cons_line_puts( "* PROBLEM: Remote provided the wrong password !" );
					_remote_wants_view = false;
					compak_send_packet( &_cpk, BAD_PASSWORD_PKID );
				}
			}	
			break;

		case BAD_PASSWORD_PKID:
			_console.cons_line_puts( "* PROBLEM: The password for remote access is invalid. Access disabled." );
/*			if ( MessageBox( _main_win.hwnd,
						"The password for remote access is invalid\n"
						"Do you want to change the password or renounce to remote access ?",
						"Invalid Password",
						MB_YESNO | MB_ICONERROR ) == IDYES )
			{
				_settings.OpenDialog( &_main_win );
			}
*/
			break;
/*
		case REMOCON_ARRAY_PKID:
			{
			IntSysMessageParser::Message	*intsys_msgp = (IntSysMessageParser::Message *)KSYS_MALLOC( sizeof(IntSysMessageParser::Message) + data_size );
			if NOT( intsys_msgp )
				return;

			intsys_msgp->_pack_id = REMOCON_ARRAY_PKID;
			intsys_msgp->_wd = _sshare.GetWidth();
			intsys_msgp->_he = _sshare.GetHeight();
			intsys_msgp->datap = (void *)(intsys_msgp + 1);
			memcpy( intsys_msgp->datap, datap, data_size );

			_intsysmsgparser.PostMsg( IntSysMessageParser::MSG_CPK_INPACK, 0, (LPARAM)intsys_msgp );
			}
*/
		}
	}
}

//===============================================================================
void DSChannel::ensureDisconnect( const char *messagep, bool is_error )
{
	compak_disconnect( &_cpk );
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
		int	x2 = x1 + _sshare.GetWidth();
		int	y2 = y1 + _sshare.GetHeight();

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
void DSChannel::handleConnectedFlow()
{
	if NOT( _is_transmitting )
		return;

	if ( _is_transmitting )
	{
		_intersys.Idle();

		if ( _frame_since_transmission == 1 || _settings.GetFlushChanged() )
		{
			SettingMsg	msg( _settings._show_my_screen,
							 _settings._share_my_screen,
							 _settings._see_remote_screen,
							 _settings._remote_pw._data );

			if ERR_ERROR( compak_send_packet( &_cpk, ACCEPTING_DESK_PKID, &msg, sizeof(msg), NULL ) )
				return;
		}
	}


	bool	do_share = (_remote_wants_view && _settings._show_my_screen);

	if ( _sshare.IsSharing() != do_share )
	{
		if ( _settings._show_my_screen )
		{
			if NOT( _sshare.StartSharing( (HWND)_main_win.hwnd ) )
			{
				KASSERT( 0 );
				return;
			}
		}
		else
		{
			_sshare.StopSharing();
		}
	}

	if ( _sshare.IsSharing() )
	{
		int cnt = compak_search_out_queue( &_cpk, DESK_IMG_PKID );
		if ( cnt <= 2 )
		{
			bool has_grabbed = _sshare.Update();

			if ( has_grabbed )
				_sshare.SendFrame( DESK_IMG_PKID, &_cpk );
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

	switch ( err = compak_idle( &_cpk ) )
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
	compak_inpack_t *inpackp;
	if ( _cpk.GetInputPack( &pack_id, &data_size, _inpack_buffp, INPACK_BUFF_SIZE ) )
	{
		processInputPacket( pack_id, _inpack_buffp, data_size );
		compak_in_packet_dispose( &_cpk );
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
