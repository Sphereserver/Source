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
            this.FormClosing += new FormClosingEventHandler(Form1_FormClosing);
            this.btn_pnlUopFile_RecountFiles.Click += delegate(object sender, EventArgs e)
            {
                UOPFile upCurrent = (UOPFile)this.gbSelectedData.Tag;
                this.num_pnlUopFile_Files.Value = upCurrent.FilesDynamicCount;
            };
            this.num_pnlUopFile_Files.Minimum = 0;
            this.num_pnlUopFile_Files.Maximum = Int32.MaxValue;
        }

        private void SetDisableIcon(bool bEnabled)
        {
            this.toolBtnRefresh.Enabled = !bEnabled;
            this.toolBtnSave.Enabled = !bEnabled;
            this.toolBtnSaveAs.Enabled = !bEnabled;
            this.toolBtnDump.Enabled = !bEnabled;
            this.toolBtnUnpack.Enabled = !bEnabled;
        }

        private void SetModifyButtons(bool bEnable)
        {
            this.btnDetailsModify.Enabled = bEnable;
            this.btnDetailsApply.Enabled = !bEnable;
            this.btnDetailsDelete.Enabled = !bEnable;
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
                    theForm.SetTextArea("Done parsing.\n\n");
                else
                    theForm.SetTextArea("ERROR while parsing file " + theFile + " !!!\n");

                if (bResult)
                {
                    theForm.SetNodes();
                    theForm.DisableOtherIcon(false);
                }

                theForm.SetLoadIcon(true);
                DoingSomeJob.Working = false;
            };

            DoingSomeJob.Working = true;
            this.SetTextArea("Parsing file " + theNewFile + " ...\n");

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

            tvFileData_AfterSelect(null, new TreeViewEventArgs(null));

            Parse();

            SetDisableIcon(false);
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
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
        }

        #region Button Details

        private void btnDetailsUnpack_Click(object sender, EventArgs e)
        {

        }

        private void btnDetailsModify_Click(object sender, EventArgs e)
        {
            // for each panel
            this.pnUopfile.Enabled = this.pnUopfile.Visible;
            // ---------------------
            SetModifyButtons(false);

        }

        private void btnDetailsApply_Click(object sender, EventArgs e)
        {
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

                this.pnUopfile.Enabled = false;
            }
            // ...

            SetModifyButtons(true);
        }

        private void btnDetailsDelete_Click(object sender, EventArgs e)
        {
            if (this.pnUopfile.Enabled)
            {

            }
            // ...

            SetModifyButtons(true);
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

        }

        private void toolBtnSaveAs_Click(object sender, EventArgs e)
        {

        }

        private void toolBtnDump_Click(object sender, EventArgs e)
        {
            DoingSomeJob.Working = true;
            string sDumpFile = Application.StartupPath + @"\" + Utility.GetFileName(UopManager.getIstance().UopPath) + ".txt";

            try
            {
                using (StreamWriter swFile = new StreamWriter(sDumpFile, false))
                {
                    swFile.Write(UopManager.getIstance().UopFile.ToString());
                }

                DoingSomeJob.Working = false;

                MessageBox.Show("Completed information dump for " + Utility.GetFileName(UopManager.getIstance().UopPath) + " to:\n" +
                                sDumpFile, "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch
            {
                DoingSomeJob.Working = false;

                MessageBox.Show("Error while writing information dump for " + Utility.GetFileName(UopManager.getIstance().UopPath) + "", 
                                "UO:KR Uop Dumper - Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            
        }

        private void toolBtnUnpack_Click(object sender, EventArgs e)
        {
            DoingSomeJob.Working = true;

            string sPath = Application.StartupPath + @"\Unpacked";

            if (!Directory.Exists(sPath))
                Directory.CreateDirectory(sPath);

            bool bResult = UopManager.getIstance().UnPack(sPath);

            DoingSomeJob.Working = false;

            if (bResult)
            {
                MessageBox.Show("Completed unpacking for " + Utility.GetFileName(UopManager.getIstance().UopPath) + " into:\n" +
                                 sPath, "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                MessageBox.Show("Error while unpacking " + Utility.GetFileName(UopManager.getIstance().UopPath) + "",
                                "UO:KR Uop Dumper", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
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
            MultiHeader
        }

        private void tvFileData_AfterSelect(object sender, TreeViewEventArgs e)
        {
            this.lbIndexList.SuspendLayout();
            this.lbIndexList.SelectedIndexChanged -= lbIndexList_SelectedIndexChanged;

            this.lbIndexList.Items.Clear();
            if ((e.Node != null) && (e.Node.Parent != null))
            {
                UOPFile uopToParse = UopManager.getIstance().UopFile;
                for (int iLabel = 0; iLabel < uopToParse.m_Content[(int)e.Node.Tag].m_ListIndex.Count; iLabel++)
                {
                    this.lbIndexList.Items.Add(String.Format("Index {0}", iLabel));
                }
            }
            else if (e.Node.Parent == null)
            {
                SetPanel(ShowPanels.RootNode, UopManager.getIstance().UopFile);
            }
            this.lbIndexList.SelectedItem = null;

            this.lbIndexList.SelectedIndexChanged += new EventHandler(lbIndexList_SelectedIndexChanged);
            this.lbIndexList.ResumeLayout(true);
        }

        private void lbIndexList_SelectedIndexChanged(object sender, EventArgs e)
        {
            ListBox lbSender = (ListBox)sender;

            switch (lbSender.SelectedIndices.Count)
            {
                case 0:
                {

                } break;


                case 1:
                {


                } break;

                default:
                {


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

                default:
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
            Dump,
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

        private void dumpToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (sender != null)
            {
                TreeNode tnTemp = this.tvFileData.GetNodeAt(this.tvFileData.PointToClient(((ToolStripMenuItem)sender).Owner.Location));
                if (tnTemp != null)
                {
                    genericToolStripMenuItem(ToolStripMenuButtons.Dump, (int)tnTemp.Tag);
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
                } break;

                case ToolStripMenuButtons.Dump:
                {
                    
                } break;

                case ToolStripMenuButtons.MoveUp:
                {
                    if (iNode != 0)
                    {
                        UOPIndexBlockHeader uBack = uopToParse.m_Content[iNode - 1];
                        uopToParse.m_Content[iNode - 1] = uopToParse.m_Content[iNode];
                        uopToParse.m_Content[iNode] = uBack;
                        RefreshData();
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