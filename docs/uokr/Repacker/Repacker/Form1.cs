using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Repacker.Mythic;

namespace Repacker
{
    public partial class Form1 : Form
    {
        #region Props
        private static SortedDictionary<string, long> hashdictionary;

        public static SortedDictionary<string, long> Dictionary
        {
            get { return hashdictionary; }
            set { hashdictionary = value; }
        }

        private static string m_folderName;
        public static string FolderName
        {
            get { return m_folderName; }
            set { m_folderName = value; }
        }
        #endregion

        public Form1()
        {
            InitializeComponent();
        }

        #region Buttons
        private void load_btn_Click(object sender, EventArgs e)
        {
            display_box.SelectionStart = display_box.Text.Length;
            display_box.ScrollToCaret();
            display_box.Text = "Waiting for folder..";

            string folderName;

            DialogResult result = load_folder.ShowDialog();
            if (result == DialogResult.OK)
            {
                folderName = load_folder.SelectedPath;
                display_box.Text = " Selected Folder:\n''" + folderName + "''\n\nNow press 'Pack It' to create .uop file or press 'See and change Hashes'..";
                //Let's pass folderName to Packer
                m_folderName = folderName;

                hashdictionary = new SortedDictionary<string, long>();
                //Panel Switching
                seehash_btn.Enabled = true;
                //End
                string[] files = Directory.GetFiles(FolderName, "*", SearchOption.AllDirectories);       //Get all files in directory ( MUST also check subDirs )
                Hash_Grid.Rows.Add(files.Length-1);
                for(int i=0;i<files.Length;i++)
                {

                    string toHash = files[i].Substring(Form1.FolderName.Length + 1);
                    toHash = toHash.Replace("\\", "/");

                    Hash_Grid.Rows[i].Cells[0].Value = toHash;
                    Hash_Grid.Rows[i].Cells[1].Value = Repack.Functs.HashMeGently(toHash).ToString( "X16" );
                    hashdictionary.Add(toHash, Repack.Functs.HashMeGently(toHash));
                }
            }
            else
                display_box.Text = "Error opening folder!";
        }

        private void pack_btn_Click(object sender, EventArgs e)
        {
            if (FolderName == null)
            {
                display_box.AppendText("\n\nError: no folder loaded.");
                return;
            }
            load_btn.Enabled = false;
            pack_btn.Enabled = false;
            seehash_btn.Enabled = false;

            //We start reading data's files + data's index and creating a list of them.
            List<R_MythicPackageIndex> toblock = new List<R_MythicPackageIndex>();

            R_MythicPackageIndex final_data = new R_MythicPackageIndex();

            //display_box.AppendText("\n" + Path.GetFileName(FolderName));
            //Read files
            display_box.AppendText("\n\nReading data's files..");
            string[] files = Directory.GetFiles(FolderName, "*", SearchOption.AllDirectories);       //Get all files in directories

            progressBar.Maximum = files.Length;
            Worker.RunWorkerAsync(files); //Worker will create index and datas List

        }

        private void About_btn_Click(object sender, EventArgs e)
        {
            About a = new About();
            a.ShowDialog();
        }

        private void seehash_btn_Click(object sender, EventArgs e)
        {
            Main_panel.Visible = false;
            Hashpanel.Visible = true;
            Form1.ActiveForm.Width = 500;
            Form1.ActiveForm.Height = 430;
        }
        #endregion

        // Let's write from other threads 
        // To Do:
        // use worker.reportProgress and OnProgressChanged
        private delegate void K_SetValueDelegate(int value);

        public void K_Setvalue(int value)
        {
            if (this.progressBar.InvokeRequired)
            {
                this.progressBar.Invoke(new K_SetValueDelegate(this.K_Setvalue), value);
            }
            else
            {
                this.progressBar.Value = progressBar.Value + value;
            }
        }
        //K end

