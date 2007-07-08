using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using UOPDefine;
using System.IO;

namespace UoKRUnpacker
{
    public partial class PatchForm : Form
    {
        private Form1 m_Form1State;

        public PatchForm()
        {
            InitializeComponent();

            int iStartName = UopManager.getIstance().UopPath.LastIndexOf('\\') + 1;

            if (iStartName != -1)
            {
                this.Text += UopManager.getIstance().UopPath.Substring(iStartName, UopManager.getIstance().UopPath.Length - iStartName);
            }
            else
            {
                this.Text += UopManager.getIstance().UopPath;
            }

            numericUpDown1.Minimum = 0;
            numericUpDown1.Maximum = UopManager.getIstance().UopFile.m_Content.Count - 1;
            numericUpDown2.Minimum = 0;
            numericUpDown2.Maximum = UopManager.getIstance().UopFile.m_Content[Decimal.ToInt32(numericUpDown1.Value)].m_ListIndex.Count - 1;

            SetLabelText();
        }

        public PatchForm(Form1 frm1State) : this()
        {
            m_Form1State = frm1State;
        }

        private void SetLabelText()
        {
            lblId.Text = "(" + numericUpDown1.Minimum.ToString() + " - " + numericUpDown1.Maximum.ToString() + ")";
            lblFile.Text = "(" + numericUpDown2.Minimum.ToString() + " - " + numericUpDown2.Maximum.ToString() + ")";
        }

        private void numericUpDown1_ValueChanged(object sender, EventArgs e)
        {
            NumericUpDown nudIndex = (NumericUpDown)sender;

            numericUpDown2.Value = 0;
            numericUpDown2.Maximum = UopManager.getIstance().UopFile.m_Content[Decimal.ToInt32(nudIndex.Value)].m_ListIndex.Count - 1;

            SetLabelText();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (this.txtPathfile.Text.Length != 0)
            {
                Replace(this.txtPathfile.Text, Decimal.ToInt32(numericUpDown1.Value), Decimal.ToInt32(numericUpDown2.Value), this.chkUncompressed.Checked);
            }
            else
            {
                MessageBox.Show("File not selected!", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void btnSelect_Click(object sender, EventArgs e)
        {
            DialogResult drFile = oFileDlgUopopen.ShowDialog(this);
            if (drFile != DialogResult.OK)
            {
                MessageBox.Show("File not selected!", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            this.txtPathfile.Text = oFileDlgUopopen.FileName;
        }

        private void Replace(string sWhat, int iIndex, int subIndex, bool bUncompressed)
        {
            UopManager upIstance = UopManager.getIstance();

            int iStartName = upIstance.UopPath.LastIndexOf('\\') + 1;
            string fileName = null;

            if (iStartName != -1)
            {
                fileName = upIstance.UopPath.Substring(iStartName, upIstance.UopPath.Length - iStartName);
            }
            else
            {
                fileName = upIstance.UopPath;
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

            if (upIstance.UopFile.m_Content.Count <= iIndex)
            {
                MessageBox.Show("Error in 'Index Block Header'", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                GC.Collect();
                return;
            }

            if (upIstance.UopFile.m_Content[iIndex].m_ListIndex.Count <= subIndex)
            {
                MessageBox.Show("Error in 'FileIndex Definition'", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                GC.Collect();
                return;
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
                    MessageBox.Show("Error in compression", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    GC.Collect();
                    return;
                }
            }

            if ((compressedStream == null) || (iDestLength == -1) || (bUncompressed && (compressedStream.Length != iDestLength)))
            {
                MessageBox.Show("Error in data to copy", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                GC.Collect();
                return;
            }

            upIstance.UopFile.m_Content[iIndex].m_ListIndex[subIndex].m_LenghtCompressed = (uint)iDestLength;
            upIstance.UopFile.m_Content[iIndex].m_ListIndex[subIndex].m_LenghtUncompressed = (uint)fileContent.Length;
            upIstance.UopFile.m_Content[iIndex].m_ListData[subIndex].m_CompressedData = new byte[iDestLength];
            Array.Copy(compressedStream, upIstance.UopFile.m_Content[iIndex].m_ListData[subIndex].m_CompressedData, iDestLength);

            upIstance.FixOffsets(iIndex, subIndex);

            if (upIstance.Write(fileName + ".new"))
            {
                m_Form1State.AppendTextArea("Writing " + fileName + ".new done succesfully.\n");
            }
            else
            {
                m_Form1State.AppendTextArea("Error in writing " + fileName + ".new\n");
            }

            GC.Collect();
            this.Close();
        }
    }
}