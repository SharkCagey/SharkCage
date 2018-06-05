using System;
using System.Windows.Forms;

namespace CageConfigurator
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] parameter)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new CageConfiguratorForm(parameter.Length > 0 ? parameter[0] : null));
        }
    }
}
