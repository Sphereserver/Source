using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Net;
using System.Text;
using System.Windows.Forms;

namespace UoKRUnpacker
{
    public partial class Form2 : Form
    {
        private WebClient m_CheckUpdates;
        private const string BTNUPDATE_CLICKED = "Checking ...";
        private const string BTNUPDATE_UNCLICK = "Check for updates";
        public const string KRUNPACKER_SITE = "http://scriptsharing.dv-team.de/dle/comment.php?dlid=865";

        public Form2()
        {
            InitializeComponent();

            this.FormClosing += new FormClosingEventHandler(Form2_FormClosing);
            this.lblUnpackerVersion.Text = "Version " + Application.ProductVersion;
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
                System.Diagnostics.Process.Start(KRUNPACKER_SITE);
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
            this.m_CheckUpdates.DownloadDataAsync(new Uri(KRUNPACKER_SITE));
            this.m_CheckUpdates.DownloadDataCompleted += new DownloadDataCompletedEventHandler(m_CheckUpdates_DownloadDataCompleted);
        }

        void m_CheckUpdates_DownloadDataCompleted(object sender, DownloadDataCompletedEventArgs e)
        {
            const string VERSION_ID_START = "&gt;&gt; Current Version:";
            const string VERSION_ID_END = " &lt;&lt;";

            if (!e.Cancelled)
            {
                if (e.Error != null)
                {
                    MessageBox.Show("Cannot contact the site for updates!", Application.ProductName + " Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                else
                {
                    try
                    {
                        string[] sResult = Encoding.ASCII.GetString(e.Result, 0, e.Result.Length).Split(new string[] { "\n" }, StringSplitOptions.None);
                        string sWithData = null;

                        for (int i = 0; i < sResult.Length; i++)
                        {
                            if (sResult[i].IndexOf(VERSION_ID_START) != -1)
                            {
                                sWithData = sResult[i];
                                break;
                            }
                        }

                        if (sWithData != null)
                        {
                            int iFound = sWithData.IndexOf(VERSION_ID_START);

                            if (iFound > 0)
                            {
                                iFound += VERSION_ID_START.Length;
                                sWithData = sWithData.Substring(iFound, sWithData.IndexOf(VERSION_ID_END) - iFound);
                                CheckUpdate(sWithData.Trim());
                            }
                        }
                    }
                    catch
                    {

                    }
                }
            }

            // -------------------------------------
            this.Cursor = Cursors.Default;
            this.btnUpdates.Text = BTNUPDATE_UNCLICK;
            this.btnUpdates.Enabled = true;
        }

        private void CheckUpdate(string sUploadedVersion)
        {
            int currentVersion = VersionToInteger(Application.ProductVersion);
            int uploadedVersion = ((sUploadedVersion != null) && (sUploadedVersion.Length > 0)) ? VersionToInteger(sUploadedVersion) : 0;

            if (uploadedVersion > currentVersion)
            {
                DialogResult drWantUpdate = MessageBox.Show("A new version (" + sUploadedVersion + ") is available.\n" +
                                                            "Do you want to open the browser to download the new one?",
                                                            "Update Information", MessageBoxButtons.YesNo, MessageBoxIcon.Information);

                if (drWantUpdate == DialogResult.Yes)
                {
                    StartBrowser();
                }
            }
            else
            {
                MessageBox.Show("You already have the latest version.", "Update Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private int VersionToInteger(string version)
        {
            string[] splittedVersion = version.Split('.');
            int iVersion = -1;

            try
            {
                iVersion = Int32.Parse(splittedVersion[3]) +
                           Int32.Parse(splittedVersion[2]) * 100 +
                           Int32.Parse(splittedVersion[1]) * 10000 +
                           Int32.Parse(splittedVersion[0]) * 1000000;
            }
            catch
            {

            }

            return iVersion;
        }
    }
}