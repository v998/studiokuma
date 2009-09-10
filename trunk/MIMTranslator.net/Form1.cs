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

Main dialog and small works
*/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;
using System.Collections;
using System.Reflection;
using System.Globalization;

namespace MIMTranslator
{
    public partial class Form1 : Form
    {
        public static Hashtable fileTypesHashtable;
        private ArrayList enginesList;
        public static XmlDocument configXml=null;

        public Form1()
        {
            InitializeComponent();
            configXml = new XmlDocument();
            configXml.Load("config.xml");
            LoadEngines();
        }

        void LoadEngines()
        {
            enginesList = new ArrayList();
            fileTypesHashtable = new Hashtable();
            Assembly a;
            Engine_Interface ei;
            XmlNodeList whiteNodeList = configXml.SelectNodes("/config/wordlist/common/whitelist/item");
            XmlNodeList blackNodeList = configXml.SelectNodes("/config/wordlist/common/blacklist/item");
            XmlNodeList superNodeList = configXml.SelectNodes("/config/wordlist/common/superlist/item");

            foreach (XmlNode xn in configXml.SelectNodes("/config/engine/item"))
            {
                if (xn.Attributes["dll"]==null)
                    a = Assembly.GetExecutingAssembly();
                else
                    a = Assembly.LoadFrom(xn.Attributes["dll"].Value);

                ei = (Engine_Interface)a.CreateInstance(xn.InnerText);

                if (ei!=null)
                {
                    ei.ReadConfig(configXml, whiteNodeList, blackNodeList, superNodeList);

                    foreach (string s in ei.GetExtensions().Split(";".ToCharArray()))
                    {
                        fileTypesHashtable[s] = ei;
                    }

                    enginesList.Add(ei);
                }
            }
        }

        private void stepButton1_Click(object sender, EventArgs e)
        {
            if (new EnumerateForm().ShowDialog(this)==DialogResult.OK)
            {
                stepButton2.PerformClick();
            }
        }

        private void importTranslationsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "langpack(*.txt)|*.txt|All Files(*.*)|*.*";

