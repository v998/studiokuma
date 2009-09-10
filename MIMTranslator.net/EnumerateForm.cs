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

UI for Files enumeration and Capturing
*/

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;

namespace MIMTranslator
{
    public partial class EnumerateForm : Form
    {
        private ArrayList filesList;
        private ArrayList removeFoldersList;
        private int folderCount;
        private XmlDocument phrasesXml = null;
        private ArrayList phrasesList;
        private DateTime lastdate;
        private Hashtable ignoreItemsHashtable;
        private Hashtable phraseFilesHashtable;
        
        public EnumerateForm()
        {
            InitializeComponent();

        }
        private void EnumerateCandidates(string folder)
        {
            if (DialogResult == DialogResult.Cancel) return;

            foreach (string s in Directory.GetDirectories(folder))
            {
                if (s.IndexOf("\\.cvs") > -1 || s.IndexOf("\\.svn") > -1) continue;
                /*
                XmlNodeList xnl = phrasesXml.SelectNodes("/phrases/directory[starts-with(@name,'" + s + "')]");
                if (xnl.Count > 0)
                {
                    if (Directory.GetLastWriteTime(s) > lastdate)
                        foreach (XmlNode xn in xnl)
                            phrasesXml.FirstChild.RemoveChild(xn);
                    else
                        continue;
                }
                */
                EnumerateCandidates(s);
            }

            if (DialogResult == DialogResult.Cancel) return;

            currentLabel.Text = folder;
            folderCount++;
            Application.DoEvents();

            foreach (string s in Directory.GetFiles(folder))
            {
                string n = (string)phraseFilesHashtable[s];

                if (n == null || long.Parse(n) != File.GetLastWriteTime(s).ToBinary())
                {
                    foreach (string s2 in Form1.fileTypesHashtable.Keys)
                        if (s.ToLower().EndsWith(s2))
                        {
                            filesList.Add(s);
                            break;
                        }
                }
                else
                {
                    phraseFilesHashtable.Remove(s);
                }
            }
            filesLabel.Text = filesList.Count.ToString();
            foldersLabel.Text = folderCount.ToString();
            Application.DoEvents();
        }

