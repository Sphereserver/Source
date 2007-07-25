namespace UoKRUnpacker
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
            this.oFileDlgUopopen = new System.Windows.Forms.OpenFileDialog();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.tvFileData = new System.Windows.Forms.TreeView();
            this.tsMainBar = new System.Windows.Forms.ToolStrip();
            this.tllblEmpty1 = new System.Windows.Forms.ToolStripLabel();
            this.toolBtnOpen = new System.Windows.Forms.ToolStripButton();
            this.toolBtnRefresh = new System.Windows.Forms.ToolStripButton();
            this.toolBtnSave = new System.Windows.Forms.ToolStripButton();
            this.tllblEmpty2 = new System.Windows.Forms.ToolStripLabel();
            this.toolBtnInfo = new System.Windows.Forms.ToolStripButton();
            this.lbIndexList = new System.Windows.Forms.ListBox();
            this.gbSelectedData = new System.Windows.Forms.GroupBox();
            this.tsMainBar.SuspendLayout();
            this.SuspendLayout();
            // 
            // oFileDlgUopopen
            // 
            this.oFileDlgUopopen.Filter = "UOKR Uop (*.uop)|*.uop";
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(12, 343);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox1.Size = new System.Drawing.Size(636, 78);
            this.textBox1.TabIndex = 0;
            // 
            // tvFileData
            // 
            this.tvFileData.Location = new System.Drawing.Point(12, 32);
            this.tvFileData.Name = "tvFileData";
            this.tvFileData.Size = new System.Drawing.Size(163, 305);
            this.tvFileData.TabIndex = 5;
            // 
            // tsMainBar
            // 
            this.tsMainBar.AutoSize = false;
            this.tsMainBar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.tsMainBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tllblEmpty1,
            this.toolBtnOpen,
            this.toolBtnSave,
            this.tllblEmpty2,
            this.toolBtnInfo,
            this.toolBtnRefresh});
            this.tsMainBar.Location = new System.Drawing.Point(0, 0);
            this.tsMainBar.Name = "tsMainBar";
            this.tsMainBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.tsMainBar.Size = new System.Drawing.Size(660, 29);
            this.tsMainBar.TabIndex = 6;
            // 
            // tllblEmpty1
            // 
            this.tllblEmpty1.Name = "tllblEmpty1";
            this.tllblEmpty1.Size = new System.Drawing.Size(13, 26);
            this.tllblEmpty1.Text = "  ";
            // 
            // toolBtnOpen
            // 
            this.toolBtnOpen.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnOpen.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnOpen.Image")));
            this.toolBtnOpen.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnOpen.Name = "toolBtnOpen";
            this.toolBtnOpen.Size = new System.Drawing.Size(23, 26);
            this.toolBtnOpen.Text = "Open";
            this.toolBtnOpen.Click += new System.EventHandler(this.toolBtnOpen_Click);
            // 
            // toolBtnRefresh
            // 
            this.toolBtnRefresh.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnRefresh.Enabled = false;
            this.toolBtnRefresh.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnRefresh.Image")));
            this.toolBtnRefresh.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnRefresh.Name = "toolBtnRefresh";
            this.toolBtnRefresh.Size = new System.Drawing.Size(23, 26);
            this.toolBtnRefresh.Text = "Refresh";
            this.toolBtnRefresh.Click += new System.EventHandler(this.toolBtnRefresh_Click);
            // 
            // toolBtnSave
            // 
            this.toolBtnSave.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnSave.Enabled = false;
            this.toolBtnSave.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnSave.Image")));
            this.toolBtnSave.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnSave.Name = "toolBtnSave";
            this.toolBtnSave.Size = new System.Drawing.Size(23, 26);
            this.toolBtnSave.Text = "Save";
            this.toolBtnSave.Click += new System.EventHandler(this.toolBtnSave_Click);
            // 
            // tllblEmpty2
            // 
            this.tllblEmpty2.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.tllblEmpty2.Name = "tllblEmpty2";
            this.tllblEmpty2.Size = new System.Drawing.Size(13, 26);
            this.tllblEmpty2.Text = "  ";
            // 
            // toolBtnInfo
            // 
            this.toolBtnInfo.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolBtnInfo.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnInfo.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnInfo.Image")));
            this.toolBtnInfo.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnInfo.Name = "toolBtnInfo";
            this.toolBtnInfo.Size = new System.Drawing.Size(23, 26);
            this.toolBtnInfo.Text = "Info";
            this.toolBtnInfo.Click += new System.EventHandler(this.toolBtnInfo_Click);
            // 
            // lbIndexList
            // 
            this.lbIndexList.FormattingEnabled = true;
            this.lbIndexList.Location = new System.Drawing.Point(181, 32);
            this.lbIndexList.Name = "lbIndexList";
            this.lbIndexList.Size = new System.Drawing.Size(160, 303);
            this.lbIndexList.TabIndex = 7;
            // 
            // gbSelectedData
            // 
            this.gbSelectedData.Location = new System.Drawing.Point(347, 33);
            this.gbSelectedData.Name = "gbSelectedData";
            this.gbSelectedData.Size = new System.Drawing.Size(301, 304);
            this.gbSelectedData.TabIndex = 8;
            this.gbSelectedData.TabStop = false;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(660, 433);
            this.Controls.Add(this.gbSelectedData);
            this.Controls.Add(this.lbIndexList);
            this.Controls.Add(this.tsMainBar);
            this.Controls.Add(this.tvFileData);
            this.Controls.Add(this.textBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "UO:KR Uop Dumper";
            this.tsMainBar.ResumeLayout(false);
            this.tsMainBar.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.OpenFileDialog oFileDlgUopopen;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.TreeView tvFileData;
        private System.Windows.Forms.ToolStrip tsMainBar;
        private System.Windows.Forms.ToolStripButton toolBtnOpen;
        private System.Windows.Forms.ToolStripButton toolBtnRefresh;
        private System.Windows.Forms.ToolStripLabel tllblEmpty1;
        private System.Windows.Forms.ListBox lbIndexList;
        private System.Windows.Forms.GroupBox gbSelectedData;
        private System.Windows.Forms.ToolStripButton toolBtnSave;
        private System.Windows.Forms.ToolStripLabel tllblEmpty2;
        private System.Windows.Forms.ToolStripButton toolBtnInfo;
    }
}

