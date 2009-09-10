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
/*
#include "evacachedfile.h"
#include "evautil.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "../MirandaQQ.h"
*/
#include "stdafx.h"

#define InfoFileName_Ext       ".info"
#define CacheFileName_Ext      ".cache"

/*!
 * This class is to implement the file related functionalities of file transfering.
 * \a EvaCachedFile provides loading fragment from a file and save a fragment to a 
 * temporary file and method to generate the final file from the temporary cached 
 * file.
 *
 */

EvaCachedFile::EvaCachedFile(const string &srcDir, const string &srcFilename, const unsigned int size)
	: m_IsLoading(false), m_DirPath(srcDir), m_FileName(srcFilename), 
	m_FileSize(size), m_State(ENone)
{
	changeFileInfo();
}

// we know that we don't need the info file for loading
EvaCachedFile::EvaCachedFile(const string &srcDir, const string &srcFilename)
	: m_IsLoading(true), m_DirPath(srcDir), m_FileName(srcFilename), m_CachedFileName(""),
	m_State(ENone)
{
	string filePath = m_DirPath + "\\" + m_FileName;
	if (_access(filePath.c_str(),0)) {
		util_log(0, "EvaCachedFile::constructor -- \"%s\" dosen't exist!\n", filePath.c_str());
		m_State = ENotExists;
		return;
	}
}

EvaCachedFile::~EvaCachedFile()
{
}

const bool EvaCachedFile::setFileInfo(const string &fileName, const unsigned int size)
{
	m_FileName = fileName;
	m_FileSize = size;
	changeFileInfo();
	return true;
	//return changeFileInfo();
}

const bool EvaCachedFile::changeFileInfo()
{
	m_CachedFileName = m_FileName + CacheFileName_Ext;
	m_InfoFileName = m_FileName + InfoFileName_Ext;

	string filePath = m_DirPath + "\\" + m_CachedFileName;
	if (!_access(filePath.c_str(),0)) {
		util_log(0, "EvaCachedFile::constructor -- \"%s\" already exist!\n", filePath.c_str());
		m_State = EExists;
		return false;
	}
	//m_InfoFileName = m_FileName + InfoFileName_Ext;
	filePath = m_DirPath + "/" + m_InfoFileName;
	if (!_access(filePath.c_str(),0)) {
		util_log(0, "EvaCachedFile::constructor -- \"%s\" already exists! delete it!\n", filePath.c_str());
		if(!DeleteFileA(filePath.c_str())){
			util_log(0, "EvaCachedFile::constructor -- cannot remove \"%s\"!\n", filePath.c_str());
			m_State = EError;
			return false;
		}
	}
	return true;
}

/*!
 *  save one fragment onto disk
 * 
 * \param offset the abusolute offset address, the start position
 * \param len the length of wanted
 * \param buf the contents to save
 * \return true if saving success, otherwise false
 *
 * \note this method is to save the data into a temporary file which 
 *         each fragment contains 8 extra header bytes \e
 * 
 */


const bool EvaCachedFile::saveFragment(const unsigned int offset, 
					const unsigned int len, 
					unsigned char *buf)
{
	if(m_IsLoading) return false;
	if( ! isNewFragment(offset, len)){
		util_log(0,"EvaCachedFile::saveFragment -- already got this fragment!\n");
		return true; // if we got it already, always return true
	}

	string fullpath = m_DirPath + "\\" + m_CachedFileName;
	FILE* fp;
	if(!(fp=fopen(fullpath.c_str(),"ab"))){
		util_log(0,"EvaCachedFile::saveFragment -- cannot open \"%s\"!\n", fullpath.c_str());
		return false;
	}
	char strHeader[8];
	memcpy(strHeader, &offset, 4);
	memcpy(strHeader+4, &len, 4);
	fwrite(strHeader,8,1,fp);
	fwrite((char*)buf,len,1,fp);
	fclose(fp);

	if( ! updateInfoFile(offset, len)) return false;

	// let user control this
// 	if( isFinished() )
// 		return generateDestFile();

	return true;
}

/*!
 *  read one fragment from the source file in the disk
 *
 * \param offset the abusolute start position of this read operation
 * \param len the length of read
 * \param buf the pre-allocated buffer all the read data written into with length of len
 * \return the exact bytes of read
 */

const unsigned int EvaCachedFile::getFragment(const unsigned int offset, 
						const unsigned int len, 
						unsigned char *buf)
{
	if(!m_IsLoading) return false;

	unsigned int bytesRead = 0;
	FILE* fp;
	if(!(fp=fopen(string(m_DirPath+"\\"+m_FileName).c_str(),"rb"))){
		util_log(0,"EvaCachedFile::getFragment -- \"%s\" dosen't exist!\n", m_FileName.c_str());
		return bytesRead;
	}
	if(!fseek(fp,offset,SEEK_SET))
		bytesRead = fread((char*)buf,1,len,fp);
	fclose(fp);
	if( !bytesRead){
		m_State = EReadError;
		return 0;
	}
	return bytesRead;
}

