using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;
using UOPDefine;

namespace UoKRUnpacker
{
    class UopManager : IDisposable
    {
        private static UopManager sm_Istance;

        public static UopManager getIstance()
        {
            return getIstance(false);
        }

        public static UopManager getIstance(bool bReset)
        {
            if (bReset && (sm_Istance != null))
            {
                sm_Istance.Dispose();
                sm_Istance = null;
            }

            return (sm_Istance = (sm_Istance == null) ? new UopManager() : sm_Istance);
        }

        private string m_UopPath;
        private UOPFile m_UopFile;

        private void UnloadUop()
        {
            if (m_UopFile != null)
            {
                m_UopFile.Dispose();
                m_UopFile = null;
            }
        }

        public UopManager()
        {
            m_UopPath = null;
            m_UopFile = null;
        }

        public UOPFile UopFile
        {
            get { return m_UopFile; }
        }

        public string UopPath
        {
            get { return m_UopPath; }
            set { m_UopPath = value; }
        }

        public bool Load()
        {
            UnloadUop();

            bool bReturn = true;

            try
            {
                using (FileStream fsToParse = new FileStream(m_UopPath, FileMode.Open))
                {
                    using (BinaryReader brToParse = new BinaryReader(fsToParse))
                    {
                        m_UopFile = UOPFile.FromBinary(brToParse);
                    }
                }
            }
            catch
            {
                m_UopFile = null;
                bReturn = false;
            }

            GC.Collect();
            return bReturn;
        }

        public bool Load(string sPath)
        {
            m_UopPath = sPath;
            return Load();
        }

        public bool Write(string sPath)
        {
            bool bReturn = true;

            try
            {
                using (FileStream fsToParse = new FileStream(sPath, FileMode.Create))
                {
                    using (BinaryWriter brToParse = new BinaryWriter(fsToParse))
                    {
                        m_UopFile.ToBinary(brToParse);
                    }
                }
            }
            catch
            {
                bReturn = false;
            }

            GC.Collect();
            return bReturn;
        }

