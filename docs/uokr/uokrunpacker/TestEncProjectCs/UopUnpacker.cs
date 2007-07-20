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
            m_UopFile = new UOPFile();

            try
            {
                using (FileStream fsToParse = new FileStream(m_UopPath, FileMode.Open))
                {
                    using (BinaryReader brToParse = new BinaryReader(fsToParse))
                    {
                        m_UopFile.m_Header.m_variousData = brToParse.ReadBytes(24);
                        m_UopFile.m_Header.m_totalIndex = brToParse.ReadUInt32();
                        m_UopFile.m_Header.m_Unknown = brToParse.ReadBytes(12);

                        bool repeatRead = true;
                        while (repeatRead)
                        {
                            UOPIndexBlockHeader uopIBHCurrent = new UOPIndexBlockHeader();
                            uopIBHCurrent.m_Files = brToParse.ReadUInt32();
                            uopIBHCurrent.m_OffsetNextIndex = brToParse.ReadUInt64();

                            for (uint iData = 0; iData < uopIBHCurrent.m_Files; iData++)
                            {
                                UOPFileIndexDef uopFIDcurrent = new UOPFileIndexDef();
                                uopFIDcurrent.m_OffsetOfDataBlock = brToParse.ReadUInt64();
                                uopFIDcurrent.m_SizeofDataHeaders = brToParse.ReadUInt32();
                                uopFIDcurrent.m_LenghtCompressed = brToParse.ReadUInt32();
                                uopFIDcurrent.m_LenghtUncompressed = brToParse.ReadUInt32();
                                uopFIDcurrent.m_Unknown1 = brToParse.ReadUInt64();
                                uopFIDcurrent.m_Unknown2 = brToParse.ReadUInt32();
                                uopFIDcurrent.m_Separator = brToParse.ReadUInt16();

                                uopIBHCurrent.m_ListIndex.Add(uopFIDcurrent);
                            }

                            for (uint iData = 0; iData < uopIBHCurrent.m_Files; iData++)
                            {
                                brToParse.BaseStream.Seek((long)(uopIBHCurrent.m_ListIndex[(int)(iData)].m_OffsetOfDataBlock), SeekOrigin.Begin);

                                UOPFileData uopFDcurrent = new UOPFileData();
                                uopFDcurrent.m_Separator = brToParse.ReadUInt32();
                                uopFDcurrent.m_Unknown = brToParse.ReadUInt64();
                                uopFDcurrent.m_CompressedData = new byte[uopIBHCurrent.m_ListIndex[(int)(iData)].m_LenghtCompressed];
                                uopFDcurrent.m_CompressedData = brToParse.ReadBytes(uopFDcurrent.m_CompressedData.Length);

                                uopIBHCurrent.m_ListData.Add(uopFDcurrent);
                            }

                            // Add to us.
                            m_UopFile.m_Content.Add(uopIBHCurrent);

                            if (uopIBHCurrent.m_OffsetNextIndex == 0)
                            {
                                repeatRead = false;
                            }
                            else
                            {
                                brToParse.BaseStream.Seek((long)(uopIBHCurrent.m_OffsetNextIndex), SeekOrigin.Begin);
                            }
                        }
                    }
                }
            }
            catch
            {
                bReturn = false;
            }

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
                        brToParse.Write(m_UopFile.m_Header.m_variousData);
                        brToParse.Write(m_UopFile.m_Header.m_totalIndex);
                        brToParse.Write(m_UopFile.m_Header.m_Unknown);

                        foreach( UOPIndexBlockHeader uopIBHCurrent in m_UopFile.m_Content)
                        {
                            brToParse.Write(uopIBHCurrent.m_Files);
                            brToParse.Write(uopIBHCurrent.m_OffsetNextIndex);

                            foreach (UOPFileIndexDef uopFIDcurrent in uopIBHCurrent.m_ListIndex)
                            {
                                brToParse.Write(uopFIDcurrent.m_OffsetOfDataBlock);
                                brToParse.Write(uopFIDcurrent.m_SizeofDataHeaders);
                                brToParse.Write(uopFIDcurrent.m_LenghtCompressed);
                                brToParse.Write(uopFIDcurrent.m_LenghtUncompressed);
                                brToParse.Write(uopFIDcurrent.m_Unknown1);
                                brToParse.Write(uopFIDcurrent.m_Unknown2);
                                brToParse.Write(uopFIDcurrent.m_Separator);
                            }

                            if (uopIBHCurrent.m_ListIndex.Count != 100)
                            {
                                for (int i = UOPFileIndexDef.SIZE * (100 - uopIBHCurrent.m_ListIndex.Count); i != 0; i--)
                                {
                                    brToParse.Write('\0');
                                }
                            }

                            //if (brToParse.BaseStream.Position != ((long)(uopIBHCurrent.m_ListIndex[0].m_OffsetOfDataBlock)) )
                            //    return false; 

                            foreach (UOPFileData uopFDcurrent in uopIBHCurrent.m_ListData)
                            {
                                brToParse.Write(uopFDcurrent.m_Separator);
                                brToParse.Write(uopFDcurrent.m_Unknown);
                                brToParse.Write(uopFDcurrent.m_CompressedData);
                            }
                        }
                    }
                }
            }
            catch
            {
                bReturn = false;
            }

            return bReturn;
        }

        public void FixOffsets(int iIndex, int subIndex)
        {
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
                    m_UopFile.m_Content[outerIndex].m_OffsetNextIndex += SumOfDataInIndex(outerIndex);
                }
            }
        }

        public ulong SumOfDataInIndex(int iIndex)
        {
            ulong resultSize = 0;

            foreach (UOPFileIndexDef ufidCurrednt in m_UopFile.m_Content[iIndex].m_ListIndex)
            {
                resultSize += ufidCurrednt.m_LenghtCompressed;
            }

            return resultSize;
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
            int iStartName = m_UopPath.LastIndexOf('\\') + 1;

            if (iStartName != -1)
            {
                sFileName = m_UopPath.Substring(iStartName, m_UopPath.Length - iStartName);
            }
            else
            {
                sFileName = m_UopPath;
            }

            sFileName = Application.StartupPath + @"\" + "NEW-" + sFileName;

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
