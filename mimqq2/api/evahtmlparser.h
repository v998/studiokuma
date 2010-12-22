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
  
#ifndef EVAHTMLPARSER_H
#define EVAHTMLPARSER_H

typedef struct CustomizedPic{
	unsigned char type;  // should be 33, 34, 36, 37, ..     0 means error
	string  fileName;
	string  shortCutName;
	bool     isFirst;
	int      occurredIndex;
	//Q_UINT8  sessionID[4];
	unsigned int sessionID;
	unsigned int ip;
	unsigned short port;
	unsigned char  fileAgentKey[16];
	string  tmpFileName;
} CustomizedPic;

typedef struct OutCustomizedPic{
	unsigned int imageLength;
	unsigned char md5[16];
	unsigned int ip;
	unsigned short port;
	unsigned short transferType;
	string fileName;
} OutCustomizedPic;

class CNetwork;

class EvaHtmlParser 
{
public:
	EvaHtmlParser() {}
	~EvaHtmlParser(){};
	list<CustomizedPic> parseCustomizedPics(char* txt, bool useRealFileName);

	string generateSendFormat(string &src, const unsigned int agentSessionID, const unsigned int agentIP, const unsigned short agentPort );
	string generateSendFormat32( string & fileName);
	list<string> getCustomImages(const string html);

	const string getConvertedMessage() const { return m_converted; }
private:
	/*
	string absImagePath;
	string absCustomCachesPath;
	*/
	static unsigned int tmpNum;
	
	list< CustomizedPic > picList;
	list<CustomizedPic> convertCustomizedPictures(char* text, bool useRealFileName = false);
	string processPic32( const string &src, CustomizedPic *args);
	string processPic33( const string &src, CustomizedPic *args);
	string processPic34( const string &src );
	string processPic36( const string &src, CustomizedPic *args);
	string processPic37( const string &src );

	char m_qunImagePath[MAX_PATH];
	string m_converted;
};

#endif
