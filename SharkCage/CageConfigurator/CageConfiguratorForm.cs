using AForge.Video.DirectShow;
using CageConfigurator.Properties;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Security.AccessControl;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using System.Text;
using System.Windows.Forms;

namespace CageConfigurator
{
    public partial class CageConfiguratorForm : Form
    {
        const int CONFIG_VERSION = 1;
        const string APPLICATION_PATH_PROPERTY = "application_path";
        const string TOKEN_PROPERTY = "token";
        const string ADDITIONAL_APP_NAME_PROPERTY = "additional_application";
        const string ADDITIONAL_APP_PATH_PROPERTY = "additional_application_path";


        private FilterInfoCollection video_device_list;
        private VideoCaptureDevice video_device;

        private bool unsaved_data = false;
        private string current_config_name = "unsaved";

        public CageConfiguratorForm(string config_to_load)
        {
            InitializeComponent();
            InitializeInputs();
            if (config_to_load != null)
            {
                CheckPath(config_to_load, ".sconfig", (string path) =>
                {
                    LoadConfig(config_to_load);
                });
            }
        }

        #region General

        private void InitializeInputs()
        {
            current_config_name = "unsaved";
            secureSecondaryPrograms.SelectedIndex = 0;
            applicationPath.ResetText();
            tokenBox.Image = null;
            SetUnsavedData(false);

            video_device_list = new FilterInfoCollection(FilterCategory.VideoInputDevice);
            foreach (FilterInfo device in video_device_list)
            {
                videoSources.Items.Add(device.Name);
            }
            videoSources.SelectedIndex = 0;
        }

        private void CheckPath(string path, string file_type, Action<string> onSuccess)
        {
            CheckPath(path, new string[] { file_type }, onSuccess);
        }

        private void CheckPath(string path, string[] file_types, Action<string> onSuccess)
        {
            if (path.Length == 0)
            {
                return;
            }

            var matching_type = file_types.Any(type =>
            {
                if (type.StartsWith("*"))
                {
                    type = type.Substring(1);
                }

                return path.EndsWith(type);
            });

            if (matching_type && File.Exists(path))
            {
                onSuccess?.Invoke(path);
            }
            else
            {
                MessageBox.Show("You specified an invalid path.", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                applicationPath.Focus();
            }
        }

        private void SetUnsavedData(bool unsaved_data)
        {
            this.unsaved_data = unsaved_data;
            var title_modifier = unsaved_data ? "*" : "";
            Text = $"Cage Configurator - {title_modifier}{current_config_name}";
        }

        private void CageConfiguratorForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            StopWebcam();
        }

        #endregion

        #region Application Path

        private void applicationBrowseButton_Click(object sender, System.EventArgs e)
        {
            const string file_type = "*.exe";

            var file_dialog = new OpenFileDialog();
            file_dialog.CheckFileExists = true;
            file_dialog.Filter = $"Application|{file_type}";
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                var chosen_file = file_dialog.FileName;
                CheckPath(chosen_file, file_type, (string path) =>
                {
                    if (applicationPath.Text != path)
                    {
                        applicationPath.Text = path;
                        SetUnsavedData(true);
                    }

                    applicationPath.SelectionStart = applicationPath.Text.Length;
                    applicationPath.ScrollToCaret();
                });
            }
        }

        private void applicationPath_Leave(object sender, EventArgs e)
        {
            CheckPath(applicationPath.Text, ".exe", (string path) =>
            {
                SetUnsavedData(true);
            });
        }

        #endregion

        #region Menu

        private void newToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (unsaved_data)
            {
                var contine_unsaved = MessageBox.Show("There are unsaved changes. Do you want to continue?", "Sharc Cage", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (contine_unsaved == DialogResult.No)
                {
                    return;
                }
            }

            InitializeInputs();
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (unsaved_data)
            {
                var contine_unsaved = MessageBox.Show("There are unsaved changes. Do you want to continue?", "Sharc Cage", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (contine_unsaved == DialogResult.No)
                {
                    return;
                }
            }

            const string file_type = ".sconfig";

            var file_dialog = new OpenFileDialog();
            file_dialog.CheckFileExists = true;
            file_dialog.Filter = $"SharkCage configuration|*{file_type}";
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                var chosen_file = file_dialog.FileName;
                CheckPath(chosen_file, file_type, LoadConfig);
            }
        }

