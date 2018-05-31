using System;
using System.ServiceProcess;
using System.Windows.Forms;

namespace CageChooser
{
    static class CageChooser
    {
        const string SERVICE_NAME = "shark-cage-service";

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            // comment out '&& FALSE' to install service on application start
#if DEBUG && FALSE
            var p = new System.Diagnostics.Process();
            p.StartInfo.FileName = "Powershell.exe";
            p.StartInfo.Verb = "runAs";
            var rootDir = Directory.GetParent(Directory.GetCurrentDirectory()).Parent;
            var scriptDir = rootDir.FullName + "\\install_service.ps1";

            p.StartInfo.Arguments = "-ExecutionPolicy Unrestricted -File \"" + scriptDir + "\" -DontStartNewContext";
            p.Start();
            p.WaitForExit();
#endif

            // check if service is running
            try
            {
                using (ServiceController sc = new ServiceController(SERVICE_NAME))
                {
                    if (sc.Status != ServiceControllerStatus.Running)
                    {
                        MessageBox.Show($"{SERVICE_NAME} is not running, please make sure it is before running this program.", "Shark Cage", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }
                }
            }
            catch (Exception)
            {
                var question_result = MessageBox.Show($"Could not check if {SERVICE_NAME} is running, would you like to continue?", "Shark Cage", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (question_result == DialogResult.No)
                {
                    return;
                }
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new CageChooserForm());
        }
    }
}
