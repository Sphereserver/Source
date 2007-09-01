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
    public partial class Form1 : Form, StupidInterface
    {
        public Form1()
        {
            InitializeComponent();
            DoingSomeJob.TheForm = this;
            this.FormClosing += delegate(object sender, FormClosingEventArgs e)
            {
                if (DoingSomeJob.Working)
                {
                    DialogResult drData = MessageBox.Show("Are you sure to close? Still working in background!", "UO:KR Uop Dumper",
                                                          MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

                    if (drData == DialogResult.No)
                    {
                        e.Cancel = true;
                    }
                }
            };

            this.toolBtnDontFix.Click += delegate(object sender, EventArgs e)
            {
                this.toolBtnDontFix.Checked = !this.toolBtnDontFix.Checked;
                //this.toolBtnDontFix.BackColor = (this.toolBtnDontFix.Checked) ? System.Drawing.Color.Gainsboro : System.Drawing.SystemColors.Control;
            };
            this.btn_pnlUopFile_RecountFiles.LinkClicked += delegate(object sender, LinkLabelLinkClickedEventArgs e)
            {
                UOPFile upCurrent = (UOPFile)this.gbSelectedData.Tag;
                this.num_pnlUopFile_Files.Value = upCurrent.FilesDynamicCount;
                this.btn_pnlUopFile_RecountFiles.LinkVisited = false;
            };
            this.btn_pnUopDatafile_RecountFiles.LinkClicked += delegate(object sender, LinkLabelLinkClickedEventArgs e)
            {
                UOPIndexBlockHeader upCurrent = (UOPIndexBlockHeader)this.gbSelectedData.Tag;
                this.nud_pnUopDatafile_Files.Value = upCurrent.FilesDynamicCount;
                this.btn_pnUopDatafile_RecountFiles.LinkVisited = false;
            };
            this.btn_pnUopHeaderAndData_PatchData.LinkClicked += delegate(object sender, LinkLabelLinkClickedEventArgs e)
            {
                UOPPairData upDataPair = (UOPPairData)this.gbSelectedData.Tag;
                this.txt_pnUopHeaderAndData_Data.Tag = AddData(upDataPair, false);
                this.btn_pnUopHeaderAndData_PatchData.LinkVisited = false;
            };
            this.btn_pnUopHeaderAndData_PatchDataUnc.LinkClicked += delegate(object sender, LinkLabelLinkClickedEventArgs e)
            {
                UOPPairData upDataPair = (UOPPairData)this.gbSelectedData.Tag;
                this.txt_pnUopHeaderAndData_Data.Tag = AddData(upDataPair, true);
                this.btn_pnUopHeaderAndData_PatchData.LinkVisited = false;
            };

            this.btn_pnUopHeaderAndData_PatchData.Tag = null;
            this.btn_pnUopHeaderAndData_PatchDataUnc.Tag = null;
            this.num_pnlUopFile_Files.Minimum = 0;
            this.num_pnlUopFile_Files.Maximum = Int32.MaxValue;
        }

        private void SetDisableIcon(bool bEnabled)
        {
            this.toolBtnRefresh.Enabled = !bEnabled;
            this.toolBtnSave.Enabled = !bEnabled;
            this.toolBtnSaveAs.Enabled = !bEnabled;
            this.toolBtnDontFix.Enabled = !bEnabled;
            this.toolBtnDump.Enabled = !bEnabled;
            this.toolBtnUnpack.Enabled = !bEnabled;
        }

        private void SetModifyButtons(bool bEnable)
        {
            this.btnDetailsModify.Enabled = bEnable;
            this.btnDetailsApply.Enabled = !bEnable;
            this.btnDetailsDelete.Enabled = !bEnable;
            this.btnDetailsUndo.Enabled = !bEnable;
        }

        private System.Collections.Hashtable AddData(UOPPairData pData, bool unCompressed)
        {
            this.txt_pnUopHeaderAndData_Data.Tag = null;

            if (this.oPatchDlgUopopen.ShowDialog(this) == DialogResult.OK)
            {
                System.Collections.Hashtable htTosend = new System.Collections.Hashtable();
                htTosend.Add("file", this.oPatchDlgUopopen.FileName);
                htTosend.Add("uncompressed", unCompressed);

                return htTosend;
            }

            return null;
        }

        private void LoadUOP()
        {
            DialogResult drFile = oFileDlgUopopen.ShowDialog(this);
            if (drFile != DialogResult.OK)
            {
                MessageBox.Show("File not selected!", "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            LoadUOP(oFileDlgUopopen.FileName);
        }

        private void LoadUOP(string theNewFile)
        {
            ThreadArgs.MyThreadDelegate mtdParse = delegate(object args)
            {
                ThreadArgs theArgs = (ThreadArgs)args;
                StupidInterface theForm = (StupidInterface)theArgs.Args[0];
                string theFile = (string)theArgs.Args[1];

                UopManager upIstance = UopManager.getIstance(true);
                upIstance.UopPath = theFile;

                bool bResult = upIstance.Load();

                if (bResult)
                    theForm.SetTextArea("Done parsing.");
                else
                    theForm.SetTextArea("ERROR while parsing file \"" + theFile + "\" !!!");

                if (bResult)
                {
                    theForm.SetNodes();
                    theForm.DisableOtherIcon(false);
                }

                theForm.SetLoadIcon(true);
                DoingSomeJob.Working = false;
            };

            DoingSomeJob.Working = true;
            this.SetTextArea("Parsing file \"" + theNewFile + "\" ...");

            this.SetDisableIcon(true);
            this.SetLoadIcon(false);
            
            System.Threading.Thread tRun = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(mtdParse));
            tRun.Start(new ThreadArgs(new object[] { this, theNewFile }));
        }

        private void Parse()
        {
            UOPFile uopToParse = UopManager.getIstance().UopFile;
            this.tvFileData.SuspendLayout();
            this.tvFileData.Nodes.Clear();

            TreeNodeCollection tncCurrent = this.tvFileData.Nodes;
            TreeNode tnRoot = new TreeNode(Utility.GetFileName(UopManager.getIstance().UopPath));
            tnRoot.Tag = -1; tncCurrent.Add(tnRoot);

            for (int iNode = 0; iNode < uopToParse.m_Content.Count; iNode++)
            {
                TreeNode tnCurrent = new TreeNode(String.Format("Header Block {0}", iNode));
                tnCurrent.ContextMenuStrip = this.ctxMenuNode;
                tnCurrent.Tag = iNode;
                tnRoot.Nodes.Add(tnCurrent);
            }

            tnRoot.Expand();
            this.tvFileData.ResumeLayout(true);
        }

        private void RefreshData()
        {
            SetDisableIcon(true);

            // sistema la groupbox
            this.pnUopfile.Visible = this.pnUopfile.Enabled = false;
            this.pnUopDatafile.Visible = this.pnUopDatafile.Enabled = false;

            tvFileData_AfterSelect(null, new TreeViewEventArgs(null));

            Parse();

            SetDisableIcon(false);
        }

        private void CommonSave(string sFile, bool bFix)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                ThreadArgs.MyThreadDelegate mtdParse = delegate(object args)
                {
                    ThreadArgs theArgs = (ThreadArgs)args;
                    StupidInterface theForm = (StupidInterface)theArgs.Args[0];
                    string theFile = (string)theArgs.Args[1];
                    bool theFix = (bool)theArgs.Args[2];

                    UopManager upIstance = UopManager.getIstance();

                    if (theFix)
                        upIstance.FixOffsets(0, 0);

                    bool bResult = upIstance.Write(theFile);

                    if (bResult)
                    {
                        theForm.SetTextArea("Done saving UOP file.");

                        if (!upIstance.UopPath.Equals(theFile, StringComparison.OrdinalIgnoreCase))
                            upIstance.UopPath = theFile;
                    }
                    else
                        theForm.SetTextArea("ERROR while saving file \"" + theFile + "\" !!!");

                    theForm.DisableOtherIcon(false);
                    theForm.SetLoadIcon(true);
                    DoingSomeJob.Working = false;
                };

                DoingSomeJob.Working = true;
                SetTextArea("Saving UOP file to \"" + sFile + "\" ...");
                this.SetDisableIcon(true);
                this.SetLoadIcon(false);

                System.Threading.Thread tRun = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(mtdParse));
                tRun.Start(new ThreadArgs(new object[] { this, sFile, bFix }));
            }
        }

        private void CommonDump(ShowPanels spType, object oCast)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                ThreadArgs.MyThreadDelegate mtdParse = delegate(object args)
                {
                    ThreadArgs theArgs = (ThreadArgs)args;
                    StupidInterface theForm = (StupidInterface)theArgs.Args[0];
                    string thePath = (string)theArgs.Args[1];
                    ShowPanels theType = (ShowPanels)theArgs.Args[2];
                    object theObject = theArgs.Args[3];

                    UopManager upIstance = UopManager.getIstance();
                    string baseName = Path.GetFileNameWithoutExtension(upIstance.UopPath);
                    string resultName = "";

                    try
                    {
                        if (!Directory.Exists(thePath))
                            Directory.CreateDirectory(thePath);

                        int i = 0, j = 0;

                        switch (spType)
                        {
                            case ShowPanels.RootNode:
                            {
                                UOPFile toDump = (UOPFile)theObject;
                                foreach (UOPIndexBlockHeader dumpTemp1 in toDump.m_Content)
                                {
                                    foreach (UOPPairData dumpTemp2 in dumpTemp1.m_ListData)
                                    {
                                        using (FileStream fsWrite = File.Create(thePath + @"\" + String.Format(StaticData.UNPACK_NAMEPATTERN, baseName, i, j, dumpTemp2.First.IsCompressed ? StaticData.UNPACK_EXT_COMP : StaticData.UNPACK_EXT_UCOMP)))
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

                                resultName = "the UOP file.";
                            } break;

                            case ShowPanels.DataNode:
                            {
                                UOPIndexBlockHeader toDump = (UOPIndexBlockHeader)theObject;
                                foreach (UOPIndexBlockHeader dumpTemp1 in upIstance.UopFile.m_Content)
                                {
                                    if (dumpTemp1.Equals(toDump))
                                        break;

                                    i++;
                                }

                                foreach (UOPPairData dumpTemp2 in toDump.m_ListData)
                                {
                                    using (FileStream fsWrite = File.Create(thePath + @"\" + String.Format(StaticData.UNPACK_NAMEPATTERN, baseName, i, j, dumpTemp2.First.IsCompressed ? StaticData.UNPACK_EXT_COMP : StaticData.UNPACK_EXT_UCOMP)))
                                    {
                                        using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                        {
                                            bwWrite.Write(dumpTemp2.Second.Extract(dumpTemp2.First.IsCompressed, dumpTemp2.First.m_LenghtUncompressed));
                                        }
                                    }

                                    j++;
                                }

                                resultName = "the Header Block " + i.ToString() + ".";
                            } break;

                            case ShowPanels.SingleHeader:
                            {
                                UOPPairData toDump = (UOPPairData)theObject;
                                foreach (UOPIndexBlockHeader dumpTemp1 in upIstance.UopFile.m_Content)
                                {
                                    foreach (UOPPairData dumpTemp2 in dumpTemp1.m_ListData)
                                    {
                                        if (dumpTemp2.Equals(toDump))
                                            break;

                                        j++;
                                    }

                                    i++;
                                    j = 0;
                                }

                                using (FileStream fsWrite = File.Create(thePath + @"\" + String.Format(StaticData.UNPACK_NAMEPATTERN, baseName, i, j, toDump.First.IsCompressed ? StaticData.UNPACK_EXT_COMP : StaticData.UNPACK_EXT_UCOMP)))
                                {
                                    using (BinaryWriter bwWrite = new BinaryWriter(fsWrite))
                                    {
                                        bwWrite.Write(toDump.Second.Extract(toDump.First.IsCompressed, toDump.First.m_LenghtUncompressed));
                                    }
                                }

                                resultName = "the Index " + j.ToString() + ".";
                            } break;
                        }

                        GC.Collect();
                        theForm.SetTextArea("Completed unpacking for " + resultName);
                    }
                    catch
                    {
                        theForm.SetTextArea("ERROR while unpacking selected data.");
                    }
                    finally
                    {
                        theForm.DisableOtherIcon(false);
                        theForm.SetLoadIcon(true);
                        DoingSomeJob.Working = false;
                    }
                };

                DoingSomeJob.Working = true;
                SetTextArea("Unpacking seleceted data to \"" + Application.StartupPath + StaticData.UNPACK_DIR + "\" ...");
                this.SetDisableIcon(true);
                this.SetLoadIcon(false);

                System.Threading.Thread tRun = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(mtdParse));
                tRun.Start(new ThreadArgs(new object[] { this, Application.StartupPath + StaticData.UNPACK_DIR, spType, oCast }));
            }
        }

        #region Button Details

        private void btnDetailsUnpack_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else if (this.pnUopfile.Visible)
            {
                toolBtnUnpack_Click(sender, e);
            }
            else if (this.pnUopDatafile.Visible)
            {
                CommonDump(ShowPanels.DataNode, this.gbSelectedData.Tag);
            }
            else if (this.pnUopHeaderAndData.Visible)
            {
                CommonDump(ShowPanels.SingleHeader, this.gbSelectedData.Tag);
            }
            else
            {
                SetPanel(ShowPanels.Nothing, null);
            }
        }

        private void btnDetailsModify_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // for each panel
            this.pnUopfile.Enabled = this.pnUopfile.Visible;
            this.pnUopDatafile.Enabled = this.pnUopDatafile.Visible;
            this.pnUopHeaderAndData.Enabled = this.pnUopHeaderAndData.Visible;
            // ---------------------
            if (this.pnUopfile.Enabled || this.pnUopDatafile.Enabled || this.pnUopHeaderAndData.Enabled)
                SetModifyButtons(false);
        }

        private void btnDetailsUndo_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            DoingSomeJob.Working = true;

            if (this.pnUopfile.Enabled)
            {
                SetPanel(ShowPanels.RootNode, this.gbSelectedData.Tag);
            }
            else if (this.pnUopDatafile.Enabled)
            {
                SetPanel(ShowPanels.DataNode, this.gbSelectedData.Tag);
            }
            else if (this.pnUopHeaderAndData.Enabled)
            {
                SetPanel(ShowPanels.SingleHeader, this.gbSelectedData.Tag);
            }
            else
            {
                SetPanel(ShowPanels.Nothing, null);
            }

            DoingSomeJob.Working = false;
        }

        private void btnDetailsApply_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            DoingSomeJob.Working = true;

            if (this.pnUopfile.Enabled)
            {
                try
                {
                    UOPFile uppeCurrent = (UOPFile)this.gbSelectedData.Tag;

                    byte[] bHeader1 = Utility.StringToByteArray(this.txt_pnlUopFile_Header1.Text, Utility.HashStringStyle.HexWithSeparator, 24);
                    byte[] bHeader2 = Utility.StringToByteArray(this.txt_pnlUopFile_Header2.Text, Utility.HashStringStyle.HexWithSeparator, 12);
                    uint uCount = Decimal.ToUInt32(this.num_pnlUopFile_Files.Value);

                    if ((bHeader1 == null) || (bHeader1 == null))
                    {
                        throw new Exception();
                    }
                    else
                    {
                        uppeCurrent.m_Header.m_variousData = bHeader1;
                        uppeCurrent.m_Header.m_totalIndex = uCount;
                        uppeCurrent.m_Header.m_Unknown = bHeader2;
                    }
                }
                catch
                {
                    MessageBox.Show("Error while parsing the data!", "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                SetPanel(ShowPanels.RootNode, this.gbSelectedData.Tag);
            }
            else if (this.pnUopDatafile.Enabled)
            {
                try
                {
                    UOPIndexBlockHeader uppeCurrent = (UOPIndexBlockHeader)this.gbSelectedData.Tag;

                    ulong ulOffset = UInt64.Parse(this.txt_pnUopDatafile_Offset.Text, System.Globalization.NumberStyles.AllowHexSpecifier);
                    uint uCount = Decimal.ToUInt32(this.nud_pnUopDatafile_Files.Value);

                    uppeCurrent.m_Files = uCount;
                    uppeCurrent.m_OffsetNextIndex = ulOffset;
                }
                catch
                {
                    MessageBox.Show("Error while parsing the data!", "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                SetPanel(ShowPanels.DataNode, this.gbSelectedData.Tag);
            }
            else if (this.pnUopHeaderAndData.Enabled)
            {
                try
                {
                    UOPPairData uppeCurrent = (UOPPairData)this.gbSelectedData.Tag;

                    // Header block
                    ulong ulOffset = UInt64.Parse(this.txt_pnUopHeaderAndData_Offset.Text, System.Globalization.NumberStyles.AllowHexSpecifier);
                    uint uiUnk1 = UInt32.Parse(this.txt_pnUopHeaderAndData_Unk1.Text, System.Globalization.NumberStyles.AllowHexSpecifier);
                    uint uiUnk2 = UInt32.Parse(this.txt_pnUopHeaderAndData_Unk2.Text, System.Globalization.NumberStyles.AllowHexSpecifier);
                    uint uiUnk3 = UInt32.Parse(this.txt_pnUopHeaderAndData_Unk3.Text, System.Globalization.NumberStyles.AllowHexSpecifier);
                    string sNewFile = null; bool bUncompressed = false;
                    if (this.txt_pnUopHeaderAndData_Data.Tag != null)
                    {
                        sNewFile = (string)(((System.Collections.Hashtable)this.txt_pnUopHeaderAndData_Data.Tag)["file"]);
                        bUncompressed = (bool)(((System.Collections.Hashtable)this.txt_pnUopHeaderAndData_Data.Tag)["uncompressed"]);
                    }
                    ulong ulUnkData = UInt64.Parse(this.txt_pnUopHeaderAndData_DataUnk1.Text, System.Globalization.NumberStyles.AllowHexSpecifier);

                    if (sNewFile != null)
                    {
                        if (UopManager.getIstance().Replace(sNewFile, uppeCurrent, bUncompressed) != UopManager.UopPatchError.Okay)
                        {
                            throw new Exception();
                        }
                    }

                    uppeCurrent.First.m_OffsetOfDataBlock = ulOffset;
                    uppeCurrent.First.m_Unknown1 = uiUnk1;
                    uppeCurrent.First.m_Unknown2 = uiUnk2;
                    uppeCurrent.First.m_Unknown3 = uiUnk2;
                }
                catch
                {
                    MessageBox.Show("Error while parsing the data!", "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                SetPanel(ShowPanels.SingleHeader, this.gbSelectedData.Tag);
            }
            else
            {
                SetPanel(ShowPanels.Nothing, null);
            }

            DoingSomeJob.Working = false;
        }

        private void btnDetailsDelete_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            DoingSomeJob.Working = true;

            if (this.pnUopfile.Enabled)
            {
                SetPanel(ShowPanels.RootNode, this.gbSelectedData.Tag);

                MessageBox.Show("You cannot delete the UOP file that way!", "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else if (this.pnUopDatafile.Enabled)
            {
                SetTextArea("Deleting selected header block ...");
                SetPanel(ShowPanels.DataNode, this.gbSelectedData.Tag);

                UopManager.getIstance().Delete((UOPIndexBlockHeader)(this.gbSelectedData.Tag));

                RefreshData();
                SetTextArea("Deleted selected header block.");
            }
            else if (this.pnUopHeaderAndData.Enabled)
            {
                SetTextArea("Deleting selected index ...");
                SetPanel(ShowPanels.SingleHeader, this.gbSelectedData.Tag);

                UopManager.getIstance().Delete((UOPPairData)(this.gbSelectedData.Tag));

                RefreshData();
                SetTextArea("Deleted selected index.");
            }
            else
            {
                SetPanel(ShowPanels.Nothing, null);
            }

            SetTextArea("");
            DoingSomeJob.Working = false;
        }

        #endregion

        #region Toolbar Buttons
        private void toolBtnOpen_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                LoadUOP();
            }
        }

        private void toolBtnSave_Click(object sender, EventArgs e)
        {
            CommonSave(UopManager.getIstance().UopPath, !this.toolBtnDontFix.Checked);
        }

        private void toolBtnSaveAs_Click(object sender, EventArgs e)
        {
            this.oFileDlgUopsave.FileName = UopManager.getIstance().UopPath;

            if (this.oFileDlgUopsave.ShowDialog(this) == DialogResult.OK)
                CommonSave(this.oFileDlgUopsave.FileName, !this.toolBtnDontFix.Checked);
        }

        private void toolBtnDump_Click(object sender, EventArgs e)
        {
            if (DoingSomeJob.Working)
            {
                MessageBox.Show("Please wait... Still working in background!", "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                ThreadArgs.MyThreadDelegate mtdParse = delegate(object args)
                {
                    ThreadArgs theArgs = (ThreadArgs)args;
                    StupidInterface theForm = (StupidInterface)theArgs.Args[0];
                    string thePath = (string)theArgs.Args[1];
                    string theFile = (string)theArgs.Args[2];

                    UopManager upIstance = UopManager.getIstance();

                    try
                    {
                        string pathandfile = thePath + @"\" + theFile;

                        if (!Directory.Exists(thePath))
                            Directory.CreateDirectory(thePath);

                        using (StreamWriter swFile = new StreamWriter(pathandfile, false))
                        {
                            swFile.Write(UopManager.getIstance().UopFile.ToString());
                        }

                        theForm.SetTextArea("Completed information dump.");
                    }
                    catch
                    {
                        theForm.SetTextArea("ERROR while writing information dump.");
                    }
                    finally
                    {
                        theForm.DisableOtherIcon(false);
                        theForm.SetLoadIcon(true);
                        DoingSomeJob.Working = false;
                    }
                };

                DoingSomeJob.Working = true;

                string sPath = Application.StartupPath + StaticData.DUMPINFO_DIR;
                string sFile = Utility.GetFileName(UopManager.getIstance().UopPath) + ".txt";

                SetTextArea("Dumping UOP file to \"" + sPath + "\\" + sFile + "\" ...");
                this.SetDisableIcon(true);
                this.SetLoadIcon(false);

                System.Threading.Thread tRun = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(mtdParse));
                tRun.Start(new ThreadArgs(new object[] { this, sPath, sFile }));
            }
        }

        private void toolBtnUnpack_Click(object sender, EventArgs e)
        {
            CommonDump(ShowPanels.RootNode, UopManager.getIstance().UopFile);
        }

        private void toolBtnClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void toolBtnRefresh_Click(object sender, EventArgs e)
        {
            DoingSomeJob.Working = true;
            RefreshData();
            DoingSomeJob.Working = false;
        }

        private void toolBtnInfo_Click(object sender, EventArgs e)
        {
            Form2 frmAbout = new Form2();
            DialogResult drResult = frmAbout.ShowDialog(this);
            if (drResult == DialogResult.OK)
            {
                frmAbout.Close();
            }
            else
            {
                Close();
            }
        }

        private void toolBtnHelp_Click(object sender, EventArgs e)
        {

        }
        #endregion

        #region Selected Data

        private enum ShowPanels : int
        {
            RootNode = 0,
            DataNode,
            SingleHeader,
            MultiHeader,
            // --------
            Nothing
        }

        private void tvFileData_AfterSelect(object sender, TreeViewEventArgs e)
        {
            this.lbIndexList.SuspendLayout();
            this.lbIndexList.SelectedIndexChanged -= lbIndexList_SelectedIndexChanged;

            this.lbIndexList.Items.Clear();
            if ((e.Node != null) && (e.Node.Parent != null))
            {
                UOPFile uopToParse = UopManager.getIstance().UopFile;
                for (int iLabel = 0; iLabel < uopToParse.m_Content[(int)e.Node.Tag].m_ListData.Count; iLabel++)
                {
                    this.lbIndexList.Items.Add(String.Format("Index {0}", iLabel));
                }

                SetPanel(ShowPanels.DataNode, UopManager.getIstance().UopFile.m_Content[(int)e.Node.Tag]);
            }
            else if ((e.Node != null) && (e.Node.Parent == null))
            {
                SetPanel(ShowPanels.RootNode, UopManager.getIstance().UopFile);
            }
            this.lbIndexList.SelectedItem = null;

            this.lbIndexList.SelectedIndexChanged += new EventHandler(lbIndexList_SelectedIndexChanged);
            this.lbIndexList.ResumeLayout(true);
        }

        private void lbIndexList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((this.tvFileData.SelectedNode == null) || (this.tvFileData.SelectedNode.Parent == null))
                return;

            ListBox lbSender = (ListBox)sender;

            switch (lbSender.SelectedIndices.Count)
            {
                case 1:
                {
                    SetPanel(ShowPanels.SingleHeader, 
                             UopManager.getIstance().UopFile.m_Content[(int)(this.tvFileData.SelectedNode.Tag)].m_ListData[lbSender.SelectedIndex]
                             );
                } break;

                case 0:
                default:
                {
                    SetPanel(ShowPanels.Nothing, null);
                } break;
            }
        }

        private void SetPanel(ShowPanels panelType, object dataObject)
        {
            this.gbSelectedData.SuspendLayout();

            this.SetModifyButtons(true);
            this.btnDetailsUnpack.Enabled = true;
            // -----------------
            this.pnUopfile.Visible = this.pnUopfile.Enabled = false;
            this.pnUopDatafile.Visible = this.pnUopDatafile.Enabled = false;
            this.pnUopHeaderAndData.Visible = this.pnUopHeaderAndData.Enabled = false;
            // -----------------
            this.gbSelectedData.Tag = dataObject;

            switch (panelType)
            {
                case ShowPanels.RootNode:
                {
                    UOPFile uppeCurrent = (UOPFile) this.gbSelectedData.Tag;
                    this.txt_pnlUopFile_Header1.Text = Utility.ByteArrayToString(uppeCurrent.m_Header.m_variousData, Utility.HashStringStyle.HexWithSeparator);
                    this.num_pnlUopFile_Files.Value = uppeCurrent.m_Header.m_totalIndex;
                    this.txt_pnlUopFile_Header2.Text = Utility.ByteArrayToString(uppeCurrent.m_Header.m_Unknown, Utility.HashStringStyle.HexWithSeparator);

                    this.pnUopfile.Visible = true;
                } break;

                case ShowPanels.DataNode:
                {
                    UOPIndexBlockHeader upCurrent = (UOPIndexBlockHeader)this.gbSelectedData.Tag;
                    this.txt_pnUopDatafile_Offset.Text = String.Format("{0:X}", upCurrent.m_OffsetNextIndex);
                    this.nud_pnUopDatafile_Files.Value = upCurrent.FilesDynamicCount;

                    this.pnUopDatafile.Visible = true;
                } break;

                case ShowPanels.SingleHeader:
                {
                    UOPPairData upCurrent = (UOPPairData)this.gbSelectedData.Tag;
                    // Header block
                    this.txt_pnUopHeaderAndData_Offset.Text = String.Format("{0:X}", upCurrent.First.m_OffsetOfDataBlock);
                    this.txt_pnUopHeaderAndData_SizeHeader.Text = upCurrent.First.m_SizeofDataHeaders.ToString();
                    this.txt_pnUopHeaderAndData_Unk1.Text = String.Format("{0:X}", upCurrent.First.m_Unknown1);
                    this.txt_pnUopHeaderAndData_Unk2.Text = String.Format("{0:X}", upCurrent.First.m_Unknown2);
                    this.txt_pnUopHeaderAndData_Unk3.Text = String.Format("{0:X}", upCurrent.First.m_Unknown3);
                    this.chk_pnUopHeaderAndData_Compressed.Checked = upCurrent.First.IsCompressed;
                    // Data block
                    byte[] bData = new byte[10]; Array.Copy(upCurrent.Second.m_CompressedData, bData, Math.Min(10, upCurrent.Second.m_CompressedData.Length));
                    this.txt_pnUopHeaderAndData_Data.Text = Utility.ByteArrayToString(bData, Utility.HashStringStyle.HexWithSeparator) + " ...";
                    this.txt_pnUopHeaderAndData_DataFlags.Text = String.Format("{0:X}", upCurrent.Second.m_DataFlag);
                    this.txt_pnUopHeaderAndData_DataLocalOffset.Text = String.Format("{0:X}", upCurrent.Second.m_LocalOffsetToData);
                    this.txt_pnUopHeaderAndData_DataUnk1.Text = String.Format("{0:X}", upCurrent.Second.m_Unknown);
                    this.lbl_pnUopHeaderAndData_SizeC.Text = String.Format("Compressed: {0}", Utility.StringFileSize(upCurrent.First.m_LenghtCompressed));
                    this.lbl_pnUopHeaderAndData_SizeU.Text = String.Format("Uncompressed: {0}", Utility.StringFileSize(upCurrent.First.m_LenghtUncompressed));

                    this.pnUopHeaderAndData.Visible = true;
                } break;

                case ShowPanels.Nothing:
                {

                } break;
            }

            this.gbSelectedData.ResumeLayout(true);
        }

        #endregion

        #region ToolStripMenu TreeView

        private enum ToolStripMenuButtons : int
        {
            Delete = 0,
            MoveUp,
            MoveDown
        }

        private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if ( sender != null )
            {
                TreeNode tnTemp = this.tvFileData.GetNodeAt(this.tvFileData.PointToClient(((ToolStripMenuItem)sender).Owner.Location));
                if (tnTemp != null)
                {
                    genericToolStripMenuItem(ToolStripMenuButtons.Delete, (int)tnTemp.Tag);
                }
            }
        }

        private void moveUpToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (sender != null)
            {
                TreeNode tnTemp = this.tvFileData.GetNodeAt(this.tvFileData.PointToClient(((ToolStripMenuItem)sender).Owner.Location));
                if (tnTemp != null)
                {
                    genericToolStripMenuItem(ToolStripMenuButtons.MoveUp, (int)tnTemp.Tag);
                }
            }
        }

        private void moveDownToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (sender != null)
            {
                TreeNode tnTemp = this.tvFileData.GetNodeAt(this.tvFileData.PointToClient(((ToolStripMenuItem)sender).Owner.Location));
                if (tnTemp != null)
                {
                    genericToolStripMenuItem(ToolStripMenuButtons.MoveDown, (int)tnTemp.Tag);
                }
            }
        }

        private void genericToolStripMenuItem(ToolStripMenuButtons tsmbClick, int iNode)
        {
            if (iNode == -1)
                return;

            UOPFile uopToParse = UopManager.getIstance().UopFile;

            switch (tsmbClick)
            {
                case ToolStripMenuButtons.Delete:
                {
                    uopToParse.m_Content.RemoveAt(iNode);
                    RefreshData();
                    SetTextArea("Deleted selected header block.");
                } break;

                case ToolStripMenuButtons.MoveUp:
                {
                    if (iNode != 0)
                    {
                        UOPIndexBlockHeader uBack = uopToParse.m_Content[iNode - 1];
                        uopToParse.m_Content[iNode - 1] = uopToParse.m_Content[iNode];
                        uopToParse.m_Content[iNode] = uBack;
                        RefreshData();
                        SetTextArea("Moved Up selected header block.");
                    }
                } break;

                case ToolStripMenuButtons.MoveDown:
                {
                    if (iNode != (uopToParse.m_Content.Count - 1))
                    {
                        UOPIndexBlockHeader uFront = uopToParse.m_Content[iNode + 1];
                        uopToParse.m_Content[iNode + 1] = uopToParse.m_Content[iNode];
                        uopToParse.m_Content[iNode] = uFront;
                        RefreshData();
                        SetTextArea("Moved Down selected header block.");
                    }
                } break;
            }
        }

        #endregion

        #region StupidInterface Membri di

        delegate void SetTextCallback(string text);
        public void SetTextArea(string sText)
        {
            if (this.InvokeRequired)
            {
                SetTextCallback stDelegeate = new SetTextCallback(SetTextArea);
                this.BeginInvoke(stDelegeate, new object[] { sText });
            }
            else
            {
                this.tslblStatus.Text = sText;
            }
        }

        delegate void SetLoadIconCallback(bool bStatus);
        public void SetLoadIcon(bool bStatus)
        {
            if (this.InvokeRequired)
            {
                SetLoadIconCallback stDelegeate = new SetLoadIconCallback(SetLoadIcon);
                this.BeginInvoke(stDelegeate, new object[] { bStatus });
            }
            else
            {
                this.toolBtnOpen.Enabled = bStatus;
            }
        }

        public void DisableOtherIcon(bool bEnable)
        {
            if (this.InvokeRequired)
            {
                SetLoadIconCallback stDelegeate = new SetLoadIconCallback(DisableOtherIcon);
                this.BeginInvoke(stDelegeate, new object[] { bEnable });
            }
            else
            {
                this.SetDisableIcon(bEnable);
            }
        }

        public void SetWaitStatus(bool bEnable)
        {
            if (this.InvokeRequired)
            {
                SetLoadIconCallback stDelegeate = new SetLoadIconCallback(SetWaitStatus);
                this.BeginInvoke(stDelegeate, new object[] { bEnable });
            }
            else
            {
                if (bEnable)
                {
                    this.SetCursor(Cursors.WaitCursor);
                    this.tslblWorking.Visible = true;
                }
                else
                {
                    this.SetCursor(Cursors.Default);
                    this.tslblWorking.Visible = false;
                }
            }
        }

        delegate void SetNodesDelegate();
        public void SetNodes()
        {
            if (this.InvokeRequired)
            {
                SetNodesDelegate stDelegeate = new SetNodesDelegate(SetNodes);
                this.BeginInvoke(stDelegeate, null);
            }
            else
            {
                this.Parse();
            }
        }

        delegate void SetCursorDelegate(Cursor cCursore);
        public void SetCursor(Cursor cCursore)
        {
            if (this.InvokeRequired)
            {
                SetCursorDelegate stDelegeate = new SetCursorDelegate(SetCursor);
                this.BeginInvoke(stDelegeate, new object[] { cCursore });
            }
            else
            {
                this.Cursor = cCursore;
            }
        }

        #endregion
    }
}