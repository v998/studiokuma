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

Translation Template (TXT->TXR) Capture Engine
*/

using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Xml;

namespace MIMTranslator
{
    class Engine_TXT: Engine_Interface
    {
        public string GetExtensions()
        {
            return ".txr";
        }

        public ArrayList ProcessFile(string filename, StreamReader sr)
        {
            ArrayList al = new ArrayList();
            string s;

            while ((s=sr.ReadLine())!=null)
            {
                if (s.StartsWith("[") && s.Trim().EndsWith("]"))
                {
                    s=s.Trim();
                    al.Add(s.Substring(1, s.Length - 2));
                }
            }

            return al;
        }

        public void ReadConfig(XmlDocument xd, XmlNodeList whiteNodeList, XmlNodeList blackNodeList, XmlNodeList superNodeList)
        {
        }
    }
}
