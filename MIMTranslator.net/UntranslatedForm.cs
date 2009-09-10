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

Stub for list editing dialog
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
    public partial class UntranslatedForm : Form
    {
        private string m_filename=null;
        private XmlNodeList m_xnl;

        public string filename {
            set
            {
                searchPanel.Visible = true;
                this.m_filename = value;
                this.Text = value;
                this.AcceptButton = searchButton;
            }
        }

        public UntranslatedForm()
        {
            InitializeComponent();

        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
        }

        private void saveButton_Click(object sender, EventArgs e)
        {
            if (m_filename == null)
                DialogResult = DialogResult.OK;
            else
            {
                DataTable dt = (DataTable)dataGridView1.DataSource;
                int c = 0;

                foreach (DataRow dr in dt.Rows)
                {
                    if (dr.RowState == DataRowState.Modified)
                    {
                        if (m_filename[0] == 'd')
                        {
                            m_xnl[c]["source"].InnerText = (string)dr[0];
                            m_xnl[c]["target"].InnerText = (string)dr[1];
                        }
                        else
                            m_xnl[c].InnerText = (string)dr[0];

                    }
                    else if (dr.RowState == DataRowState.Deleted)
                    {
                        m_xnl[c].OwnerDocument.RemoveChild(m_xnl[c]);
                    }
                    else if (dr.RowState == DataRowState.Added)
                    {
                        XmlDocument xd=m_xnl[0].OwnerDocument;
                        XmlNode xnItem = xd.CreateElement("item");
                        if (m_filename[0] == 'd')
                        {
                            XmlNode xnSub = xd.CreateElement("source");
                            xnSub.InnerText = (string)dr[0];
                            xnItem.AppendChild(xnSub);
                            xnSub = xd.CreateElement("target");
                            xnSub.InnerText = (string)dr[1];
                            xnItem.AppendChild(xnSub);
                        }
                        else
                            xnItem.InnerText = (string)dr[0];

                        xd.FirstChild.AppendChild(xnItem);
                    }
                    c++;
                }

                m_xnl[0].OwnerDocument.Save(m_filename);
            }
        }

        private void searchButton_Click(object sender, EventArgs e)
        {
            if (criteriaTextBox.Text.Trim().Length == 0)
                MessageBox.Show("You must enter search criteria to limit number of results!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
            {
                XmlDocument xd = new XmlDocument();
                xd.Load(m_filename);
                XmlNodeList xnl = xd.SelectNodes(m_filename[0] == 'd' ? "/translations/item[contains(source,'" + criteriaTextBox.Text + "')]" : "/phrases/item[contains(text(),'" + criteriaTextBox.Text + "')]");

                if (xnl.Count == 0)
                    MessageBox.Show("No Match Found.");
                else
                {
                    DataTable dt = new DataTable();
                    DataRow dr;

                    if (m_filename[0] == 'd')
                    {
                        dt.Columns.Add("source");
                        dt.Columns.Add("target");
                    }
                    else
                        dt.Columns.Add("item");

                    foreach (XmlNode xn in xnl)
                    {
                        dr = dt.NewRow();
                        if (m_filename[0] == 'd')
                        {
                            dr[0] = xn["source"].InnerText;
                            dr[1] = xn["target"].InnerText;
                        }
                        else
                            dr[0] = xn.InnerText;

                        dt.Rows.Add(dr);
                    }

                    dt.AcceptChanges();

                    dataGridView1.DataSource = dt;
                    dataGridView1.Columns[m_filename[0] == 'd'?1:0].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;

                    m_xnl = xnl;
                }
                
            }
        }
    }
}