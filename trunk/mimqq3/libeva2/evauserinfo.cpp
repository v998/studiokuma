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

#include "evauserinfo.h"
#include "evadefines.h"
#include <cstring>
#include <cstdlib>

// important: all string stored are encoded by "GB18030"

ContactInfo::ContactInfo() 
{ 
	/*
	infos.reserve(QQ_CONTACT_FIELDS); 
	infos.push_back("-");   // qq number 
	infos.push_back("EVA");   // nick
	*/
};

ContactInfo::ContactInfo(const unsigned char *buf, const int len)  
{
	parseData(buf, len);
}

ContactInfo::ContactInfo( const ContactInfo &rhs)
{
	// infos = rhs.details();	
	char* pszTemp;
	unsigned short fieldlen;

	for (map<Info_Index,char*>::const_iterator iter=rhs.details().begin(); iter!=rhs.details().end(); iter++) {
		fieldlen=htons(*(unsigned short*)iter->second);
		pszTemp=(char*)malloc(fieldlen+3);
		memcpy(pszTemp,iter->second,fieldlen+3);
		infos[iter->first]=pszTemp;
	}

}

ContactInfo::~ContactInfo() {
	for (map<Info_Index,char*>::iterator iter=infos.begin(); iter!=infos.end(); iter++)
		free(iter->second);
}

void ContactInfo::parseData(const unsigned char *buf, const int len)  
{
	int start = 0;
	int pos=0;
	char* pszTemp;
	infos.clear();

	pszTemp=(char*)malloc(sizeof(unsigned int)+3);
	*(unsigned short*)pszTemp=htons(4);
	memcpy(pszTemp+2,buf+pos,4); pos+=4;
	pszTemp[6]=0;
	infos[Info_qqID]=pszTemp;

	pos+=4; // 4 zeros

	unsigned short fields;
	unsigned short fieldtype;
	unsigned short fieldlen;

	fields=htons(*(unsigned int*)(buf+pos)); pos+=2;

	// Implementation Notice: due to write-back support, first 2 bytes are used as length of data
	// One extra null byte is added for each data, so UTF-8 strings can be processed as-is
	// So each data will be 3 bytes longer than designated size.
	for (unsigned short c=0; c<fields; c++) {
		fieldtype=htons(*(unsigned short*)(buf+pos)); pos+=2;
		fieldlen=htons(*(unsigned short*)(buf+pos))+2;
		pszTemp=(char*)malloc(fieldlen+1);
		((char*)memcpy(pszTemp,buf+pos,fieldlen))[fieldlen]=0;
		pos+=fieldlen;
		infos[(Info_Index)fieldtype]=pszTemp;
	}
}

bool ContactInfo::operator== ( const ContactInfo &rhs ) const
{
    if( infos.size() != rhs.details().size() ) return false;    
    return (infos==rhs.details());
}

ContactInfo &ContactInfo::operator= ( const ContactInfo &rhs )
{
	if( this == &rhs)   return *this;    
	char* pszTemp;
	// infos = rhs.details();    
	for (map<Info_Index,char*>::const_iterator iter=rhs.details().begin(); iter!=rhs.details().end(); iter++) {
		pszTemp=(char*)malloc(*(unsigned short*)iter->second+3);
		memcpy(pszTemp,iter->second,*(unsigned short*)iter->second+3);
		infos[iter->first]=pszTemp;
	}
	return *this;
}

const char* ContactInfo::at(const Info_Index index) { 
	return infos[index]; 
}

const char* ContactInfo::at(const int index) {
	return infos[(Info_Index)index];
} 

/*  ======================================================= */

GetUserInfoPacket::GetUserInfoPacket()
	: OutPacket(QQ_CMD_GET_USER_INFO, true),
	  qqNum(-1)
{
}

GetUserInfoPacket::GetUserInfoPacket(const int id)
	: OutPacket(QQ_CMD_GET_USER_INFO, true),
	  qqNum(id)
{
}

GetUserInfoPacket::GetUserInfoPacket(const GetUserInfoPacket &rhs)
	: OutPacket(rhs)
{
	qqNum = rhs.getUserQQ();
}