void EvaCachedFile::setCheckValues(const unsigned char *fileNameMd5, const unsigned char *fileMd5)
{
	memcpy(m_FileNameMd5, fileNameMd5, 16);
	memcpy(m_FileMd5, fileMd5, 16);	
}

const bool EvaCachedFile::isFinished()
{
	return isInfoFinished();
}

const bool EvaCachedFile::isNewFragment(const unsigned int offset, const unsigned int /*len*/)
{
	if(m_IsLoading) return false;
	map<unsigned int, unsigned int>::iterator iter;
	iter = m_FragInfo.find(offset);
	// we might need to check the length of this fragment
	if(iter != m_FragInfo.end()){
		m_State = EDupFragment;
		return false;
	}
	return true;
}

const bool EvaCachedFile::updateInfoFile(const unsigned int offset, const unsigned int len)
{
	if(m_IsLoading) return false;
	FILE* fp;
	if(!(fp=fopen(string(m_DirPath+"\\"+m_InfoFileName).c_str(),"wb"))){
		util_log(0, "EvaCachedFile::updateInfoFile -- cannot update info file!\n");
		m_State = EInfoOpen;
		return false;
	}
	unsigned char namesize=m_FileName.size();
	m_FragInfo[offset] = len;

	// we save the basic info first
	fwrite(&namesize,1,1,fp);
	fwrite(m_FileName.c_str(),namesize,1,fp);
	fwrite(&m_FileSize,4,1,fp);
	fwrite(m_FileNameMd5,16,1,fp);
	fwrite(m_FileMd5,16,1,fp);

	map<unsigned int, unsigned int>::iterator iter;
	unsigned int qoffset, qlen;
	for(iter=m_FragInfo.begin(); iter!=m_FragInfo.end(); ++iter){
		qoffset = iter->first;
		qlen = iter->second;
		fwrite(&qoffset,4,1,fp);
		fwrite(&qlen,4,1,fp);
	}
	fclose(fp);
	return true;
}

const bool EvaCachedFile::loadInfoFile()
{
	if(m_IsLoading) return false;
	FILE* fp;
	if(!(fp=fopen(string(m_DirPath+"\\"+m_InfoFileName).c_str(),"rb"))){
		util_log(0, "EvaCachedFile::loadInfoFile -- no info file available.\n");
		m_State = EInfoOpen;
		return false;
	}
	
	char* fileName;
	unsigned char fileNameLen;
	fread(&fileNameLen,1,1,fp);
	fileName=new char[fileNameLen+1];
	fread(fileName,fileNameLen,1,fp);
	fileName[fileNameLen]=0;

	if (string(fileName)!=m_FileName) {
		delete[] fileName;
		fclose(fp);
		return false;
	}
	delete[] fileName;

	unsigned int tmp;

	fread(&tmp,4,1,fp);
	if(tmp != m_FileSize) {
		fclose(fp);
		return false;
	}

	char *fnmd5 = new char[16];
	fread(fnmd5,16,1,fp);
	if(memcmp(fnmd5, m_FileNameMd5, 16)){
		m_State = EMd5Error;
		fclose(fp);
		delete [] fnmd5;
		return false;
	}
	delete [] fnmd5;

	char *fmd5 = new char[16];
	fread(fmd5,16,1,fp);
	if(memcmp(fmd5, m_FileMd5, 16)){
		m_State = EMd5Error;
		fclose(fp);
		delete [] fmd5;
		return false;
	}
	delete [] fmd5;

	unsigned int qoffset, qlen;
	while(!feof(fp)){
		fread(&qoffset,4,1,fp);
		fread(&qlen,4,1,fp);
		m_FragInfo[qoffset]=qlen;
	}
	fclose(fp);
	return true;
}

// we only test the size of the file
const bool EvaCachedFile::isInfoFinished()
{
	if(m_IsLoading) return false;

	unsigned int tmp = 0;
	map<unsigned int, unsigned int>::iterator iter;
	//Q_UINT32 qoffset, qlen;
	for(iter=m_FragInfo.begin(); iter!=m_FragInfo.end(); ++iter){
		tmp += iter->second;
	}
	util_log(0,"EvaCachedFile::isInfoFinished -- tmp: %d, m_FileSize: %d\n", tmp, m_FileSize);
	if(tmp == m_FileSize) return true;
	return false;
}

const unsigned int EvaCachedFile::getNextOffset()
{
	if(m_IsLoading) return 0;

	unsigned int offset = 0;
	map<unsigned int, unsigned int>::iterator iter;
	for(iter=m_FragInfo.begin(); iter!=m_FragInfo.end(); ++iter){
		offset += iter->second;
	}
	return offset;
}

