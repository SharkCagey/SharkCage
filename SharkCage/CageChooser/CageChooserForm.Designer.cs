namespace CageChooser
{
    partial class CageChooserForm
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
            this.secureSecondaryApplicationLabel = new System.Windows.Forms.Label();
            this.secureSecondaryPrograms = new System.Windows.Forms.ComboBox();
            this.lruConfigs = new System.Windows.Forms.ListBox();
            this.lruConfigsLabel = new System.Windows.Forms.Label();
            this.configChooseLabel = new System.Windows.Forms.Label();
            this.configBrowseButton = new System.Windows.Forms.Button();
            this.openButton = new System.Windows.Forms.Button();
            this.configPath = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // secureSecondaryApplicationLabel
            // 
            this.secureSecondaryApplicationLabel.AutoSize = true;
            this.secureSecondaryApplicationLabel.Location = new System.Drawing.Point(16, 79);
            this.secureSecondaryApplicationLabel.Name = "secureSecondaryApplicationLabel";
            this.secureSecondaryApplicationLabel.Size = new System.Drawing.Size(239, 13);
            this.secureSecondaryApplicationLabel.TabIndex = 1;
            this.secureSecondaryApplicationLabel.Text = "Would you like to open an additional application?";
            // 
            // secureSecondaryPrograms
            // 
            this.secureSecondaryPrograms.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.secureSecondaryPrograms.FormattingEnabled = true;
            this.secureSecondaryPrograms.Items.AddRange(new object[] {
            "None",
            "Keepass"});
            this.secureSecondaryPrograms.Location = new System.Drawing.Point(261, 76);
            this.secureSecondaryPrograms.Name = "secureSecondaryPrograms";
            this.secureSecondaryPrograms.Size = new System.Drawing.Size(201, 21);
            this.secureSecondaryPrograms.TabIndex = 2;
            this.secureSecondaryPrograms.SelectedIndexChanged += new System.EventHandler(this.secureSecondaryPrograms_SelectedIndexChanged);
            // 
            // lruConfigs
            // 
            this.lruConfigs.FormattingEnabled = true;
            this.lruConfigs.Location = new System.Drawing.Point(16, 135);
            this.lruConfigs.Name = "lruConfigs";
            this.lruConfigs.Size = new System.Drawing.Size(446, 134);
            this.lruConfigs.TabIndex = 3;
            this.lruConfigs.SelectedIndexChanged += new System.EventHandler(this.lruConfigs_SelectedIndexChanged);
            this.lruConfigs.KeyUp += new System.Windows.Forms.KeyEventHandler(this.lruConfigs_KeyUp);
            // 
            // lruConfigsLabel
            // 
            this.lruConfigsLabel.AutoSize = true;
            this.lruConfigsLabel.Location = new System.Drawing.Point(13, 112);
            this.lruConfigsLabel.Name = "lruConfigsLabel";
            this.lruConfigsLabel.Size = new System.Drawing.Size(147, 13);
            this.lruConfigsLabel.TabIndex = 4;
            this.lruConfigsLabel.Text = "Recently used configurations:";
            // 
            // configChooseLabel
            // 
            this.configChooseLabel.AutoSize = true;
            this.configChooseLabel.Location = new System.Drawing.Point(16, 16);
            this.configChooseLabel.Name = "configChooseLabel";
            this.configChooseLabel.Size = new System.Drawing.Size(137, 13);
            this.configChooseLabel.TabIndex = 6;
            this.configChooseLabel.Text = "Selected config to execute:";
            // 
            // configBrowseButton
            // 
            this.configBrowseButton.Location = new System.Drawing.Point(366, 37);
            this.configBrowseButton.Name = "configBrowseButton";
            this.configBrowseButton.Size = new System.Drawing.Size(96, 24);
            this.configBrowseButton.TabIndex = 7;
            this.configBrowseButton.Text = "Browse ...";
            this.configBrowseButton.UseVisualStyleBackColor = true;
            this.configBrowseButton.Click += new System.EventHandler(this.configBrowseButton_Click);
            // 
            // openButton
            // 
            this.openButton.Location = new System.Drawing.Point(16, 289);
            this.openButton.Name = "openButton";
            this.openButton.Size = new System.Drawing.Size(446, 23);
            this.openButton.TabIndex = 8;
            this.openButton.Text = "Open";
            this.openButton.UseVisualStyleBackColor = true;
            this.openButton.Click += new System.EventHandler(this.openButton_Click);
            // 
            // configPath
            // 
            this.configPath.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::CageChooser.Properties.Settings.Default, "PersistentConfigPath", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.configPath.Location = new System.Drawing.Point(16, 39);
            this.configPath.Name = "configPath";
            this.configPath.Size = new System.Drawing.Size(340, 20);
            this.configPath.TabIndex = 5;
            this.configPath.Text = global::CageChooser.Properties.Settings.Default.PersistentConfigPath;
            this.configPath.Leave += new System.EventHandler(this.configPath_Leave);
            // 
            // CageChooserForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(478, 329);
            this.Controls.Add(this.openButton);
            this.Controls.Add(this.configBrowseButton);
            this.Controls.Add(this.configChooseLabel);
            this.Controls.Add(this.configPath);
            this.Controls.Add(this.lruConfigsLabel);
            this.Controls.Add(this.lruConfigs);
            this.Controls.Add(this.secureSecondaryPrograms);
            this.Controls.Add(this.secureSecondaryApplicationLabel);
            this.Name = "CageChooserForm";
            this.Text = "Shark Cage";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.CageChooser_FormClosed);
            this.Load += new System.EventHandler(this.CageChooser_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Label secureSecondaryApplicationLabel;
        private System.Windows.Forms.ComboBox secureSecondaryPrograms;
        private System.Windows.Forms.ListBox lruConfigs;
        private System.Windows.Forms.Label lruConfigsLabel;
        private System.Windows.Forms.TextBox configPath;
        private System.Windows.Forms.Label configChooseLabel;
        private System.Windows.Forms.Button configBrowseButton;
        private System.Windows.Forms.Button openButton;
    }
}