        #region worker
        private void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            if (e.Argument is string[])
            {
                List<R_MythicPackageIndex> toblock = new List<R_MythicPackageIndex>();
                string[] files = (string[])e.Argument;
                bool error = false;
                for (int k = 0; (k < files.Length) && !error; k++) //Should read ALL files in folder
                {
                    toblock.Add(R_MythicPackageIndex.Pack(files[k], ref error));
                    K_Setvalue(1);
                }

                if (error)
                    e.Result = "An Error was encountered while working. See MessageBox";
                else
                    e.Result = toblock;
            }
            if (e.Argument is List<R_MythicPackageIndex>)
            {
                List<R_MythicPackageIndex> indexes= (List<R_MythicPackageIndex>)e.Argument;
                //Now we should create Blocks of 100 elements
                List<R_MythicPackageBlock> blocks = new List<R_MythicPackageBlock>();
                int i = 0;
                for (int k = 0; k < indexes.Count; )
                {
                    blocks.Add(R_MythicPackageBlock.Pack(indexes, k, ref i));//Creates block struct
                    k = i;
                    K_Setvalue(1);
                }

                //Now we have all blocks, let's do the header.
                R_MythicPackageHeader package = new R_MythicPackageHeader();
                package = R_MythicPackageHeader.Create(blocks);

                e.Result = package;
            }
            else if (e.Argument is R_MythicPackageHeader)
            {
                R_MythicPackageHeader towrite = (R_MythicPackageHeader)e.Argument;
                //We should now write down .uop format using package
                string name =  ((Path.GetFileName(FolderName)).Insert((Path.GetFileName(FolderName)).Length,".uop"));           //Where to write uop
                //Choose writing method
                //write1(towrite, name);
                //write2(towrite, name);
                write3(towrite, name);
                e.Result = true;
            }


        }
        private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (e.Error != null)
            {
            }
            else if (e.Result is string)
            {
                display_box.Text = (string)e.Result;
                progressBar.Value = 0;
            }
            else if (e.Result is List<R_MythicPackageIndex>)
            {
                display_box.AppendText("\n1-Datas and Index created, now working on Blocks..");
                progressBar.Value = progressBar.Maximum;
                progressBar.Maximum = ((List<R_MythicPackageIndex>)e.Result).Count;
                progressBar.Value = progressBar.Minimum;
                Worker.RunWorkerAsync((List<R_MythicPackageIndex>)e.Result);
            }
            else if (e.Result is R_MythicPackageHeader)
            {
                display_box.AppendText("\n2-Blocks and Header created, now working on writing..");
                progressBar.Value = progressBar.Maximum;
                progressBar.Maximum = ((R_MythicPackageHeader)e.Result).Blocks.Count * 200;
                progressBar.Value = progressBar.Minimum;
                Worker.RunWorkerAsync((R_MythicPackageHeader)e.Result);
            }
            else if (e.Result is bool)
            {
                pack_btn.Enabled = true;
                load_btn.Enabled = true;
                if ((bool)e.Result)
                {
                    display_box.AppendText("\nThe file was successfully created, enjoy!");
                }
                progressBar.Value = progressBar.Minimum;
            }
        }
        #endregion

        #region Offsets + Write
        //Third write method RIGHT SIZE RIGHT POSITION
        private void write3(R_MythicPackageHeader towrite, string name)
        {
            using (FileStream uop = new FileStream(name, System.IO.FileMode.Create, System.IO.FileAccess.Write))
            {
                //Declarations
                List<List<long>> offset = new List<List<long>>();

                //First, we calculate space in stream
                using (BinaryWriter writer = new BinaryWriter(uop))
                {
                    //Write first Header, then blocks.
                    writer.Write(towrite.Start);
                    writer.Write(towrite.Version);
                    writer.Write(towrite.Misc);
                    writer.Write(towrite.HeaderSize);
                    writer.Write(towrite.Zeros);
                    writer.Write(towrite.BlockSize);
                    writer.Write(towrite.FileCount);
                    byte zero = 0;
                    for (int i = 0; i < towrite.Fill; i++)
                    {
                        writer.Write(zero);
                    }
                    // Header is done, let's write Blocks
                    long nextblockoffset = towrite.HeaderSize;

                    for (int i = 0; i < towrite.Blocks.Count; i++)
                    {
                        //Need to set nextblockoffset
                        nextblockoffset += C_offset(true, towrite, i);
                        if (i == towrite.Blocks.Count - 1)
                            towrite.Blocks[i].NextBlock = 0;
                        else
                            towrite.Blocks[i].NextBlock = nextblockoffset;
                        //
                        writer.Write(towrite.Blocks[i].FileCount);  //Ok
                        writer.Write(towrite.Blocks[i].NextBlock);  //Ok

                        for (int j = 0; j < towrite.Blocks[i].Files.Count; j++)
                        {
                            //We write INDEXES
                            //need to set datablockoffset
                            if (towrite.Blocks[i].Files[j].DataBlock != null)
                            {
                                towrite.Blocks[i].Files[j].DataBlockOffset = B_offset(towrite, i, j);
                            }
                            else
                                towrite.Blocks[i].Files[j].DataBlockOffset = 0;
                            //
                            writer.Write(towrite.Blocks[i].Files[j].DataBlockOffset);   //ToDo
                            writer.Write(towrite.Blocks[i].Files[j].HeadLength);        //Fixed?
                            writer.Write(towrite.Blocks[i].Files[j].CompressedSize);    //Ok
                            writer.Write(towrite.Blocks[i].Files[j].DecompressedSize);  //Ok
                            writer.Write(towrite.Blocks[i].Files[j].FileHash);          //Wrong
                            writer.Write(towrite.Blocks[i].Files[j].CRC);               //Wrong
                            writer.Write(towrite.Blocks[i].Files[j].Flag);              //Ok
                            //
                            K_Setvalue(1);
                        }
                        for (int j = 0; j < towrite.Blocks[i].Files.Count; j++)
                        {
                            //We write DATA
                            if (towrite.Blocks[i].Files[j].DataBlock != null)
                            {
                                writer.Write(towrite.Blocks[i].Files[j].DataBlock.Flag);        //Fixed
                                writer.Write(towrite.Blocks[i].Files[j].DataBlock.DataOffset);  //Fixed
                                writer.Write(towrite.Blocks[i].Files[j].DataBlock.DateTime);    //Ok
                                writer.Write(towrite.Blocks[i].Files[j].DataBlock.Data);        //Ok
                            }
                            K_Setvalue(1);
                        }
                    }
                }
            }
        }

        //Calculate Offsets v2.0
        private long C_offset(bool nextblock, R_MythicPackageHeader towrite, int i)
        {
            long offsettodatablock = 0;
            //BlockOffset
            offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].FileCount)).Length;
            offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].NextBlock)).Length;
            for (int j = 0; j < (towrite.Blocks[i]).FileCount; j++)
            {
                //Index Offset
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].DataBlockOffset)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].HeadLength)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].CompressedSize)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].DecompressedSize)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].FileHash)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].CRC)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].Flag)).Length;

                //DataBlock
                if (towrite.Blocks[i].Files[j].DataBlock != null && nextblock)
                {
                    //calculate offset to compressed data byte[]
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].DataBlock.Flag)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].DataBlock.DataOffset)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[j].DataBlock.DateTime)).Length;
                    offsettodatablock += (towrite.Blocks[i].Files[j].DataBlock.Data).Length;
                }
            }        
            return offsettodatablock;
        }
        //
        private long B_offset(R_MythicPackageHeader towrite, int a, int j)
        {
            long offsettodatablock = towrite.HeaderSize;
            int limit = 0;

            for (int i = 0; i < a+1; i++)
            {
                //BlockOffset
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].FileCount)).Length;
                offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].NextBlock)).Length;

                for (int k = 0; k < 100; k++)
                //for (int k = 0; k < (towrite.Blocks[i]).FileCount; k++)
                {
                    //Index Offset
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].DataBlockOffset)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].HeadLength)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].CompressedSize)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].DecompressedSize)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].FileHash)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].CRC)).Length;
                    offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].Flag)).Length;
                }
                if (i < a)
                    limit = (towrite.Blocks[i]).FileCount;
                else if (i == a)
                {
                    limit = j;
                    if (j == 0)
                        return offsettodatablock;
                }

                for (int k = 0; k < limit; k++)
                {
                    //DataBlock
                    if (towrite.Blocks[i].Files[k].DataBlock != null)
                    {
                        //calculate offset to compressed data byte[]
                        offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].DataBlock.Flag)).Length;
                        offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].DataBlock.DataOffset)).Length;
                        offsettodatablock += (BitConverter.GetBytes(towrite.Blocks[i].Files[k].DataBlock.DateTime)).Length;
                        offsettodatablock += (towrite.Blocks[i].Files[k].DataBlock.Data).Length;
                    }
                }
            }
            return offsettodatablock;
        }
        #endregion

        #region koksi
        private void button1_Click(object sender, EventArgs e)
        {
            Hashpanel.Visible = false;
            Main_panel.Visible = true;
            Form1.ActiveForm.Width = 300;
            Form1.ActiveForm.Height = 240;
        }

        private void dataGridView1_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            string name = Hash_Grid.Rows[e.RowIndex].Cells[0].Value.ToString();
            ulong hash = Convert.ToUInt64(Hash_Grid.Rows[e.RowIndex].Cells[1].Value.ToString(),16);
            hashdictionary.Remove(name);
            hashdictionary.Add(name, (long)hash);
            //hashdictionary
        }
        #endregion

        private void Form1_Load(object sender, EventArgs e)
        {
            this.Width = 300;
            this.Height = 240;
            this.seehash_btn.Enabled = false;
        }
    }
}
