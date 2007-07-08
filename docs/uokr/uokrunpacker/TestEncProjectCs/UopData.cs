using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace UOPDefine
{
    class UOPGeneralHeader
    {
        public const int SIZE = 24 + 12 + 4;

        public byte[] m_variousData; // 24
        public uint m_totalIndex;
        public byte[] m_Unknown;    // 12

        public UOPGeneralHeader()
        {
            m_variousData = new byte[24];
            m_totalIndex = 0;
            m_Unknown = new byte[12];
        }
    }

    class UOPIndexBlockHeader : IDisposable
    {
        public const int SIZE = 8 + 4;

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
    }

    class UOPFileIndexDef
    {
        public const int SIZE = 8 + 4 + 4 + 4 + 8 + 4 + 2;

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
    }

    class UOPFileData
    {
        public const int SIZE = 8 + 4;

        public uint m_Separator; // ( WORD[2] 0x0008 0x0003 )
        public ulong m_Unknown; // UNKNOWN, possibly a CRC
        public byte[] m_CompressedData;

        public UOPFileData()
        {
            m_Separator = 0;
            m_Unknown = 0; // qword
        }
    }

    class UOPFile : IDisposable
    {
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
    }
}