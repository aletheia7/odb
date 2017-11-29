/* $Id: odbtpctl.cpp,v 1.2 2004/06/02 20:12:21 rtwitty Exp $ */
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
#include "stdafx.h"
#include "ODBTPServCtrl.h"


int main(int argc, char* argv[])
{
    CODBTPServCtrl ServCtrl;
    PSTR           pszCmd = argc > 1 ? argv[1] : "help";

    ServCtrl.DoCommand( pszCmd );

	return 0;
}
