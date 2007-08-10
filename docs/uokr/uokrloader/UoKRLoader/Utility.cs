using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Win32;
using System.IO;

namespace UoKRLoader
{
    class StaticData
    {
        public const string KRLOADER_SITE = "http://scriptsharing.dv-team.de/dle/comment.php?dlid=861";

        public const string LAUNCH_CFG = "login.cfg";
        public const string UOKR_REGKEY = @"Origin Worlds Online\Ultima Online Kingdom Reborn\1.0";
        public const string UOKR_CLIENT = @"uokr.exe";
        public const string UOKR_PATCHCLIENT = @"uokr.patched.exe";

        #region IP Address const
        private static byte[] UOKR_IPDATA_0 = { 0x6A, 0x39, 0x68, 0x99, 0x00, 0x00, 0x00, 0x68, 0x9F, 0x00, 0x00, 0x00, 0x56, 
                                             0xB1, 0xC4, 0xE8, 0xC7, 0xFF, 0xFF, 0xFF, 0x66, 0xC7, 0x46, 0x04, 0x5F, 0x1E, 
                                             0xC3, 0x66, 0xC7, 0x40, 0x04, 0x5F, 0x1E };
        // private static byte[] UOKR_IPDATA_1 = { 0xC7, 0x00, 0x39, 0xC4, 0x99, 0x9F, 0x66, 0xC7, 0x40, 0x04, 0x5F, 0x1E, 0xC3, 
        //                                     0xCC, 0xCC, 0xCC, 0x66, 0xC7, 0x40, 0x04, 0x5F, 0x1E };
        private static byte[] UOKR_IPDATA_1 = { 0x8B, 0xC6, 0xC7, 0x44, 0x24, 0x0C, 0x39, 0xC4, 0x99, 0x9F, 0x66, 0xC7, 0x44, 0x24,
                                                0x10, 0x5F, 0x1E};

        public static int UOKR_IPDATA_VERSION
        {
            get { return 2; }
        }

        public static byte[] GetIPData(int iVersion)
        {
            byte[] ipThing = null;

            switch (iVersion)
            {
                case 1:
                    {
                        ipThing = new byte[StaticData.UOKR_IPDATA_1.Length];
                        StaticData.UOKR_IPDATA_1.CopyTo(ipThing, 0);

                    } break;

                case 0:
                    {
                        ipThing = new byte[StaticData.UOKR_IPDATA_0.Length];
                        StaticData.UOKR_IPDATA_0.CopyTo(ipThing, 0);
                    } break;

                default:
                    {

                    } break;
            }

            return ipThing;
        }

        public static byte[] GetPatchedIPData(int iVersion, System.Net.IPAddress theIP, uint thePort)
        {
            byte[] patchedThing = null;

            switch (iVersion)
            {
                case 1:
                    {
                        patchedThing = new byte[StaticData.UOKR_IPDATA_1.Length];
                        StaticData.UOKR_IPDATA_1.CopyTo(patchedThing, 0);
                        //patchedThing[2] = theIP.GetAddressBytes()[3];
                        //patchedThing[3] = theIP.GetAddressBytes()[2];
                        //patchedThing[4] = theIP.GetAddressBytes()[1];
                        //patchedThing[5] = theIP.GetAddressBytes()[0];
                        //patchedThing[10] = (byte)(thePort & 0xFF);
                        //patchedThing[11] = (byte)((thePort & 0xFF00) >> 8);
                        //patchedThing[20] = (byte)(thePort & 0xFF);
                        //patchedThing[21] = (byte)((thePort & 0xFF00) >> 8);
                        patchedThing[6] = theIP.GetAddressBytes()[3];
                        patchedThing[7] = theIP.GetAddressBytes()[2];
                        patchedThing[8] = theIP.GetAddressBytes()[1];
                        patchedThing[9] = theIP.GetAddressBytes()[0];
                        patchedThing[15] = (byte)(thePort & 0xFF);
                        patchedThing[16] = (byte)((thePort & 0xFF00) >> 8);
                    } break;

                case 0:
                    {
                        patchedThing = new byte[StaticData.UOKR_IPDATA_0.Length];
                        StaticData.UOKR_IPDATA_0.CopyTo(patchedThing, 0);
                        patchedThing[1] = theIP.GetAddressBytes()[3];
                        patchedThing[3] = theIP.GetAddressBytes()[1];
                        patchedThing[8] = theIP.GetAddressBytes()[0];
                        patchedThing[14] = theIP.GetAddressBytes()[2];
                        patchedThing[24] = (byte)(thePort & 0xFF);
                        patchedThing[25] = (byte)((thePort & 0xFF00) >> 8);
                        patchedThing[30] = (byte)(thePort & 0xFF);
                        patchedThing[31] = (byte)((thePort & 0xFF00) >> 8);
                    } break;

                default:
                    {

                    } break;
            }

            return patchedThing;
        }

