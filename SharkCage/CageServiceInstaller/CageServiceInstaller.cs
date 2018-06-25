using System;
using System.ComponentModel;
using System.Configuration.Install;

namespace CageServiceInstaller
{
    [RunInstaller(true)]
    public partial class CageServiceInstaller : Installer
    {
        const string service_name = "shark-cage-service";

        public override void Commit(System.Collections.IDictionary savedState)
        {
            base.Commit(savedState);

            string target_dir = Context.Parameters["DP_TargetDir"].ToString();
            InstallAndStartService(target_dir);
        }

        public override void Uninstall(System.Collections.IDictionary savedState)
        {
            base.Uninstall(savedState);

            UninstallService();
        }


        public override void Install(System.Collections.IDictionary stateSaver)
        {
            base.Install(stateSaver);
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
                    ServiceState state = ServiceInstaller.GetServiceStatus(service_name);

                    if (state == ServiceState.Running)
                    {
                        ServiceInstaller.StopService(service_name);
                    }

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