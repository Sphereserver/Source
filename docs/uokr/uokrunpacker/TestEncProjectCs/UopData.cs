using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using UoKRUnpacker;

namespace UOPDefine
{
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

        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(this.GetType()))
                return false;

            UOPFile objCurrent = (UOPFile)obj;

            if (!m_Header.Equals(m_Header))
                return false;

            if (m_Content.Count != objCurrent.m_Content.Count)
                return false;

            for (int i = 0; i < m_Content.Count; i++)
            {
                if (!m_Content[i].Equals(objCurrent.m_Content[i]))
                    return false;
            }

            return true;
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

        public override bool Equals(object obj)
        {
            if ( !obj.GetType().Equals(this.GetType()) )
                return false;

            UOPGeneralHeader objCurrent = (UOPGeneralHeader)obj;
            return ((m_totalIndex == objCurrent.m_totalIndex) && Array.Equals(m_Unknown, objCurrent.m_Unknown) && Array.Equals(m_variousData, objCurrent.m_variousData));
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
        public List<UOPPairData> m_ListData;

        public UOPIndexBlockHeader()
        {
            m_Files = 0;
            m_OffsetNextIndex = 0;
            m_ListData = new List<UOPPairData>();
        }

        public void Dispose()
        {
            m_ListData.Clear();
        }

        public int FilesDynamicCount
        {
            get { return m_ListData.Count; }
        }

        public ulong TotalSizeOfCompressedData
        {
            get
            {
                ulong resultSize = 0;

                foreach (UOPPairData ufidCurrednt in m_ListData)
                {
                    resultSize += ufidCurrednt.First.m_LenghtCompressed;
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

            for (int i = 0; i < m_ListData.Count; i++)
            {
                result.AppendLine(String.Format("{0}", m_ListData[i].First.ToString()));
                result.AppendLine(String.Format("{0}", m_ListData[i].Second.ToString()));
            }

            return result.ToString();
        }

        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(this.GetType()))
                return false;

            UOPIndexBlockHeader objCurrent = (UOPIndexBlockHeader)obj;

            if (m_Files != objCurrent.m_Files)
                return false;

            if (m_OffsetNextIndex != objCurrent.m_OffsetNextIndex)
                return false;

            if (m_ListData.Count != objCurrent.m_ListData.Count)
                return false;

            for (int i = 0; i < m_ListData.Count; i++)
            {
                if (!m_ListData[i].Equals(objCurrent.m_ListData[i]))
                    return false;
            }

            return true;
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_Files);
            bStream.Write(m_OffsetNextIndex);

            foreach (UOPPairData uopFIDcurrent in m_ListData)
            {
                uopFIDcurrent.First.ToBinary(bStream);
            }

            if (m_ListData.Count != 100)
            {
                for (int i = UOPFileIndexDef.SIZE * (100 - m_ListData.Count); i != 0; i--)
                {
                    bStream.Write('\0');
                }
            }

            foreach (UOPPairData uopFDcurrent in m_ListData)
            {
                uopFDcurrent.Second.ToBinary(bStream);
            }
        }

        public byte[] Extract(UOPFileIndexDef id)
        {
            for (int i = 0; i < m_ListData.Count; i++)
                if (m_ListData[i].First.Equals(id))
                    return m_ListData[i].Second.Extract(m_ListData[i].First.IsCompressed, m_ListData[i].First.m_LenghtUncompressed);

            return null;
        }

        public byte[] Extract(int id)
        {
            if ((id < 0) || (id > m_ListData.Count))
                return null;

            return m_ListData[id].Second.Extract(m_ListData[id].First.IsCompressed, m_ListData[id].First.m_LenghtUncompressed);
        }

        public static UOPIndexBlockHeader FromBinary(System.IO.BinaryReader bStream)
        {
            UOPIndexBlockHeader toReturn = new UOPIndexBlockHeader();

            toReturn.m_Files = bStream.ReadUInt32();
            toReturn.m_OffsetNextIndex = bStream.ReadUInt64();

            for (uint iData = 0; iData < toReturn.m_Files; iData++)
            {
                UOPFileIndexDef uopIndextemp = UOPFileIndexDef.FromBinary(bStream);

                long oldPos = bStream.BaseStream.Position;
                bStream.BaseStream.Seek((long)(uopIndextemp.m_OffsetOfDataBlock), System.IO.SeekOrigin.Begin);

                UOPFileData uopDatatemp = UOPFileData.FromBinary(bStream, uopIndextemp.m_LenghtCompressed);

                toReturn.m_ListData.Add(new UOPPairData(uopIndextemp, uopDatatemp));
                bStream.BaseStream.Seek(oldPos, System.IO.SeekOrigin.Begin);
            }

            return toReturn;
        }
    }

    class UOPPairData
    {
        private UOPFileIndexDef m_FirstData;
        private UOPFileData m_SecondData;

        public UOPPairData(UOPFileIndexDef object1, UOPFileData object2)
        {
            m_FirstData = object1;
            m_SecondData = object2;
        }

        public UOPFileIndexDef First
        {
            get { return m_FirstData; }
        }

        public UOPFileData Second
        {
            get { return m_SecondData; }
        }

        public bool ReplaceData(byte[] bData, uint uncompressedLength)
        {
            if ( bData == null )
                return false;

            bool bReturn = false;
            uint compressedLength = (uint)(bData.Length);
            bool isCompressed = (compressedLength != uncompressedLength);

            try
            {
                this.Second.m_CompressedData = new byte[compressedLength];
                Array.Copy(bData, this.Second.m_CompressedData, compressedLength);

                this.First.m_LenghtCompressed = compressedLength;
                this.First.m_LenghtUncompressed = uncompressedLength;
                this.First.IsCompressed = isCompressed;

                bReturn = true;
            }
            catch
            {

            }

            return bReturn;
        }

        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(this.GetType()))
                return false;

            UOPPairData objPair = (UOPPairData)obj;

            return objPair.First.Equals(this.First) && objPair.Second.Equals(this.Second);
        }
    }

    class UOPFileIndexDef
    {
        public const int SIZE = 8 + 4 + 4 + 4 + 4 + 4 + 4 + 2;
        private static readonly string NAME = "UOPFileIndexDef";

        public ulong m_OffsetOfDataBlock;
        public uint m_SizeofDataHeaders;
        public uint m_LenghtCompressed;
        public uint m_LenghtUncompressed;
        public uint m_Unknown1;
        public uint m_Unknown2;
        public uint m_Unknown3;
        public ushort m_CompressedFlag;

        public UOPFileIndexDef()
        {
            m_OffsetOfDataBlock = m_Unknown1 = 0;
            m_LenghtCompressed = m_LenghtUncompressed = m_SizeofDataHeaders = m_Unknown1 = m_Unknown2 = m_Unknown3 = 0;
            m_CompressedFlag = 0;
        }

        public bool IsCompressed
        {
            get { return (m_CompressedFlag != 0); }
            set { m_CompressedFlag = (ushort)((value) ? 1 : 0); }
        }

        public bool IsReallyCompressed
        {
            get { return (m_LenghtCompressed != m_LenghtUncompressed);  }
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
            result.AppendLine(String.Format("\t\t- Unknown Data 3: 0x{0:X}", m_Unknown3));
            result.Append(String.Format("\t\t- Compressed Flag: 0x{0:X}", m_CompressedFlag));

            return result.ToString();
        }

        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(this.GetType()))
                return false;

            UOPFileIndexDef objCurrent = (UOPFileIndexDef)obj;

            return ((m_OffsetOfDataBlock == objCurrent.m_OffsetOfDataBlock) && (m_SizeofDataHeaders == objCurrent.m_SizeofDataHeaders) &&
                    (m_LenghtCompressed == objCurrent.m_LenghtCompressed) && (m_LenghtUncompressed == objCurrent.m_LenghtUncompressed) &&
                    (m_Unknown1 == objCurrent.m_Unknown1) && (m_Unknown2 == objCurrent.m_Unknown2) && (m_Unknown3 == objCurrent.m_Unknown3) &&
                    (m_CompressedFlag == objCurrent.m_CompressedFlag));
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_OffsetOfDataBlock);
            bStream.Write(m_SizeofDataHeaders);
            bStream.Write(m_LenghtCompressed);
            bStream.Write(m_LenghtUncompressed);
            bStream.Write(m_Unknown1);
            bStream.Write(m_Unknown2);
            bStream.Write(m_Unknown3);
            bStream.Write(m_CompressedFlag);
        }

        public static UOPFileIndexDef FromBinary(System.IO.BinaryReader bStream)
        {
            UOPFileIndexDef toReturn = new UOPFileIndexDef();

            toReturn.m_OffsetOfDataBlock = bStream.ReadUInt64();
            toReturn.m_SizeofDataHeaders = bStream.ReadUInt32();
            toReturn.m_LenghtCompressed = bStream.ReadUInt32();
            toReturn.m_LenghtUncompressed = bStream.ReadUInt32();
            toReturn.m_Unknown1 = bStream.ReadUInt32();
            toReturn.m_Unknown2 = bStream.ReadUInt32();
            toReturn.m_Unknown3 = bStream.ReadUInt32();
            toReturn.m_CompressedFlag = bStream.ReadUInt16();

            return toReturn;
        }
    }

    class UOPFileData
    {
        public const int SIZE = 2 + 2 + 8;
        private static readonly string NAME = "UOPFileData";

        public ushort m_DataFlag; // ( 0x0003 )
        public ushort m_LocalOffsetToData; // Because they could enlarge the next field
        public ulong m_Unknown; // UNKNOWN, possibly a CRC
        public byte[] m_CompressedData;

        public UOPFileData()
        {
            m_DataFlag = m_LocalOffsetToData = 0;
            m_Unknown = 0; // qword
        }

        public override string ToString()
        {
            StringBuilder result = new StringBuilder();

            result.AppendLine(String.Format("\t{0} (size: {1}/{2})", NAME, SIZE, SIZE + m_CompressedData.Length));
            result.AppendLine(String.Format("\t\t- Data Flag: 0x{0:X}", m_DataFlag));
            result.AppendLine(String.Format("\t\t- Local Offset To Data: 0x{0:X}", m_LocalOffsetToData));
            result.Append(String.Format("\t\t- Unknown Data: 0x{0:X}", m_Unknown));

            return result.ToString();
        }

        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(this.GetType()))
                return false;

            UOPFileData objCurrent = (UOPFileData)obj;

            return ((m_DataFlag == objCurrent.m_DataFlag) && (m_LocalOffsetToData == objCurrent.m_LocalOffsetToData) && (m_Unknown == objCurrent.m_Unknown) 
                     && Array.Equals(m_CompressedData, objCurrent.m_CompressedData));
        }

        public void ToBinary(System.IO.BinaryWriter bStream)
        {
            bStream.Write(m_DataFlag);
            bStream.Write(m_LocalOffsetToData);
            bStream.Write(m_Unknown);
            bStream.Write(m_CompressedData);
        }

        public byte[] Extract(bool isCompressed, uint uncompressedLength)
        {
            if (!isCompressed)
            {
                return m_CompressedData;
            }
            else
            {
                int iUncompressLength = ((int)(uncompressedLength));
                byte[] bUnCompressData = new byte[iUncompressLength];
                ZLibError zResult = Compressor.Decompress(bUnCompressData, ref iUncompressLength, m_CompressedData, m_CompressedData.Length);

                return (zResult == ZLibError.Okay) ? bUnCompressData : null;
            }
        }

        public static UOPFileData FromBinary(System.IO.BinaryReader bStream, uint length)
        {
            UOPFileData toReturn = new UOPFileData();

            toReturn.m_DataFlag = bStream.ReadUInt16();
            toReturn.m_LocalOffsetToData = bStream.ReadUInt16();
            toReturn.m_Unknown = bStream.ReadUInt64();
            toReturn.m_CompressedData = new byte[length];
            toReturn.m_CompressedData = bStream.ReadBytes(toReturn.m_CompressedData.Length);

            return toReturn;
        }
    }
}