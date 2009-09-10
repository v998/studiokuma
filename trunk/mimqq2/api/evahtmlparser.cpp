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
#include <sstream>

unsigned int EvaHtmlParser::tmpNum = 0;

std::list<CustomizedPic> EvaHtmlParser::parseCustomizedPics(char* txt, bool useRealFileName) {
	std::list< CustomizedPic > picList = convertCustomizedPictures(txt, useRealFileName);
	return picList;
}

std::list< CustomizedPic > EvaHtmlParser::convertCustomizedPictures(char* text, bool useRealFileName)
{
	char* pszText=text;
	char* pszCopy=pszText;

	int pos=0;
	std::string contents;
	std::string converted="";
	std::string img;
	int type;

	if (useRealFileName) {
		WCHAR szTemp[MAX_PATH];
		FoldersGetCustomPathW(CNetwork::m_folders[0],szTemp,MAX_PATH,L"QQ\\QunImages");
		LPSTR pszANSI=mir_u2a(szTemp);
		strcpy(m_qunImagePath,pszANSI);
		mir_free(pszANSI);
		strcat(m_qunImagePath,"\\");
		// CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)"QQ\\QunImages\\",(LPARAM)m_qunImagePath);
	} else
		*m_qunImagePath=0;

	while (pszText) {
		pszText=strstr(pszText,"[ZDY]");
		if (pszText && strstr(pszText,"[/ZDY]")) {
			CustomizedPic args;
			char charStore;
			if (pszText-pszCopy>0) converted.append(pszCopy,(int)(pszText-pszCopy));
			pszText[8]=0;
			type=atoi(pszText+6);
			pszText[8]=']';
			charStore=strstr(pszText,"[/ZDY]")[6];
			strstr(pszText,"[/ZDY]")[6]=0;
			contents=pszText;
			strstr(pszText,"[/ZDY]")[6]=charStore;

			args.type=0; // 0 means error;
			printf("Contents=%s\n",contents.c_str());
			
			if (type) {
				switch(type){
					case 32:
						img = processPic32(contents, &args);
						break;
					case 33:
						img = processPic33(contents,&args);
						tmpNum++;
						break;
					case 34:
						img = processPic34(contents);
						break;
					case 36:
						img = processPic36(contents, &args);
						tmpNum++;
						break;
					case 37:
						img = processPic37(contents);
						break;
					default:
						img = "";
				}
				converted.append(img);
				pszText=strstr(pszText,"[/ZDY]")+6;
				pszCopy=pszText;

				if(type==32 || type == 33 || type == 36) picList.push_back(args);
			}
		}
	}
	if (*pszCopy) converted.append(pszCopy);

	m_converted=converted;

	return picList;
}

// for 0x32
std::string EvaHtmlParser::processPic32( const std::string &src, CustomizedPic * args )
{
	std::string imgName = src.substr(9, src.length()-9-11);

	//QChar dot = imgName.at(imgName.length() - 4);
	args->type = 32;
	/*args->fileName = imgName.left(imgName.length() - ((dot=='.')?0:2));
	printf("EvaHtmlParser::processPic32 -- args->fileName: %s\n", args->fileName.ascii());
	QFile file(absCustomCachesPath + "/" + imgName);
	if(file.exists())
		args->tmpFileName = absCustomCachesPath + "/" + imgName;
	else
		args->tmpFileName = "/t_" + imgName;*/
	args->fileName=imgName;
	args->tmpFileName=m_qunImagePath + args->fileName;

	ostringstream o;
	o << DBGetContactSettingWord(NULL,g_dllname,QQ_HTTPDPORT,170);
	return "[img]http://127.0.0.1:" + o.str() + "/qunimage/" + args->fileName +"[/img]";
}

// for 33, only fileName and shortCutName are useful.
std::string EvaHtmlParser::processPic33(const std::string &src, CustomizedPic * args )
{
	//QString contents = src.mid(9, src.length()-9-11);
	std::string contents=src.substr(9,src.length()-9-11);
	args->isFirst = true;
	args->type = 33;
	args->fileName = contents.substr(0,36);
	args->shortCutName = contents.substr(36,contents.length()-36);
	//args->tmpFileName = ((absCustomCachesPath)?absCustomCachesPath:"~/.eva/customCaches") + "/" + "tmp" + QString::number(tmpNum) + ".png";
	args->tmpFileName=string(m_qunImagePath) + "tmp1235.png";

	ostringstream o;
	o << DBGetContactSettingWord(NULL,g_dllname,QQ_HTTPDPORT,170);

	return "[img]http://127.0.0.1:" + o.str() + "/qunimage/" + args->fileName +"[/img]";
}

