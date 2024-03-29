/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2010 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
#ifndef M_VERSION_H__
#define M_VERSION_H__

#ifndef MIRANDA_MAKE_VERSION
#define MIRANDA_MAKE_VERSION(a,b,c,d)   (((((DWORD)(a))&0xFF)<<24)|((((DWORD)(b))&0xFF)<<16)|((((DWORD)(c))&0xFF)<<8)|(((DWORD)(d))&0xFF))
#endif

#define MIRANDA_VERSION_FILEVERSION 0,10,0,8
#define MIRANDA_VERSION_STRING      "0.10.0.8"
#define MIRANDA_VERSION_CORE        MIRANDA_MAKE_VERSION(0, 10, 0, 8)
#define MIRANDA_VERSION_CORE_STRING "0.10.0"
                                                                              
#endif // M_VERSION_H__                                                       
