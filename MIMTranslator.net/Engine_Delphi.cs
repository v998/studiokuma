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

Delphi/Pascal DPR/PAS/DFM Capture Engine
*/

using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Xml;

namespace MIMTranslator
{
    class Engine_Delphi: Engine_Interface
    {
        ArrayList whiteList;
        ArrayList blackList;
        ArrayList superList;

        public string GetExtensions()
        {
            return ".dpr;.dfm;.pas";
        }

        private int MatchList(string s, ArrayList list, int start)
        {
            int pos = 0;

            foreach (string s2 in list)
                if ((pos=s.IndexOf(s2,start)) > -1)
                    return pos;

            return -1;
        }

        public ArrayList ProcessDFM(StreamReader sr)
        {
            ArrayList al = new ArrayList();
            string s;

            while ((s = sr.ReadLine()) != null)
            {
                if (s.IndexOf("'") > -1 && s.IndexOf("HelpKeyword = '") == -1 && s.IndexOf("Font.Name = '") == -1)
                {
                    s = s.Replace("'#39'", "\x01").Replace("'#13'", @"\n").Replace("'#13#10'", @"\r\n");
                    int start = s.IndexOf("'");
                    int end = s.IndexOf("'", start + 1);

                    if (start > -1 && end > -1 && end-start!=1)
                    {
                        al.Add(s.Substring(start+1,end-start-1).Replace("\x01,", "'"));
                    }
                }

            }

            return al;
        }

        public ArrayList ProcessPAS(string filename, StreamReader sr)
        {
            ArrayList al = new ArrayList();
            string s, s2;
            bool isSuper = false;
            int line = 1;

            while ((s = sr.ReadLine()) != null)
            {
                if (!isSuper && MatchList(s, superList, 0) > -1)
                {
                    isSuper = true;
                    Console.WriteLine("S: " + s);
                }

                int pos = 0;

                while (isSuper || (pos = MatchList(s, whiteList, pos)) > -1)
                {
                    s = s.Replace("'#39'", "\x01").Replace("''","\x01");
                    pos = s.IndexOf('\'', pos);
                    if (pos > -1)
                    {
                        pos++;
                        int endpos = s.IndexOf('\'', pos);
                        if (endpos == -1)
                        {
                            //throw new IndexOutOfRangeException("Unterminated line found at line " + line.ToString() + "\n\n" + s);
                            Console.WriteLine("Warning: Unterminated line found at {0}:{1}", filename, line);
                            break;
                        }
                        s2 = s.Substring(pos, endpos - pos).Replace("\x01", "'");
                        if (MatchList(s2, blackList, 0) == -1 && endpos-pos!=0 )
                        {
                            al.Add(s2);
                        }
                        pos = endpos + 1;
                    }
                    else
                        break;

                }
                if (isSuper && s.IndexOf(");") > -1) isSuper = false;
                line++;
            }

            return al;
        }

        public ArrayList ProcessFile(string filename, StreamReader sr)
        {
            if (filename.ToLower().EndsWith(".dfm"))
                return ProcessDFM(sr);
            else
                return ProcessPAS(filename, sr);
        }

        public void ReadConfig(XmlDocument xd, XmlNodeList whiteNodeList, XmlNodeList blackNodeList, XmlNodeList superNodeList)
        {
            whiteList = new ArrayList();
            blackList = new ArrayList();
            superList = new ArrayList();

            ArrayList al = whiteList;
            XmlNodeList xnl = null;
            string s = null;

            for (int c = 0; c < 3; c++)
            {
                switch (c)
                {
                    case 0: xnl = whiteNodeList; al = whiteList; s = "whitelist"; break;
                    case 1: xnl = blackNodeList; al = blackList; s = "blacklist"; break;
                    case 2: xnl = superNodeList; al = superList; s = "superlist"; break;
                }

                foreach (XmlNode xn in xnl)
                {
                    al.Add(xn.InnerText);
                }

                foreach (XmlNode xn in xd.SelectNodes("/config/wordlist/delphi/"+s+"/item"))
                {
                    al.Add(xn.InnerText);
                }
            }
        }
    }
}