std::string EvaHtmlParser::processPic34(const std::string &src)
{
	//QString contents = src.mid(9, src.length()-9-11);
	std::string contents=src.substr(9,src.length()-9-11);

	//int occurredIndex = contents.latin1()[0] - 'A'; 
	int occurredIndex = contents.c_str()[0] - 'A'; 
	if(occurredIndex >= (int)(picList.size())){
		return "(UnknownPic34)";
	}
	int index=0;
	std::list<CustomizedPic>::iterator iter;
	for(iter=picList.begin(); iter!=picList.end(); ++iter){
		if(index == occurredIndex){
			return "[img]" + iter->tmpFileName + "[/img]";
		}
		index++;
	}
	return "(Unknown34pic)";
}

std::string EvaHtmlParser::processPic36( const std::string &src, CustomizedPic * args )
{
	std::string contents=src.substr(10,src.length()-9-11-1);
	args->isFirst = true;
	args->type = 36;
	uint pos = 0;
	unsigned char shortLen = contents.c_str()[pos++] - 'A';
	
	// start getting session key,  it's 4 bytes long but represented in ascii expression of hex with 8 bytes long
	bool ok=true;
	unsigned short sessionLen = atoi(contents.substr(pos, 2).c_str()) - 16 - 2; // we have to minus 2, because we need get rid ot these 2 bytes. 16 is ip(8 bytes) & port(8 bytes)
	unsigned int sessionLen2=0;
	sscanf(contents.substr(pos,2).c_str(),"%x",&sessionLen2);
	sessionLen=sessionLen2;
	sessionLen-=18;

	pos+=2;

	// FIXME: we should use sessionLen to get the session string not fix value 8
	//QString strSession = contents.mid(pos, 8); 
	
	//uint tmp4 = strSession.stripWhiteSpace().toInt(&ok, 16);
	//pszTmp=strSession.c_str();
	//while (*pszTmp==' ') pszTmp++;

	uint tmp4;
	sscanf(contents.substr(pos, 8).c_str()," %x",&tmp4);

	args->sessionID = tmp4;
	pos+=8;// note sessionLen is 8 
	
	// we sort ip out now
	//QString strIP = contents.mid(pos, 8);
	uint ip;
	sscanf(contents.substr(pos, 8).c_str(),"%x",&ip);
	args->ip = ntohl(ip);
	pos+=8;
	
	// port 
	//QString strPort = contents.mid(pos, 8).stripWhiteSpace();
	//std::string strPort = contents.substr(pos, 8).stripWhiteSpace();
	int port;
	sscanf(contents.substr(pos,8).c_str()," %x",&port);
	args->port = port & 0xffff; 
	pos+=8;
	
	
	//QString strFileAgentKey = contents.mid(pos, 16);
	//memcpy(args->fileAgentKey, strFileAgentKey.latin1(), 16);
	//memcpy(args->fileAgentKey, contents.substr(pos, 16).c_str(), 16);
	memmove(args->fileAgentKey, contents.substr(pos, 16).c_str(), 16);
	pos+=16;
	
	
	//args->fileName = contents.mid(pos, strlen(contents.ascii()) - pos - shortLen - 1); // we have to minus the short cut and the last byte 
	args->fileName = contents.substr(pos, strlen(contents.c_str()) - pos - shortLen - 1); // we have to minus the short cut and the last byte 
	
	pos+=(uint)strlen(args->fileName.c_str());
	
	//args->shortCutName = contents.mid(pos, shortLen);
	args->shortCutName = contents.substr(pos, shortLen);
	args->tmpFileName = m_qunImagePath + args->fileName;

	util_log(0,"ZDY36: c=%s, f=%s, i=%d, p=%d",contents.c_str(), args->fileName.c_str(),args->ip,args->port);
	ostringstream o;
	o << DBGetContactSettingWord(NULL,g_dllname,QQ_HTTPDPORT,170);
	return "[img]http://127.0.0.1:" + o.str() + "/qunimage/" + args->fileName +"[/img]";
}

