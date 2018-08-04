using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace CageConfigurator
{
    static class Program
    {
        internal class NativeMethods
        {
            [DllImport("user32.dll")]
            public static extern IntPtr GetThreadDesktop(int dwThreadId);

            [DllImport("kernel32.dll")]
            public static extern int GetCurrentThreadId();

            [DllImport("advapi32.dll", SetLastError = true)]
            public static extern int GetSecurityInfo(
                IntPtr handle,
                SE_OBJECT_TYPE objectType,
                SECURITY_INFORMATION securityInfo,
                out IntPtr sidOwner,
                out IntPtr sidGroup,
                out IntPtr dacl,
                out IntPtr sacl,
                out IntPtr securityDescriptor);

            [DllImport("advapi32.dll", SetLastError = true)]
            public static extern int GetExplicitEntriesFromAclW(
              IntPtr pacl,
              ref ulong pcCountOfExplicitEntries,
              out IntPtr pListOfExplicitEntries
            );

            [DllImport("kernel32.dll")]
            public static extern IntPtr LocalFree(IntPtr hMem);

            public enum SE_OBJECT_TYPE
            {
                SE_UNKNOWN_OBJECT_TYPE,
                SE_FILE_OBJECT,
                SE_SERVICE,
                SE_PRINTER,
                SE_REGISTRY_KEY,
                SE_LMSHARE,
                SE_KERNEL_OBJECT,
                SE_WINDOW_OBJECT,
                SE_DS_OBJECT,
                SE_DS_OBJECT_ALL,
                SE_PROVIDER_DEFINED_OBJECT,
                SE_WMIGUID_OBJECT,
                SE_REGISTRY_WOW64_32KEY
            };

            public enum SECURITY_INFORMATION
            {
                OWNER_SECURITY_INFORMATION = 1,
                GROUP_SECURITY_INFORMATION = 2,
                DACL_SECURITY_INFORMATION = 4,
                SACL_SECURITY_INFORMATION = 8,
            };

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 0)]
            public struct EXPLICIT_ACCESS
            {
                public uint grfAccessPermissions;
                public uint grfAccessMode;
                public uint grfInheritance;
                public TRUSTEE Trustee;
            };

            public enum MULTIPLE_TRUSTEE_OPERATION
            {
                NO_MULTIPLE_TRUSTEE,
                TRUSTEE_IS_IMPERSONATE
            }

            public enum TRUSTEE_FORM
            {
                TRUSTEE_IS_SID,
                TRUSTEE_IS_NAME,
                TRUSTEE_BAD_FORM,
                TRUSTEE_IS_OBJECTS_AND_SID,
                TRUSTEE_IS_OBJECTS_AND_NAME
            }

            public enum TRUSTEE_TYPE
            {
                TRUSTEE_IS_UNKNOWN,
                TRUSTEE_IS_USER,
                TRUSTEE_IS_GROUP,
                TRUSTEE_IS_DOMAIN,
                TRUSTEE_IS_ALIAS,
                TRUSTEE_IS_WELL_KNOWN_GROUP,
                TRUSTEE_IS_DELETED,
                TRUSTEE_IS_INVALID,
                TRUSTEE_IS_COMPUTER
            }

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 0)]
            public struct TRUSTEE : IDisposable
            {
                public IntPtr pMultipleTrustee;
                public MULTIPLE_TRUSTEE_OPERATION MultipleTrusteeOperation;
                public TRUSTEE_FORM TrusteeForm;
                public TRUSTEE_TYPE TrusteeType;
                public IntPtr ptstrName;

                void IDisposable.Dispose()
                {
                    if (ptstrName != IntPtr.Zero) Marshal.Release(ptstrName);
                }

                public string Name { get { return Marshal.PtrToStringAuto(ptstrName); } }
            }

            [DllImport("advapi32.dll", SetLastError = true)]
            public static extern bool AllocateAndInitializeSid(
                ref SidIdentifierAuthority pIdentifierAuthority,
                byte nSubAuthorityCount,
                uint dwSubAuthority0, uint dwSubAuthority1,
                uint dwSubAuthority2, uint dwSubAuthority3,
                uint dwSubAuthority4, uint dwSubAuthority5,
                uint dwSubAuthority6, uint dwSubAuthority7,
                out IntPtr pSid);

            [DllImport("advapi32.dll")]
            public static extern IntPtr FreeSid(IntPtr pSid);

            [DllImport("advapi32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool EqualSid(IntPtr pSid1, IntPtr pSid2);

            [StructLayout(LayoutKind.Sequential)]
            public struct SidIdentifierAuthority
            {
                [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6, ArraySubType = UnmanagedType.I1)]
                public byte[] Value;
            }
        }

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] parameter)
        {
            bool isReleaseMode = true;

#if DEBUG
            isReleaseMode = false;
#endif

            if (isReleaseMode && !StartedInCage())
            {

                MessageBox.Show("CageConfigurator can only be run on a secure desktop. Please start the Configurator manually with the corresponding entry in the CageChooser.",
                    "SharkCage", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new CageConfiguratorForm(parameter.Length > 0 ? parameter[0] : null));
            }
        }

        // FIXME might be easier to ask the service for this info in the future? :/
        static bool StartedInCage()
        {
            try
            {
                var desktop = NativeMethods.GetThreadDesktop(NativeMethods.GetCurrentThreadId());

                IntPtr owner_sid = IntPtr.Zero;
                IntPtr group_sid = IntPtr.Zero;
                IntPtr dacl;
                IntPtr sacl = IntPtr.Zero;
                IntPtr security_descriptor;

                int val = NativeMethods.GetSecurityInfo(
                    desktop,
                    NativeMethods.SE_OBJECT_TYPE.SE_WINDOW_OBJECT,
                    NativeMethods.SECURITY_INFORMATION.DACL_SECURITY_INFORMATION,
                    out owner_sid,
                    out group_sid,
                    out dacl,
                    out sacl,
                    out security_descriptor);

                NativeMethods.LocalFree(security_descriptor);

                ulong entry_count = 0;
                IntPtr entries_pointer;

                NativeMethods.GetExplicitEntriesFromAclW(dacl, ref entry_count, out entries_pointer);

                if (entry_count == 2)
                {
                    bool admin_sid_all_denied = false;
                    bool shark_sid_all_granted = false;

                    IntPtr shark_entry = entries_pointer;
                    IntPtr admin_entry = IntPtr.Add(entries_pointer, Marshal.SizeOf(typeof(NativeMethods.EXPLICIT_ACCESS)));

                    NativeMethods.EXPLICIT_ACCESS ea_shark = (NativeMethods.EXPLICIT_ACCESS)Marshal.PtrToStructure(shark_entry, typeof(NativeMethods.EXPLICIT_ACCESS));
                    NativeMethods.EXPLICIT_ACCESS ea_admin = (NativeMethods.EXPLICIT_ACCESS)Marshal.PtrToStructure(admin_entry, typeof(NativeMethods.EXPLICIT_ACCESS));

                    const uint GENERIC_ALL = 0x10000000;
                    const uint STANDARD_RIGHTS_REQUIRED = 0x000F0000;
                    const uint SYNCHRONIZE = 0x00100000;
                    const uint PROCESS_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF;

                    const int GRANT_ACCESS = 1;
                    const int NO_INHERITANCE = 0;

                    if (ea_admin.Trustee.TrusteeForm == NativeMethods.TRUSTEE_FORM.TRUSTEE_IS_SID)
                    {
                        const int nt_security_authority = 5;
                        const int built_in_domain_rid = 32;
                        const int domain_alias_rid_admins = 544;
                        var nt_authority = new NativeMethods.SidIdentifierAuthority();
                        nt_authority.Value = new byte[] { 0, 0, 0, 0, 0, nt_security_authority };

                        IntPtr admin_sid = IntPtr.Zero;
                        NativeMethods.AllocateAndInitializeSid(ref nt_authority, 2, built_in_domain_rid, domain_alias_rid_admins, 0, 0, 0, 0, 0, 0, out admin_sid);
                        if (NativeMethods.EqualSid(ea_admin.Trustee.ptstrName, admin_sid))
                        {
                            if ((ea_admin.grfAccessPermissions & (~PROCESS_ALL_ACCESS)) == 0
                                && ea_admin.grfAccessMode == GRANT_ACCESS
                                && ea_admin.grfInheritance == NO_INHERITANCE)
                            {
                                admin_sid_all_denied = true;
                            }
                        }

                        if (admin_sid != IntPtr.Zero)
                        {
                            NativeMethods.FreeSid(admin_sid);
                        }
                    }

                    if (ea_shark.Trustee.TrusteeForm == NativeMethods.TRUSTEE_FORM.TRUSTEE_IS_SID)
                    {
                        if ((ea_shark.grfAccessPermissions & GENERIC_ALL) == 0
                                && ea_shark.grfAccessMode == GRANT_ACCESS
                                && ea_shark.grfInheritance == NO_INHERITANCE)
                        {
                            shark_sid_all_granted = true;
                        }
                    }

                    if (admin_sid_all_denied && shark_sid_all_granted)
                    {
                        return true;
                    }
                }
                NativeMethods.LocalFree(entries_pointer);
            }
            catch (Exception e)
            {
                MessageBox.Show("Could not determine on which desktop the process is running: " + e.ToString());
                Environment.Exit(1);
            }

            return false;
        }
    }
}