        #endregion

        #region Encryption const
        public static byte[] UOKR_LOGDATA = { 0x8A, 0x4F, 0x04, 0x30, 0x08, 0x8B, 0x4F, 0x04, 0x8B, 0x47, 0x08, 0xFF, 0x45, 
                                              0xF0, 0x8B, 0xD0 };
        public static byte[] UOKR_LOGPATCHDATA = { 0x8A, 0x4F, 0x04, 0x90, 0x90, 0x8B, 0x4F, 0x04, 0x8B, 0x47, 0x08, 0xFF, 
                                                   0x45, 0xF0, 0x8B, 0xD0 };
        public static byte[] UOKR_ENCDATA = { 0x74, 0x32, 0x8B, 0x30, 0x33, 0x31, 0x8B, 0x55, 0x10, 0x33, 0x75, 0xD4, 0x89, 
                                              0x32, 0x8B, 0x70, 0x04, 0x33, 0x71, 0x04, 0x33, 0x75, 0xD8, 0x89, 0x72, 0x04, 
                                              0x8B, 0x70, 0x08, 0x33, 0x71, 0x08, 0x33, 0x75, 0xDC, 0x89, 0x72, 0x08, 0x8B, 
                                              0x40, 0x0C, 0x33, 0x41, 0x0C, 0x33, 0x45, 0xE0, 0x89, 0x42, 0x0C };
        public static byte[] UOKR_ENCPATCHDATA = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x8B, 0x55, 0x10, 0x90, 0x90, 0x90, 0x90, 
                                                   0xC7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0xC7, 
                                                   0x42, 0x04, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0xC7, 
                                                   0x42, 0x08, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0xC7, 
                                                   0x42, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
                                                   0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                                                   0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                                                   0x90, 0x90, 0x90, 0x90, 0x90 };
        #endregion
    }

    class Utility
    {
        public static string GetExePath(string subName)
        {
            try
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey(string.Format(@"SOFTWARE\{0}", subName));

                if (key == null)
                {
                    key = Registry.CurrentUser.OpenSubKey(string.Format(@"SOFTWARE\{0}", subName));
                    if (key == null)
                    {
                        return null;
                    }
                }

                string path = key.GetValue("ExePath") as string;
                if (((path == null) || (path.Length <= 0)) || (!Directory.Exists(path) && !File.Exists(path)))
                {
                    path = key.GetValue("Install Dir") as string;
                    if (((path == null) || (path.Length <= 0)) || (!Directory.Exists(path) && !File.Exists(path)))
                    {
                        return null;
                    }
                }

                path = Path.GetDirectoryName(path);
                if ((path == null) || !Directory.Exists(path))
                {
                    return null;
                }

                return path;
            }
            catch
            {
                return null;
            }
        }

        public static int Search(Stream pc, byte[] buffer, bool bFile)
        {
            int basicOffset = bFile ? 0 : 0x400000;
            int count = 0x1000 + buffer.Length;
            byte[] buffer2 = new byte[count];
            int num2 = 0;

            while (true)
            {
                pc.Seek((long)(basicOffset + (num2 * 0x1000)), SeekOrigin.Begin);
                if (pc.Read(buffer2, 0, count) != count)
                {
                    break;
                }
                for (int i = 0; i < 0x1000; i++)
                {
                    bool flag = true;
                    for (int j = 0; flag && (j < buffer.Length); j++)
                    {
                        flag = buffer[j] == buffer2[i + j];
                    }
                    if (flag)
                    {
                        return ((basicOffset + (num2 * 0x1000)) + i);
                    }
                }
                num2++;
            }

            return 0;
        }

        public static int VersionToInteger(string version)
        {
            string[] splittedVersion = version.Split('.');
            int iVersion = -1;

            try
            {
                iVersion = Int32.Parse(splittedVersion[3]) +
                           Int32.Parse(splittedVersion[2]) * 100 +
                           Int32.Parse(splittedVersion[1]) * 10000 +
                           Int32.Parse(splittedVersion[0]) * 1000000;
            }
            catch
            {

            }

            return iVersion;
        }
    }
}
