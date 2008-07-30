using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Ultima;
using System.IO;

namespace MapMULToUOP
{
    public partial class MainForm : Form
    {
        public String GetAppTemp()
        {
            String path = Application.StartupPath;
            if (path.Substring(path.Length - 1) != "\\")
                path += "\\";
            path += "Temp\\";
            return path;
        }

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            if(Client.Directories.Count>0)
                UOFolderBox.Text=Client.Directories[0];
        }

        private void browseButton_Click(object sender, EventArgs e)
        {
            if (opf.ShowDialog() == DialogResult.OK)
                convBox.Text = opf.FileName;

        }

        private void browseButton2_Click(object sender, EventArgs e)
        {
            if (fBD.ShowDialog() == DialogResult.OK)
                UOFolderBox.Text = fBD.SelectedPath;
        }

        private void genButton_Click(object sender, EventArgs e)
        {
            if (mainTab.SelectedTab.Name == "MapPage")
            {
                if (genButton.Text == "Generate")
                {
                    GraphicTableParser gtp = new GraphicTableParser(convBox.Text);
                    Dictionary<ushort, ushort> ConvTable = gtp.Parse();
                    if (ConvTable == null)
                    {
                        MessageBox.Show("Conversion table not loaded!");
                        return;
                    }
                    String apppath = GetAppTemp();
                    if (Directory.Exists(apppath))
                        Directory.Delete(apppath, true);
                    Directory.CreateDirectory(apppath);
                    Map mappa = null;
                    int facetId = 0;
                    if (britanniaButton.Checked)
                    {
                        mappa = Map.Felucca;
                        facetId = 0;
                    }
                    else if (britanniaAltButton.Checked)
                    {
                        mappa = Map.Trammel;
                        facetId = 1;
                    }
                    else if (ilshenarButton.Checked)
                    {
                        mappa = Map.Ilshenar;
                        facetId = 2;
                    }
                    else if (malasButton.Checked)
                    {
                        mappa = Map.Malas;
                        facetId = 3;
                    }
                    else if (tokunoButton.Checked)
                    {
                        mappa = Map.Tokuno;
                        facetId = 4;
                    }
                    BlockWriter bw = new BlockWriter(facetId, mappa, apppath,ConvTable);
                    bw.Write();
                    MessageBox.Show("Completed.");
                    genButton.Text = "Compress & Pack";
                }
                else
                {

                }
            }
        }

        private void resButton_Click(object sender, EventArgs e)
        {
            if(MessageBox.Show("Are You Sure?","",MessageBoxButtons.YesNo,MessageBoxIcon.Question)==DialogResult.Yes)
                genButton.Text = "Generate";
        }
    }
}