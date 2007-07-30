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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.oFileDlgUopopen = new System.Windows.Forms.OpenFileDialog();
            this.tvFileData = new System.Windows.Forms.TreeView();
            this.tsMainBar = new System.Windows.Forms.ToolStrip();
            this.tllblEmpty1 = new System.Windows.Forms.ToolStripLabel();
            this.toolBtnOpen = new System.Windows.Forms.ToolStripButton();
            this.toolBtnRefresh = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.toolBtnSave = new System.Windows.Forms.ToolStripButton();
            this.tllblEmpty2 = new System.Windows.Forms.ToolStripLabel();
            this.toolBtnInfo = new System.Windows.Forms.ToolStripButton();
            this.toolBtnSaveAs = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.toolBtnDump = new System.Windows.Forms.ToolStripButton();
            this.toolBtnUnpack = new System.Windows.Forms.ToolStripButton();
            this.toolBtnHelp = new System.Windows.Forms.ToolStripButton();
            this.toolBtnClose = new System.Windows.Forms.ToolStripButton();
            this.gbSelectedData = new System.Windows.Forms.GroupBox();
            this.lbIndexList = new System.Windows.Forms.ListBox();
            this.ctxMenuNode = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.dumpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.moveUpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.moveDownToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ssStatusBar = new System.Windows.Forms.StatusStrip();
            this.tslblStatus = new System.Windows.Forms.ToolStripStatusLabel();
            this.oFileDlgUopsave = new System.Windows.Forms.SaveFileDialog();
            this.tslblWorking = new System.Windows.Forms.ToolStripStatusLabel();
            this.tslblEmpty = new System.Windows.Forms.ToolStripStatusLabel();
            this.tsMainBar.SuspendLayout();
            this.ctxMenuNode.SuspendLayout();
            this.ssStatusBar.SuspendLayout();
            this.SuspendLayout();
            // 
            // oFileDlgUopopen
            // 
            this.oFileDlgUopopen.Filter = "UOKR Uop (*.uop)|*.uop";
            // 
            // tvFileData
            // 
            this.tvFileData.Location = new System.Drawing.Point(12, 34);
            this.tvFileData.Name = "tvFileData";
            this.tvFileData.Size = new System.Drawing.Size(163, 368);
            this.tvFileData.TabIndex = 5;
            this.tvFileData.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvFileData_AfterSelect);
            // 
            // tsMainBar
            // 
            this.tsMainBar.AutoSize = false;
            this.tsMainBar.BackColor = System.Drawing.SystemColors.Control;
            this.tsMainBar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.tsMainBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tllblEmpty1,
            this.toolBtnOpen,
            this.toolBtnRefresh,
            this.toolStripSeparator1,
            this.toolBtnSave,
            this.tllblEmpty2,
            this.toolBtnInfo,
            this.toolBtnSaveAs,
            this.toolStripSeparator2,
            this.toolBtnDump,
            this.toolBtnUnpack,
            this.toolBtnHelp,
            this.toolBtnClose});
            this.tsMainBar.Location = new System.Drawing.Point(0, 0);
            this.tsMainBar.Name = "tsMainBar";
            this.tsMainBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.tsMainBar.Size = new System.Drawing.Size(660, 31);
            this.tsMainBar.TabIndex = 6;
            // 
            // tllblEmpty1
            // 
            this.tllblEmpty1.Name = "tllblEmpty1";
            this.tllblEmpty1.Size = new System.Drawing.Size(13, 28);
            this.tllblEmpty1.Text = "  ";
            // 
            // toolBtnOpen
            // 
            this.toolBtnOpen.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnOpen.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnOpen.Image")));
            this.toolBtnOpen.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnOpen.Name = "toolBtnOpen";
            this.toolBtnOpen.Size = new System.Drawing.Size(23, 28);
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
            this.toolBtnRefresh.Size = new System.Drawing.Size(23, 28);
            this.toolBtnRefresh.Text = "Refresh";
            this.toolBtnRefresh.Click += new System.EventHandler(this.toolBtnRefresh_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 31);
            // 
            // toolBtnSave
            // 
            this.toolBtnSave.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnSave.Enabled = false;
            this.toolBtnSave.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnSave.Image")));
            this.toolBtnSave.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnSave.Name = "toolBtnSave";
            this.toolBtnSave.Size = new System.Drawing.Size(23, 28);
            this.toolBtnSave.Text = "Save";
            this.toolBtnSave.ToolTipText = "Save changes to the current UOP";
            this.toolBtnSave.Click += new System.EventHandler(this.toolBtnSave_Click);
            // 
            // tllblEmpty2
            // 
            this.tllblEmpty2.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.tllblEmpty2.Name = "tllblEmpty2";
            this.tllblEmpty2.Size = new System.Drawing.Size(13, 28);
            this.tllblEmpty2.Text = "  ";
            // 
            // toolBtnInfo
            // 
            this.toolBtnInfo.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolBtnInfo.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnInfo.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnInfo.Image")));
            this.toolBtnInfo.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnInfo.Name = "toolBtnInfo";
            this.toolBtnInfo.Size = new System.Drawing.Size(23, 28);
            this.toolBtnInfo.Text = "Info";
            this.toolBtnInfo.Click += new System.EventHandler(this.toolBtnInfo_Click);
            // 
            // toolBtnSaveAs
            // 
            this.toolBtnSaveAs.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnSaveAs.Enabled = false;
            this.toolBtnSaveAs.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnSaveAs.Image")));
            this.toolBtnSaveAs.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnSaveAs.Name = "toolBtnSaveAs";
            this.toolBtnSaveAs.Size = new System.Drawing.Size(23, 28);
            this.toolBtnSaveAs.Text = "Save As ...";
            this.toolBtnSaveAs.Click += new System.EventHandler(this.toolBtnSaveAs_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 31);
            // 
            // toolBtnDump
            // 
            this.toolBtnDump.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnDump.Enabled = false;
            this.toolBtnDump.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnDump.Image")));
            this.toolBtnDump.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnDump.Name = "toolBtnDump";
            this.toolBtnDump.Size = new System.Drawing.Size(23, 28);
            this.toolBtnDump.Text = "Dump";
            this.toolBtnDump.ToolTipText = "Dump information";
            this.toolBtnDump.Click += new System.EventHandler(this.toolBtnDump_Click);
            // 
            // toolBtnUnpack
            // 
            this.toolBtnUnpack.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnUnpack.Enabled = false;
            this.toolBtnUnpack.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnUnpack.Image")));
            this.toolBtnUnpack.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnUnpack.Name = "toolBtnUnpack";
            this.toolBtnUnpack.Size = new System.Drawing.Size(23, 28);
            this.toolBtnUnpack.Text = "Unpack";
            this.toolBtnUnpack.Click += new System.EventHandler(this.toolBtnUnpack_Click);
            // 
            // toolBtnHelp
            // 
            this.toolBtnHelp.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolBtnHelp.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnHelp.Enabled = false;
            this.toolBtnHelp.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnHelp.Image")));
            this.toolBtnHelp.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnHelp.Name = "toolBtnHelp";
            this.toolBtnHelp.Size = new System.Drawing.Size(23, 28);
            this.toolBtnHelp.Text = "Help";
            this.toolBtnHelp.Click += new System.EventHandler(this.toolBtnHelp_Click);
            // 
            // toolBtnClose
            // 
            this.toolBtnClose.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolBtnClose.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnClose.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnClose.Image")));
            this.toolBtnClose.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnClose.Name = "toolBtnClose";
            this.toolBtnClose.Size = new System.Drawing.Size(23, 28);
            this.toolBtnClose.Text = "Close";
            this.toolBtnClose.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageAboveText;
            this.toolBtnClose.Click += new System.EventHandler(this.toolBtnClose_Click);
            // 
            // gbSelectedData
            // 
            this.gbSelectedData.Location = new System.Drawing.Point(347, 32);
            this.gbSelectedData.Name = "gbSelectedData";
            this.gbSelectedData.Size = new System.Drawing.Size(301, 370);
            this.gbSelectedData.TabIndex = 8;
            this.gbSelectedData.TabStop = false;
            this.gbSelectedData.Text = "Details";
            // 
            // lbIndexList
            // 
            this.lbIndexList.FormattingEnabled = true;
            this.lbIndexList.Location = new System.Drawing.Point(181, 34);
            this.lbIndexList.Name = "lbIndexList";
            this.lbIndexList.Size = new System.Drawing.Size(160, 368);
            this.lbIndexList.TabIndex = 7;
            this.lbIndexList.SelectedIndexChanged += new System.EventHandler(this.lbIndexList_SelectedIndexChanged);
            // 
            // ctxMenuNode
            // 
            this.ctxMenuNode.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.deleteToolStripMenuItem,
            this.dumpToolStripMenuItem,
            this.moveUpToolStripMenuItem,
            this.moveDownToolStripMenuItem});
            this.ctxMenuNode.Name = "ctxMenuNode";
            this.ctxMenuNode.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.ctxMenuNode.ShowItemToolTips = false;
            this.ctxMenuNode.Size = new System.Drawing.Size(132, 92);
            // 
            // deleteToolStripMenuItem
            // 
            this.deleteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem.Image")));
            this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
            this.deleteToolStripMenuItem.Size = new System.Drawing.Size(131, 22);
            this.deleteToolStripMenuItem.Text = "Delete";
            this.deleteToolStripMenuItem.Click += new System.EventHandler(this.deleteToolStripMenuItem_Click);
            // 
            // dumpToolStripMenuItem
            // 
            this.dumpToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("dumpToolStripMenuItem.Image")));
            this.dumpToolStripMenuItem.Name = "dumpToolStripMenuItem";
            this.dumpToolStripMenuItem.Size = new System.Drawing.Size(131, 22);
            this.dumpToolStripMenuItem.Text = "Dump To ...";
            this.dumpToolStripMenuItem.Click += new System.EventHandler(this.dumpToolStripMenuItem_Click);
            // 
            // moveUpToolStripMenuItem
            // 
            this.moveUpToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("moveUpToolStripMenuItem.Image")));
            this.moveUpToolStripMenuItem.Name = "moveUpToolStripMenuItem";
            this.moveUpToolStripMenuItem.Size = new System.Drawing.Size(131, 22);
            this.moveUpToolStripMenuItem.Text = "Move Up";
            this.moveUpToolStripMenuItem.Click += new System.EventHandler(this.moveUpToolStripMenuItem_Click);
            // 
            // moveDownToolStripMenuItem
            // 
            this.moveDownToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("moveDownToolStripMenuItem.Image")));
            this.moveDownToolStripMenuItem.Name = "moveDownToolStripMenuItem";
            this.moveDownToolStripMenuItem.Size = new System.Drawing.Size(131, 22);
            this.moveDownToolStripMenuItem.Text = "Move Down";
            this.moveDownToolStripMenuItem.Click += new System.EventHandler(this.moveDownToolStripMenuItem_Click);
            // 
            // ssStatusBar
            // 
            this.ssStatusBar.BackColor = System.Drawing.SystemColors.Control;
            this.ssStatusBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tslblStatus,
            this.tslblEmpty,
            this.tslblWorking});
            this.ssStatusBar.Location = new System.Drawing.Point(0, 408);
            this.ssStatusBar.Name = "ssStatusBar";
            this.ssStatusBar.Size = new System.Drawing.Size(660, 25);
            this.ssStatusBar.SizingGrip = false;
            this.ssStatusBar.TabIndex = 9;
            this.ssStatusBar.Text = "Status Bar";
            // 
            // tslblStatus
            // 
            this.tslblStatus.AutoSize = false;
            this.tslblStatus.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top)
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right)
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this.tslblStatus.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.tslblStatus.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.tslblStatus.Name = "tslblStatus";
            this.tslblStatus.Size = new System.Drawing.Size(400, 20);
            this.tslblStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // oFileDlgUopsave
            // 
            this.oFileDlgUopsave.Filter = "UOKR Uop (*.uop)|*.uop";
            // 
            // tslblWorking
            // 
            this.tslblWorking.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.tslblWorking.Image = ((System.Drawing.Image)(resources.GetObject("tslblWorking.Image")));
            this.tslblWorking.Name = "tslblWorking";
            this.tslblWorking.Size = new System.Drawing.Size(16, 20);
            this.tslblWorking.Visible = false;
            // 
            // tslblEmpty
            // 
            this.tslblEmpty.AutoSize = false;
            this.tslblEmpty.Name = "tslblEmpty";
            this.tslblEmpty.Size = new System.Drawing.Size(229, 20);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(660, 433);
            this.Controls.Add(this.gbSelectedData);
            this.Controls.Add(this.tsMainBar);
            this.Controls.Add(this.lbIndexList);
            this.Controls.Add(this.tvFileData);
            this.Controls.Add(this.ssStatusBar);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "UO:KR Uop Dumper";
            this.tsMainBar.ResumeLayout(false);
            this.tsMainBar.PerformLayout();
            this.ctxMenuNode.ResumeLayout(false);
            this.ssStatusBar.ResumeLayout(false);
            this.ssStatusBar.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.OpenFileDialog oFileDlgUopopen;
        private System.Windows.Forms.TreeView tvFileData;
        private System.Windows.Forms.ToolStrip tsMainBar;
        private System.Windows.Forms.ToolStripButton toolBtnOpen;
        private System.Windows.Forms.ToolStripButton toolBtnRefresh;
        private System.Windows.Forms.ToolStripLabel tllblEmpty1;
        private System.Windows.Forms.GroupBox gbSelectedData;
        private System.Windows.Forms.ToolStripButton toolBtnSave;
        private System.Windows.Forms.ToolStripLabel tllblEmpty2;
        private System.Windows.Forms.ToolStripButton toolBtnInfo;
        private System.Windows.Forms.ListBox lbIndexList;
        private System.Windows.Forms.ToolStripButton toolBtnClose;
        private System.Windows.Forms.ContextMenuStrip ctxMenuNode;
        private System.Windows.Forms.ToolStripMenuItem deleteToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem dumpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem moveUpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem moveDownToolStripMenuItem;
        private System.Windows.Forms.ToolStripButton toolBtnDump;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton toolBtnSaveAs;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripButton toolBtnUnpack;
        private System.Windows.Forms.ToolStripButton toolBtnHelp;
        private System.Windows.Forms.StatusStrip ssStatusBar;
        private System.Windows.Forms.ToolStripStatusLabel tslblStatus;
        private System.Windows.Forms.SaveFileDialog oFileDlgUopsave;
        private System.Windows.Forms.ToolStripStatusLabel tslblWorking;
        private System.Windows.Forms.ToolStripStatusLabel tslblEmpty;
    }
}

