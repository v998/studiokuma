/* MirandaQQ2 (libeva Version)
* Copyright(C) 2005-2007 Studio KUMA. Written by Stark Wong.
*
* Distributed under terms and conditions of GNU GPLv2.
*
* Plugin framework based on BaseProtocol. Copyright (C) 2004 Daniel Savi (dss@brturbo.com)
*
* This plugin utilizes the libeva library. Copyright(C) yunfan.

Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-5  Richard Hughes, Roland Rabien & Tristan Van de Vreede
*/
/***************************************************************************
 *   Copyright (C) 2005 by yunfan                                          *
 *   yunfan_zg@163.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "StdAfx.h"
#include "evaqtutil.h"

const bool EvaHelper::getFileMD5( const std::string fileName, char *md5)
{
	if(!md5) return false;
	LPWSTR pszFilename=mir_a2u_cp(fileName.c_str(),936);
	FILE* fp=_wfopen(pszFilename,L"rb");
	unsigned int len=0;
	if (!fp) {
		mir_free(pszFilename);
		return false;
	}

	fseek(fp,0,SEEK_END);
	
	len = ftell(fp);

	fseek(fp,0,SEEK_SET);

	char *buf = new char[len];
	unsigned int numRead = fread(buf,1,len,fp);
	fclose(fp);
	if(numRead!=len){
		printf("len=%d, numRead=%d\n",len,numRead);

		// LEAK?
		delete[] buf;
		mir_free(pszFilename);
		return false;
	}
	memcpy(md5, EvaUtil::doMd5(buf, len), 16);
	util_log(0,"MD5 of %s=%016x",fileName.c_str(), buf);
	// LEAK
	delete[] buf;
	mir_free(pszFilename);
	return true;
}

const std::string EvaHelper::md5ToString(const char *md5)
{
	char strMd5[33]={0};
	for (int i=0; i<16; i++) {
		sprintf(strMd5+strlen(strMd5),"%02x",(int)(unsigned char)md5[i]);
	}
	strupr(strMd5);
	return std::string(strMd5);
}
