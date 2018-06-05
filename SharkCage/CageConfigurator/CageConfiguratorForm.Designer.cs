namespace CageConfigurator
{
    partial class CageConfiguratorForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.openCageChooserButton = new System.Windows.Forms.Button();
            this.applicationLabel = new System.Windows.Forms.Label();
            this.saveButton = new System.Windows.Forms.Button();
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.newToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.applicationBrowseButton = new System.Windows.Forms.Button();
            this.applicationPath = new System.Windows.Forms.TextBox();
            this.secureSecondaryProgramLabel = new System.Windows.Forms.Label();
            this.secureSecondaryPrograms = new System.Windows.Forms.ComboBox();
            this.tokenLabel = new System.Windows.Forms.Label();
            this.tokenBrowseButton = new System.Windows.Forms.Button();
            this.tokenWebcamButton = new System.Windows.Forms.Button();
            this.tokenBox = new System.Windows.Forms.PictureBox();
            this.menuStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tokenBox)).BeginInit();
            this.SuspendLayout();
            // 
            // openCageChooserButton
            // 
            this.openCageChooserButton.Location = new System.Drawing.Point(12, 425);
            this.openCageChooserButton.Name = "openCageChooserButton";
            this.openCageChooserButton.Size = new System.Drawing.Size(219, 24);
            this.openCageChooserButton.TabIndex = 0;
            this.openCageChooserButton.Text = "Open Cage Chooser";
            this.openCageChooserButton.UseVisualStyleBackColor = true;
            this.openCageChooserButton.Click += new System.EventHandler(this.openCageChooserButton_Click);
            // 
            // applicationLabel
            // 
            this.applicationLabel.AutoSize = true;
            this.applicationLabel.Location = new System.Drawing.Point(13, 40);
            this.applicationLabel.Name = "applicationLabel";
            this.applicationLabel.Size = new System.Drawing.Size(144, 13);
            this.applicationLabel.TabIndex = 1;
            this.applicationLabel.Text = "Select an application to start:";
            // 
            // saveButton
            // 
            this.saveButton.Location = new System.Drawing.Point(249, 425);
            this.saveButton.Name = "saveButton";
            this.saveButton.Size = new System.Drawing.Size(219, 24);
            this.saveButton.TabIndex = 2;
            this.saveButton.Text = "Save configuration ...";
            this.saveButton.UseVisualStyleBackColor = true;
            this.saveButton.Click += new System.EventHandler(this.saveButton_Click);
            // 
            // menuStrip
            // 
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.newToolStripMenuItem,
            this.openToolStripMenuItem});
            this.menuStrip.Location = new System.Drawing.Point(0, 0);
            this.menuStrip.Name = "menuStrip";
            this.menuStrip.Size = new System.Drawing.Size(484, 24);
            this.menuStrip.TabIndex = 3;
            this.menuStrip.Text = "menuStrip";
            // 
            // newToolStripMenuItem
            // 
            this.newToolStripMenuItem.Name = "newToolStripMenuItem";
            this.newToolStripMenuItem.Size = new System.Drawing.Size(43, 20);
            this.newToolStripMenuItem.Text = "New";
            this.newToolStripMenuItem.Click += new System.EventHandler(this.newToolStripMenuItem_Click);
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(48, 20);
            this.openToolStripMenuItem.Text = "Open";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.openToolStripMenuItem_Click);
            // 
            // applicationBrowseButton
            // 
            this.applicationBrowseButton.Location = new System.Drawing.Point(372, 54);
            this.applicationBrowseButton.Name = "applicationBrowseButton";
            this.applicationBrowseButton.Size = new System.Drawing.Size(96, 24);
            this.applicationBrowseButton.TabIndex = 4;
            this.applicationBrowseButton.Text = "Browse ...";
            this.applicationBrowseButton.UseVisualStyleBackColor = true;
            this.applicationBrowseButton.Click += new System.EventHandler(this.applicationBrowseButton_Click);
            // 
            // applicationPath
            // 
            this.applicationPath.Location = new System.Drawing.Point(12, 56);
            this.applicationPath.Name = "applicationPath";
            this.applicationPath.Size = new System.Drawing.Size(354, 20);
            this.applicationPath.TabIndex = 5;
            this.applicationPath.Leave += new System.EventHandler(this.applicationPath_Leave);
            // 
            // secureSecondaryProgramLabel
            // 
            this.secureSecondaryProgramLabel.AutoSize = true;
            this.secureSecondaryProgramLabel.Location = new System.Drawing.Point(12, 103);
            this.secureSecondaryProgramLabel.Name = "secureSecondaryProgramLabel";
            this.secureSecondaryProgramLabel.Size = new System.Drawing.Size(236, 13);
            this.secureSecondaryProgramLabel.TabIndex = 6;
            this.secureSecondaryProgramLabel.Text = "Select (secure) application to be opened as well:";
            // 
            // secureSecondaryPrograms
            // 
            this.secureSecondaryPrograms.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.secureSecondaryPrograms.FormattingEnabled = true;
            this.secureSecondaryPrograms.Items.AddRange(new object[] {
            "None",
            "Keepass"});
            this.secureSecondaryPrograms.Location = new System.Drawing.Point(254, 100);
            this.secureSecondaryPrograms.Name = "secureSecondaryPrograms";
            this.secureSecondaryPrograms.Size = new System.Drawing.Size(214, 21);
            this.secureSecondaryPrograms.TabIndex = 7;
            this.secureSecondaryPrograms.SelectedIndexChanged += new System.EventHandler(this.secureSecondaryPrograms_SelectedIndexChanged);
            // 
            // tokenLabel
            // 
            this.tokenLabel.AutoSize = true;
            this.tokenLabel.Location = new System.Drawing.Point(12, 142);
            this.tokenLabel.Name = "tokenLabel";
            this.tokenLabel.Size = new System.Drawing.Size(251, 13);
            this.tokenLabel.TabIndex = 8;
            this.tokenLabel.Text = "Select (picture) token associated with configuration:";
            // 
            // tokenBrowseButton
            // 
            this.tokenBrowseButton.Location = new System.Drawing.Point(372, 382);
            this.tokenBrowseButton.Name = "tokenBrowseButton";
            this.tokenBrowseButton.Size = new System.Drawing.Size(96, 24);
            this.tokenBrowseButton.TabIndex = 9;
            this.tokenBrowseButton.Text = "Browse ...";
            this.tokenBrowseButton.UseVisualStyleBackColor = true;
            this.tokenBrowseButton.Click += new System.EventHandler(this.tokenBrowseButton_Click);
            // 
            // tokenWebcamButton
            // 
            this.tokenWebcamButton.Location = new System.Drawing.Point(270, 382);
            this.tokenWebcamButton.Name = "tokenWebcamButton";
            this.tokenWebcamButton.Size = new System.Drawing.Size(96, 24);
            this.tokenWebcamButton.TabIndex = 10;
            this.tokenWebcamButton.Text = "Use webcam";
            this.tokenWebcamButton.UseVisualStyleBackColor = true;
            // 
            // tokenBox
            // 
            this.tokenBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.tokenBox.Location = new System.Drawing.Point(12, 158);
            this.tokenBox.Name = "tokenBox";
            this.tokenBox.Size = new System.Drawing.Size(456, 218);
            this.tokenBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.tokenBox.TabIndex = 11;
            this.tokenBox.TabStop = false;
            // 
            // CageConfiguratorForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(484, 461);
            this.Controls.Add(this.tokenBox);
            this.Controls.Add(this.tokenWebcamButton);
            this.Controls.Add(this.tokenBrowseButton);
            this.Controls.Add(this.tokenLabel);
            this.Controls.Add(this.secureSecondaryPrograms);
            this.Controls.Add(this.secureSecondaryProgramLabel);
            this.Controls.Add(this.applicationPath);
            this.Controls.Add(this.applicationBrowseButton);
            this.Controls.Add(this.saveButton);
            this.Controls.Add(this.applicationLabel);
            this.Controls.Add(this.openCageChooserButton);
            this.Controls.Add(this.menuStrip);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MainMenuStrip = this.menuStrip;
            this.Name = "CageConfiguratorForm";
            this.Text = "Cage Configurator";
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tokenBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button openCageChooserButton;
        private System.Windows.Forms.Label applicationLabel;
        private System.Windows.Forms.Button saveButton;
        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem newToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.Button applicationBrowseButton;
        private System.Windows.Forms.TextBox applicationPath;
        private System.Windows.Forms.Label secureSecondaryProgramLabel;
        private System.Windows.Forms.ComboBox secureSecondaryPrograms;
        private System.Windows.Forms.Label tokenLabel;
        private System.Windows.Forms.Button tokenBrowseButton;
        private System.Windows.Forms.Button tokenWebcamButton;
        private System.Windows.Forms.PictureBox tokenBox;
    }
}

