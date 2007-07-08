using System;
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Security.Cryptography;
using UOPDefine;

namespace UoKRUnpacker
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private string HashToArray(byte[] theHash)
        {
            StringBuilder sbBuffer = new StringBuilder("0x");
            foreach (byte bSingle in theHash)
            {
                sbBuffer.AppendFormat("{0:X}", bSingle);
            }

            return sbBuffer.ToString();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            LoadUOP();
            ParseDump(false);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            LoadUOP();
            ParseDump(true);
        }

        private void btnParsePatch_Click(object sender, EventArgs e)
        {
            LoadUOP();

            PatchForm frmPatch = new PatchForm(this);
            frmPatch.ShowDialog(this);
        }

        private void LoadUOP()
        {
            ResetTextArea();

            DialogResult drFile = oFileDlgUopopen.ShowDialog(this);
            if (drFile != DialogResult.OK)
            {
                MessageBox.Show("File not selected!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            AppendTextArea("Parsing file " + oFileDlgUopopen.FileName + " ...\n");

            UopManager upIstance = UopManager.getIstance(true);
            upIstance.UopPath = oFileDlgUopopen.FileName;
            if (!upIstance.Load())
            {
                AppendTextArea("ERROR while parsing file " + oFileDlgUopopen.FileName + " !!!\n");
                return;
            }

            AppendTextArea("Done parsing.\n\n");
        }

        private void ParseDump(bool bDump)
        {
            UOPFile uopToParse = UopManager.getIstance().UopFile;

            {
                AppendTextArea("Header: ");
                foreach (byte bCurrent in uopToParse.m_Header.m_variousData)
                    AppendTextArea(bCurrent.ToString() + " ");
                AppendTextArea("\n");
                AppendTextArea("UnkData: ");
                foreach (byte bCurrent in uopToParse.m_Header.m_Unknown)
                    AppendTextArea(bCurrent.ToString() + " ");
                AppendTextArea("\n");


                AppendTextArea("Total Index Blocks: " + uopToParse.m_Header.m_totalIndex.ToString() + "\n");
                int iCurrentindexDef = 0;
                foreach (UOPIndexBlockHeader aCurrent in uopToParse.m_Content)
                {
                    AppendTextArea("-- Index[" + iCurrentindexDef.ToString() + "] track files: " + aCurrent.m_Files.ToString() + "\n");
                    AppendTextArea("-- Index[" + iCurrentindexDef.ToString() + "] total compressed data: " + UopManager.getIstance().SumOfDataInIndex(iCurrentindexDef).ToString() + " bytes\n");
                    int iCurrentIdef = 0;

                    if (!bDump)
                    {
                        StringBuilder sbTemp = new StringBuilder();

                        foreach (UOPFileIndexDef bCurrent in aCurrent.m_ListIndex)
                        {
                            System.Threading.Thread.Sleep(1);

                            UOPFileData dCurrent = aCurrent.m_ListData[iCurrentIdef];

                            sbTemp.AppendLine("---- Index[" + iCurrentIdef.ToString() + "] def file comp/uncomp: " + bCurrent.m_LenghtCompressed.ToString() + "/" + bCurrent.m_LenghtUncompressed.ToString());
                            sbTemp.AppendLine("---- Index[" + iCurrentIdef.ToString() + "] Unk1: " + String.Format("0x{0:X}", bCurrent.m_Unknown1) + " Unk2: " + String.Format("0x{0:X}", bCurrent.m_Unknown2));
                            sbTemp.AppendLine("---- Index[" + iCurrentIdef.ToString() + "] OffsetOfThisBlock: " + String.Format("0x{0:X}", bCurrent.m_OffsetOfDataBlock) + " UnkData2: " + String.Format("0x{0:X}", dCurrent.m_Unknown));

                            iCurrentIdef++;
                        }

                        AppendTextArea(sbTemp.ToString());
                    }

                    iCurrentindexDef++;
                }

                if (!bDump)
                    return;

                AppendTextArea("\n\nStart dumping.\n");

                int iStartName = oFileDlgUopopen.FileName.LastIndexOf('\\') + 1;
                int iEndName = oFileDlgUopopen.FileName.LastIndexOf('.');

                string fileName = oFileDlgUopopen.FileName.Substring(iStartName, iEndName - iStartName);
                string newPath = Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf('\\'));
                newPath += @"\Unpacked";

                if (!Directory.Exists(newPath))
                    Directory.CreateDirectory(newPath);

                StringBuilder sbUnpackstring = new StringBuilder();

                for (int i = 0; i < uopToParse.m_Content.Count; i++)
                {
                    UOPIndexBlockHeader aCurrent = uopToParse.m_Content[i];
                    for (int j = 0; j < aCurrent.m_ListIndex.Count; j++)
                    {
                        System.Threading.Thread.Sleep(1);

                        string sFileName = fileName + "_" + i.ToString() + "_" + j.ToString();

                        UOPFileIndexDef uopFIDcurrent = aCurrent.m_ListIndex[j];
                        UOPFileData uopFDcurrent = aCurrent.m_ListData[j];

                        int iUncompressLength = ((int)(uopFIDcurrent.m_LenghtUncompressed));
                        byte[] bUnCompressData = new byte[iUncompressLength];
                        ZLibError zResult = Compressor.Decompress(bUnCompressData, ref iUncompressLength, uopFDcurrent.m_CompressedData, ((int)(uopFIDcurrent.m_LenghtCompressed)));
                        if (zResult == ZLibError.Okay)
                        {
                            using (FileStream fsWrite = File.Create(newPath + @"\" + sFileName + ".dat"))
                            {
                                using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                {
                                    bwWrite.Write(bUnCompressData);
                                }
                            }

                            sbUnpackstring.AppendLine("Dumping " + sFileName + ".dat ... OK!\n");
                        }
                        else
                        {
                            using (FileStream fsWrite = File.Create(newPath + @"\" + sFileName + ".raw"))
                            {
                                using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                {
                                    bwWrite.Write(uopFDcurrent.m_CompressedData);
                                }
                            }

                            sbUnpackstring.AppendLine("Dumping " + sFileName + ".raw ... ERROR! (" + zResult.ToString() + ")\n");
                        }
                    }
                }

                AppendTextArea(sbUnpackstring.ToString());
            }

            GC.Collect();
            AppendTextArea("Done dumping.\n");
        }

        #region IForm1State Membri di

        delegate void SetTextCallback(string text);
        delegate void ResetTextCallback();

        public void AppendTextArea(string sText)
        {
            if (this.InvokeRequired)
            {
                SetTextCallback stDelegeate = new SetTextCallback(AppendTextArea);
                this.BeginInvoke(stDelegeate, new object[] { sText });
            }
            else
            {
                textBox1.AppendText(sText);
            }
        }

        public void SetTextArea(string sText)
        {
            if (this.InvokeRequired)
            {
                SetTextCallback stDelegeate = new SetTextCallback(SetTextArea);
                this.BeginInvoke(stDelegeate, new object[] { sText });
            }
            else
            {
                textBox1.Text = sText;
            }
        }

        public void ResetTextArea()
        {
            if (this.InvokeRequired)
            {
                ResetTextCallback stDelegeate = new ResetTextCallback(ResetTextArea);
                this.BeginInvoke(stDelegeate);
            }
            else
            {
                textBox1.Clear();
            }
        }

        #endregion
    }
}