        private void LoadConfig(string config_path)
        {
            var json = JObject.Parse(File.ReadAllText(config_path));
            var application_path = json.GetValue(APPLICATION_PATH_PROPERTY).ToString();
            var token = json.GetValue(TOKEN_PROPERTY).ToString();
            var additional_app = json.GetValue(ADDITIONAL_APP_NAME_PROPERTY).ToString();
            var additional_app_path = json.GetValue(ADDITIONAL_APP_PATH_PROPERTY).ToString();

            applicationPath.Text = application_path;
            tokenBox.Image = GetImageFromBase64(token);
            LoadAdditionalApp(additional_app, additional_app_path);
            current_config_name = config_path;
            Text = $"Cage Configurator - {current_config_name}";
        }

        private void LoadAdditionalApp(string additional_app, string additional_app_path)
        {
            switch (additional_app)
            {
                case "Keepass":
                    secureSecondaryPrograms.SelectedIndex = 1;
                    Settings.Default.PeristentKeepassPath = additional_app_path;
                    break;
                default:
                    secureSecondaryPrograms.SelectedIndex = 0;
                    break;

            }
        }

        private System.Drawing.Image GetImageFromBase64(string base64)
        {
            byte[] bytes = Convert.FromBase64String(base64);
            var ms = new MemoryStream(bytes, 0, bytes.Length);

            return System.Drawing.Image.FromStream(ms, true);

        }

        #endregion

        #region Secondary Applications

        private void secureSecondaryPrograms_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (secureSecondaryPrograms.SelectedIndex != 0)
            {
                // FIXME service must verify signature of these trusted applications!
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

            SetUnsavedData(true);
        }

        #endregion

        #region Token