            if (ofd.ShowDialog(this)==DialogResult.OK)
            {
                StreamReader sr = new StreamReader(ofd.OpenFile(), false);
                string s;
                for (int c=0; c<10; c++)
                {
                    if ((s = sr.ReadLine())==null)
                    {
                        return;
                    }
                    else
                    {
                        if (s.StartsWith("Locale:"))
                        {
                            CultureInfo ci = new CultureInfo(int.Parse(s.Substring(7).Trim(), System.Globalization.NumberStyles.AllowHexSpecifier));
                            sr.DiscardBufferedData();
                            sr = new StreamReader(sr.BaseStream, Encoding.GetEncoding(ci.TextInfo.ANSICodePage));
                            sr.BaseStream.Seek(0, SeekOrigin.Begin);
                            break;
                        }
                    }
                }

                XmlDocument xd=new XmlDocument();
                XmlElement xeRoot;
                if (File.Exists("dictionary.xml") && MessageBox.Show(this, "Clear existing translations?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
                {
                    xd.Load("dictionary.xml");
                    xeRoot = (XmlElement)xd.SelectSingleNode("/translations");
                }
                else
                {
                    xeRoot = xd.CreateElement("translations");
                    xd.AppendChild(xeRoot);
                }

                XmlElement xe, xe2;
                string eng=null;
                int count = 0;

                while ((s=sr.ReadLine())!=null)
                {
                    if (s.StartsWith("["))
                        eng = s.Substring(1, s.Length - 2);
                    else if (eng!=null)
                    {
                        xe = xd.CreateElement("item");
                        xeRoot.AppendChild(xe);
                        xe2 = xd.CreateElement("source");
                        xe2.InnerText = eng;
                        xe.AppendChild(xe2);
                        xe2 = xd.CreateElement("target");
                        xe2.InnerText = s;
                        xe.AppendChild(xe2);
                        eng = null;
                        count++;
                    }

                }
                sr.Close();

                xd.Save("dictionary.xml");

                MessageBox.Show(string.Format("{0} Translations added, total {1} dictionary items", count, xeRoot.ChildNodes.Count),"Process Completed",MessageBoxButtons.OK,MessageBoxIcon.Information);
            }
        }

        private void stepButton2_Click(object sender, EventArgs e)
        {
            XmlDocument xdDict = new XmlDocument();
            Hashtable ht = new Hashtable();
            XmlDocument phrasesXml = new XmlDocument();
            int translated = 0, untranslated = 0;
            if (File.Exists("dictionary.xml")) xdDict.Load("dictionary.xml");
            phrasesXml.Load("phrases.xml");

            foreach (XmlNode xn in xdDict.SelectNodes("/translations/item"))
            {
                ht[xn.FirstChild.InnerText] = xn.LastChild.InnerText;
            }

            foreach (XmlNode xn in phrasesXml.FirstChild.ChildNodes)
            {
                // Directories
                foreach (XmlNode xn2 in xn.ChildNodes)
                {
                    // items
                    string s = (string)ht[xn2.InnerText];

                    if (s != null)
                    {
                        XmlAttribute xa = phrasesXml.CreateAttribute("translated");
                        xa.Value = s;
                        xn2.Attributes.Append(xa);
                    }
                    else
                        untranslated++;

                    translated++;
                }
            }

            phrasesXml.Save("translated.xml");
            MessageBox.Show(string.Format("Translation Completed, total {0}, {1} untranslated", translated, untranslated), "Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void stepButton3_Click(object sender, EventArgs e)
        {
            UntranslatedForm f = new UntranslatedForm();

            {
                XmlDocument xd = new XmlDocument();
                xd.Load("translated.xml");

                DataTable dt = new DataTable();
                DataRow dr;
                Hashtable htDistinct = new Hashtable();
                dt.Columns.Add("source");
                dt.Columns[0].ReadOnly = true;
                dt.Columns.Add("target");

                foreach (XmlNode xn in xd.SelectNodes("/phrases/directory/item[not(@translated)]"))
                {
                    if (htDistinct[xn.InnerText] == null && xn.InnerText.Length > 0)
                    {
                        dr = dt.NewRow();
                        dr[0] = xn.InnerText;
                        dr[1] = xn.InnerText;
                        //try {
                        dt.Rows.Add(dr);
                        /*}
                        catch (ConstraintException ce)
                        {

                        }*/
                        htDistinct[xn.InnerText] = 1;
                    }
                }
                f.dataGridView1.DataSource = dt;
                f.dataGridView1.Columns[0].HeaderText = "Original";
                f.dataGridView1.Columns[1].HeaderText = "Translated";
                f.dataGridView1.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            }

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                DataTable dt = (DataTable)f.dataGridView1.DataSource;
                int count = 0;
                XmlDocument xd = new XmlDocument();
                XmlNode xnItem;
                XmlNode xnInner;

                if (File.Exists("dictionary.xml"))
                    xd.Load("dictionary.xml");
                else
                    xd.AppendChild(xd.CreateElement("translations"));

                XmlNode xnRoot = xd.FirstChild;

                foreach (DataRow dr in dt.Rows)
                {
                    if (!((string)dr[0]).Equals((string)dr[1]))
                    {
                        xnItem = xd.CreateElement("item");
                        xnRoot.AppendChild(xnItem);
                        xnInner = xd.CreateElement("source");
                        xnItem.AppendChild(xnInner);
                        xnInner.InnerText = (string)dr[0];
                        xnInner = xd.CreateElement("target");
                        xnItem.AppendChild(xnInner);
                        xnInner.InnerText = (string)dr[1];

                        count++;
                    }
                }

                xd.Save("dictionary.xml");
                MessageBox.Show(string.Format("{0} items added", count));

                stepButton2.PerformClick();
            }
        }

        private void importIgnoresPhrasesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Miranda Translator exported file(*.txt)|*.txt|All Files(*.*)|*.*";

            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                StreamReader sr = new StreamReader(ofd.OpenFile(), false);
                string s;
                XmlDocument xd = new XmlDocument();
                XmlElement xeRoot;
                if (File.Exists("ignore.xml") && MessageBox.Show(this, "Clear existing items?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
                {
                    xd.Load("ignore.xml");
                    xeRoot = (XmlElement)xd.SelectSingleNode("/phrases");
                }
                else
                {
                    xeRoot = xd.CreateElement("phrases");
                    xd.AppendChild(xeRoot);
                }

                XmlElement xe;
                int count = 0;

                while ((s = sr.ReadLine()) != null)
                {
                    if (s.StartsWith("["))
                    {
                        xe = xd.CreateElement("item");
                        xeRoot.AppendChild(xe);
                        xe.InnerText = s.Substring(1, s.Length - 2);
                        count++;
                    }
                }
                sr.Close();

                xd.Save("ignore.xml");

                MessageBox.Show(string.Format("{0} items added, total {1} ignored items", count, xeRoot.ChildNodes.Count), "Process Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private void stepButton4_Click(object sender, EventArgs e)
        {
            XmlDocument xdIgnore = new XmlDocument();
            XmlDocument xdTranslated = new XmlDocument();
            XmlElement xeRoot;
            if (File.Exists("ignore.xml"))
            {
                xdIgnore.Load("ignore.xml");
                xeRoot = (XmlElement)xdIgnore.SelectSingleNode("/phrases");
            }
            else
            {
                xeRoot = xdIgnore.CreateElement("phrases");
                xdIgnore.AppendChild(xeRoot);
            }

            Hashtable htDistinct = new Hashtable();
            XmlNode xnNew;
            int count = 0;

            xdTranslated.Load("translated.xml");
            foreach (XmlNode xn in xdTranslated.SelectNodes("/phrases/directory/item[not(@translated)]"))
            {
                if (htDistinct[xn.InnerText] == null)
                {
                    xnNew = xdIgnore.CreateElement("item");
                    xeRoot.AppendChild(xnNew);
                    xnNew.InnerText = xn.InnerText;
                    
                    htDistinct[xn.InnerText] = 1;
                    count++;
                }
                xnNew = xn.ParentNode;
                xnNew.RemoveChild(xn);
                if (xnNew.ChildNodes.Count == 0) xnNew.ParentNode.RemoveChild(xnNew);
            }

            foreach (XmlNode xn in xeRoot.SelectNodes("*[not(text())]")) {
                xeRoot.RemoveChild(xn);
            }

            xdIgnore.Save("ignore.xml");
            xdTranslated.Save("translated.xml");

            MessageBox.Show(string.Format("{0} items added, total {1} ignored items", count, xeRoot.ChildNodes.Count), "Process Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void stepButton5_Click(object sender, EventArgs e)
        {
            XmlNode xnOutput = configXml.SelectSingleNode("/config/output");
            XmlNode xnFilename = xnOutput.SelectSingleNode("filename");
            if (xnFilename==null)
            {
                xnFilename = configXml.CreateElement("filename");
                xnFilename.InnerText="langpack_???.txt";
                xnOutput.AppendChild(xnFilename);
            }

            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "langpack (langpack_*.txt)|langpack_*.txt|All Files(*.*)|*.*";
            sfd.FileName = xnFilename.InnerText;
            if (sfd.ShowDialog(this)==DialogResult.OK)
            {
                xnFilename.InnerText = sfd.FileName;
                configXml.Save("config.xml");

                CultureInfo ci = new CultureInfo(int.Parse(xnOutput.SelectSingleNode("locale").InnerText, NumberStyles.AllowHexSpecifier));
                StreamWriter sw = new StreamWriter(sfd.OpenFile(), Encoding.GetEncoding(ci.TextInfo.ANSICodePage));
                DateTime dtExe=File.GetLastWriteTime(Application.ExecutablePath);

                sw.WriteLine("Miranda Language Pack Version 1");
                sw.WriteLine("Language: " + xnOutput.SelectSingleNode("language").InnerText);
                sw.WriteLine("Locale: " + xnOutput.SelectSingleNode("locale").InnerText);
                sw.WriteLine("Authors: " + xnOutput.SelectSingleNode("authors").InnerText);
                sw.WriteLine("Author-email: "+xnOutput.SelectSingleNode("author-email").InnerText);
                sw.WriteLine("Plugins-included: " + xnOutput.SelectSingleNode("plugins-included").InnerText);
                sw.WriteLine();
                sw.WriteLine("; This file is created by Miranda Translator.NET {0} ({1}/{2}/{3})",Application.ProductVersion,dtExe.Year,dtExe.Month,dtExe.Day);
                sw.WriteLine("; Visit http://www.studiokuma.com/Miranda/helper/ for more information.");
                sw.WriteLine();

                XmlNode xnFLID = xnOutput.SelectSingleNode("flid");
                DateTime dtNow = DateTime.Now;

                if (xnFLID != null && xnFLID.InnerText != "")
                {
                    sw.WriteLine("; FLID: " + xnFLID.InnerText, dtNow.Year % 100, dtNow.Month, dtNow.Day);
                    sw.WriteLine();
                }

                sw.WriteLine("; Generated on {0} {1} Local Time", dtNow.ToShortDateString(), dtNow.ToShortTimeString());
                sw.WriteLine("; Additional Info: .net Framework v{0}", System.Environment.Version);
                sw.WriteLine();

                xnFLID = xnOutput.SelectSingleNode("prepend");
                if (xnFLID != null && xnFLID.InnerText != "")
                {
                    try {
                        StreamReader sr = new StreamReader(xnFLID.InnerText, Encoding.GetEncoding(ci.TextInfo.ANSICodePage));
                        sw.WriteLine(sr.ReadToEnd());
                        sr.Close();
                    }
                    catch (FileNotFoundException fnf)
                    {
                        MessageBox.Show("Warning: Prepend file not found. Check Settings.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }

                XmlDocument xd = new XmlDocument();
                Hashtable htDistinct=new Hashtable();

                xd.Load("translated.xml");
                foreach (XmlNode xn in xd.FirstChild.ChildNodes)
                {
                    // Directories
                    bool fNew = true;

                    foreach (XmlNode xn2 in xn.ChildNodes)
                    {
                        if (htDistinct[xn2.InnerText]==null)
                        {
                            if (xn2.Attributes["translated"]!=null)
                            {
                                if (fNew)
                                {
                                    sw.WriteLine();
                                    sw.WriteLine("; {0}", xn.Attributes["name"].Value);
                                    fNew = false;
                                }
                                sw.WriteLine("[{0}]",xn2.InnerText);
                                sw.WriteLine(xn2.Attributes["translated"].Value);
                            }
                            htDistinct[xn2.InnerText]=1;
                        }
                    }
                }

                xnFLID = xnOutput.SelectSingleNode("append");
                if (xnFLID != null && xnFLID.InnerText != "")
                {
                    try
                    {
                        StreamReader sr = new StreamReader(xnFLID.InnerText, Encoding.GetEncoding(ci.TextInfo.ANSICodePage));
                        sw.WriteLine(sr.ReadToEnd());
                        sr.Close();
                    }
                    catch (FileNotFoundException fnf)
                    {
                        MessageBox.Show("Warning: Append file not found. Check Settings.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                }

                sw.Close();

                MessageBox.Show("OK");
            }
        }

        private void settingsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new SettingsForm(configXml).ShowDialog(this);
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("MirandaTranslator.NET v" + Application.ProductVersion + "\n\nCopyright(C) 2008 Studio Kuma. All Rights Reserved.\nhttp://www.studiokuma.com/Miranda/helper/","About",MessageBoxButtons.OK,MessageBoxIcon.Information);
        }

        private void sourceSettingButton_Click(object sender, EventArgs e)
        {
            SettingsForm f = new SettingsForm(configXml);
            f.sourceListButton_Click(sender, e);
            f.Dispose();
        }

        private void editPhrasesButton_Click(object sender, EventArgs e)
        {
            UntranslatedForm f = new UntranslatedForm();
            f.filename = "dictionary.xml";
            f.ShowDialog();
        }

        private void editIgnoreButton_Click(object sender, EventArgs e)
        {
            UntranslatedForm f = new UntranslatedForm();
            f.filename = "ignore.xml";
            f.ShowDialog();
        }

        private void importIgnoredButton_Click(object sender, EventArgs e)
        {
            importIgnoresPhrasesToolStripMenuItem.PerformClick();
        }

        private void importPhrasesButton_Click(object sender, EventArgs e)
        {
            importTranslationsToolStripMenuItem.PerformClick();
        }

        private void exportDictionaryButton_Click(object sender, EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "Langpack compatible files(*.txt)|*.txt|All Files(*.*)|*.*";
            if (sfd.ShowDialog(this) == DialogResult.OK)
            {
                XmlNode xnOutput = configXml.SelectSingleNode("/config/output");
                CultureInfo ci = new CultureInfo(int.Parse(xnOutput.SelectSingleNode("locale").InnerText, NumberStyles.AllowHexSpecifier));
                StreamWriter sw = new StreamWriter(sfd.OpenFile(), Encoding.GetEncoding(ci.TextInfo.ANSICodePage));
                XmlDocument xd = new XmlDocument();
                int c = 0;
                xd.Load("dictionary.xml");
                foreach (XmlNode xn in xd.FirstChild.ChildNodes)
                {
                    sw.WriteLine("[{0}]\n{1}", xn["source"].InnerText, xn["target"].InnerText);
                    c++;
                }

                sw.Close();
                MessageBox.Show(string.Format("Exported Completed. {0} items wrote.",c), "Operation Successful", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private void exportUntranslatedButton_Click(object sender, EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "Langpack compatible files(*.txt)|*.txt|All Files(*.*)|*.*";
            if (sfd.ShowDialog(this) == DialogResult.OK)
            {
                StreamWriter sw = new StreamWriter(sfd.OpenFile(), Encoding.ASCII);
                XmlDocument xd = new XmlDocument();
                xd.Load("translated.xml");

                DataTable dt = new DataTable();
                DataRow dr;
                Hashtable htDistinct = new Hashtable();
                int c = 0;

                foreach (XmlNode xn in xd.SelectNodes("/phrases/directory/item[not(@translated)]"))
                {
                    if (htDistinct[xn.InnerText] == null && xn.InnerText.Length > 0)
                    {
                        sw.WriteLine("[{0}]\n", xn.InnerText);
                        htDistinct[xn.InnerText] = 1;
                        c++;
                    }
                }

                sw.Close();
                MessageBox.Show(string.Format("Exported Completed. {0} items wrote.",c), "Operation Successful", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }
    }
}