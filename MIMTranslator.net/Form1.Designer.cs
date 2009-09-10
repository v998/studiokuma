namespace MIMTranslator
{
    partial class Form1
    {
        /// <summary>
        /// 必要なデザイナ変数です。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 使用中のリソースをすべてクリーンアップします。
        /// </summary>
        /// <param name="disposing">マネージ リソースが破棄される場合 true、破棄されない場合は false です。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows フォーム デザイナで生成されたコード

        /// <summary>
        /// デザイナ サポートに必要なメソッドです。このメソッドの内容を
        /// コード エディタで変更しないでください。
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.importTranslationsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.importIgnoresPhrasesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.stepButton1 = new System.Windows.Forms.Button();
            this.stepButton2 = new System.Windows.Forms.Button();
            this.stepButton3 = new System.Windows.Forms.Button();
            this.stepButton4 = new System.Windows.Forms.Button();
            this.stepButton5 = new System.Windows.Forms.Button();
            this.sourceSettingButton = new System.Windows.Forms.Button();
            this.importPhrasesButton = new System.Windows.Forms.Button();
            this.exportUntranslatedButton = new System.Windows.Forms.Button();
            this.editIgnoreButton = new System.Windows.Forms.Button();
            this.exportDictionaryButton = new System.Windows.Forms.Button();
            this.editPhrasesButton = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.importIgnoredButton = new System.Windows.Forms.Button();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.aboutToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(357, 26);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.settingsToolStripMenuItem,
            this.toolStripSeparator1,
            this.importTranslationsToolStripMenuItem,
            this.importIgnoresPhrasesToolStripMenuItem,
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(40, 22);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // settingsToolStripMenuItem
            // 
            this.settingsToolStripMenuItem.Name = "settingsToolStripMenuItem";
            this.settingsToolStripMenuItem.Size = new System.Drawing.Size(226, 22);
            this.settingsToolStripMenuItem.Text = "&Settings";
            this.settingsToolStripMenuItem.Click += new System.EventHandler(this.settingsToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(223, 6);
            // 
            // importTranslationsToolStripMenuItem
            // 
            this.importTranslationsToolStripMenuItem.Name = "importTranslationsToolStripMenuItem";
            this.importTranslationsToolStripMenuItem.Size = new System.Drawing.Size(226, 22);
            this.importTranslationsToolStripMenuItem.Text = "Import &Translations...";
            this.importTranslationsToolStripMenuItem.Click += new System.EventHandler(this.importTranslationsToolStripMenuItem_Click);
            // 
            // importIgnoresPhrasesToolStripMenuItem
            // 
            this.importIgnoresPhrasesToolStripMenuItem.Name = "importIgnoresPhrasesToolStripMenuItem";
            this.importIgnoresPhrasesToolStripMenuItem.Size = new System.Drawing.Size(226, 22);
            this.importIgnoresPhrasesToolStripMenuItem.Text = "Import &Ignores Phrases...";
            this.importIgnoresPhrasesToolStripMenuItem.Click += new System.EventHandler(this.importIgnoresPhrasesToolStripMenuItem_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(223, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(226, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(54, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            // 
            // stepButton1
            // 
            this.stepButton1.Location = new System.Drawing.Point(12, 29);
            this.stepButton1.Name = "stepButton1";
            this.stepButton1.Size = new System.Drawing.Size(260, 23);
            this.stepButton1.TabIndex = 1;
            this.stepButton1.Text = "Grab from Source";
            this.stepButton1.UseVisualStyleBackColor = true;
            this.stepButton1.Click += new System.EventHandler(this.stepButton1_Click);
            // 
            // stepButton2
            // 
            this.stepButton2.Location = new System.Drawing.Point(12, 58);
            this.stepButton2.Name = "stepButton2";
            this.stepButton2.Size = new System.Drawing.Size(260, 23);
            this.stepButton2.TabIndex = 2;
            this.stepButton2.Text = "&Translate using Dictionary (Auto)";
            this.stepButton2.UseVisualStyleBackColor = true;
            this.stepButton2.Click += new System.EventHandler(this.stepButton2_Click);
            // 
            // stepButton3
            // 
            this.stepButton3.Location = new System.Drawing.Point(12, 87);
            this.stepButton3.Name = "stepButton3";
            this.stepButton3.Size = new System.Drawing.Size(260, 23);
            this.stepButton3.TabIndex = 3;
            this.stepButton3.Text = "Check &Untranslated";
            this.stepButton3.UseVisualStyleBackColor = true;
            this.stepButton3.Click += new System.EventHandler(this.stepButton3_Click);
            // 
            // stepButton4
            // 
            this.stepButton4.Location = new System.Drawing.Point(13, 117);
            this.stepButton4.Name = "stepButton4";
            this.stepButton4.Size = new System.Drawing.Size(259, 23);
            this.stepButton4.TabIndex = 4;
            this.stepButton4.Text = "&Ignore Untranslated";
            this.stepButton4.UseVisualStyleBackColor = true;
            this.stepButton4.Click += new System.EventHandler(this.stepButton4_Click);
            // 
            // stepButton5
            // 
            this.stepButton5.Location = new System.Drawing.Point(13, 147);
            this.stepButton5.Name = "stepButton5";
            this.stepButton5.Size = new System.Drawing.Size(259, 23);
            this.stepButton5.TabIndex = 5;
            this.stepButton5.Text = "&Generate Langpack";
            this.stepButton5.UseVisualStyleBackColor = true;
            this.stepButton5.Click += new System.EventHandler(this.stepButton5_Click);
            // 
            // sourceSettingButton
            // 
            this.sourceSettingButton.Location = new System.Drawing.Point(279, 28);
            this.sourceSettingButton.Name = "sourceSettingButton";
            this.sourceSettingButton.Size = new System.Drawing.Size(30, 23);
            this.sourceSettingButton.TabIndex = 6;
            this.sourceSettingButton.Text = "...";
            this.toolTip1.SetToolTip(this.sourceSettingButton, "Setup Sources");
            this.sourceSettingButton.UseVisualStyleBackColor = true;
            this.sourceSettingButton.Click += new System.EventHandler(this.sourceSettingButton_Click);
            // 
            // importPhrasesButton
            // 
            this.importPhrasesButton.Location = new System.Drawing.Point(279, 86);
            this.importPhrasesButton.Name = "importPhrasesButton";
            this.importPhrasesButton.Size = new System.Drawing.Size(30, 23);
            this.importPhrasesButton.TabIndex = 7;
            this.importPhrasesButton.Text = "<<";
            this.toolTip1.SetToolTip(this.importPhrasesButton, "Import Translations");
            this.importPhrasesButton.UseVisualStyleBackColor = true;
            this.importPhrasesButton.Click += new System.EventHandler(this.importPhrasesButton_Click);
            // 
            // exportUntranslatedButton
            // 
            this.exportUntranslatedButton.Location = new System.Drawing.Point(316, 86);
            this.exportUntranslatedButton.Name = "exportUntranslatedButton";
            this.exportUntranslatedButton.Size = new System.Drawing.Size(30, 23);
            this.exportUntranslatedButton.TabIndex = 8;
            this.exportUntranslatedButton.Text = ">>";
            this.toolTip1.SetToolTip(this.exportUntranslatedButton, "Export Untranslated");
            this.exportUntranslatedButton.UseVisualStyleBackColor = true;
            this.exportUntranslatedButton.Click += new System.EventHandler(this.exportUntranslatedButton_Click);
            // 
            // editIgnoreButton
            // 
            this.editIgnoreButton.Location = new System.Drawing.Point(316, 115);
            this.editIgnoreButton.Name = "editIgnoreButton";
            this.editIgnoreButton.Size = new System.Drawing.Size(30, 23);
            this.editIgnoreButton.TabIndex = 9;
            this.editIgnoreButton.Text = "...";
            this.toolTip1.SetToolTip(this.editIgnoreButton, "Edit Ignore List");
            this.editIgnoreButton.UseVisualStyleBackColor = true;
            this.editIgnoreButton.Click += new System.EventHandler(this.editIgnoreButton_Click);
            // 
            // exportDictionaryButton
            // 
            this.exportDictionaryButton.Location = new System.Drawing.Point(316, 58);
            this.exportDictionaryButton.Name = "exportDictionaryButton";
            this.exportDictionaryButton.Size = new System.Drawing.Size(30, 23);
            this.exportDictionaryButton.TabIndex = 10;
            this.exportDictionaryButton.Text = ">>";
            this.toolTip1.SetToolTip(this.exportDictionaryButton, "Export Dictionary");
            this.exportDictionaryButton.UseVisualStyleBackColor = true;
            this.exportDictionaryButton.Click += new System.EventHandler(this.exportDictionaryButton_Click);
            // 
            // editPhrasesButton
            // 
            this.editPhrasesButton.Location = new System.Drawing.Point(279, 58);
            this.editPhrasesButton.Name = "editPhrasesButton";
            this.editPhrasesButton.Size = new System.Drawing.Size(30, 23);
            this.editPhrasesButton.TabIndex = 11;
            this.editPhrasesButton.Text = "...";
            this.toolTip1.SetToolTip(this.editPhrasesButton, "Edit Dictionary");
            this.editPhrasesButton.UseVisualStyleBackColor = true;
            this.editPhrasesButton.Click += new System.EventHandler(this.editPhrasesButton_Click);
            // 
            // importIgnoredButton
            // 
            this.importIgnoredButton.Location = new System.Drawing.Point(279, 117);
            this.importIgnoredButton.Name = "importIgnoredButton";
            this.importIgnoredButton.Size = new System.Drawing.Size(30, 23);
            this.importIgnoredButton.TabIndex = 12;
            this.importIgnoredButton.Text = "<<";
            this.toolTip1.SetToolTip(this.importIgnoredButton, "Import Ignored Phrases");
            this.importIgnoredButton.UseVisualStyleBackColor = true;
            this.importIgnoredButton.Click += new System.EventHandler(this.importIgnoredButton_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(357, 181);
            this.Controls.Add(this.importIgnoredButton);
            this.Controls.Add(this.editPhrasesButton);
            this.Controls.Add(this.exportDictionaryButton);
            this.Controls.Add(this.editIgnoreButton);
            this.Controls.Add(this.exportUntranslatedButton);
            this.Controls.Add(this.importPhrasesButton);
            this.Controls.Add(this.sourceSettingButton);
            this.Controls.Add(this.stepButton5);
            this.Controls.Add(this.stepButton4);
            this.Controls.Add(this.stepButton3);
            this.Controls.Add(this.stepButton2);
            this.Controls.Add(this.stepButton1);
            this.Controls.Add(this.menuStrip1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MainMenuStrip = this.menuStrip1;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "MirandaTranslator.NET";
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.Button stepButton1;
        private System.Windows.Forms.ToolStripMenuItem importTranslationsToolStripMenuItem;
        private System.Windows.Forms.Button stepButton2;
        private System.Windows.Forms.Button stepButton3;
        private System.Windows.Forms.ToolStripMenuItem importIgnoresPhrasesToolStripMenuItem;
        private System.Windows.Forms.Button stepButton4;
        private System.Windows.Forms.Button stepButton5;
        private System.Windows.Forms.ToolStripMenuItem settingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.Button sourceSettingButton;
        private System.Windows.Forms.Button importPhrasesButton;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Button exportUntranslatedButton;
        private System.Windows.Forms.Button editIgnoreButton;
        private System.Windows.Forms.Button exportDictionaryButton;
        private System.Windows.Forms.Button editPhrasesButton;
        private System.Windows.Forms.Button importIgnoredButton;
    }
}

