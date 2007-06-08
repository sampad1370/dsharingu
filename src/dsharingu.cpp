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

//#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <gl/glew.h>

#include "psys.h"
#include "pnetlib_compak.h"
#include "pnetlib_socksupport.h"
#include "appbase3.h"
#include "dsharingu_protocol.h"
#include "dsinstance.h"
#include "resource.h"

//==================================================================
static bool				_do_quit_flag;
static DSharinguApp		*_basechannel;
static DSharinguApp		*_extrachannel;
static DSharinguApp		*_extrachannel2;

#ifdef _DEBUG
#define USE_EXTRA_CHANNEL
//#define USE_EXTRA_CHANNEL2
#endif

#ifndef RELEASE_BUILD
#define USE_EXTRA_CHANNEL
#endif

//==================================================================
int main_requestquit(void)
{
	return _do_quit_flag;
}

//==================================================================
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	PSYS::Initialize();
	HICON	hicon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICO_APPL) );
	WinSys::Init( hInstance, hicon, _T("DSHARINGU") );

	// set float 2 int conversion to chop off !
	_control87( _RC_CHOP, _MCW_RC );
	ss_init_winsock();

	const TCHAR	*cmd_linep = GetCommandLine();
	bool	start_minimized = (_tcsstr( cmd_linep, _T("/minimized") ) != NULL);

	_basechannel = new DSharinguApp( _T("dsharingu.cfg") );
	_basechannel->Create( start_minimized );

#ifdef USE_EXTRA_CHANNEL
	_extrachannel = new DSharinguApp( _T("dsharingu2.cfg") );
	_extrachannel->Create( start_minimized );
#endif

#ifdef USE_EXTRA_CHANNEL2
	_extrachannel2 = new DSharinguApp( "dsharingu3.cfg" );
	_extrachannel2->Create( start_minimized );
#endif

	while (1)
	{
		if ( _basechannel->Idle() || _basechannel->NeedQuit() )
			break;

		#ifdef USE_EXTRA_CHANNEL
		if ( _extrachannel->Idle() || _extrachannel->NeedQuit() )
			break;
		#endif

		#ifdef USE_EXTRA_CHANNEL2
		if ( _extrachannel2->Idle() || _extrachannel2->NeedQuit() )
			break;
		#endif

		Sleep( 20 );
	}

	if ( _basechannel )
		delete _basechannel;

	#ifdef USE_EXTRA_CHANNEL
	if ( _extrachannel )
		delete _extrachannel;
	#endif

	#ifdef USE_EXTRA_CHANNEL2
	if ( _extrachannel2 )
		delete _extrachannel2;
	#endif

	return 0;
}