        private void CaptureFiles()
        {
            Engine_Interface ei;
            StreamReader sr;
            ArrayList al;
            int filecount=0;

            //phrasesList = new ArrayList();
            processLabel.Text = "Capturing phrases";
            foldersLabel.Text = filesLabel.Text = "0";
            foldersTitleLabel.Text = "Phrases:";
            enumerateProgressBar.Maximum = filesList.Count;
            enumerateProgressBar.Value = 0;
            Application.DoEvents();

            foreach (string s in filesList)
            {
                currentLabel.Text = s;
                Application.DoEvents();
                sr = new StreamReader(s, Encoding.Default, true);
                ei = (Engine_Interface)Form1.fileTypesHashtable[s.Substring(s.LastIndexOf('.')).ToLower()];
                al = ei.ProcessFile(s, sr);
                phrasesList.Add("; " + s);
                if (al.Count > 0)
                {
                    //bool added = false;
                    //phrasesList.Add("; " + s);
                    //phrasesList.AddRange(al);
                    foreach (string s2 in al)
                    {
                        if (ignoreItemsHashtable[s2]==null && s2.Trim().Length>0 && s2!="\r\n")
                        {
                            /*
                            if (!added)
                            {
                                added = true;
                                phrasesList.Add("; " + s);
                            }
                            */
                            phrasesList.Add(s2);
                        } else
                        {
                            Console.WriteLine("dup");
                        }
                    }
                    /*
                    foreach (string s2 in al)
                        if (phrasesList.IndexOf(s2) == -1)
                        {
                            if (!added) {
                                added = true;
                                phrasesList.Add("; " + s);
                            }
                            phrasesList.Add(s2);
                        }
                     */
                }
                sr.Close();
                enumerateProgressBar.Value++;
                filecount++;
                foldersLabel.Text = (phrasesList.Count - filecount).ToString();
                filesLabel.Text = enumerateProgressBar.Value.ToString()+"/"+enumerateProgressBar.Maximum.ToString();
                Application.DoEvents();
                if (DialogResult == DialogResult.Cancel) return;
            }

            //XmlDocument xd = new XmlDocument();
            //XmlNode rootNode = xd.CreateNode(XmlNodeType.Element, "phrases", "");
            XmlNode rootNode = phrasesXml.SelectSingleNode("/phrases");
            XmlNode childNode;
            XmlNode dirNode = null;
            XmlAttribute xa;
            //xd.AppendChild(rootNode);
            foreach (string s in phrasesList)
            {
                if (s.StartsWith("; "))
                {
                    dirNode = phrasesXml.CreateElement("directory");
                    xa = phrasesXml.CreateAttribute("name");
                    xa.Value = s.Substring(2);
                    dirNode.Attributes.Append(xa);
                    xa = phrasesXml.CreateAttribute("datetime");
                    xa.Value = File.GetLastWriteTime(s.Substring(2)).ToBinary().ToString();
                    dirNode.Attributes.Append(xa);
                    rootNode.AppendChild(dirNode);
                }
                else
                {
                    childNode = phrasesXml.CreateElement("item");
                    childNode.InnerText = s;
                    dirNode.AppendChild(childNode);
                }
            }
            phrasesXml.Save("phrases.xml");
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            timer1.Stop();
            DialogResult = DialogResult.OK;

            filesList = new ArrayList();
            removeFoldersList = new ArrayList();

            folderCount = 0;

            processLabel.Text = "Enumerating Candidates";
            foldersLabel.Text = filesLabel.Text = "0";

            XmlNode dateNode = Form1.configXml.SelectSingleNode("/config/lastupdate");
            phrasesXml = new XmlDocument();
            phraseFilesHashtable = new Hashtable();
            if (dateNode == null || !File.Exists("phrases.xml") || MessageBox.Show("Clear all cache and capture again?", "Recapture", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button2) == DialogResult.Yes)
            {
                phrasesXml.AppendChild(phrasesXml.CreateElement("phrases"));
                lastdate = DateTime.MinValue;
            }
            else
            {
                lastdate = DateTime.FromBinary(long.Parse(dateNode.InnerText));
                phrasesXml.Load("phrases.xml");

                foreach (XmlNode xn in phrasesXml.FirstChild.ChildNodes)
                {
                    phraseFilesHashtable[xn.Attributes["name"].Value]=xn.Attributes["datetime"].Value;
                }
            }

            //lastdate = DateTime.MinValue;

            phrasesList = new ArrayList();

            foreach (XmlNode xn in Form1.configXml.SelectNodes("/config/source/item"))
            {
                try {
                    EnumerateCandidates(xn.InnerText);
                } 
                catch (DirectoryNotFoundException ex)
                {
                    MessageBox.Show("Specified Source path is invalid. Check settings.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

            if (File.Exists("ignore.xml"))
            {
                XmlDocument xd = new XmlDocument();
                xd.Load("ignore.xml");
                ignoreItemsHashtable = new Hashtable();

                foreach (XmlNode xn in xd.FirstChild.ChildNodes)
                {
                    ignoreItemsHashtable[xn.InnerText] = 1;
                }
            }

            foreach (string s in phraseFilesHashtable.Keys)
            {
                XmlNode xn = phrasesXml.SelectSingleNode("/phrases/directory[@name='" + s + "']");
                xn.ParentNode.RemoveChild(xn);
            }

            if (DialogResult != DialogResult.Cancel)
            {
                CaptureFiles();
                XmlNode xn = Form1.configXml.SelectSingleNode("/config");
                XmlNode xn2 = xn.SelectSingleNode("lastupdate");
                if (xn2 == null)
                {
                    xn2 = Form1.configXml.CreateElement("lastupdate");
                    xn.AppendChild(xn2);
                }
                xn2.InnerText = DateTime.Now.ToBinary().ToString();
                Form1.configXml.Save("config.xml");
                MessageBox.Show("Completed");
            }

            if (DialogResult == DialogResult.Cancel)
                MessageBox.Show("Cancelled");

            Close();
        }
    }
}