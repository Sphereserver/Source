using System;
using System.IO;
using Ultima;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MapFileDecryptor
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (oFD1.ShowDialog() == DialogResult.OK)
                textBox1.Text = oFD1.FileName;

        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (textBox1.Text == "")
                return;
            UopParser p = new UopParser(textBox1.Text);
            if (p.Ready())
            {
                uint cells, delims,statics;
                long until;
                List<MapElem> res=p.Parse(out cells, out delims, out statics, out until);
                MessageBox.Show("Stored " + res.Count + " elems. "+cells+" cells, "+delims+" delimiters, "+statics+" statics. Stop at "+until+" pos to EOF");
            }
            else
                MessageBox.Show("Error in opening file.");
        }

        private void AddDelimiters(BinaryWriter bw, int BlockX, int BlockY, int X, int Y)
        {
            
        }

        private void button3_Click(object sender, EventArgs e)
        {
            string newPath = Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf('\\'));
            newPath += @"\Created\";

            if (!Directory.Exists(newPath))
                Directory.CreateDirectory(newPath);
            TileMatrix tm = new TileMatrix(0, 0, 6144, 4096);
            TransTable tt = new TransTable();
            for (int BlockX = 0; BlockX < 96; BlockX++)
            {
                for (int BlockY = 0; BlockY < 64; BlockY++)
                {
                    int BlockID = BlockX * 64 + BlockY + 1;
                    int firstID = BlockID / 100;
                    int secondID = BlockID - firstID * 100;
                    BlockID--;
                    String filename = newPath+"facet0_" + firstID + "_" + secondID + ".dat";
                    BinaryWriter bw = new BinaryWriter(File.Create(filename));
                    Tile t;
                    HuedTile[] ht;
                    bool overlap = false;
                    for (int X = BlockX * 64; X < (BlockX + 1) * 64; X++)
                    {
                        for (int Y = BlockY * 64; Y < (BlockY + 1) * 64; Y++)
                        {
                            t = tm.GetLandTile(X, Y);
                            if (!overlap)
                                bw.Write((byte)0);
                            else
                                overlap = false;
                            if (bw.BaseStream.Position == 1)
                                bw.Write((UInt16)BlockID);
                            else
                                bw.Write((UInt16)0);
                            bw.Write((SByte)t.Z);
                            bw.Write((UInt16)tt[t.ID]);
                            bw.Write(UopParser.Flip((UInt16)t.ID));
                            AddDelimiters(bw, BlockX, BlockY, X, Y);
                            ht = tm.GetStaticTiles(X, Y);
                            if (ht!=null)
                            {
                                for (int i = 0; i < ht.Length; i++)
                                {
                                    if (!overlap)
                                        bw.Write((byte)0);
                                    else
                                        overlap = false;
                                    bw.Write((byte)0);
                                    if (i == 0)
                                        bw.Write((byte)ht.Length);
                                    else
                                        bw.Write((byte)0);
                                    bw.Write((UInt16)ht[i].ID);
                                    bw.Write((UInt16)0);
                                    bw.Write((sbyte)ht[i].Z);
                                    bw.Write((UInt16)ht[i].Hue);
                                    overlap = true;
                                }
                            }                            
                        }
                    }
                    bw.Write(new byte[] { 0, 0, 0 });
                    bw.Close();
                }
            }

        }
    }
}