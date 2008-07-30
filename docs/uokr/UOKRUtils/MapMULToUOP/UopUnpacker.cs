using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MapMULToUOP
{
   /* class UopManager : IDisposable
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
                    newOffset += UOPFileData.SIZE + m_UopFile.m_Content[outerIndexForCalc].m_ListIndex[innerIndexForCalc].m_LenghtCompressed;
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

        #region IDisposable Membri di

        public void Dispose()
        {
            UnloadUop();
        }

        #endregion
    }*/
}