GetUserInfoPacket &GetUserInfoPacket::operator=(const GetUserInfoPacket &rhs)
{
	*((OutPacket *)this) = (OutPacket)rhs;
	qqNum = rhs.getUserQQ();
        return *this;
}

int GetUserInfoPacket::putBody(unsigned char *buf)
{
	int pos=0;
	pos+=EvaUtil::write16(buf+pos,0x0001);
	pos+=EvaUtil::write32(buf+pos,qqNum);
	memset(buf+pos,0,22); pos+=22;
	pos+=EvaUtil::write16(buf+pos,0x001a); // Number of fields
	pos+=EvaUtil::write16(buf+pos,0x4e22);
	pos+=EvaUtil::write16(buf+pos,0x4e25);
	pos+=EvaUtil::write16(buf+pos,0x4e26);
	pos+=EvaUtil::write16(buf+pos,0x4e27);
	pos+=EvaUtil::write16(buf+pos,0x4e29);
	pos+=EvaUtil::write16(buf+pos,0x4e2a);
	pos+=EvaUtil::write16(buf+pos,0x4e2b);
	pos+=EvaUtil::write16(buf+pos,0x4e2c);

	pos+=EvaUtil::write16(buf+pos,0x4e2d);
	pos+=EvaUtil::write16(buf+pos,0x4e2e);
	pos+=EvaUtil::write16(buf+pos,0x4e2f);
	pos+=EvaUtil::write16(buf+pos,0x4e30);
	pos+=EvaUtil::write16(buf+pos,0x4e31);
	pos+=EvaUtil::write16(buf+pos,0x4e33);
	pos+=EvaUtil::write16(buf+pos,0x4e35);
	pos+=EvaUtil::write16(buf+pos,0x4e36);

	pos+=EvaUtil::write16(buf+pos,0x4e37);
	pos+=EvaUtil::write16(buf+pos,0x4e38);
	pos+=EvaUtil::write16(buf+pos,0x4e3f);
	pos+=EvaUtil::write16(buf+pos,0x4e30);
	pos+=EvaUtil::write16(buf+pos,0x4e41);
	pos+=EvaUtil::write16(buf+pos,0x4e42);
	pos+=EvaUtil::write16(buf+pos,0x4e43);
	pos+=EvaUtil::write16(buf+pos,0x4e45);

	pos+=EvaUtil::write16(buf+pos,0x520b);
	pos+=EvaUtil::write16(buf+pos,0x520f);

    return pos;
}

/*  ======================================================= */

GetUserInfoReplyPacket::GetUserInfoReplyPacket(qqclient* client, unsigned char *buf, int len)
	: InPacket(client, buf, len)
{
}

GetUserInfoReplyPacket::GetUserInfoReplyPacket( const GetUserInfoReplyPacket &rhs)
	: InPacket(rhs)
{
	mContactInfo = rhs.contactInfo();
}

GetUserInfoReplyPacket &GetUserInfoReplyPacket::operator=(const GetUserInfoReplyPacket &rhs)
{
	*((InPacket *)this) = (InPacket)rhs;
	mContactInfo = rhs.contactInfo();
        return *this;
}

void GetUserInfoReplyPacket::parseBody() 
{
	if (htons(*(unsigned short*)decryptedBuf)!=0x0001)
		fprintf(stderr,"GetUserInfoReply->parseBody: Incorrect start of reply!\n");
	else if (decryptedBuf[2]!=0x00) {
		fprintf(stderr,"GetUserInfoReply->parseBody: Query result not 0!\n");
	} else {
		mContactInfo.parseData(decryptedBuf+3, bodyLength);
		int j = mContactInfo.details().size();
		if(j < QQ_CONTACT_FIELDS)
			fprintf(stderr, "GetUserInfoReply->parseBody: number of fields wrong\n");
		else if(j > QQ_CONTACT_FIELDS)
			fprintf(stderr, "GetUserInfoReply->parseBody: number of fields might be wrong!\n");
	}
}

/*  ======================================================= */

