using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.IO;

namespace MapMULToUOP
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
        public ulong m_OffsetOfDataBlock = 0;
        public uint m_SizeofDataHeaders = 12;
        public uint m_LenghtCompressed;
        public uint m_LenghtUncompressed;
        public long fileHash;
        public uint adlerCRC32;
        public ushort compressedFlag = 1;

        public const int SIZE = 34;

    }

    class UOPFileData
    {
        public ushort m_Flag_compressed = 8;
        public ushort m_Offset = 3;
        public long m_FileTime;
        public byte[] m_CompressedData;

        public uint Hash()
        {
            MemoryStream ms = new MemoryStream();
            BinaryWriter bw = new BinaryWriter(ms);
            bw.Write(m_Flag_compressed);
            bw.Write(m_Offset);
            bw.Write(m_CompressedData);
            bw.Close();
            byte[] source=new byte[ms.Length];
            ms.Position=0;
            ms.Read(source,0,(int)ms.Length);
            ms.Close();
            return Compressor.AdlerCheckSum32(source);
        }

        public ulong GetLength()
        {
            return 12 + (ulong)m_CompressedData.Length;
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