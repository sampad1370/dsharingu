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
///
//==================================================================

//#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <gl/glew.h>

#include "psys.h"
#include "compak.h"
#include "pnetlib_socksupport.h"
#include "appbase3.h"
#include "dsharingu_protocol.h"
#include "dsinstance.h"
#include "resource.h"

//==================================================================
static bool				_do_quit_flag;
static DSharinguApp		*_basechannel;
static DSharinguApp		*_extrachannel;

#ifdef _DEBUG
#define USE_EXTRA_CHANNEL
#endif
//#define USE_EXTRA_CHANNEL

//==================================================================
bool main_start( void *hinstance )
{
	try {

		psys_init();
		HICON	hicon = LoadIcon( (HINSTANCE)hinstance, MAKEINTRESOURCE(IDI_ICO_APPL) );
		win_system_init( (HINSTANCE)hinstance, hicon, "DSHARINGU" );

		// set float 2 int conversion to chop off !
		_control87( _RC_CHOP, _MCW_RC );
		ss_init_winsock();

		const char	*cmd_linep = GetCommandLine();
		bool	start_minimized = (strstr( cmd_linep, "/minimized" ) != NULL);

		_basechannel = new DSharinguApp( "dsharingu.cfg" );
		_basechannel->Create( start_minimized );
	#ifdef USE_EXTRA_CHANNEL
		_extrachannel = new DSharinguApp( "dsharingu2.cfg" );
		_extrachannel->Create( start_minimized );
	#endif
	}
	catch ( exception *e )
	{
		psys_msg_printf( "", 0, 
			"Exception: %s\n",
			e->what() );

		return false;
	}
	
	return true;
}

//==================================================================
static void handleChannel( DSharinguApp *appp )
{
	appp->Idle();
	/*
	switch ( chanp->Idle() )
	{
	case DSChannel::STATE_QUIT:
		_do_quit_flag = 1;
		break;
	}
	*/
}

//==================================================================
void main_anim(void)
{
	handleChannel( _basechannel );

#ifdef USE_EXTRA_CHANNEL
	handleChannel( _extrachannel );
#endif

	Sleep( 20 );
}

//==================================================================
int main_requestquit(void)
{
	return _do_quit_flag;
}

//==================================================================
void main_quit(void)
{
}
