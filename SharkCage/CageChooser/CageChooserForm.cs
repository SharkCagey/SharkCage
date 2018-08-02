using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace CageChooser
{
    public partial class CageChooserForm : Form
    {
        private const string REGISTRY_KEY = @"HKEY_LOCAL_MACHINE\SOFTWARE\SharkCage";
        private Dictionary<string, string> config_name_value_mapping = new Dictionary<string, string>();

        private class NativeMethods
        {
            [DllImport("SharedFunctionality.dll", CallingConvention = CallingConvention.Cdecl)]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool SendConfig(
                [MarshalAs(UnmanagedType.LPWStr)] string config_path,
                [MarshalAs(UnmanagedType.LPWStr)] StringBuilder result,
                int max_result_length
            );
        }

        public CageChooserForm()
        {
            InitializeComponent();
        }

        #region General

        private void CageChooser_Load(object sender, EventArgs e)
        {
            LoadConfigList();
        }

        private void LoadConfigList()
        {
            config_name_value_mapping.Clear();
            registeredConfigs.Items.Clear();

            var subkey = REGISTRY_KEY.Replace("HKEY_LOCAL_MACHINE\\", "");
            var config_registry_key = Path.Combine(subkey, "Configs");
            var key = Registry.LocalMachine.OpenSubKey(config_registry_key, false);

            if (key != null)
            {
                foreach (var value_name in key.GetValueNames())
                {

                    var value = (key.GetValue(value_name) as string) ?? String.Empty;
                    config_name_value_mapping.Add(value_name, value);
                    registeredConfigs.Items.Insert(registeredConfigs.Items.Count, value_name);
                }
                if (registeredConfigs.Items.Count > 0)
                {
                    registeredConfigs.SelectedIndex = 0;
                }
            }

            openButton.Enabled = registeredConfigs.SelectedItem != null;
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            bool bHandled = false;
            switch (keyData)
            {
                case Keys.Enter:
                    if (registeredConfigs.SelectedItem != null)
                    {
                        SendToCage();
                    }
                    break;
                case Keys.F5:
                    LoadConfigList();

                    bHandled = true;
                    break;
                default:
                    return base.ProcessCmdKey(ref msg, keyData);
            }
            return bHandled;
        }

        #endregion

        #region Cage Service

        private void SendToCage()
        {
            try
            {
                var selected_item = registeredConfigs.SelectedItem?.ToString() ?? String.Empty;
                var config_path = String.Empty;
                config_name_value_mapping.TryGetValue(selected_item, out config_path);

                if (config_path != String.Empty)
                {
                    int capacity = 1000;
                    StringBuilder sb = new StringBuilder(capacity);

                    bool result = NativeMethods.SendConfig(config_path, sb, capacity);

                    if (!result)
                    {
                        MessageBox.Show("Error communicating with service: " + sb.ToString(), "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                    // bring the form back in focus
                    Activate();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An exception occured while trying to send messages to cage service: {ex.ToString()}");
            }
        }

        private void openButton_Click(object sender, EventArgs e)
        {
            SendToCage();
        }

        #endregion

        #region CageConfigurator

        private void openCageConfiguratorButton_Click(object sender, EventArgs e)
        {
            var install_dir = Registry.GetValue(REGISTRY_KEY, "InstallDir", "") as string ?? String.Empty;

            if (install_dir == String.Empty)
            {
                MessageBox.Show("Could not read installation directory from registry, opening CageConfigurator not possible", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var p = new System.Diagnostics.Process();
            p.StartInfo.FileName = $@"{install_dir}\CageConfigurator.exe";

            var selected_item = registeredConfigs.SelectedItem?.ToString() ?? String.Empty;
            var config_path = String.Empty;
            config_name_value_mapping.TryGetValue(selected_item, out config_path);
            if (config_path != String.Empty)
            {
                p.StartInfo.Arguments = $@"""{config_path}""";
            }
            p.Start();
        }

        #endregion

        private void registeredConfigs_SelectedIndexChanged(object sender, EventArgs e)
        {
            openButton.Enabled = registeredConfigs.SelectedItem != null;
        }
    }
}
