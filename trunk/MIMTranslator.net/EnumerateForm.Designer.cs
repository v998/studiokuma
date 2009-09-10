namespace MIMTranslator
{
    partial class EnumerateForm
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
            this.label1 = new System.Windows.Forms.Label();
            this.processLabel = new System.Windows.Forms.Label();
            this.foldersTitleLabel = new System.Windows.Forms.Label();
            this.foldersLabel = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.filesLabel = new System.Windows.Forms.Label();
            this.enumerateProgressBar = new System.Windows.Forms.ProgressBar();
            this.currentLabel = new System.Windows.Forms.Label();
            this.cancelButton = new System.Windows.Forms.Button();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(48, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "Process:";
            // 
            // processLabel
            // 
            this.processLabel.Location = new System.Drawing.Point(82, 13);
            this.processLabel.Name = "processLabel";
            this.processLabel.Size = new System.Drawing.Size(385, 12);
            this.processLabel.TabIndex = 1;
            // 
            // foldersTitleLabel
            // 
            this.foldersTitleLabel.AutoSize = true;
            this.foldersTitleLabel.Location = new System.Drawing.Point(13, 42);
            this.foldersTitleLabel.Name = "foldersTitleLabel";
            this.foldersTitleLabel.Size = new System.Drawing.Size(45, 12);
            this.foldersTitleLabel.TabIndex = 2;
            this.foldersTitleLabel.Text = "Folders:";
            // 
            // foldersLabel
            // 
            this.foldersLabel.Location = new System.Drawing.Point(82, 42);
            this.foldersLabel.Name = "foldersLabel";
            this.foldersLabel.Size = new System.Drawing.Size(383, 12);
            this.foldersLabel.TabIndex = 3;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(13, 58);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(32, 12);
            this.label3.TabIndex = 4;
            this.label3.Text = "Files:";
            // 
            // filesLabel
            // 
            this.filesLabel.Location = new System.Drawing.Point(82, 58);
            this.filesLabel.Name = "filesLabel";
            this.filesLabel.Size = new System.Drawing.Size(383, 12);
            this.filesLabel.TabIndex = 5;
            // 
            // enumerateProgressBar
            // 
            this.enumerateProgressBar.Location = new System.Drawing.Point(15, 86);
            this.enumerateProgressBar.Name = "enumerateProgressBar";
            this.enumerateProgressBar.Size = new System.Drawing.Size(450, 23);
            this.enumerateProgressBar.TabIndex = 6;
            // 
            // currentLabel
            // 
            this.currentLabel.Location = new System.Drawing.Point(13, 112);
            this.currentLabel.Name = "currentLabel";
            this.currentLabel.Size = new System.Drawing.Size(452, 16);
            this.currentLabel.TabIndex = 7;
            // 
            // cancelButton
            // 
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = new System.Drawing.Point(203, 134);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 8;
            this.cancelButton.Text = "Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Interval = 500;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // EnumerateForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelButton;
            this.ClientSize = new System.Drawing.Size(479, 169);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.currentLabel);
            this.Controls.Add(this.enumerateProgressBar);
            this.Controls.Add(this.filesLabel);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.foldersLabel);
            this.Controls.Add(this.foldersTitleLabel);
            this.Controls.Add(this.processLabel);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "EnumerateForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Grab From Source";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label foldersTitleLabel;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button cancelButton;
        private System.Windows.Forms.Label processLabel;
        private System.Windows.Forms.Label foldersLabel;
        private System.Windows.Forms.Label filesLabel;
        private System.Windows.Forms.ProgressBar enumerateProgressBar;
        private System.Windows.Forms.Label currentLabel;
        private System.Windows.Forms.Timer timer1;
    }
}