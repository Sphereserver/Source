using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MapTableCreator
{
    public partial class GridForm : Form
    {
        private Facet facet;
        private LinkedListNode<BlockFile> enumerator;
        private Panel drawpanel;

        private const int LINELEN = 14;

        public GridForm(Facet f)
        {
            facet = f;
            InitializeComponent();
            drawpanel = new Panel();
            drawpanel.BorderStyle = BorderStyle.FixedSingle;
            drawpanel.Location = new Point(0, 0);
            drawpanel.Size = new Size(3520, 3520);
            drawpanel.Paint += new PaintEventHandler(p_Paint);
            this.main_panel.Controls.Add(drawpanel);
            if (f != null)
            {
                this.Text = "Grid for facet" + facet.facetId;
                enumerator = facet.blockfiles.First;
            }
        }

        private void GridForm_Load(object sender, EventArgs e)
        {
            if (facet == null)
            {
                Close();
                return;
            }
            DisplayGrid();
        }

        private void DisplayGrid()
        {
            this.Text = "Grid for facet" + facet.facetId + ", BlockID:" + enumerator.Value.header.FileID;
            drawpanel.Invalidate();
        }

        void DrawMapPoint(MapPoint mp, Graphics g)
        {
            Rectangle r = new Rectangle(mp.offsetx * 55, mp.offsety * 55, 55, 55);
            g.DrawRectangle(Pens.Black, r);
            StringFormat f=new StringFormat();
            f.Alignment=StringAlignment.Center;
            f.LineAlignment=StringAlignment.Center;
            g.DrawString(mp.x+Environment.NewLine+mp.y+((mp.lt.Type3==0)?"":(Environment.NewLine+mp.lt.Type3)),main_panel.Font,(mp.static_list.Count>0)?Brushes.Red:Brushes.Black,r,f);
            foreach(Delimiter d in mp.delimiter_list)
            {
                Point p1=new Point(mp.offsetx * 55,mp.offsety * 55), p2=new Point(mp.offsetx * 55,mp.offsety * 55);
                switch (d.direction)
                {
                    case DelimiterDirection.BOTTOM:
                        p1.X += 27;
                        p1.Y += 55 - LINELEN;
                        p2.X += 27;
                        p2.Y += 55;
                        break;
                    case DelimiterDirection.BOTTOMLEFT:
                        p1.X += 0;
                        p1.Y += 55;
                        p2.X += LINELEN;
                        p2.Y += 55 - LINELEN;
                        break;
                    case DelimiterDirection.BOTTOMRIGHT:
                        p1.X += 55;
                        p1.Y += 55;
                        p2.X += 55 - LINELEN;
                        p2.Y += 55 - LINELEN;
                        break;
                    case DelimiterDirection.LEFT:
                        p1.X += 0;
                        p1.Y += 27;
                        p2.X += LINELEN;
                        p2.Y += 27;
                        break;
                    case DelimiterDirection.RIGHT:
                        p1.X += 55 - LINELEN;
                        p1.Y += 27;
                        p2.X += 55;
                        p2.Y += 27;
                        break;
                    case DelimiterDirection.TOP:
                        p1.X += 27;
                        p1.Y += 0;
                        p2.X += 27;
                        p2.Y += LINELEN;
                        break;
                    case DelimiterDirection.TOPLEFT:
                        p1.X += 0;
                        p1.Y += 0;
                        p2.X += LINELEN;
                        p2.Y += LINELEN;
                        break;
                    case DelimiterDirection.TOPRIGHT:
                        p1.X += 55;
                        p1.Y += 0;
                        p2.X += 55 - LINELEN;
                        p2.Y += LINELEN;
                        break;
                }
                g.DrawLine(Pens.Black, p1, p2);
            }
        }

        void p_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            foreach (MapPoint mp in enumerator.Value.blocks)
            {
                DrawMapPoint(mp,g);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (enumerator.Next != null)
            {
                enumerator = enumerator.Next;
                DisplayGrid();
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (enumerator.Previous != null)
            {
                enumerator = enumerator.Previous;
                DisplayGrid();
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            CoordForm frm = new CoordForm();
            if (frm.ShowDialog() == DialogResult.OK)
            {
                int XBlock = frm.x/64;
                int YBlock = frm.y/64;
                int BlockNumber=XBlock*64+YBlock;
                enumerator = facet.blockfiles.First;
                for (int i = BlockNumber; i >= 0; i--)
                {
                    if (enumerator.Next == null)
                        break;
                    enumerator = enumerator.Next;
                }
                DisplayGrid();
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            this.Text = "Searching...";
            bool trovato = false;
            for (; enumerator != null; enumerator = enumerator.Next)
            {
                BlockFile bfile = enumerator.Value;
                int angoli = 0;
                foreach (MapPoint m in bfile.blocks)
                {
                    if (m.offsetx == 0 && m.offsety == 0)
                        angoli++;
                    if (m.offsetx == 0 && m.offsety == 63)
                        angoli++;
                    if (m.offsetx == 63 && m.offsety == 0)
                        angoli++;
                    if (m.offsetx == 63 && m.offsety == 63)
                        angoli++;
                }
                if (angoli <= 2)
                {
                    trovato = true;
                    break;
                }
            }
            if (!trovato)
                enumerator = facet.blockfiles.First;
            DisplayGrid();
        }
    }
}