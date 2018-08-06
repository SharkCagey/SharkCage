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
            this.components = new System.ComponentModel.Container();
            this.registeredConfigs = new System.Windows.Forms.ListBox();
            this.registeredConfigsLabel = new System.Windows.Forms.Label();
            this.openButton = new System.Windows.Forms.Button();
            this.refreshTooltip = new System.Windows.Forms.ToolTip(this.components);
            this.SuspendLayout();
            // 
            // registeredConfigs
            // 
            this.registeredConfigs.FormattingEnabled = true;
            this.registeredConfigs.Location = new System.Drawing.Point(16, 34);
            this.registeredConfigs.Name = "registeredConfigs";
            this.registeredConfigs.Size = new System.Drawing.Size(446, 186);
            this.registeredConfigs.TabIndex = 4;
            this.refreshTooltip.SetToolTip(this.registeredConfigs, "Press F5 to refresh this list");
            this.registeredConfigs.SelectedIndexChanged += new System.EventHandler(this.registeredConfigs_SelectedIndexChanged);
            // 
            // registeredConfigsLabel
            // 
            this.registeredConfigsLabel.AutoSize = true;
            this.registeredConfigsLabel.Location = new System.Drawing.Point(13, 16);
            this.registeredConfigsLabel.Name = "registeredConfigsLabel";
            this.registeredConfigsLabel.Size = new System.Drawing.Size(130, 13);
            this.registeredConfigsLabel.TabIndex = 3;
            this.registeredConfigsLabel.Text = "Registered configurations:";
            // 
            // openButton
            // 
            this.openButton.Location = new System.Drawing.Point(16, 238);
            this.openButton.Name = "openButton";
            this.openButton.Size = new System.Drawing.Size(446, 23);
            this.openButton.TabIndex = 6;
            this.openButton.Text = "Start";
            this.openButton.UseVisualStyleBackColor = true;
            this.openButton.Click += new System.EventHandler(this.openButton_Click);
            // 
            // refreshTooltip
            // 
            this.refreshTooltip.AutoPopDelay = 30000;
            this.refreshTooltip.InitialDelay = 500;
            this.refreshTooltip.ReshowDelay = 100;
            // 
            // CageChooserForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(478, 276);
            this.Controls.Add(this.openButton);
            this.Controls.Add(this.registeredConfigsLabel);
            this.Controls.Add(this.registeredConfigs);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "CageChooserForm";
            this.ShowIcon = false;
            this.Text = "Cage Chooser";
            this.Load += new System.EventHandler(this.CageChooser_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.ListBox registeredConfigs;
        private System.Windows.Forms.Label registeredConfigsLabel;
        private System.Windows.Forms.Button openButton;
        private System.Windows.Forms.ToolTip refreshTooltip;
    }
}

