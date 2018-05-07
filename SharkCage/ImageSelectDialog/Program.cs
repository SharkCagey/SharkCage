using System;
using System.Windows.Forms;

namespace SharkCage
{
    static class ImageSelectDialog
    {
        const string configPath = @"C:\sharkcage\config.txt";

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new ImageSelectForm(configPath));
        }
    }
}