        public bool UnPack(string sPath)
        {
            bool bReturn = true;
            string fileName = Path.GetFileNameWithoutExtension(m_UopPath);
            int i = 0, j = 0;

            try
            {
                foreach (UOPIndexBlockHeader dumpTemp1 in UopFile.m_Content)
                {
                    foreach (UOPPairData dumpTemp2 in dumpTemp1.m_ListData)
                    {
                        using (FileStream fsWrite = File.Create(sPath + @"\" + String.Format(StaticData.UNPACK_NAMEPATTERN, fileName, i, j, dumpTemp2.First.IsCompressed ? StaticData.UNPACK_EXT_COMP : StaticData.UNPACK_EXT_UCOMP)))
                        {
                            using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                            {
                                bwWrite.Write(dumpTemp2.Second.Extract(dumpTemp2.First.IsCompressed, dumpTemp2.First.m_LenghtUncompressed));
                            }
                        }

                        j++;
                    }

                    i++;
                    j = 0;
                }
            }
            catch
            {
                bReturn = false;
            }

            GC.Collect();
            return bReturn;
        }

        public void FixOffsets(int iIndex, int subIndex)
        {
            // Fix every IndexHeader
            foreach (UOPIndexBlockHeader uopibhCurrent in m_UopFile.m_Content)
            {
                uopibhCurrent.m_Files = (uint)(uopibhCurrent.FilesDynamicCount);
            }

            // Fix total file count
            int iCurrentFiles = m_UopFile.FilesDynamicCount;
            if (iCurrentFiles != (int)(m_UopFile.m_Header.m_totalIndex))
            {
                m_UopFile.m_Header.m_totalIndex = (uint)(iCurrentFiles);
            }

            // Fix compression flag
            foreach (UOPIndexBlockHeader uopibhCurrent in m_UopFile.m_Content)
            {
                foreach (UOPPairData uopPairCurrent in uopibhCurrent.m_ListData)
                {
                    uopPairCurrent.First.IsCompressed = uopPairCurrent.First.IsReallyCompressed;
                }
            }

            // Fix offsets starting from (iIndex,subIndex)
            for (int outerIndex = iIndex; outerIndex < m_UopFile.m_Content.Count; outerIndex++)
            {
                for (int innerIndex = subIndex; innerIndex < m_UopFile.m_Content[outerIndex].m_ListData.Count; innerIndex++)
                {
                    int outerIndexForCalc, innerIndexForCalc;
                    ulong newOffset;

                    if (innerIndex == 0)
                    {
                        if (outerIndex == 0)
                        {
                            innerIndexForCalc = outerIndexForCalc = 0;
                        }
                        else
                        {
                            outerIndexForCalc = outerIndex - 1;
                            innerIndexForCalc = m_UopFile.m_Content[outerIndexForCalc].m_ListData.Count - 1;
                        }
                    }
                    else
                    {
                        outerIndexForCalc = outerIndex;
                        innerIndexForCalc = innerIndex - 1;
                    }

                    newOffset = m_UopFile.m_Content[outerIndexForCalc].m_ListData[innerIndexForCalc].First.m_OffsetOfDataBlock;
                    newOffset += ((innerIndex == 0) && (outerIndex == 0)) ? 0 : (UOPFileData.SIZE + m_UopFile.m_Content[outerIndexForCalc].m_ListData[innerIndexForCalc].First.m_LenghtCompressed);
                    if (outerIndex != outerIndexForCalc)
                    {
                        newOffset += UOPIndexBlockHeader.SIZE + (UOPFileIndexDef.SIZE * 100); 
                    }

                    m_UopFile.m_Content[outerIndex].m_ListData[innerIndex].First.m_OffsetOfDataBlock = newOffset;
                }

                // After the first ride, we start from the begin
                subIndex = 0;

                if ( m_UopFile.m_Content[outerIndex].m_OffsetNextIndex != 0 )
                {
                    int outerIndexForCalc = outerIndex - 1;

                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex = (ulong)((outerIndexForCalc == -1) ? 0 : m_UopFile.m_Content[outerIndexForCalc].m_OffsetNextIndex);
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += (ulong)((outerIndexForCalc == -1) ? UOPGeneralHeader.SIZE : 0);
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += (ulong)(UOPIndexBlockHeader.SIZE) ;
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += (ulong)(UOPFileIndexDef.SIZE * 100);
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += (ulong)(UOPFileData.SIZE * m_UopFile.m_Content[outerIndex].m_ListData.Count);
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += m_UopFile.m_Content[outerIndex].TotalSizeOfCompressedData;
                }
            }
        }

        public UopPatchError Replace(string sWhat, UOPPairData upData, bool bUncompressed)
        {
            if (upData == null)
            {
                return UopPatchError.IndexBlockError;
            }

            if (( sWhat == null ) || (!File.Exists(sWhat)))
            {
                return UopPatchError.FileError;
            }

            byte[] fileContent = null;
            using (FileStream fsToParse = new FileStream(sWhat, FileMode.Open))
            {
                using (BinaryReader brToParse = new BinaryReader(fsToParse))
                {
                    long fSize = fsToParse.Seek(0, SeekOrigin.End);
                    fileContent = new byte[fSize];
                    fsToParse.Seek(0, SeekOrigin.Begin);
                    fileContent = brToParse.ReadBytes((int)fSize);
                }
            }

            byte[] compressedStream = null;
            int iDestLength = -1;

            if (bUncompressed)
            {
                compressedStream = fileContent;
                iDestLength = fileContent.Length;
            }
            else
            {
                compressedStream = new byte[(int)Compressor.CompressBound((ulong)(fileContent.Length))];
                iDestLength = compressedStream.Length;
                if (ZLibError.Okay != Compressor.Compress(compressedStream, ref iDestLength, fileContent, fileContent.Length))
                {
                    GC.Collect();
                    return UopPatchError.CompressionError;
                }
            }

            if ((compressedStream == null) || (iDestLength == -1) || (bUncompressed && (compressedStream.Length != iDestLength)))
            {
                GC.Collect();
                return UopPatchError.BufferError;
            }

            bool bResultReplace = upData.ReplaceData(compressedStream, (uint)(fileContent.Length));

            GC.Collect();
            return bResultReplace ? UopPatchError.Okay : UopPatchError.ReplaceError;
        }

        public UopPatchError Replace(string sWhat, int iIndex, int subIndex, bool bUncompressed)
        {
            if (m_UopFile.m_Content.Count <= iIndex)
            {
                return UopPatchError.IndexBlockError;
            }

            if (m_UopFile.m_Content[iIndex].m_ListData.Count <= subIndex)
            {
                return UopPatchError.FileIndexError;
            }

            return Replace(sWhat, m_UopFile.m_Content[iIndex].m_ListData[subIndex], bUncompressed);
        }

        public enum UopPatchError : int
        {
            ReplaceError = -7,
            WriteError = -6,
            BufferError = -5,
            CompressionError = -4,
            FileIndexError = -3,
            IndexBlockError = -2,
            FileError = -1,

            Okay = 0,
        }


        #region IDisposable Membri di

        public void Dispose()
        {
            UnloadUop();
        }

        #endregion
    }
}
