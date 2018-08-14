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
            public static extern uint GetExplicitEntriesFromAclW(
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
                public int grfAccessMode;
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
            [return: MarshalAs(UnmanagedType.I1)]
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
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool EqualSid(IntPtr pSid1, IntPtr pSid2);

            [StructLayout(LayoutKind.Sequential)]
            public struct SidIdentifierAuthority
            {
                [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6, ArraySubType = UnmanagedType.I1)]
                public byte[] Value;
            }

            public enum DESKTOP_ACCESS_RIGHTS : uint
            {
                DESKTOP_READOBJECTS = 0x0001,
                DESKTOP_CREATEWINDOW = 0x0002,
                DESKTOP_CREATEMENU = 0x0004,
                DESKTOP_HOOKCONTROL = 0x0008,
                DESKTOP_JOURNALRECORD = 0x0010,
                DESKTOP_JOURNALPLAYBACK = 0x0020,
                DESKTOP_ENUMERATE = 0x0040,
                DESKTOP_WRITEOBJECTS = 0x0080,
                DESKTOP_SWITCHDESKTOP = 0x0100,
                DELETE = 0x00010000,
                READ_CONTROL = 0x00020000,
                WRITE_DAC = 0x00040000,
                WRITE_OWNER = 0x00080000
            }
        }

        public class NativeCallException : Exception
        {
            public NativeCallException(string message)
               : base(message)
            {
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
            // fixme comment back in
            if (/*isReleaseMode &&*/!StartedInCage())
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

        static bool StartedInCage()
        {
            IntPtr security_descriptor = IntPtr.Zero;
            IntPtr entries_pointer = IntPtr.Zero;
            bool desktop_access_right_status = false;

            try
            {
                var desktop = NativeMethods.GetThreadDesktop(NativeMethods.GetCurrentThreadId());
                if (desktop == IntPtr.Zero)
                {
                    throw new NativeCallException("Failed to retrieve desktop handle.");
                }

                IntPtr owner_sid = IntPtr.Zero;
                IntPtr group_sid = IntPtr.Zero;
                IntPtr dacl;
                IntPtr sacl = IntPtr.Zero;

                if (NativeMethods.GetSecurityInfo(
                    desktop,
                    NativeMethods.SE_OBJECT_TYPE.SE_WINDOW_OBJECT,
                    NativeMethods.SECURITY_INFORMATION.DACL_SECURITY_INFORMATION,
                    out owner_sid,
                    out group_sid,
                    out dacl,
                    out sacl,
                    out security_descriptor) != 0)
                {
                    throw new NativeCallException("Failed to retrieve security info.");
                }

                ulong entry_count = 0;
                if (NativeMethods.GetExplicitEntriesFromAclW(dacl, ref entry_count, out entries_pointer) != 0)
                {
                    throw new NativeCallException("Failed to retrieve acl entries.");
                }

                if (entry_count == 3)
                {
                    bool system_sid_correct_access = false;
                    bool admin_sid_correct_access = false;
                    bool shark_sid_correct_access = false;

                    IntPtr shark_entry = entries_pointer;
                    IntPtr admin_entry = IntPtr.Add(shark_entry, Marshal.SizeOf(typeof(NativeMethods.EXPLICIT_ACCESS)));
                    IntPtr system_entry = IntPtr.Add(admin_entry, Marshal.SizeOf(typeof(NativeMethods.EXPLICIT_ACCESS)));

                    NativeMethods.EXPLICIT_ACCESS ea_shark = (NativeMethods.EXPLICIT_ACCESS)Marshal.PtrToStructure(shark_entry, typeof(NativeMethods.EXPLICIT_ACCESS));
                    NativeMethods.EXPLICIT_ACCESS ea_admin = (NativeMethods.EXPLICIT_ACCESS)Marshal.PtrToStructure(admin_entry, typeof(NativeMethods.EXPLICIT_ACCESS));
                    NativeMethods.EXPLICIT_ACCESS ea_system = (NativeMethods.EXPLICIT_ACCESS)Marshal.PtrToStructure(system_entry, typeof(NativeMethods.EXPLICIT_ACCESS));

                    const int GRANT_ACCESS = 1;
                    const int NO_INHERITANCE = 0;

                    if (ea_shark.Trustee.TrusteeForm == NativeMethods.TRUSTEE_FORM.TRUSTEE_IS_SID)
                    {
                        const NativeMethods.DESKTOP_ACCESS_RIGHTS desired_shark_permissions = NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_READOBJECTS
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_CREATEWINDOW
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_CREATEMENU
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_HOOKCONTROL
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_JOURNALRECORD
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_JOURNALPLAYBACK
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_ENUMERATE
                            | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_WRITEOBJECTS;

                        if ((ea_shark.grfAccessPermissions & (uint)desired_shark_permissions) == (uint)desired_shark_permissions
                                && (ea_shark.grfAccessPermissions & (~((uint)desired_shark_permissions))) == 0
                                && ea_shark.grfAccessMode == GRANT_ACCESS
                                && ea_shark.grfInheritance == NO_INHERITANCE)
                        {
                            shark_sid_correct_access = true;
                        }
                    }

                    if (ea_admin.Trustee.TrusteeForm == NativeMethods.TRUSTEE_FORM.TRUSTEE_IS_SID)
                    {
                        const int nt_security_authority = 5;
                        const int security_builtin_domain_rid = 32;
                        const int domain_alias_rid_admins = 544;
                        var nt_authority = new NativeMethods.SidIdentifierAuthority();
                        nt_authority.Value = new byte[] { 0, 0, 0, 0, 0, nt_security_authority };

                        IntPtr admin_sid = IntPtr.Zero;
                        if (!NativeMethods.AllocateAndInitializeSid(
                            ref nt_authority,
                            2,
                            security_builtin_domain_rid,
                            domain_alias_rid_admins,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            out admin_sid))
                        {
                            throw new NativeCallException("Failed to allocate or initialize admin security identifier.");
                        }

                        if (ea_admin.Trustee.ptstrName == IntPtr.Zero)
                        {
                            throw new NativeCallException("Failed to retrieve admin sid.");
                        }
                        else if (NativeMethods.EqualSid(ea_admin.Trustee.ptstrName, admin_sid))
                        {
                            NativeMethods.DESKTOP_ACCESS_RIGHTS desired_admin_permissions = NativeMethods.DESKTOP_ACCESS_RIGHTS.DELETE
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_ENUMERATE
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.READ_CONTROL
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.WRITE_DAC
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.WRITE_OWNER;

                            if ((ea_admin.grfAccessPermissions & (uint)desired_admin_permissions) == (uint)desired_admin_permissions
                                && (ea_admin.grfAccessPermissions & (~((uint)desired_admin_permissions))) == 0
                                && ea_admin.grfAccessMode == GRANT_ACCESS
                                && ea_admin.grfInheritance == NO_INHERITANCE)
                            {
                                admin_sid_correct_access = true;
                            }
                        }

                        if (admin_sid != IntPtr.Zero)
                        {
                            NativeMethods.FreeSid(admin_sid);
                        }
                    }

                    if (ea_system.Trustee.TrusteeForm == NativeMethods.TRUSTEE_FORM.TRUSTEE_IS_SID)
                    {
                        const int nt_security_authority = 5;
                        const int security_local_system_rid = 18;
                        var nt_authority = new NativeMethods.SidIdentifierAuthority();
                        nt_authority.Value = new byte[] { 0, 0, 0, 0, 0, nt_security_authority };

                        IntPtr system_sid = IntPtr.Zero;
                        if (!NativeMethods.AllocateAndInitializeSid(ref nt_authority,
                            1,
                            security_local_system_rid,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            out system_sid))
                        {
                            throw new NativeCallException("Failed to allocate or initialize system security identifier.");
                        }

                        if (ea_system.Trustee.ptstrName == IntPtr.Zero)
                        {
                            throw new NativeCallException("Failed to retrieve system sid.");
                        }
                        else if (NativeMethods.EqualSid(ea_system.Trustee.ptstrName, system_sid))
                        {
                            const NativeMethods.DESKTOP_ACCESS_RIGHTS desired_system_permissions = NativeMethods.DESKTOP_ACCESS_RIGHTS.DELETE
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_READOBJECTS
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_CREATEWINDOW
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_CREATEMENU
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_HOOKCONTROL
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_JOURNALRECORD
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_JOURNALPLAYBACK
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_ENUMERATE
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_WRITEOBJECTS
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.DESKTOP_SWITCHDESKTOP
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.READ_CONTROL
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.WRITE_DAC
                                | NativeMethods.DESKTOP_ACCESS_RIGHTS.WRITE_OWNER;

                            if ((ea_system.grfAccessPermissions & (uint)desired_system_permissions) == (uint)desired_system_permissions
                                && (ea_system.grfAccessPermissions & (~((uint)desired_system_permissions))) == 0
                                && ea_system.grfAccessMode == GRANT_ACCESS
                                && ea_system.grfInheritance == NO_INHERITANCE)
                            {
                                system_sid_correct_access = true;
                            }
                        }

                        if (system_sid != IntPtr.Zero)
                        {
                            NativeMethods.FreeSid(system_sid);
                        }
                    }

                    if (system_sid_correct_access && admin_sid_correct_access && shark_sid_correct_access)
                    {
                        desktop_access_right_status = true;
                    }
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Could not determine on which desktop the process is running: " + e.ToString());
                Environment.Exit(1);
            }
            finally
            {
                NativeMethods.LocalFree(entries_pointer);
                NativeMethods.LocalFree(security_descriptor);
            }

            return desktop_access_right_status;
        }
    }
}