const bool EvaCachedFile::isFileCorrect()
{
	if(m_IsLoading) return true; // if we are loading file, this would be always true

	// check dest-file size
	FILE* fp=fopen(string(m_DirPath + "\\" + m_FileName).c_str(),"rb");
	if (!fp)
		return false;

	fseek(fp,0,SEEK_END);
	
	if(ftell(fp) != m_FileSize) return false;

	// check dest-file md5 value
	char *md5 = new char[16];
	if(!getSourceFileMd5(md5)){
		m_State = EMd5Error;
		fclose(fp);
		return false;
	}

	if(memcmp(m_FileMd5, md5, 16)){
		m_State = EMd5Error;
		fclose(fp);
		return false;
	}

	// check dest-file name md5
	getSourceFileNameMd5(md5);
	if(memcmp(m_FileNameMd5, md5, 16)){
		util_log(0, "EvaCachedFile::isFileCorrect -- \"%s\" file name saved might be wrong but file contents should be all right.\n", m_FileName.c_str());
	}
	delete []md5;
	fclose(fp);
	return true;
}

const bool EvaCachedFile::generateDestFile()
{
	if(m_IsLoading) return false;
	if(m_DirPath.empty()) return false;
	string cachedFileName = m_DirPath + "/" + m_FileName + CacheFileName_Ext;
	string destFileName = m_DirPath + "/" + m_FileName;
	FILE* cached;
	FILE* dest;

	if(!(cached=fopen(cachedFileName.c_str(),"rb"))){
		util_log(0, "EvaCachedFile::generateDestFile -- cannot open cached file \"%s\"!\n", cachedFileName.c_str());
		return false;
	}
	if(!(dest=fopen(destFileName.c_str(),"wb"))){
		util_log(0, "EvaCachedFile::generateDestFile -- cannot create destination file \"%s\"!\n", destFileName.c_str());
		fclose(cached);
		return false;
	}

	char *buf; // we might create a fixed-size buffer to save time
	char strHeader[8];
	unsigned int offset, len;
	while(!feof(cached)){
		if (fread(strHeader,1,8,cached)!=8) break;
		memcpy(&offset, strHeader, 4);
		memcpy(&len, strHeader+4, 4);
		buf = new char [len];
		fread(buf,len,1,cached);

		fseek(dest,offset,SEEK_SET);
		fwrite(buf,len,1,dest);
		delete []buf;
	}
	fclose(cached);
	fclose(dest);
	if(!isFileCorrect()){
		//dest.remove();
		util_log(0, "EvaCachedFile::generateDestFile -- md5 checking wrong\n");
		return false;
	}
	// actually we got all we want, we don't care about the result of removing following files
	DeleteFileA(cachedFileName.c_str());
	DeleteFileA(string(m_DirPath + "/" + m_InfoFileName).c_str());
	m_FragInfo.clear();
	return true;
}

const bool EvaCachedFile::calculateFileMd5(const string& fullpath, char *md5Buf)
{
	FILE* fp=fopen(fullpath.c_str(),"rb");
	if(!fp){
		util_log(0, "EvaCachedFile::calculateFileMd5 -- \"%s\" dosen't exist!\n", fullpath.c_str());
		return false;
	}
	fseek(fp,0,SEEK_END);
	unsigned int len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(len > MaxMd5CheckLength)
		len = MaxMd5CheckLength;
	char *buf = new char[len];

	unsigned int numRead = fread(buf,1,len,fp);
	fclose(fp);
	if(numRead!=len){
		delete []buf;
		return false;
	}
	memcpy(md5Buf, EvaUtil::doMd5(buf, len), 16);
	delete []buf;
	return true;
}

const bool EvaCachedFile::getSourceFileMd5(char *md5)
{
	if(!md5) return false;
	if(m_DirPath.empty() || m_FileName.empty()) return false;
	return calculateFileMd5(m_DirPath + "\\" + m_FileName, md5);
}

const bool EvaCachedFile::getSourceFileNameMd5(char *md5)
{
	if(!md5) return false;
	memcpy(md5, EvaUtil::doMd5((char*)m_FileName.c_str(), m_FileName.size()), 16);
	return true;
}

const unsigned int EvaCachedFile::getFileSize()
{
	if(!m_IsLoading) return m_FileSize;
	FILE* fp=fopen(string(m_DirPath+"\\"+m_FileName).c_str(),"rb");
	if (!fp) return 0;

	fseek(fp,0,SEEK_END);
	unsigned int fSize=ftell(fp);
	fclose(fp);
	return fSize;
}

void EvaCachedFile::setDestFileDir( const string & dir )
{
	if(m_IsLoading) return;
	m_DirPath = dir;
	changeFileInfo();
}
