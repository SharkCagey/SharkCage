using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace CageConfigurator
{
    static class Program
    {
        private class NativeMethods
        {
            [DllImport("CageNetwork.dll", CallingConvention = CallingConvention.Cdecl)]
            public static extern void SendConfig(
                [MarshalAs(UnmanagedType.LPWStr)] string config_path
            );

            [DllImport("user32.dll")]
            public static extern IntPtr GetThreadDesktop(int dwThreadId);

            [DllImport("kernel32.dll")]
            public static extern int GetCurrentThreadId();

            //[DllImport("user32")]
            //public static extern int GetUserObjectSecurity(int hObj, ref int pSIRequested, ref SECURITY_DESCRIPTOR pSd, int nLength, ref int lpnLengthNeeded);
        }

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] parameter)
        {
            var desktop = NativeMethods.GetThreadDesktop(NativeMethods.GetCurrentThreadId());

            bool started_in_cage = false;
            if (!started_in_cage)
            {
                //NativeMethods.SendConfig(@"C:\sharkcage\CageConfigurator.sconfig");
            }
            else
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new CageConfiguratorForm(parameter.Length > 0 ? parameter[0] : null));
            }
        }
    }
}