ModifyInfoPacket::ModifyInfoPacket( ) 
	: OutPacket(QQ_CMD_MODIFY_INFO, true)
{
}

ModifyInfoPacket::ModifyInfoPacket( const ContactInfo & info )
	: OutPacket(QQ_CMD_MODIFY_INFO, true),
	newInfo(info), currentPwd(""), newPwd("")
{
}

ModifyInfoPacket::ModifyInfoPacket( const ModifyInfoPacket & rhs )
	: OutPacket(rhs)
{
	currentPwd = rhs.getPassword();
	newPwd = rhs.getNewPassword();
	newInfo = rhs.getContactInfo();
}

ModifyInfoPacket & ModifyInfoPacket::operator =( const ModifyInfoPacket & rhs )
{
	*((OutPacket*)this) = (OutPacket)rhs;
	currentPwd = rhs.getPassword();
	newPwd = rhs.getNewPassword();
	newInfo = rhs.getContactInfo();	
	return *this;
}

int ModifyInfoPacket::putBody( unsigned char * buf )
{
	int pos=0;	
	/*
	if( currentPwd != "" && newPwd != ""){
		memcpy(buf, currentPwd.c_str(), currentPwd.length());
		pos+=currentPwd.length();
		buf[pos++] = DELIMIT;
		memcpy(buf+pos, newPwd.c_str(), newPwd.length());
		pos+=newPwd.length();
	}else
		buf[pos++] = DELIMIT;
	
	buf[pos++] = DELIMIT;
	
	for(int i=1; i<QQ_CONTACT_FIELDS; i++){
		memcpy(buf+pos, newInfo.at(i).c_str(),newInfo.at(i).length());
		pos+=newInfo.at(i).length();
		buf[pos++] = DELIMIT;
	}
	*/
	*(unsigned char*)buf=qqClient->data.user_token.len; pos++;
	memcpy(buf+pos,qqClient->data.user_token.data,qqClient->data.user_token.len);
	pos+=*(unsigned char*)buf;
	pos+=EvaUtil::write32(buf+pos,0x00010000);
	memset(buf+pos,0,20); pos+=20;
	pos+=EvaUtil::write16(buf+pos,newInfo.count());

	// Implementation Notice: Write-back support - first 2 bytes are used as length of data
	// So remember to align the data correctly, or crash will occur!
	// p.s. For UTF-8 strings, don't include the null character for data size
	for (map<ContactInfo::Info_Index,char*>::const_iterator iter=newInfo.details().begin(); iter!=newInfo.details().end(); iter++) {
		pos+=EvaUtil::write16(buf+pos,(unsigned short)iter->first); // Field Type
		pos+=EvaUtil::write16(buf+pos,*(unsigned short*)iter->second); // Field Size
		memcpy(buf+pos,iter->second+2,*(unsigned short*)iter->second); // Field Data
		pos+=*(unsigned short*)iter->second;
	}

	return pos;
}

/*  ======================================================= */

ModifyInfoReplyPacket::ModifyInfoReplyPacket(qqclient* client, unsigned char * buf, int len )
	: InPacket(client, buf, len),
	accepted(false)
{
}

ModifyInfoReplyPacket::ModifyInfoReplyPacket( const ModifyInfoReplyPacket & rhs )
	: InPacket(rhs)
{
	accepted = rhs.isAccepted();
}

ModifyInfoReplyPacket & ModifyInfoReplyPacket::operator =( const ModifyInfoReplyPacket & rhs )
{
	*((InPacket*)this) = (InPacket)rhs;
	accepted = rhs.isAccepted();
	return *this;
}

void ModifyInfoReplyPacket::parseBody( )
{
	/*
	char *str = (char *)malloc((bodyLength+1) * sizeof(char));
	memcpy(str, decryptedBuf, bodyLength);
	str[bodyLength]=0x00;
	
	char myQQ[20];
	sprintf(myQQ, "%d", getQQ());
	char *pos = strstr(str, myQQ);
	if( pos != str )
		accepted = false;
	else
		accepted = true;
	*/
	accepted=decryptedBuf[1]==1;
}

