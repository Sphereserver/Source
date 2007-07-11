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
            string filename = null;

            UopManager.UopPatchError upResult = UopManager.getIstance().Replace(sWhat, iIndex, subIndex, bUncompressed, ref filename);
            GC.Collect();

            switch (upResult)
            {
                case UopManager.UopPatchError.Okay:
                {
                    m_Form1State.AppendTextArea("Patching to " + filename + " done succesfully.\n");
                } break;

                case UopManager.UopPatchError.WriteError:
                {
                    m_Form1State.AppendTextArea("ERROR while patching to " + filename + "\n");
                } break;

                case UopManager.UopPatchError.FileError:
                {
                    MessageBox.Show("File " + sWhat + "don't exist", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } break;

                case UopManager.UopPatchError.IndexBlockError:
                {
                    MessageBox.Show("Error in 'Index Block Header' selection", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } break;

                case UopManager.UopPatchError.FileIndexError:
                {
                    MessageBox.Show("Error in 'FileIndex Definition' selection", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } break;

                case UopManager.UopPatchError.CompressionError:
                {
                    MessageBox.Show("Error while compressing the patch file", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } break;

                case UopManager.UopPatchError.BufferError:
                {
                    MessageBox.Show("Error while copying the patch data", "Patcher Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } break;

                default:
                    break;
            }

            if ((upResult == UopManager.UopPatchError.Okay) || (upResult == UopManager.UopPatchError.WriteError))
            {
                this.Close();
            }
        }
    }
}