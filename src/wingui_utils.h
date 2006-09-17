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

#ifndef WINGUI_UTILS_H
#define WINGUI_UTILS_H

#include <windows.h>
#include "data_schema.h"

void DlgEnableItem( HWND hwnd, int id, BOOL onoff );
void SetDlgItemInt( HWND hwnd, int id, int val );
int GetDlgItemInt( HWND hwnd, int id );
bool IsDlgButtonON( HWND hwnd, u_int item_id );
bool GetDlgItemSHA1PW( HWND hDlg, int nIDDlgItem, sha1_t *sha1hashp );
void SetDlgEditForReview( HWND hwnd, u_int item_id );
void SetDlgItemUnchangedPassword( HWND hwnd, u_int item_id );
bool IsDlgEditPasswordChanged( HWND hwnd, u_int item_id );

enum WGUTCheckPWMsg
{
	CHECKPW_MSG_GOOD,
	CHECKPW_MSG_BAD,
	CHECKPW_MSG_EMPTY,
	CHECKPW_MSG_UNCHANGED,
};
WGUTCheckPWMsg GetDlgEditPasswordState( HWND hwnd, u_int item_id, const char *prompt_titlep=NULL );


#endif