        private void tokenBrowseButton_Click(object sender, EventArgs e)
        {
            string[] file_types = { "*.bmp", "*.png", "*.jpeg", "*.jpg" };

            var file_dialog = new OpenFileDialog();
            file_dialog.CheckFileExists = true;
            file_dialog.Filter = $"Picture|{String.Join(";", file_types)}";
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                var chosen_file = file_dialog.FileName;
                CheckPath(chosen_file, file_types, (string path) =>
                {
                    if (tokenBox.ImageLocation != path)
                    {
                        tokenBox.ImageLocation = path;
                        tokenBox.Load();
                        SetUnsavedData(true);
                    }
                });
            }
        }

        private void tokenWebcamButton_Click(object sender, EventArgs e)
        {
            // FIXME denied access with the Windows 10 privacy options
            // is not yet detectable and could lead to bugs -> find a solution
            if (video_device == null || !video_device.IsRunning)
            {
                InitializeWebcam();
            }
            else
            {
                StopWebcam();
            }
        }

        private void InitializeWebcam()
        {
            video_device = new VideoCaptureDevice(video_device_list[videoSources.SelectedIndex].MonikerString);
            video_device.NewFrame += (s, args) =>
            {
                var frame = args.Frame.Clone() as Bitmap;
                tokenBox.Image = frame;
            };
            video_device.Start();

            videoSources.Visible = true;
            tokenWebcamButton.Text = "Capture Frame";
        }

        private void StopWebcam()
        {
            if (video_device != null)
            {
                video_device.Stop();
                video_device = null;
                videoSources.Visible = false;
                tokenWebcamButton.Text = "Use webcam";
            }
        }

        #endregion

        #region CageChooser

        private void openCageChooserButton_Click(object sender, EventArgs e)
        {
            const string registry_key = @"HKEY_LOCAL_MACHINE\SOFTWARE\SharkCage";
            var install_dir = Registry.GetValue(registry_key, "InstallDir", "") as string;

            if (install_dir.Length == 0)
            {
                MessageBox.Show("Could not read installation directory from registry, opening CageChooser not possible", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var p = new System.Diagnostics.Process();
            p.StartInfo.FileName = $@"{install_dir}\CageChooser.exe";
            p.Start();
        }

        #endregion

        #region Serialization

        private void saveButton_Click(object sender, EventArgs e)
        {
            if (applicationPath.Text.Length == 0 || tokenBox.Image == null)
            {
                MessageBox.Show("You need to specify a application and a token to save the configuration.", "SharkCage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var secondary_path = GetSecondaryApplicationPath(secureSecondaryPrograms.Text);
            if (secondary_path.Length == 0 && secureSecondaryPrograms.SelectedIndex != 0)
            {
                MessageBox.Show("You specified a secondary program but did not provide a corresponding location. Please reselect the application, provide the location and try again.",
                    "SharkCage", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            var file_dialog = new SaveFileDialog();
            file_dialog.FileName = Path.GetFileNameWithoutExtension(applicationPath.Text);
            file_dialog.Filter = "SharkCage configuration|*.sconfig";
            file_dialog.DefaultExt = "sconfig";
            file_dialog.AddExtension = true;
            var result = file_dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                StringBuilder sb = new StringBuilder();
                StringWriter sw = new StringWriter(sb);

                using (JsonWriter writer = new JsonTextWriter(sw))
                {
                    writer.Formatting = Formatting.Indented;

                    writer.WriteStartObject();
                    writer.WritePropertyName("application_name");
                    writer.WriteValue(FileVersionInfo.GetVersionInfo(applicationPath.Text)?.FileDescription);
                    writer.WritePropertyName(APPLICATION_PATH_PROPERTY);
                    writer.WriteValue(applicationPath.Text);
                    writer.WritePropertyName("has_signature");
                    writer.WriteValue(IsFileSigned(applicationPath.Text));
                    writer.WritePropertyName("binary_hash");
                    writer.WriteValue(GetSha512Hash(applicationPath.Text));
                    writer.WritePropertyName(TOKEN_PROPERTY);
                    writer.WriteValue(GetBase64FromImage(tokenBox.Image));
                    writer.WritePropertyName("token_format");
                    writer.WriteValue(Path.GetExtension(tokenBox.ImageLocation));
                    writer.WritePropertyName(ADDITIONAL_APP_NAME_PROPERTY);
                    writer.WriteValue(secureSecondaryPrograms.Text);
                    writer.WritePropertyName(ADDITIONAL_APP_PATH_PROPERTY);
                    writer.WriteValue(secondary_path);
                    writer.WritePropertyName("creation_date");
                    writer.WriteValue((Int32)(DateTime.UtcNow.Subtract(new DateTime(1970, 1, 1))).TotalSeconds);
                    writer.WritePropertyName("config_version");
                    writer.WriteValue(CONFIG_VERSION);
                    writer.WriteEndObject();
                }

                // generate acl for config (only admin group has access)
                IdentityReference built_in_administrators = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);
                var file_security = new FileSecurity();
                file_security.SetOwner(built_in_administrators);
                foreach (FileSystemAccessRule fs_access_rule in file_security.GetAccessRules(true, true, typeof(SecurityIdentifier)))
                {
                    file_security.RemoveAccessRule(fs_access_rule);
                }
                file_security.AddAccessRule(new FileSystemAccessRule(built_in_administrators, FileSystemRights.FullControl, AccessControlType.Allow));
                file_security.SetAccessRuleProtection(true, false);

                // create file with acl
                using (var file_stream = File.Create(file_dialog.FileName, 1024, FileOptions.None, file_security))
                {
                    // write to file
                    using (var stream_writer = new StreamWriter(file_stream))
                    {
                        stream_writer.Write(sb.ToString());
                    }
                }

                current_config_name = file_dialog.FileName;
                SetUnsavedData(false);
            }
        }

        private string GetBase64FromImage(System.Drawing.Image image)
        {
            using (MemoryStream ms = new MemoryStream())
            {
                ImageFormat format = ImageFormat.MemoryBmp.Equals(image.RawFormat) ? ImageFormat.Bmp : image.RawFormat;
                image.Save(ms, format);
                byte[] image_bytes = ms.ToArray();

                string base64 = Convert.ToBase64String(image_bytes);
                return base64;
            }
        }

        private static string GetSha512Hash(string file_path)
        {
            using (var bs = new BufferedStream(File.OpenRead(file_path), 1048576))
            {
                var sha = new SHA512Managed();
                byte[] hash = sha.ComputeHash(bs);
                return BitConverter.ToString(hash).Replace("-", String.Empty);
            }
        }

        private string GetSecondaryApplicationPath(string name)
        {
            switch (name)
            {
                case "Keepass":
                    return Settings.Default.PeristentKeepassPath;
                default:
                    return String.Empty;
            }
        }

        private bool IsFileSigned(string file_path)
        {
            try
            {
                X509Certificate.CreateFromSignedFile(file_path);
                return true;
            }
            catch
            {
                return false;
            }
        }

        #endregion
    }
}
