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

            TreeNodeCollection tncCurrent = this.tvFileData.Nodes;
            TreeNode tnRoot = new TreeNode(GetFileName(UopManager.getIstance().UopPath));
            tncCurrent.Add(tnRoot);

            int iNode = 0;
            foreach (UOPIndexBlockHeader uibhCurrent in uopToParse.m_Content)
            {
                TreeNode tnCurrent = new TreeNode(String.Format("Header Block {0}", iNode++));
                tnCurrent.Tag = uibhCurrent;
                tnRoot.Nodes.Add(tnCurrent);
            }

            this.tvFileData.ResumeLayout(true);
            SetDisableIcon(false);
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

        private string GetFileName(string sInput)
        {
            int iStartName = sInput.LastIndexOf('\\') + 1;

            if (iStartName != -1)
            {
                return sInput.Substring(iStartName, sInput.Length - iStartName);
            }
            else
            {
                return sInput;
            }
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

        private void toolBtnOpen_Click(object sender, EventArgs e)
        {
            if (LoadUOP())
            {
                Parse();
            }
        }

        private void toolBtnSave_Click(object sender, EventArgs e)
        {

        }

        private void toolBtnRefresh_Click(object sender, EventArgs e)
        {

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
    }
}