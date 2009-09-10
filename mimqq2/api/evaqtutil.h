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
 
#ifndef EVAQTUTIL_H 
#define EVAQTUTIL_H
#define EvaRequestCustomizedPicEvent 65531
#define EvaPictureReadyEvent      65530
#define EvaSendPictureReadyEvent      65529

class QCustomEvent {
public:
	QCustomEvent(int t) { this->eventType=t; }
	int type() { return this->eventType; }
private:
	int eventType;
};

class EvaAskForCustomizedPicEvent : public QCustomEvent
{
public:
	EvaAskForCustomizedPicEvent() : QCustomEvent(EvaRequestCustomizedPicEvent) {};
	void setPicList( const std::list<CustomizedPic> &list) { picList = list; }
	std::list<CustomizedPic> getPicList() { return picList; }
	void setQunID(const int id) { qunID = id; }
	const int getQunID() const { return qunID; }
	void setQunIM(ReceivedQunIM* rqim) { qunIM=rqim; }
	ReceivedQunIM* getQunIM() { return qunIM; }
private:
	std::list<CustomizedPic> picList;
	int qunID;
	ReceivedQunIM* qunIM;
};

class EvaSendCustomizedPicEvent : public QCustomEvent
{
public:
	EvaSendCustomizedPicEvent() : QCustomEvent(EvaSendPictureReadyEvent) {};
	void setPicList( const std::list<OutCustomizedPic> &list) { picList = list; }
	std::list<OutCustomizedPic> getPicList() { return picList; }
	void setQunID(const int id) { qunID = id; }
	const int getQunID() const { return qunID; }
	void setMessage(const std::string msg) { message=msg; }
	const std::string getMessage() const { return message; }
private:
	std::list<OutCustomizedPic> picList;
	int qunID;
	std::string message;
};

class EvaPicReadyEvent : public QCustomEvent
{
public:
	EvaPicReadyEvent() : QCustomEvent(EvaPictureReadyEvent) {};
	void setFileName( const std::string &name) { fileName = name; }
	std::string getFileName() { return fileName; }
	
	void setTmpFileName( const std::string &name) { tmpFileName = name; }
	std::string getTmpFileName() { return tmpFileName; }
	
	void setQunID(const int id) { qunID = id; }
	const int getQunID() const { return qunID; }
private:
	int qunID;
	std::string fileName;
	std::string tmpFileName;
};

class EvaHelper// : public QThread
{
public:
	// static method for calculating md5 of a file, should be the absolute path. note: char *md5 must be allocated before calling this method
	static const bool getFileMD5(const std::string fileName, char *md5);

	// convert MD5 into string expression, all converted string are in upper case
	static const std::string md5ToString(const char *md5);
};

#endif
