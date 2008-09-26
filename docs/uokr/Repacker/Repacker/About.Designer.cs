namespace Repacker
{
    partial class About
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(About));
            this.Close_btn = new System.Windows.Forms.Button();
            this.Authors_lbl = new System.Windows.Forms.Label();
            this.changelog_box = new System.Windows.Forms.TextBox();
            this.changelog_lbl = new System.Windows.Forms.Label();
            this.auth_lbl2 = new System.Windows.Forms.Label();
            this.linkLabel1 = new System.Windows.Forms.LinkLabel();
            this.linkLabel2 = new System.Windows.Forms.LinkLabel();
            this.SuspendLayout();
            // 
            // Close_btn
            // 
            this.Close_btn.Location = new System.Drawing.Point(12, 144);
            this.Close_btn.Name = "Close_btn";
            this.Close_btn.Size = new System.Drawing.Size(46, 23);
            this.Close_btn.TabIndex = 0;
            this.Close_btn.Text = "Close";
            this.Close_btn.UseVisualStyleBackColor = true;
            this.Close_btn.Click += new System.EventHandler(this.Close_btn_Click);
            // 
            // Authors_lbl
            // 
            this.Authors_lbl.AutoSize = true;
            this.Authors_lbl.Location = new System.Drawing.Point(12, 9);
            this.Authors_lbl.Name = "Authors_lbl";
            this.Authors_lbl.Size = new System.Drawing.Size(46, 13);
            this.Authors_lbl.TabIndex = 1;
            this.Authors_lbl.Text = "Authors:";
            // 
            // changelog_box
            // 
            this.changelog_box.Location = new System.Drawing.Point(79, 25);
            this.changelog_box.Multiline = true;
            this.changelog_box.Name = "changelog_box";
            this.changelog_box.ReadOnly = true;
            this.changelog_box.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.changelog_box.Size = new System.Drawing.Size(277, 142);
            this.changelog_box.TabIndex = 2;
            this.changelog_box.Text = resources.GetString("changelog_box.Text");
            // 
            // changelog_lbl
            // 
            this.changelog_lbl.AutoSize = true;
            this.changelog_lbl.Location = new System.Drawing.Point(12, 28);
            this.changelog_lbl.Name = "changelog_lbl";
            this.changelog_lbl.Size = new System.Drawing.Size(61, 13);
            this.changelog_lbl.TabIndex = 3;
            this.changelog_lbl.Text = "Changelog:";
            // 
            // auth_lbl2
            // 
            this.auth_lbl2.AutoSize = true;
            this.auth_lbl2.Location = new System.Drawing.Point(76, 9);
            this.auth_lbl2.Name = "auth_lbl2";
            this.auth_lbl2.Size = new System.Drawing.Size(66, 13);
            this.auth_lbl2.TabIndex = 4;
            this.auth_lbl2.Text = "Kons - Koksi";
            // 
            // linkLabel1
            // 
            this.linkLabel1.AutoSize = true;
            this.linkLabel1.Location = new System.Drawing.Point(191, 9);
            this.linkLabel1.Name = "linkLabel1";
            this.linkLabel1.Size = new System.Drawing.Size(48, 13);
            this.linkLabel1.TabIndex = 5;
            this.linkLabel1.TabStop = true;
            this.linkLabel1.Text = "WebSite";
            this.linkLabel1.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked);
            // 
            // linkLabel2
            // 
            this.linkLabel2.AutoSize = true;
            this.linkLabel2.Location = new System.Drawing.Point(283, 9);
            this.linkLabel2.Name = "linkLabel2";
            this.linkLabel2.Size = new System.Drawing.Size(73, 13);
            this.linkLabel2.TabIndex = 6;
            this.linkLabel2.TabStop = true;
            this.linkLabel2.Text = "Uo-Dev forum";
            this.linkLabel2.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel2_LinkClicked);
            // 
            // About
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(368, 179);
            this.Controls.Add(this.linkLabel2);
            this.Controls.Add(this.linkLabel1);
            this.Controls.Add(this.auth_lbl2);
            this.Controls.Add(this.changelog_lbl);
            this.Controls.Add(this.changelog_box);
            this.Controls.Add(this.Authors_lbl);
            this.Controls.Add(this.Close_btn);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "About";
            this.Text = "About";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button Close_btn;
        private System.Windows.Forms.Label Authors_lbl;
        private System.Windows.Forms.TextBox changelog_box;
        private System.Windows.Forms.Label changelog_lbl;
        private System.Windows.Forms.Label auth_lbl2;
        private System.Windows.Forms.LinkLabel linkLabel1;
        private System.Windows.Forms.LinkLabel linkLabel2;
    }
}