using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Repacker.Mythic;

namespace Repacker.Repack
{
    public class Functs
    {
        public static long HashMeGently(string s)
        {
            const uint SEED = 0xDEADBEEF;
            uint eax, ecx, edx, ebx, esi, edi;

            eax = ecx = edx = ebx = esi = edi = 0;
            ebx = edi = esi = (uint)s.Length +SEED;

            int i = 0;

            for (i = 0; i + 12 < s.Length; i += 12)
            {
                edi = (uint)((s[i + 7] << 24) | (s[i + 6] << 16) | (s[i + 5] << 8) | s[i + 4]) + edi;
                esi = (uint)((s[i + 11] << 24) | (s[i + 10] << 16) | (s[i + 9] << 8) | s[i + 8]) + esi;
                edx = (uint)((s[i + 3] << 24) | (s[i + 2] << 16) | (s[i + 1] << 8) | s[i]) - esi;

                edx = (edx + ebx) ^ (esi >> 28) ^ (esi << 4);
                esi += edi;
                edi = (edi - edx) ^ (edx >> 26) ^ (edx << 6);
                edx += esi;
                esi = (esi - edi) ^ (edi >> 24) ^ (edi << 8);
                edi += edx;
                ebx = (edx - esi) ^ (esi >> 16) ^ (esi << 16);
                esi += edi;
                edi = (edi - ebx) ^ (ebx >> 13) ^ (ebx << 19);
                ebx += esi;
                esi = (esi - edi) ^ (edi >> 28) ^ (edi << 4);
                edi += ebx;
            }

            if (s.Length - i > 0)
            {
                switch (s.Length - i)
                {
                    case 12:
                        esi += (uint)s[i + 11] << 24;
                        goto case 11;
                    case 11:
                        esi += (uint)s[i + 10] << 16;
                        goto case 10;
                    case 10:
                        esi += (uint)s[i + 9] << 8;
                        goto case 9;
                    case 9:
                        esi += (uint)s[i + 8];
                        goto case 8;
                    case 8:
                        edi += (uint)s[i + 7] << 24;
                        goto case 7;
                    case 7:
                        edi += (uint)s[i + 6] << 16;
                        goto case 6;
                    case 6:
                        edi += (uint)s[i + 5] << 8;
                        goto case 5;
                    case 5:
                        edi += (uint)s[i + 4];
                        goto case 4;
                    case 4:
                        ebx += (uint)s[i + 3] << 24;
                        goto case 3;
                    case 3:
                        ebx += (uint)s[i + 2] << 16;
                        goto case 2;
                    case 2:
                        ebx += (uint)s[i + 1] << 8;
                        goto case 1;
                    case 1:
                        ebx += (uint)s[i];
                        break;
                }

                esi = (esi ^ edi) - ((edi >> 18) ^ (edi << 14));
                ecx = (esi ^ ebx) - ((esi >> 21) ^ (esi << 11));
                edi = (edi ^ ecx) - ((ecx >> 7) ^ (ecx << 25));
                esi = (esi ^ edi) - ((edi >> 16) ^ (edi << 16));
                edx = (esi ^ ecx) - ((esi >> 28) ^ (esi << 4));
                edi = (edi ^ edx) - ((edx >> 18) ^ (edx << 14));
                eax = (esi ^ edi) - ((edi >> 8) ^ (edi << 24));

                return ((long)edi << 32) | eax;
            }

            return ((long)esi << 32) | eax;
        }
    }
    public class AdlerChecksum
    {
        // parameters
        #region
        /// <summary>
        /// AdlerBase is Adler-32 checksum algorithm parameter.
        /// </summary>
        public const uint AdlerBase = 0xFFF1;
        /// <summary>
        /// AdlerStart is Adler-32 checksum algorithm parameter.
        /// </summary>
        public const uint AdlerStart = 0x0001;
        /// <summary>
        /// AdlerBuff is Adler-32 checksum algorithm parameter.
        /// </summary>
        public const uint AdlerBuff = 0x0400;
        /// Adler-32 checksum value
        private uint m_unChecksumValue = 0;
        #endregion
        /// <value>
        /// ChecksumValue is property which enables the user
        /// to get Adler-32 checksum value for the last calculation 
        /// </value>
        public uint ChecksumValue
        {
            get
            {
                return m_unChecksumValue;
            }
        }
        /// <summary>
        /// Calculate Adler-32 checksum for buffer
        /// </summary>
        /// <param name="bytesBuff">Bites array for checksum calculation</param>
        /// <param name="unAdlerCheckSum">Checksum start value (default=1)</param>
        /// <returns>Returns true if the checksum values is successflly calculated</returns>
        public bool MakeForBuff(byte[] bytesBuff, uint unAdlerCheckSum)
        {
            if (Object.Equals(bytesBuff, null))
            {
                m_unChecksumValue = 0;
                return false;
            }
            int nSize = bytesBuff.GetLength(0);
            if (nSize == 0)
            {
                m_unChecksumValue = 0;
                return false;
            }
            uint unSum1 = unAdlerCheckSum & 0xFFFF;
            uint unSum2 = (unAdlerCheckSum >> 16) & 0xFFFF;
            for (int i = 0; i < nSize; i++)
            {
                unSum1 = (unSum1 + bytesBuff[i]) % AdlerBase;
                unSum2 = (unSum1 + unSum2) % AdlerBase;
            }
            m_unChecksumValue = (unSum2 << 16) + unSum1;
            return true;
        }
        /// <summary>
        /// Calculate Adler-32 checksum for buffer
        /// </summary>
        /// <param name="bytesBuff">Bites array for checksum calculation</param>
        /// <returns>Returns true if the checksum values is successflly calculated</returns>
        public bool MakeForBuff(byte[] bytesBuff)
        {
            return MakeForBuff(bytesBuff, AdlerStart);
        }
        /// <summary>
        /// Calculate Adler-32 checksum for file
        /// </summary>
        /// <param name="sPath">Path to file for checksum calculation</param>
        /// <returns>Returns true if the checksum values is successflly calculated</returns>
        public bool MakeForFile(string sPath)
        {
            try
            {
                if (!File.Exists(sPath))
                {
                    m_unChecksumValue = 0;
                    return false;
                }
                FileStream fs = new FileStream(sPath, FileMode.Open, FileAccess.Read);
                if (Object.Equals(fs, null))
                {
                    m_unChecksumValue = 0;
                    return false;
                }
                if (fs.Length == 0)
                {
                    m_unChecksumValue = 0;
                    return false;
                }
                m_unChecksumValue = AdlerStart;
                byte[] bytesBuff = new byte[AdlerBuff];
                for (uint i = 0; i < fs.Length; i++)
                {
                    uint index = i % AdlerBuff;
                    bytesBuff[index] = (byte)fs.ReadByte();
                    if ((index == AdlerBuff - 1) || (i == fs.Length - 1))
                    {
                        if (!MakeForBuff(bytesBuff, m_unChecksumValue))
                        {
                            m_unChecksumValue = 0;
                            return false;
                        }
                    }
                }
            }
            catch
            {
                m_unChecksumValue = 0;
                return false;
            }
            return true;
        }
        /// <summary>
        /// Equals determines whether two files (buffers) 
        /// have the same checksum value (identical).
        /// </summary>
        /// <param name="obj">A AdlerChecksum object for comparison</param>
        /// <returns>Returns true if the value of checksum is the same
        /// as this instance; otherwise, false
        /// </returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;
            if (this.GetType() != obj.GetType())
                return false;
            AdlerChecksum other = (AdlerChecksum)obj;
            return (this.ChecksumValue == other.ChecksumValue);
        }
        /// <summary>
        /// operator== determines whether AdlerChecksum objects are equal.
        /// </summary>
        /// <param name="objA">A AdlerChecksum object for comparison</param>
        /// <param name="objB">A AdlerChecksum object for comparison</param>
        /// <returns>Returns true if the values of its operands are equal</returns>
        public static bool operator ==(AdlerChecksum objA, AdlerChecksum objB)
        {
            if (Object.Equals(objA, null) && Object.Equals(objB, null)) return true;
            if (Object.Equals(objA, null) || Object.Equals(objB, null)) return false;
            return objA.Equals(objB);
        }
        /// <summary>
        /// operator!= determines whether AdlerChecksum objects are not equal.
        /// </summary>
        /// <param name="objA">A AdlerChecksum object for comparison</param>
        /// <param name="objB">A AdlerChecksum object for comparison</param>
        /// <returns>Returns true if the values of its operands are not equal</returns>
        public static bool operator !=(AdlerChecksum objA, AdlerChecksum objB)
        {
            return !(objA == objB);
        }
        /// <summary>
        /// GetHashCode returns hash code for this instance.
        /// </summary>
        /// <returns>hash code of AdlerChecksum</returns>
        public override int GetHashCode()
        {
            return ChecksumValue.GetHashCode();
        }
        /// <summary>
        /// ToString is a method for current AdlerChecksum object
        /// representation in textual form.
        /// </summary>
        /// <returns>Returns current checksum or
        /// or "Unknown" if checksum value is unavailable 
        /// </returns>
        public override string ToString()
        {
            if (ChecksumValue != 0)
                return ChecksumValue.ToString();
            return "Unknown";
        }
    }

}
