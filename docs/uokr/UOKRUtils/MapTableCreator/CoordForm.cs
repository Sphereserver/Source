using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MapTableCreator
{
    public partial class CoordForm : Form
    {
        public ushort x, y;

        public CoordForm()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            x = (ushort)numericX.Value;
            y = (ushort)numericY.Value;
            DialogResult = DialogResult.OK;
        }
    }
}