std::string EvaHtmlParser::processPic37( const std::string &src)
{
	//QString contents = src.mid(9, src.length()-9-11);
	std::string contents=src.substr(9,src.length()-9-11);
	uint pos = 0;
	//int occurredIndex = contents.latin1()[pos++] - 'A'; 
	int occurredIndex = contents.c_str()[pos++] - 'A'; 

	//Q_UINT8 shortLen = contents.latin1()[pos++] - 'A';

	if(occurredIndex >= (int)picList.size()){
		return "(UnknownPic37)";
	}
	int index = 0;
	std::list<CustomizedPic>::iterator iter;
	for(iter=picList.begin(); iter!=picList.end(); ++iter){
		if(index == occurredIndex){
			return "[img]" + iter->tmpFileName + "[/img]";
		}
		index++;
	}
	return "(UnknownPic37)";
}

std::string EvaHtmlParser::generateSendFormat32( std::string & fileName) {
	UUID uuid;
	unsigned char* stringUUID;
	char* pszUUID;
	char* pszUUID1;
	string ret;

	UuidCreate(&uuid);
	UuidToStringA(&uuid,&stringUUID);
	pszUUID=pszUUID1=strdup((char*)stringUUID);

	while (*pszUUID1) {
		*pszUUID1=toupper(*pszUUID1);
		pszUUID1++;
	}

	//ret="[ZDY][32]{"+string((char*)stringUUID)+"}0"+string(strrchr(fileName.c_str(),'.'))+"[/32][/ZDY]";
	ret="{"+string(pszUUID)+"}0"+string(strrchr(fileName.c_str(),'.'));
	free(pszUUID);
	RpcStringFreeA(&stringUUID);
	return ret;
}

std::string EvaHtmlParser::generateSendFormat( std::string & fileName, const unsigned int agentSessionID, 
			const unsigned int agentIP, const unsigned short agentPort )
{
	std::string shortcutStr = "";
	if(fileName.c_str()[0]!='{')
		shortcutStr = (fileName.length() > 7)?(fileName.substr(1,6)):(fileName);
	//QString shortcutStr = "abc";
	/*std::string lenStr;
	
	lenStr.sprintf("%3d",strlen(fileName.ascii()) + 50 + strlen(shortcutStr.ascii()));*/
	
	char sessionStr[9];
	sprintf(sessionStr,"%8x", agentSessionID);
	
	//QString ipStr;
	//QString tmpStr;
	char ipStr[9]={0};
	char* tmpStr=ipStr;
	/*
	char tmp = new char[4];
	memcpy(tmp, &agentIP, 4);
	for(int i = 0; i <4; i++){
		sprintf(tmpStr,"%02x",(unsigned char)tmp[i]);
		tmpStr+=2;
	}
	*/
	for(int i = 0; i <4; i++){
		sprintf(tmpStr,"%02x",*(((unsigned char*)&agentIP)+i));
		tmpStr+=2;
	}
	
	char portStr[9];
	sprintf(portStr,"%8x", agentPort);
	
	std::string contents = "[36]";
	contents+="e"; // new pic
	int scLen = shortcutStr.length();
	//char scCh = 'A' + scLen;
	char scCh[2]="A";
	*scCh+=scLen;
	contents+=scCh; // length of short cut.
	contents+="1A"; // following length
	contents+=sessionStr;
	contents+=ipStr;
	contents+=portStr;
	char tmp[17];
	//tmp = new char[17];
	memcpy(tmp, Packet::getFileAgentKey(), 16);
	tmp[16]=0x00;
	contents+=std::string(tmp);
	//delete tmp;
	contents+=fileName;
	contents+=shortcutStr;
	contents+="A";
	
	return "[ZDY]"+contents+"[/36][/ZDY]";
}

std::list<string> EvaHtmlParser::getCustomImages( const string html )
{
	std::list<string> picList;

	char* pszHTML=strdup(html.c_str());
	char* pszHTML2=pszHTML;

	while (strstr(pszHTML2,"[img]")) {
		pszHTML2=strstr(pszHTML2,"[img]")+5;
		if (strstr(pszHTML2,"[/img]")) {
			*strstr(pszHTML2,"[/img]")=0;
			if (strnicmp(pszHTML2,"http",4))
				picList.push_back(pszHTML2);
			pszHTML2+=(strlen(pszHTML2)+6);
		} else
			printf("Warning: Non-matching [img] detected!");
	}
	free(pszHTML);

	return picList;
}