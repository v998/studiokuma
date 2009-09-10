/*
Miranda Translator.NET

Copyright 2000-2008 Miranda IM project,
Copyright 2008 Stark Wong

all portions of this codebase are copyrighted to Stark Wong, and the people
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

Resource File (RC) Capture Engine
*/

using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Xml;

namespace MIMTranslator
{
    class Engine_RC: Engine_Interface
    {
        ArrayList blackList;
        ArrayList whiteList;

        public string GetExtensions()
        {
            return ".rc";
        }

        private int MatchList(string s, ArrayList list, int start, bool matchstart)
        {
            int pos = 0;

            foreach (string s2 in list)
                if ((pos = s.IndexOf(s2, start)) > -1 && (!matchstart || pos==0))
                    return pos;

            return -1;
        }

        public ArrayList ProcessFile(string filename, StreamReader sr)
        {
            ArrayList al = new ArrayList();
            string s;
            bool isCapturing = false;
            int beginlevel = 0;

            while ((s=sr.ReadLine())!=null)
            {
                s=s.Trim();
                if (s.EndsWith("DISCARDABLE")) s = s.Substring(0, s.LastIndexOf("DISCARDABLE")).Trim();

                if (!isCapturing && (/*s.IndexOf("DIALOGEX") > -1||*/s.EndsWith("MENU"))||s.IndexOf("DIALOG")>-1) // the last form is for clist_nicer
                {
                    isCapturing = true;
                }
                else if (isCapturing && s.Equals("BEGIN"))
                {
                    beginlevel++;
                }
                else if (isCapturing && s.Trim().Equals("END"))
                {
                    beginlevel--;
                    if (beginlevel==0) isCapturing = false;
                }
                else if (isCapturing)
                {
                    s = s.Replace("\"\"", "\x01");
                    if (MatchList(s,whiteList,0,true)==0 && MatchList(s,blackList,0,false)==-1 && !(s.IndexOf("\"Sys")>-1 && s.IndexOf("32\"")>-1))
                    {
                        int start = s.IndexOf('"');
                        //int comma = s.IndexOf(',');
                        int end = s.IndexOf('"', start + 1);
                        int comma = s.IndexOf(',', end + 1);
                        if (start > -1 && end > -1 && (comma == -1 || comma > end) && end - start > 1)
                        {
                            s = s.Substring(start + 1, end - start - 1).Replace("\x01", "\"");
                            if (!s.Equals("Tree1") && s.Trim().Length > 0) al.Add(s);
                        }
                    }
                }

            }

            return al;
        }

        public void ReadConfig(XmlDocument xd, XmlNodeList whiteNodeList, XmlNodeList blackNodeList, XmlNodeList superNodeList)
        {
            blackList = new ArrayList();
            whiteList = new ArrayList();

            foreach (XmlNode xn in xd.SelectNodes("/config/wordlist/rc/blacklist/item"))
            {
                blackList.Add(xn.InnerText);
            }

            foreach (XmlNode xn in xd.SelectNodes("/config/wordlist/rc/whitelist/item"))
            {
                whiteList.Add(xn.InnerText);
            }
        }
    }
}
