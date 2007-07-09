namespace UoKRUnpacker
{
    partial class Form2
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form2));
            this.lblUnpackerVersion = new System.Windows.Forms.Label();
            this.lblCopyright = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.linkSite = new System.Windows.Forms.LinkLabel();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnUpdates = new System.Windows.Forms.Button();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // lblUnpackerVersion
            // 
            this.lblUnpackerVersion.AutoSize = true;
            this.lblUnpackerVersion.Location = new System.Drawing.Point(65, 27);
            this.lblUnpackerVersion.Name = "lblUnpackerVersion";
            this.lblUnpackerVersion.Size = new System.Drawing.Size(42, 13);
            this.lblUnpackerVersion.TabIndex = 2;
            this.lblUnpackerVersion.Text = "Version";
            // 
            // lblCopyright
            // 
            this.lblCopyright.AutoSize = true;
            this.lblCopyright.Location = new System.Drawing.Point(65, 43);
            this.lblCopyright.Name = "lblCopyright";
            this.lblCopyright.Size = new System.Drawing.Size(190, 13);
            this.lblCopyright.TabIndex = 3;
            this.lblCopyright.Text = "Copyright © 2007 Two Unknown Guys";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(65, 72);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(215, 39);
            this.label3.TabIndex = 4;
            this.label3.Text = "You\'re free to use, decompile and whatever\r\nyou can think of this program.\r\nBased" +
                " on available docs by various authors.";
            // 
            // linkSite
            // 
            this.linkSite.AutoSize = true;
            this.linkSite.LinkBehavior = System.Windows.Forms.LinkBehavior.HoverUnderline;
            this.linkSite.Location = new System.Drawing.Point(65, 12);
            this.linkSite.Name = "linkSite";
            this.linkSite.Size = new System.Drawing.Size(104, 13);
            this.linkSite.TabIndex = 5;
            this.linkSite.TabStop = true;
            this.linkSite.Text = "UO:KR Uop Dumper";
            this.linkSite.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkSite_LinkClicked);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnOK.Location = new System.Drawing.Point(68, 124);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(68, 23);
            this.btnOK.TabIndex = 6;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            // 
            // btnUpdates
            // 
            this.btnUpdates.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnUpdates.Location = new System.Drawing.Point(161, 124);
            this.btnUpdates.Name = "btnUpdates";
            this.btnUpdates.Size = new System.Drawing.Size(110, 23);
            this.btnUpdates.TabIndex = 7;
            this.btnUpdates.Text = "Check for updates";
            this.btnUpdates.UseVisualStyleBackColor = true;
            this.btnUpdates.Click += new System.EventHandler(this.btnUpdates_Click);
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(12, 12);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(32, 32);
            this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.pictureBox1.TabIndex = 8;
            this.pictureBox1.TabStop = false;
            // 
            // Form2
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(297, 159);
            this.ControlBox = false;
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.btnUpdates);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.linkSite);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.lblCopyright);
            this.Controls.Add(this.lblUnpackerVersion);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "Form2";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About ...";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblUnpackerVersion;
        private System.Windows.Forms.Label lblCopyright;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.LinkLabel linkSite;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnUpdates;
        private System.Windows.Forms.PictureBox pictureBox1;
    }
}