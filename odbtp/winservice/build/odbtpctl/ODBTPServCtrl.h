/* $Id: ODBTPServCtrl.h,v 1.2 2004/06/02 20:12:21 rtwitty Exp $ */
/*
    odbtpctl - ODBTP service controller

    Copyright (C) 2002-2004 Robert E. Twitty <rtwitty@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// ODBTPServCtrl.h: interface for the CODBTPServCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBTPSERVCTRL_H__5222B673_4E26_11D6_812D_0050DA0B930B__INCLUDED_)
#define AFX_ODBTPSERVCTRL_H__5222B673_4E26_11D6_812D_0050DA0B930B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\ntservice\ntservice.h"

class CODBTPServCtrl : public CNTServCtrl
{
public:
	CODBTPServCtrl();
	virtual ~CODBTPServCtrl();

    BOOL OnInstall();
	void OnCommand( PCSTR pszCmd );
	void OnError( PCSTR pszError );
	void OnMessage( PCSTR pszMessage );
};

#endif // !defined(AFX_ODBTPSERVCTRL_H__5222B673_4E26_11D6_812D_0050DA0B930B__INCLUDED_)
