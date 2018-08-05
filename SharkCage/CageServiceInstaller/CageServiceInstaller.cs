using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using System;
using System.Collections;
using System.ComponentModel;
using System.Configuration.Install;
using System.IO;
using System.Security.AccessControl;
using System.Security.Cryptography;
using System.Security.Principal;

namespace CageServiceInstaller
{
    [RunInstaller(true)]
    public partial class CageServiceInstaller : Installer
    {
        const string service_name = "shark-cage-service";

        public override void Install(System.Collections.IDictionary state_saver)
        {
            base.Install(state_saver);
        }

        public override void Commit(System.Collections.IDictionary saved_state)
        {
            base.Commit(saved_state);

            string target_dir = Context.Parameters["DP_TargetDir"].ToString();

            AssimilateConfigToCurrentSystem(target_dir);

            InstallAndStartService(target_dir);
        }

        public override void Rollback(IDictionary saved_state)
        {
            base.Rollback(saved_state);
            UninstallService();
        }

        public override void Uninstall(System.Collections.IDictionary saved_state)
        {
            base.Uninstall(saved_state);
            UninstallService();
        }

        private void AssimilateConfigToCurrentSystem(string dir_path)
        {
            var config_path = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.CommonDocuments),
                "SharkCage\\CageConfigurator.sconfig");

            Registry.SetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\SharkCage\Configs", "CageConfigurator", config_path);

            MoveConfigFileToConfigPath(config_path, dir_path);

            UpdateContentOfConfigFile(config_path, dir_path);

            UpdateSecurityAclsOfConfigFile(config_path);
        }

        private void MoveConfigFileToConfigPath(string config_path, string dir_path)
        {
            if (File.Exists(config_path))
            {
                File.Delete(config_path);
            }

            File.Move(Path.Combine(dir_path, "CageConfigurator.sconfig"), config_path);
        }

        private void UpdateContentOfConfigFile(string config_path, string dir_path)
        {
            const string APPLICATION_PATH_PROPERTY = "application_path";
            const string APPLICATION_HASH_PROPERTY = "binary_hash";
            var path_to_config_exe = Path.Combine(dir_path, "CageConfigurator.exe");

            var json = JObject.Parse(File.ReadAllText(config_path));
            var application_path = json.GetValue(APPLICATION_PATH_PROPERTY).ToString();
            json[APPLICATION_PATH_PROPERTY] = path_to_config_exe;
            json[APPLICATION_HASH_PROPERTY] = GetSha512Hash(path_to_config_exe);
            var output = Newtonsoft.Json.JsonConvert.SerializeObject(json, Newtonsoft.Json.Formatting.Indented);

            File.WriteAllText(config_path, output);
        }

        private void UpdateSecurityAclsOfConfigFile(string config_path)
        {
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

            File.SetAccessControl(config_path, file_security);
        }

        private string GetSha512Hash(string file_path)
        {
            const int buffer_size = 1048576; // ~ 1MB per read
            using (var bs = new BufferedStream(File.OpenRead(file_path), buffer_size))
            {
                var sha = new SHA512Managed();
                byte[] hash = sha.ComputeHash(bs);
                return BitConverter.ToString(hash).Replace("-", String.Empty);
            }
        }

        private void InstallAndStartService(string dir_path)
        {
            try
            {
                UninstallService();
                ServiceInstaller.InstallAndStart(service_name, service_name, dir_path + "CageService.exe");
            }
            catch (Exception ex)
            {
                throw new InstallException("An exception occured while installing the service", ex);
            }
        }

        private void UninstallService()
        {
            try
            {
                if (ServiceInstaller.ServiceIsInstalled(service_name))
                {
                    ServiceInstaller.Uninstall(service_name);
                }
            }
            catch (Exception ex)
            {
                throw new InstallException("An exception occured while uninstalling the service", ex);
            }
        }
    }
}