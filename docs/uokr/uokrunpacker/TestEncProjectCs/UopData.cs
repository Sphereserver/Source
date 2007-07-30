using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using UoKRUnpacker;

namespace UOPDefine
{
    class UOPGeneralHeader
    {
        public const int SIZE = 24 + 12 + 4;
        private static readonly string NAME = "UOPGeneralHeader";

        public byte[] m_variousData; // 24
        public uint m_totalIndex;
        public byte[] m_Unknown;    // 12

        public UOPGeneralHeader()
        {
            m_variousData = new byte[24];
            m_totalIndex = 0;
            m_Unknown = new byte[12];
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("{0} (size: {1})", NAME, SIZE));
            result.AppendLine(String.Format("- VariousData (size: {0}): {1}", m_variousData.Length, Utility.ByteArrayToString(m_variousData, Utility.HashStringStyle.BigHex)));
            result.AppendLine(String.Format("- Total Indexes: {0}", m_totalIndex));
            result.Append(String.Format("- UnknownData (size: {0}): {1}", m_Unknown.Length, Utility.ByteArrayToString(m_Unknown, Utility.HashStringStyle.BigHex)));

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_variousData);
            bStream.Write(m_totalIndex);
            bStream.Write(m_Unknown);
        }

        public static UOPGeneralHeader FromBinary(System.IO.BinaryReader bStream)
        {
            UOPGeneralHeader toReturn = new UOPGeneralHeader();

            toReturn.m_variousData = bStream.ReadBytes(24);
            toReturn.m_totalIndex = bStream.ReadUInt32();
            toReturn.m_Unknown = bStream.ReadBytes(12);

            return toReturn;
        }
    }

    class UOPIndexBlockHeader : IDisposable
    {
        public const int SIZE = 8 + 4;
        private static readonly string NAME = "UOPIndexBlockHeader";

        public uint m_Files;
        public ulong m_OffsetNextIndex;
        public List<UOPFileIndexDef> m_ListIndex;
        public List<UOPFileData> m_ListData;

        public UOPIndexBlockHeader()
        {
            m_Files = 0;
            m_OffsetNextIndex = 0;
            m_ListIndex = new List<UOPFileIndexDef>();
            m_ListData = new List<UOPFileData>();
        }

        public void Dispose()
        {
            m_ListIndex.Clear();
            m_ListData.Clear();
        }

        public int FilesDynamicCount
        {
            get { return m_ListIndex.Count; }
        }

        public ulong TotalSizeOfCompressedData
        {
            get
            {
                ulong resultSize = 0;

                foreach (UOPFileIndexDef ufidCurrednt in m_ListIndex)
                {
                    resultSize += ufidCurrednt.m_LenghtCompressed;
                }

                return resultSize;
            }
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("{0} (size: {1})", NAME, SIZE));
            result.AppendLine(String.Format("- Total files: {0}", m_Files));
            result.AppendLine(String.Format("- Offset Next Index: 0x{0:X}", m_OffsetNextIndex));

            for (int i = 0; i < m_ListIndex.Count; i++)
            {
                result.AppendLine(String.Format("{0}", m_ListIndex[i].ToString()));
                result.AppendLine(String.Format("{0}", m_ListData[i].ToString()));
            }

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_Files);
            bStream.Write(m_OffsetNextIndex);

            foreach (UOPFileIndexDef uopFIDcurrent in m_ListIndex)
            {
                uopFIDcurrent.ToBinary(bStream);
            }

            if (m_ListIndex.Count != 100)
            {
                for (int i = UOPFileIndexDef.SIZE * (100 - m_ListIndex.Count); i != 0; i--)
                {
                    bStream.Write('\0');
                }
            }

            foreach (UOPFileData uopFDcurrent in m_ListData)
            {
                uopFDcurrent.ToBinary(bStream);
            }
        }

        public static UOPIndexBlockHeader FromBinary(System.IO.BinaryReader bStream)
        {
            UOPIndexBlockHeader toReturn = new UOPIndexBlockHeader();

            toReturn.m_Files = bStream.ReadUInt32();
            toReturn.m_OffsetNextIndex = bStream.ReadUInt64();

            for (uint iData = 0; iData < toReturn.m_Files; iData++)
            {
                toReturn.m_ListIndex.Add(UOPFileIndexDef.FromBinary(bStream));
            }

            for (uint iData = 0; iData < toReturn.m_Files; iData++)
            {
                bStream.BaseStream.Seek((long)(toReturn.m_ListIndex[(int)(iData)].m_OffsetOfDataBlock), System.IO.SeekOrigin.Begin);
                toReturn.m_ListData.Add(UOPFileData.FromBinary(bStream, toReturn.m_ListIndex[(int)(iData)].m_LenghtCompressed));
            }

            return toReturn;
        }
    }

    class UOPFileIndexDef
    {
        public const int SIZE = 8 + 4 + 4 + 4 + 8 + 4 + 2;
        private static readonly string NAME = "UOPFileIndexDef";

        public ulong m_OffsetOfDataBlock;
        public uint m_SizeofDataHeaders;
        public uint m_LenghtCompressed;
        public uint m_LenghtUncompressed;
        public ulong m_Unknown1;
        public uint m_Unknown2;
        public ushort m_Separator;

        public UOPFileIndexDef()
        {
            m_OffsetOfDataBlock = m_Unknown1 = 0;
            m_LenghtCompressed = m_LenghtUncompressed = m_SizeofDataHeaders = m_Unknown2 = 0;
            m_Separator = 0;
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("\t{0} (size: {1})", NAME, SIZE));
            result.AppendLine(String.Format("\t\t- Offset of DataBlock: 0x{0:X}", m_OffsetOfDataBlock));
            result.AppendLine(String.Format("\t\t- Size of Data Header: {0}", m_SizeofDataHeaders));
            result.AppendLine(String.Format("\t\t- Length Compressed/Uncompressed: {0}/{1} bytes", m_LenghtCompressed, m_LenghtUncompressed));
            result.AppendLine(String.Format("\t\t- Unknown Data 1: 0x{0:X}", m_Unknown1));
            result.AppendLine(String.Format("\t\t- Unknown Data 2: 0x{0:X}", m_Unknown2));
            result.Append(String.Format("\t\t- Separator: 0x{0:X}", m_Separator));

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_OffsetOfDataBlock);
            bStream.Write(m_SizeofDataHeaders);
            bStream.Write(m_LenghtCompressed);
            bStream.Write(m_LenghtUncompressed);
            bStream.Write(m_Unknown1);
            bStream.Write(m_Unknown2);
            bStream.Write(m_Separator);
        }

        public static UOPFileIndexDef FromBinary(System.IO.BinaryReader bStream)
        {
            UOPFileIndexDef toReturn = new UOPFileIndexDef();

            toReturn.m_OffsetOfDataBlock = bStream.ReadUInt64();
            toReturn.m_SizeofDataHeaders = bStream.ReadUInt32();
            toReturn.m_LenghtCompressed = bStream.ReadUInt32();
            toReturn.m_LenghtUncompressed = bStream.ReadUInt32();
            toReturn.m_Unknown1 = bStream.ReadUInt64();
            toReturn.m_Unknown2 = bStream.ReadUInt32();
            toReturn.m_Separator = bStream.ReadUInt16();

            return toReturn;
        }
    }

    class UOPFileData
    {
        public const int SIZE = 8 + 4;
        private static readonly string NAME = "UOPFileData";

        public uint m_Separator; // ( WORD[2] 0x0008 0x0003 )
        public ulong m_Unknown; // UNKNOWN, possibly a CRC
        public byte[] m_CompressedData;

        public UOPFileData()
        {
            m_Separator = 0;
            m_Unknown = 0; // qword
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("\t{0} (size: {1}/{2})", NAME, SIZE, SIZE + m_CompressedData.Length));
            result.AppendLine(String.Format("\t\t- Separator: 0x{0:X}", m_Separator));
            result.Append(String.Format("\t\t- Unknown Data: 0x{0:X}", m_Unknown));

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_Separator);
            bStream.Write(m_Unknown);
            bStream.Write(m_CompressedData);
        }

        public static UOPFileData FromBinary(System.IO.BinaryReader bStream, uint length)
        {
            UOPFileData toReturn = new UOPFileData();

            toReturn.m_Separator = bStream.ReadUInt32();
            toReturn.m_Unknown = bStream.ReadUInt64();
            toReturn.m_CompressedData = new byte[length];
            toReturn.m_CompressedData = bStream.ReadBytes(toReturn.m_CompressedData.Length);

            return toReturn;
        }
    }

    class UOPFile : IDisposable
    {
        private static readonly string NAME = "UOPFile";

        public UOPGeneralHeader m_Header;
        public List<UOPIndexBlockHeader> m_Content;

        public UOPFile()
        {
            m_Header = new UOPGeneralHeader();
            m_Content = new List<UOPIndexBlockHeader>();
        }

        public void Dispose()
        {
            foreach (UOPIndexBlockHeader uoibhNow in m_Content)
            {
                uoibhNow.Dispose();
            }

            m_Content.Clear();
        }

        public int FilesDynamicCount
        {
            get 
            {
                int toReturn = 0;

                foreach (UOPIndexBlockHeader current in m_Content)
                {
                    toReturn += current.FilesDynamicCount;
                }

                return toReturn; 
            }
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("{0}", NAME));
            result.AppendLine(String.Format("{0}", m_Header.ToString()));
            foreach (UOPIndexBlockHeader uoibhNow in m_Content)
            {
                result.AppendLine(String.Format("{0}", uoibhNow.ToString()));
            }

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            m_Header.ToBinary(bStream);
            foreach (UOPIndexBlockHeader uoibhNow in m_Content)
            {
                uoibhNow.ToBinary(bStream);
            }
        }

        public static UOPFile FromBinary(System.IO.BinaryReader bStream)
        {
            UOPFile toReturn = new UOPFile();

            toReturn.m_Header = UOPGeneralHeader.FromBinary(bStream);

            bool repeatRead = true;
            while (repeatRead)
            {
                UOPIndexBlockHeader uopIBHCurrent = UOPIndexBlockHeader.FromBinary(bStream);
                toReturn.m_Content.Add(uopIBHCurrent);

                if (uopIBHCurrent.m_OffsetNextIndex == 0)
                {
                    repeatRead = false;
                }
                else
                {
                    bStream.BaseStream.Seek((long)(uopIBHCurrent.m_OffsetNextIndex), System.IO.SeekOrigin.Begin);
                }
            }

            return toReturn;
        }
    }
}