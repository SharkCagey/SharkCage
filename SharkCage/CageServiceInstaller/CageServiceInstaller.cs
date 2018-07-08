using System;
using System.Collections;
using System.ComponentModel;
using System.Configuration.Install;

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

        private void InstallAndStartService(string path)
        {
            try
            {
                UninstallService();
                ServiceInstaller.InstallAndStart(service_name, service_name, path);
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