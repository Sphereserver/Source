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
                result.AppendLine(String.Format("- {0}", m_ListIndex[i].ToString()));
                result.AppendLine(String.Format("- {0}", m_ListData[i].ToString()));
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

            result.AppendLine(String.Format("{0} (size: {1})", NAME, SIZE));
            result.AppendLine(String.Format("- Offset of DataBlock: 0x{0:X}", m_OffsetOfDataBlock));
            result.AppendLine(String.Format("- Size of Data Header: {0}", m_SizeofDataHeaders));
            result.AppendLine(String.Format("- Length Compressed/Uncompressed: {0}/{1} bytes", m_LenghtCompressed, m_LenghtUncompressed));
            result.AppendLine(String.Format("- Unknown Data 1: 0x{0:X}", m_Unknown1));
            result.AppendLine(String.Format("- Unknown Data 2: 0x{0:X}", m_Unknown2));
            result.Append(String.Format("- Separator: 0x{0:X}", m_Separator));

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

            result.AppendLine(String.Format("{0} (size: {1}/{2})", NAME, SIZE, SIZE + m_CompressedData.Length));
            result.AppendLine(String.Format("- Separator: 0x{0:X}", m_Separator));
            result.Append(String.Format("- Unknown Data: 0x{0:X}", m_Unknown));

            return result.ToString();
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_Separator);
            bStream.Write(m_Unknown);
            bStream.Write(m_CompressedData);
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
    }
}