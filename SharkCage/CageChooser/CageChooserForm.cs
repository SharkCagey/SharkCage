using CageChooser.Properties;
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
            [DllImport("CageNetwork.dll")]
            public static extern void StartCageManager();

            [DllImport("CageNetwork.dll", CallingConvention = CallingConvention.Cdecl)]
            public static extern void SendConfigAndExternalProgram(
                [MarshalAs(UnmanagedType.LPWStr)] string config_path,
                [MarshalAs(UnmanagedType.LPWStr)] string external_program_name
            );
        }

        public CageChooserForm()
        {
            InitializeComponent();
            secureSecondaryPrograms.SelectedIndex = 0;
        }

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

        private void configBrowseButton_Click(object sender, EventArgs e)
        {
            var file_dialog = new OpenFileDialog();
            file_dialog.CheckFileExists = true;
            file_dialog.Filter = "SharkCage configuration|*.sconfig";
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                var chosen_file = file_dialog.FileName;
                checkConfigPath(chosen_file, (string path) =>
                {
                    configPath.Text = path;
                    addToLRUconfigs(path);

                    configPath.SelectionStart = configPath.Text.Length;
                    configPath.ScrollToCaret();
                });
            }
        }

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

        private void configPath_Leave(object sender, EventArgs e)
        {
            checkConfigPath(configPath.Text, addToLRUconfigs);
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

        private void checkConfigPath(string config_path, Action<string> onSuccess)
        {
            if (config_path.Length == 0)
            {
                return;
            }

            if (/*FIXME comment this back in once service can handle config filesconfig_path.EndsWith(".sconfig") &&*/ File.Exists(config_path))
            {
                onSuccess(config_path);
            }
            else
            {
                MessageBox.Show("Can only choose existing .sconfig files!", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                configPath.Focus();
            }
        }

        private void secureSecondaryPrograms_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (secureSecondaryPrograms.SelectedIndex != 0)
            {
                switch (secureSecondaryPrograms.SelectedItem.ToString())
                {
                    case "Keepass":
                        if (Settings.Default.PeristentKeepassPath.Length == 0)
                        {
                            MessageBox.Show("There is no path saved for this program, please select it now.", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                            var keepass_file_dialog = new OpenFileDialog();
                            keepass_file_dialog.CheckFileExists = true;
                            keepass_file_dialog.Filter = "Keepass|Keepass.exe";
                            var result = keepass_file_dialog.ShowDialog();
                            if (result == DialogResult.OK)
                            {
                                Settings.Default.PeristentKeepassPath = keepass_file_dialog.FileName;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        private void openButton_Click(object sender, EventArgs e)
        {
            try
            {
                if (configPath.Text != String.Empty)
                {
                    NativeMethods.StartCageManager();
                    var secondary_program = secureSecondaryPrograms.Text == "None" ? null : secureSecondaryPrograms.Text;
                    NativeMethods.SendConfigAndExternalProgram(configPath.Text, secondary_program);

                    // bring the form back in focus
                    Activate();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An exception occured while trying to send messages to cage service: {ex.ToString()}");
            }
        }

        private void lruConfigs_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete && lruConfigs.SelectedItem != null)
            {
                lruConfigs.Items.Remove(lruConfigs.SelectedItem);
            }
        }
    }
}
