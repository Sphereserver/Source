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
            this.FormClosing += new FormClosingEventHandler(Form1_FormClosing);
        }

        private void SetDisableIcon(bool bEnabled)
        {
            this.toolBtnRefresh.Enabled = !bEnabled;
            
            this.toolBtnSave.Enabled = !bEnabled;
        }

        private bool LoadUOP()
        {
            ResetTextArea();
            SetDisableIcon(true);

            DialogResult drFile = oFileDlgUopopen.ShowDialog(this);
            if (drFile != DialogResult.OK)
            {
                MessageBox.Show("File not selected!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }

            AppendTextArea("Parsing file " + oFileDlgUopopen.FileName + " ...\n");

            UopManager upIstance = UopManager.getIstance(true);
            upIstance.UopPath = oFileDlgUopopen.FileName;
            if (!upIstance.Load())
            {
                AppendTextArea("ERROR while parsing file " + oFileDlgUopopen.FileName + " !!!\n");
                GC.Collect();
                return false;
            }

            AppendTextArea("Done parsing.\n\n");
            return true;
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

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Se abbiamo robba aperta chiedere conferma
        }

        private void toolBtnOpen_Click(object sender, EventArgs e)
        {
            if (LoadUOP())
            {
                Parse();
                SetDisableIcon(false);
            }
        }

        private void toolBtnSave_Click(object sender, EventArgs e)
        {

        }

        private void toolBtnDump_Click(object sender, EventArgs e)
        {

        }

        private void toolBtnClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void toolBtnRefresh_Click(object sender, EventArgs e)
        {
            RefreshData();
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
    }
}