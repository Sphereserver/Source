using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;
using System.Collections;

namespace MapTableCreator
{
    public partial class Form1 : Form
    {
        private Facet f;

        private bool closed = false;
        private bool stopcollect = false;
        private bool handle_select = false;

        private TextWriter errwr, outwr;
        public Form1()
        {
            InitializeComponent();
        }

        class FileComparer : IComparer
        {
            private int GetValue(String s)
            {
                int indb = s.LastIndexOf("\\");
                if (indb == -1)
                    indb = 0;
                int ind = s.IndexOf("_", indb);
                int ind2 = s.IndexOf("_", ind + 1);
                int ind3 = s.IndexOf(".dat");
                String num1 = s.Substring(ind+1, ind2 - ind-1);
                String num2 = s.Substring(ind2+1, ind3 - ind2-1);
                return Convert.ToInt32(num1) * 100 + Convert.ToInt32(num2);
            }

           int IComparer.Compare(Object a, Object b)
            {
                return GetValue((String)a).CompareTo(GetValue((String)b));
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (fbd.ShowDialog() == DialogResult.OK)
                textBox2.Text = fbd.SelectedPath;
        }

        class ThreadParam
        {
            public String[] files;
            public int num;
        }

        private void Inc()
        {
            progressBar1.Value++;
            if ((progressBar1.Value % 100) == 0)
            {
                GC.Collect();
                Application.DoEvents();
            }
        }
        private delegate void IncDelegate();

        private void MyThreadCalc(object Param)
        {
            ThreadParam tp = (ThreadParam)Param;
            String[] files = tp.files;

            foreach (String s in files)
            {
                Parser p = new Parser(s, errwr, outwr, writeOnlyCheck.Checked, DelimiterCheck.Checked, StaticCheck.Checked, TileCheck.Checked);
                BlockFile b = p.Parse();
                lock (f.blockfileslist)
                {
                    if (numBlocks.Value > 0 && f.blockfileslist.Count >= numBlocks.Value)
                    {
                        stopcollect = true;
                        if (parseOnly.Checked)
                            break;
                    }
                    if (!stopcollect)
                        f.blockfileslist.Add(b);
                }
                if (closed)
                    break;
                progressBar1.BeginInvoke(new IncDelegate(Inc));
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            handle_select = false;
            this.blockbox.Text = "";
            blocklist.DataSource = null;
            bool genera = extendedCheck.Checked;
            stopcollect = false;
            groupBox1.Enabled = false;
            groupBox2.Enabled = false;
            f=new Facet();
            blocklist.DataSource = null;
            GC.Collect();
            String[] files = Directory.GetFiles(textBox2.Text, "*.dat");
            IComparer icomp = new FileComparer();
            Array.Sort(files, icomp);
            progressBar1.Minimum = 0;
            progressBar1.Maximum = (parseOnly.Checked && numBlocks.Value>0) ? (int)numBlocks.Value : files.Length;
            progressBar1.Value = 0;
            errwr = new StreamWriter("error.log");
            outwr = null;
            if(genera)
                outwr = new StreamWriter("outuput.log");
            int div = (parseOnly.Checked && numBlocks.Value > 0) ? (int)numBlocks.Value : files.Length / 3;
            if (div > 50)
            {
                bw1.RunWorkerAsync(files);
            }
            else
            {
                foreach (String s in files)
                {
                    Parser p = new Parser(s, errwr, outwr, writeOnlyCheck.Checked, DelimiterCheck.Checked, StaticCheck.Checked, TileCheck.Checked);
                    BlockFile b = p.Parse();
                    if (numBlocks.Value > 0 && f.blockfileslist.Count >= numBlocks.Value)
                    {
                        stopcollect = true;
                        if (parseOnly.Checked)
                            break;
                    }
                    if (!stopcollect)
                    {
                        f.blockfiles.AddLast(b);
                        f.blockfileslist.Add(b);
                    }
                    Inc();
                    if (closed)
                        break;
                }
                bw1_RunWorkerCompleted(null, null);
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            closed = true;
        }

        private void blocklist_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (!handle_select)
                return;
            
            BlockFile bf = blocklist.SelectedItem as BlockFile;
            blockbox.Text = bf.Dump();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            GridForm frm = new GridForm(f);
            frm.Show();
        }

        private void bw1_DoWork(object sender, DoWorkEventArgs e)
        {
            String[] files = (String[])e.Argument;
            int realsize = (parseOnly.Checked && numBlocks.Value > 0) ? (int)numBlocks.Value : files.Length;
            int div = realsize / 3;
            ThreadParam puno = new ThreadParam();
            puno.files = new string[div];
            Array.Copy(files, 0, puno.files, 0, div);
            puno.num = 1;
            ThreadParam pdue = new ThreadParam();
            pdue.files = new string[div];
            Array.Copy(files, div, pdue.files, 0, div);
            pdue.num = 2;
            ThreadParam ptre = new ThreadParam();
            ptre.files = new string[realsize - 2 * div];
            Array.Copy(files, 2 * div, ptre.files, 0, realsize - 2 * div);
            ptre.num = 3;
            Thread t1 = new Thread(new ParameterizedThreadStart(MyThreadCalc));
            Thread t2 = new Thread(new ParameterizedThreadStart(MyThreadCalc));
            Thread t3 = new Thread(new ParameterizedThreadStart(MyThreadCalc));
            t1.Start(puno);
            t2.Start(pdue);
            t3.Start(ptre);
            t1.Join();
            t2.Join();
            t3.Join();
        }

        private void bw1_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (sender != null)
            {
                f.blockfileslist.Sort();
                foreach (BlockFile b in f.blockfileslist)
                {
                    f.blockfiles.AddLast(b);
                }
            }
            blocklist.DataSource = null;
            blocklist.DataSource = f.blockfileslist;
            blocklist.DisplayMember = "Name";
            CurrencyManager cm=this.BindingContext[f.blockfileslist] as CurrencyManager;
            if (cm != null)
                cm.Refresh();
            errwr.Close();
            if (outwr != null)
                outwr.Close();
            StreamWriter table = new StreamWriter("table.txt");
            foreach (ushort MLG in Parser.table.Keys)
            {
                ushort value;
                Parser.table.TryGetValue(MLG, out value);
                table.WriteLine(MLG + "->" + value);
            }
            table.Close();
            handle_select = true;
            MessageBox.Show("Completato", "UOKR Map Decoder");
            groupBox1.Enabled = true;
            groupBox2.Enabled = true;
        }
    }
}