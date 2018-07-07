using CageChooser.Properties;
using Microsoft.Win32;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace CageChooser
{
    public partial class CageChooserForm : Form
    {
        private class NativeMethods
        {
            [DllImport("SharedFunctionality.dll", CallingConvention = CallingConvention.Cdecl)]
            public static extern void SendConfigAndExternalProgram(
                [MarshalAs(UnmanagedType.LPWStr)] string config_path
            );
        }

        public CageChooserForm()
        {
            InitializeComponent();
        }

        #region General

        private void CageChooser_Load(object sender, EventArgs e)
        {
            if (!File.Exists(Settings.Default.PersistentConfigPath))
            {
                configPath.Text = String.Empty;
            }
            if (Settings.Default.PersistentLRUConfigs != null)
            {
                foreach (string lruConfig in Settings.Default.PersistentLRUConfigs)
                {
                    if (File.Exists(lruConfig))
                    {
                        lruConfigs.Items.Add(lruConfig);
                    }
                }
            }
        }

        private void CageChooser_FormClosed(object sender, FormClosedEventArgs e)
        {
            Settings.Default.PersistentLRUConfigs = new System.Collections.Specialized.StringCollection();
            foreach (string lruConfig in lruConfigs.Items)
            {
                Settings.Default.PersistentLRUConfigs.Add(lruConfig);
            }
            Settings.Default.Save();
        }

        #endregion

        #region Config

        private void configBrowseButton_Click(object sender, EventArgs e)
        {
            var file_dialog = new OpenFileDialog();
            file_dialog.CheckFileExists = true;
            file_dialog.Filter = "SharkCage configuration|*.sconfig";
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                var chosen_file = file_dialog.FileName;
                CheckConfigPath(chosen_file, (string path) =>
                {
                    configPath.Text = path;
                    addToLRUconfigs(path);

                    configPath.SelectionStart = configPath.Text.Length;
                    configPath.ScrollToCaret();
                });
            }
        }

        private void configPath_Leave(object sender, EventArgs e)
        {
            CheckConfigPath(configPath.Text, addToLRUconfigs);
        }

        private void CheckConfigPath(string config_path, Action<string> onSuccess)
        {
            if (config_path.Length == 0)
            {
                return;
            }

            if (config_path.EndsWith(".sconfig") && File.Exists(config_path))
            {
                onSuccess(config_path);
            }
            else
            {
                MessageBox.Show("Can only choose existing .sconfig files!", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                configPath.Focus();
            }
        }

        #endregion

        #region Last Recently Used List

        private void lruConfigs_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lruConfigs.SelectedItem != null)
            {
                var selected_item = lruConfigs.SelectedItem.ToString();
                configPath.Text = selected_item;

                configPath.SelectionStart = configPath.Text.Length;
                configPath.ScrollToCaret();
            }
        }

        private void addToLRUconfigs(string config_path)
        {
            if (lruConfigs.Items.Contains(config_path))
            {
                lruConfigs.Items.Remove(config_path);
            }
            while (lruConfigs.Items.Count > 9)
            {
                lruConfigs.Items.RemoveAt(lruConfigs.Items.Count - 1);
            }
            lruConfigs.Items.Insert(0, config_path);
        }

        private void lruConfigs_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete && lruConfigs.SelectedItem != null)
            {
                lruConfigs.Items.Remove(lruConfigs.SelectedItem);
            }
        }

        #endregion

        #region Cage Service

        private void openButton_Click(object sender, EventArgs e)
        {
            try
            {
                if (configPath.Text != String.Empty)
                {
                    NativeMethods.SendConfigAndExternalProgram(configPath.Text);

                    // bring the form back in focus
                    Activate();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An exception occured while trying to send messages to cage service: {ex.ToString()}");
            }
        }

        #endregion

        #region CageConfigurator

        private void openCageConfiguratorButton_Click(object sender, EventArgs e)
        {
            const string registry_key = @"HKEY_LOCAL_MACHINE\SOFTWARE\SharkCage";
            var install_dir = Registry.GetValue(registry_key, "InstallDir", "") as string;

            if (install_dir.Length == 0)
            {
                MessageBox.Show("Could not read installation directory from registry, opening CageConfigurator not possible", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var p = new System.Diagnostics.Process();
            p.StartInfo.FileName = $@"{install_dir}\CageConfigurator.exe";
            p.StartInfo.Arguments = $@"""{configPath.Text}""";
            p.Start();
        }

        #endregion
    }
}
