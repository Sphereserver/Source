namespace UoKRLoader
{
    partial class Form1
    {
        /// <summary>
        /// Variabile di progettazione necessaria.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Liberare le risorse in uso.
        /// </summary>
        /// <param name="disposing">ha valore true se le risorse gestite devono essere eliminate, false in caso contrario.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Codice generato da Progettazione Windows Form

        /// <summary>
        /// Metodo necessario per il supporto della finestra di progettazione. Non modificare
        /// il contenuto del metodo con l'editor di codice.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.txtUokrPath = new System.Windows.Forms.TextBox();
            this.lblClient = new System.Windows.Forms.Label();
            this.panel1 = new System.Windows.Forms.Panel();
            this.btnAbout = new System.Windows.Forms.Button();
            this.btnPatch = new System.Windows.Forms.Button();
            this.cmbEncryption = new System.Windows.Forms.ComboBox();
            this.lblEnc = new System.Windows.Forms.Label();
            this.btnLaunch = new System.Windows.Forms.Button();
            this.ckbRemind = new System.Windows.Forms.CheckBox();
            this.nudPort = new System.Windows.Forms.NumericUpDown();
            this.lblPort = new System.Windows.Forms.Label();
            this.lblIP = new System.Windows.Forms.Label();
            this.txtIptopatch = new System.Windows.Forms.TextBox();
            this.ofdUOKRClient = new System.Windows.Forms.OpenFileDialog();
            this.lnkOpen = new System.Windows.Forms.LinkLabel();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudPort)).BeginInit();
            this.SuspendLayout();
            // 
            // txtUokrPath
            // 
            this.txtUokrPath.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.txtUokrPath.Location = new System.Drawing.Point(12, 25);
            this.txtUokrPath.Name = "txtUokrPath";
            this.txtUokrPath.ReadOnly = true;
            this.txtUokrPath.Size = new System.Drawing.Size(445, 20);
            this.txtUokrPath.TabIndex = 0;
            this.txtUokrPath.TabStop = false;
            this.txtUokrPath.WordWrap = false;
            // 
            // lblClient
            // 
            this.lblClient.AutoSize = true;
            this.lblClient.Location = new System.Drawing.Point(9, 9);
            this.lblClient.Name = "lblClient";
            this.lblClient.Size = new System.Drawing.Size(100, 13);
            this.lblClient.TabIndex = 100;
            this.lblClient.Text = "UO:KR Client path: ";
            // 
            // panel1
            // 
            this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panel1.Controls.Add(this.btnAbout);
            this.panel1.Controls.Add(this.btnPatch);
            this.panel1.Controls.Add(this.cmbEncryption);
            this.panel1.Controls.Add(this.lblEnc);
            this.panel1.Controls.Add(this.btnLaunch);
            this.panel1.Controls.Add(this.ckbRemind);
            this.panel1.Controls.Add(this.nudPort);
            this.panel1.Controls.Add(this.lblPort);
            this.panel1.Controls.Add(this.lblIP);
            this.panel1.Controls.Add(this.txtIptopatch);
            this.panel1.Location = new System.Drawing.Point(12, 51);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(445, 115);
            this.panel1.TabIndex = 101;
            // 
            // btnAbout
            // 
            this.btnAbout.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnAbout.Location = new System.Drawing.Point(407, 78);
            this.btnAbout.Name = "btnAbout";
            this.btnAbout.Size = new System.Drawing.Size(25, 25);
            this.btnAbout.TabIndex = 6;
            this.btnAbout.TabStop = false;
            this.btnAbout.Text = "?";
            this.btnAbout.UseVisualStyleBackColor = true;
            this.btnAbout.Click += new System.EventHandler(this.btnAbout_Click);
            // 
            // btnPatch
            // 
            this.btnPatch.Location = new System.Drawing.Point(336, 44);
            this.btnPatch.Name = "btnPatch";
            this.btnPatch.Size = new System.Drawing.Size(96, 28);
            this.btnPatch.TabIndex = 5;
            this.btnPatch.Text = "Pa&tch  ...";
            this.btnPatch.UseVisualStyleBackColor = true;
            this.btnPatch.Click += new System.EventHandler(this.btnPatch_Click);
            // 
            // cmbEncryption
            // 
            this.cmbEncryption.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbEncryption.FormattingEnabled = true;
            this.cmbEncryption.Location = new System.Drawing.Point(81, 74);
            this.cmbEncryption.Name = "cmbEncryption";
            this.cmbEncryption.Size = new System.Drawing.Size(102, 21);
            this.cmbEncryption.TabIndex = 2;
            // 
            // lblEnc
            // 
            this.lblEnc.AutoSize = true;
            this.lblEnc.Location = new System.Drawing.Point(15, 74);
            this.lblEnc.Name = "lblEnc";
            this.lblEnc.Size = new System.Drawing.Size(60, 13);
            this.lblEnc.TabIndex = 104;
            this.lblEnc.Text = "Encryption:";
            // 
            // btnLaunch
            // 
            this.btnLaunch.Location = new System.Drawing.Point(336, 10);
            this.btnLaunch.Name = "btnLaunch";
            this.btnLaunch.Size = new System.Drawing.Size(96, 28);
            this.btnLaunch.TabIndex = 4;
            this.btnLaunch.Text = "&Launch";
            this.btnLaunch.UseVisualStyleBackColor = true;
            this.btnLaunch.Click += new System.EventHandler(this.btnLaunch_Click);
            // 
            // ckbRemind
            // 
            this.ckbRemind.Location = new System.Drawing.Point(234, 78);
            this.ckbRemind.Name = "ckbRemind";
            this.ckbRemind.Size = new System.Drawing.Size(85, 17);
            this.ckbRemind.TabIndex = 3;
            this.ckbRemind.Text = "Remind it";
            this.ckbRemind.UseVisualStyleBackColor = true;
            // 
            // nudPort
            // 
            this.nudPort.Location = new System.Drawing.Point(81, 43);
            this.nudPort.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.nudPort.Name = "nudPort";
            this.nudPort.Size = new System.Drawing.Size(102, 20);
            this.nudPort.TabIndex = 1;
            this.nudPort.Value = new decimal(new int[] {
            2593,
            0,
            0,
            0});
            // 
            // lblPort
            // 
            this.lblPort.AutoSize = true;
            this.lblPort.Location = new System.Drawing.Point(15, 43);
            this.lblPort.Name = "lblPort";
            this.lblPort.Size = new System.Drawing.Size(29, 13);
            this.lblPort.TabIndex = 103;
            this.lblPort.Text = "Port:";
            // 
            // lblIP
            // 
            this.lblIP.AutoSize = true;
            this.lblIP.Location = new System.Drawing.Point(15, 14);
            this.lblIP.Name = "lblIP";
            this.lblIP.Size = new System.Drawing.Size(20, 13);
            this.lblIP.TabIndex = 102;
            this.lblIP.Text = "IP:";
            // 
            // txtIptopatch
            // 
            this.txtIptopatch.Location = new System.Drawing.Point(81, 14);
            this.txtIptopatch.MaxLength = 255;
            this.txtIptopatch.Name = "txtIptopatch";
            this.txtIptopatch.Size = new System.Drawing.Size(238, 20);
            this.txtIptopatch.TabIndex = 0;
            this.txtIptopatch.Text = "127.0.0.1";
            // 
            // ofdUOKRClient
            // 
            this.ofdUOKRClient.FileName = "openFileDialog1";
            this.ofdUOKRClient.Filter = "UO:KR Client|uokr.exe";
            // 
            // lnkOpen
            // 
            this.lnkOpen.AutoSize = true;
            this.lnkOpen.Location = new System.Drawing.Point(366, 9);
            this.lnkOpen.Name = "lnkOpen";
            this.lnkOpen.Size = new System.Drawing.Size(93, 13);
            this.lnkOpen.TabIndex = 10;
            this.lnkOpen.TabStop = true;
            this.lnkOpen.Text = "Select a new path";
            this.lnkOpen.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.lnkOpen_LinkClicked);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(469, 178);
            this.Controls.Add(this.lnkOpen);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.lblClient);
            this.Controls.Add(this.txtUokrPath);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "UO:KR Client Loader";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nudPort)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox txtUokrPath;
        private System.Windows.Forms.Label lblClient;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.TextBox txtIptopatch;
        private System.Windows.Forms.Label lblIP;
        private System.Windows.Forms.NumericUpDown nudPort;
        private System.Windows.Forms.Label lblPort;
        private System.Windows.Forms.Button btnLaunch;
        private System.Windows.Forms.CheckBox ckbRemind;
        private System.Windows.Forms.OpenFileDialog ofdUOKRClient;
        private System.Windows.Forms.ComboBox cmbEncryption;
        private System.Windows.Forms.Label lblEnc;
        private System.Windows.Forms.Button btnPatch;
        private System.Windows.Forms.Button btnAbout;
        private System.Windows.Forms.LinkLabel lnkOpen;
    }
}

