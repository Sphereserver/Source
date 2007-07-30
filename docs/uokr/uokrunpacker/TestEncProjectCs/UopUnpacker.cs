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
            string fileName = m_UopPath.Substring((m_UopPath.LastIndexOf('\\') + 1), m_UopPath.LastIndexOf('.') - (m_UopPath.LastIndexOf('\\') + 1));

            try
            {
                for (int i = 0; i < m_UopFile.m_Content.Count; i++)
                {
                    UOPIndexBlockHeader aCurrent = m_UopFile.m_Content[i];
                    for (int j = 0; j < aCurrent.m_ListIndex.Count; j++)
                    {
                        string sFileName = fileName + "_" + i.ToString() + "_" + j.ToString();

                        UOPFileIndexDef uopFIDcurrent = aCurrent.m_ListIndex[j];
                        UOPFileData uopFDcurrent = aCurrent.m_ListData[j];

                        int iUncompressLength = ((int)(uopFIDcurrent.m_LenghtUncompressed));
                        byte[] bUnCompressData = new byte[iUncompressLength];
                        ZLibError zResult = Compressor.Decompress(bUnCompressData, ref iUncompressLength, uopFDcurrent.m_CompressedData, ((int)(uopFIDcurrent.m_LenghtCompressed)));
                        if (zResult == ZLibError.Okay)
                        {
                            using (FileStream fsWrite = File.Create(sPath + @"\" + sFileName + ".dat"))
                            {
                                using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                {
                                    bwWrite.Write(bUnCompressData);
                                }
                            }
                        }
                        else
                        {
                            using (FileStream fsWrite = File.Create(sPath + @"\" + sFileName + ".raw"))
                            {
                                using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                {
                                    bwWrite.Write(uopFDcurrent.m_CompressedData);
                                }
                            }
                        }
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

        public void FixOffsets(int iIndex, int subIndex)
        {
            int iCurrentFiles = m_UopFile.FilesDynamicCount;
            if (iCurrentFiles != (int)(m_UopFile.m_Header.m_totalIndex))
            {
                m_UopFile.m_Header.m_totalIndex = (uint)(iCurrentFiles);
            }

            for (int outerIndex = iIndex; outerIndex < m_UopFile.m_Content.Count; outerIndex++)
            {
                for (int innerIndex = subIndex; innerIndex < m_UopFile.m_Content[outerIndex].m_ListIndex.Count; innerIndex++)
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
                            innerIndexForCalc = m_UopFile.m_Content[outerIndexForCalc].m_ListIndex.Count - 1;
                        }
                    }
                    else
                    {
                        outerIndexForCalc = outerIndex;
                        innerIndexForCalc = innerIndex - 1;
                    }

                    newOffset = m_UopFile.m_Content[outerIndexForCalc].m_ListIndex[innerIndexForCalc].m_OffsetOfDataBlock;
                    newOffset += ((innerIndex == 0) && (outerIndex == 0)) ? 0 : (UOPFileData.SIZE + m_UopFile.m_Content[outerIndexForCalc].m_ListIndex[innerIndexForCalc].m_LenghtCompressed);
                    if (outerIndex != outerIndexForCalc)
                    {
                        newOffset += UOPIndexBlockHeader.SIZE + (UOPFileIndexDef.SIZE * 100); 
                    }

                    m_UopFile.m_Content[outerIndex].m_ListIndex[innerIndex].m_OffsetOfDataBlock = newOffset;
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

        public UopPatchError Replace(string sWhat, int iIndex, int subIndex, bool bUncompressed)
        {
            if (!File.Exists(sWhat))
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

            if (m_UopFile.m_Content.Count <= iIndex)
            {
                GC.Collect();
                return UopPatchError.IndexBlockError;
            }

            if (m_UopFile.m_Content[iIndex].m_ListIndex.Count <= subIndex)
            {
                GC.Collect();
                return UopPatchError.FileIndexError;
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

            m_UopFile.m_Content[iIndex].m_ListIndex[subIndex].m_LenghtCompressed = (uint)iDestLength;
            m_UopFile.m_Content[iIndex].m_ListIndex[subIndex].m_LenghtUncompressed = (uint)fileContent.Length;
            m_UopFile.m_Content[iIndex].m_ListData[subIndex].m_CompressedData = new byte[iDestLength];
            Array.Copy(compressedStream, m_UopFile.m_Content[iIndex].m_ListData[subIndex].m_CompressedData, iDestLength);

            GC.Collect();
            return UopPatchError.Okay;
        }

        public UopPatchError Replace(string sWhat, int iIndex, int subIndex, bool bUncompressed, ref string sFileName)
        {
            sFileName = Utility.GetPathForSave(m_UopPath);

            UopPatchError repError = Replace(sWhat, iIndex, subIndex, bUncompressed);
            if (repError != UopPatchError.Okay)
            {
                return repError;
            }

            FixOffsets(iIndex, subIndex);
            bool bResult = Write(sFileName);

            GC.Collect();
            return bResult ? UopPatchError.Okay : UopPatchError.WriteError;
        }

        public enum UopPatchError : int
        {
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
