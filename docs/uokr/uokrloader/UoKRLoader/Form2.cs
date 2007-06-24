using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace UoKRLoader
{
    public partial class Form2 : Form
    {
        public Form2()
        {
            InitializeComponent();

            this.lblLoaderVersion.Text = "Version: " + Application.ProductVersion;
        }

        private void linkSite_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start("http://scriptsharing.dv-team.de/comment.php?dlid=859");
            }
            catch
            {

            }
        }
    }
}