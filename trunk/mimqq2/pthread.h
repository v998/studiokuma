/**
 * OpenQ for Miranda IM
 *
 * Copyright (C) 2005 Studio KUMA
 * Copyright (C) 2004 Puzzlebird 
 *
 * This plugin includes codes from The OpenQ Project, Miranda Yahoo Plugin,
 * and BaseProtocol demonstration plugin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * $Id: pthread.h,v 1.1 2005/08/13 04:50:57 starkwong Exp $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * Code borrowed for myYahoo plugin. Fixed to compile on Mingw by G.Feldman
 * Original Copyright (c) 2003 Robert Rainwater
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#ifndef PTHREAD_H
#define PTHREAD_H

/* qqnetwork use only
typedef struct {
	char count;
	time_t sent;
	OutPacket* packet;
} sentpacket_t;
*/

typedef struct {
	unsigned int sessionid;
	unsigned int ip;
	unsigned short port;
	std::string message;
} postqunimage_t;

#endif
