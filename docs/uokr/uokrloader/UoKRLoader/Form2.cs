using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Net;
using System.Text;
using System.Windows.Forms;

namespace UoKRLoader
{
    public partial class Form2 : Form
    {
        private WebClient m_CheckUpdates;
        private const string BTNUPDATE_CLICKED = "Checking ...";
        private const string BTNUPDATE_UNCLICK = "Check for updates";

        public Form2()
        {
            InitializeComponent();

            this.FormClosing += new FormClosingEventHandler(Form2_FormClosing);
            this.lblLoaderVersion.Text = "Version " + Application.ProductVersion;
            this.lblCopyright.Text = "Copyright © 2007 " + Application.CompanyName;
            this.btnUpdates.Text = BTNUPDATE_UNCLICK;
        }

        void Form2_FormClosing(object sender, FormClosingEventArgs e)
        {
            CleanUpWebclient();
        }

        private void CleanUpWebclient()
        {
            if (this.m_CheckUpdates != null)
            {
                if (this.m_CheckUpdates.IsBusy)
                    this.m_CheckUpdates.CancelAsync();

                this.m_CheckUpdates.Dispose();
            }
        }

        private void StartBrowser()
        {
            try
            {
                System.Diagnostics.Process.Start(StaticData.KRLOADER_SITE);
            }
            catch
            {

            }
        }

        private void linkSite_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            StartBrowser();
        }

        private void btnUpdates_Click(object sender, EventArgs e)
        {
            this.btnUpdates.Enabled = false;
            this.btnUpdates.Text = BTNUPDATE_CLICKED;
            this.Cursor = Cursors.AppStarting;

            CleanUpWebclient();

            this.m_CheckUpdates = new WebClient();
            this.m_CheckUpdates.DownloadDataAsync(new Uri(StaticData.KRLOADER_SITE));
            this.m_CheckUpdates.DownloadDataCompleted += new DownloadDataCompletedEventHandler(m_CheckUpdates_DownloadDataCompleted);
        }

        void m_CheckUpdates_DownloadDataCompleted(object sender, DownloadDataCompletedEventArgs e)
        {   
            this.Cursor = Cursors.Default;
            this.btnUpdates.Text = BTNUPDATE_UNCLICK;
            this.btnUpdates.Enabled = true;

            if (!e.Cancelled)
            {
                if (e.Error != null)
                {
                    MessageBox.Show("Cannot contact the site for updates!", Application.ProductName + " Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                else
                {
                    //  Encoding.ASCII.GetString(e.Result, 0, e.Result.Length).Split(new string[] { "\n" }, StringSplitOptions.None)
                }
            }
        }
    }
}