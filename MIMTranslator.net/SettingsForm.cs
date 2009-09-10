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

Settings Dialog
*/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace MIMTranslator
{
    public partial class SettingsForm : Form
    {
        private XmlDocument configXml;
        private string lastfile=null;

        public SettingsForm(XmlDocument xd)
        {
            InitializeComponent();

            configXml = xd;

            XmlNode xnRoot = configXml.SelectSingleNode("/config/output");

            foreach (XmlNode xn in xnRoot.ChildNodes)
            {
                switch (xn.Name)
                {
                    case "language": languageTextBox.Text = xn.InnerText; break;
                    case "locale": localeTextBox.Text = xn.InnerText; break;
                    case "last-modified-using": lastModifiedUsingTextBox.Text = xn.InnerText; break;
                    case "authors": authorsTextBox.Text = xn.InnerText; break;
                    case "author-email": authorEmailTextBox.Text = xn.InnerText; break;
                    case "plugins-included": pluginsIncludedTextBox.Text = xn.InnerText; break;
                    case "flid": flidTextBox.Text = xn.InnerText; break;
                    case "prepend": prependFileTextBox.Text = xn.InnerText; break;
                    case "append": appendFileTextBox.Text = xn.InnerText; break;
                    case "filename": lastfile = xn.InnerText; break;
                }
            }
        }

        private void LoadEditor(string location)
        {
            UntranslatedForm f = new UntranslatedForm();
            DataTable dt = new DataTable();
            DataRow dr;
            dt.Columns.Add("source");

            foreach (XmlNode xn in configXml.SelectNodes("/config/"+location+"/item"))
            {
                dr = dt.NewRow();
                dr[0] = xn.InnerText;
                dt.Rows.Add(dr);
            }
            f.dataGridView1.DataSource = dt;
            f.dataGridView1.Columns[0].HeaderText = "Data";
            f.dataGridView1.Columns[0].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

            f.Text = "Property Editor - /config/" + location;

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                XmlNode xnRoot = configXml.SelectSingleNode("/config/"+location);
                XmlNode xnNew;
                xnRoot.RemoveAll();

                foreach (DataRow dr2 in ((DataTable)f.dataGridView1.DataSource).Rows)
                {
                    xnNew = configXml.CreateElement("item");
                    xnNew.InnerText = (string)dr2[0];
                    xnRoot.AppendChild(xnNew);
                }

                configXml.Save("config.xml");
            }
        }

        public void sourceListButton_Click(object sender, EventArgs e)
        {
            UntranslatedForm f = new UntranslatedForm();
            DataTable dt = new DataTable();
            DataRow dr;
            XmlAttribute xa;
            dt.Columns.Add("source");
            dt.Columns.Add("target");

            foreach (XmlNode xn in configXml.SelectNodes("/config/source/item"))
            {
                xa=xn.Attributes["title"];
                dr = dt.NewRow();
                dr[0] = xa==null?"":xa.Value;
                dr[1] = xn.InnerText;
                dt.Rows.Add(dr);
            }
            f.dataGridView1.DataSource = dt;
            f.dataGridView1.Columns[0].HeaderText = "Description";
            f.dataGridView1.Columns[1].HeaderText = "Path";
            f.dataGridView1.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

            f.Text = "Property Editor - /config/source";

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                XmlNode xnRoot = configXml.SelectSingleNode("/config/source");
                XmlNode xnNew;
                xnRoot.RemoveAll();

                foreach (DataRow dr2 in ((DataTable)f.dataGridView1.DataSource).Rows)
                {
                    xnNew = configXml.CreateElement("item");
                    xnNew.InnerText = (string)dr2[1];
                    xnRoot.AppendChild(xnNew);

                    if (dr2[0] != DBNull.Value && ((string)dr2[0]) != "")
                    {
                        xa = configXml.CreateAttribute("title");
                        xa.Value = (string)dr2[0];
                        xnNew.Attributes.Append(xa);
                    }
                }

                configXml.Save("config.xml");
            }            
        }

        private void engineListButton_Click(object sender, EventArgs e)
        {
            UntranslatedForm f = new UntranslatedForm();
            DataTable dt = new DataTable();
            DataRow dr;
            XmlAttribute xa;
            dt.Columns.Add("dll");
            dt.Columns.Add("assembly");

            foreach (XmlNode xn in configXml.SelectNodes("/config/engine/item"))
            {
                dr = dt.NewRow();
                dr[1] = xn.InnerText;

                xa = xn.Attributes["dll"];
                if (xa!=null)
                {
                    dr[0] = xa.Value;
                }
                dt.Rows.Add(dr);
            }
            f.dataGridView1.DataSource = dt;
            f.dataGridView1.Columns[0].HeaderText = "DLL";
            f.dataGridView1.Columns[1].HeaderText = "Assembly";
            f.dataGridView1.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

            f.Text = "Property Editor - /config/engine";

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                XmlNode xnRoot = configXml.SelectSingleNode("/config/engine");
                XmlNode xnNew;
                xnRoot.RemoveAll();

                foreach (DataRow dr2 in ((DataTable)f.dataGridView1.DataSource).Rows)
                {
                    xnNew = configXml.CreateElement("item");
                    xnNew.InnerText = (string)dr2[1];
                    if (dr2[0] != DBNull.Value && ((string)dr2[0]).Trim().Length > 0)
                    {
                        xa = configXml.CreateAttribute("dll");
                        xa.Value = (string)dr2[0];
                        xnNew.Attributes.Append(xa);
                    }
                    xnRoot.AppendChild(xnNew);
                }

                configXml.Save("config.xml");
            }
        }

        private void whitelistButton_Click(object sender, EventArgs e)
        {
            LoadEditor("wordlist/common/whitelist");
        }

        private void blacklistButton_Click(object sender, EventArgs e)
        {
            LoadEditor("wordlist/common/blacklist");
        }

        private void superlistButton_Click(object sender, EventArgs e)
        {
            LoadEditor("wordlist/common/superlist");
        }

        private void saveButton_Click(object sender, EventArgs e)
        {
            XmlNode xnRoot = configXml.SelectSingleNode("/config/output");

            xnRoot.RemoveAll();

            foreach (Control ctl in groupBox1.Controls)
            {
                XmlNode xnNew = null;

                switch (ctl.Name)
                {
                    case "languageTextBox": xnNew = configXml.CreateElement("language"); break;
                    case "localeTextBox": xnNew = configXml.CreateElement("locale"); break;
                    case "lastModifiedUsingTextBox": xnNew = configXml.CreateElement("last-modified-using"); break;
                    case "authorsTextBox": xnNew = configXml.CreateElement("authors"); break;
                    case "authorEmailTextBox": xnNew = configXml.CreateElement("author-email"); break;
                    case "pluginsIncludedTextBox": xnNew = configXml.CreateElement("plugins-included"); break;
                    case "flidTextBox": xnNew=configXml.CreateElement("flid"); break;
                    case "prependFileTextBox": xnNew=configXml.CreateElement("prepend"); break;
                    case "appendFileTextBox": xnNew=configXml.CreateElement("append"); break;
                }

                if (xnNew != null)
                {
                    xnNew.InnerText = ctl.Text;
                    xnRoot.AppendChild(xnNew);
                    xnNew = null;
                }
            }

            if (lastfile != null)
            {
                XmlNode xnNew=configXml.CreateElement("filename");
                xnNew.InnerText = lastfile;
                xnRoot.AppendChild(xnNew);
            }

            configXml.Save("config.xml");
            Close();
       }
    }
}