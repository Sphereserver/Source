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
            this.toolBtnDontFix = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.toolBtnDump = new System.Windows.Forms.ToolStripButton();
            this.toolBtnUnpack = new System.Windows.Forms.ToolStripButton();
            this.toolBtnHelp = new System.Windows.Forms.ToolStripButton();
            this.toolBtnClose = new System.Windows.Forms.ToolStripButton();
            this.gbSelectedData = new System.Windows.Forms.GroupBox();
            this.pnUopDatafile = new System.Windows.Forms.Panel();
            this.btn_pnUopDatafile_RecountFiles = new System.Windows.Forms.LinkLabel();
            this.nud_pnUopDatafile_Files = new System.Windows.Forms.NumericUpDown();
            this.lbl_pnUopDatafile_Files = new System.Windows.Forms.Label();
            this.txt_pnUopDatafile_Offset = new System.Windows.Forms.TextBox();
            this.lbl_pnUopDatafile_Offset = new System.Windows.Forms.Label();
            this.btnDetailsUnpack = new System.Windows.Forms.Button();
            this.btnDetailsDelete = new System.Windows.Forms.Button();
            this.btnDetailsApply = new System.Windows.Forms.Button();
            this.btnDetailsModify = new System.Windows.Forms.Button();
            this.pnUopfile = new System.Windows.Forms.Panel();
            this.btn_pnlUopFile_RecountFiles = new System.Windows.Forms.LinkLabel();
            this.label3 = new System.Windows.Forms.Label();
            this.lblFiles = new System.Windows.Forms.Label();
            this.lblManyData = new System.Windows.Forms.Label();
            this.txt_pnlUopFile_Header2 = new System.Windows.Forms.TextBox();
            this.num_pnlUopFile_Files = new System.Windows.Forms.NumericUpDown();
            this.txt_pnlUopFile_Header1 = new System.Windows.Forms.TextBox();
            this.lbIndexList = new System.Windows.Forms.ListBox();
            this.ctxMenuNode = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.moveUpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.moveDownToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ssStatusBar = new System.Windows.Forms.StatusStrip();
            this.tslblStatus = new System.Windows.Forms.ToolStripStatusLabel();
            this.tslblWorking = new System.Windows.Forms.ToolStripStatusLabel();
            this.oFileDlgUopsave = new System.Windows.Forms.SaveFileDialog();
            this.pnUopHeaderAndData = new System.Windows.Forms.Panel();
            this.tsMainBar.SuspendLayout();
            this.gbSelectedData.SuspendLayout();
            this.pnUopDatafile.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nud_pnUopDatafile_Files)).BeginInit();
            this.pnUopfile.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.num_pnlUopFile_Files)).BeginInit();
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
            this.toolBtnDontFix,
            this.toolStripSeparator2,
            this.toolBtnDump,
            this.toolBtnUnpack,
            this.toolBtnHelp,
            this.toolBtnClose});
            this.tsMainBar.Location = new System.Drawing.Point(0, 0);
            this.tsMainBar.Name = "tsMainBar";
            this.tsMainBar.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.tsMainBar.Size = new System.Drawing.Size(672, 31);
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
            // toolBtnDontFix
            // 
            this.toolBtnDontFix.BackColor = System.Drawing.SystemColors.Control;
            this.toolBtnDontFix.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.toolBtnDontFix.Enabled = false;
            this.toolBtnDontFix.Image = ((System.Drawing.Image)(resources.GetObject("toolBtnDontFix.Image")));
            this.toolBtnDontFix.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolBtnDontFix.Name = "toolBtnDontFix";
            this.toolBtnDontFix.Size = new System.Drawing.Size(23, 28);
            this.toolBtnDontFix.Text = "Don\'t Fix Offsets";
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
            this.gbSelectedData.Controls.Add(this.pnUopHeaderAndData);
            this.gbSelectedData.Controls.Add(this.pnUopDatafile);
            this.gbSelectedData.Controls.Add(this.btnDetailsUnpack);
            this.gbSelectedData.Controls.Add(this.btnDetailsDelete);
            this.gbSelectedData.Controls.Add(this.btnDetailsApply);
            this.gbSelectedData.Controls.Add(this.btnDetailsModify);
            this.gbSelectedData.Controls.Add(this.pnUopfile);
            this.gbSelectedData.Location = new System.Drawing.Point(347, 32);
            this.gbSelectedData.Name = "gbSelectedData";
            this.gbSelectedData.Size = new System.Drawing.Size(313, 370);
            this.gbSelectedData.TabIndex = 8;
            this.gbSelectedData.TabStop = false;
            this.gbSelectedData.Text = "Details";
            // 
            // pnUopDatafile
            // 
            this.pnUopDatafile.Controls.Add(this.btn_pnUopDatafile_RecountFiles);
            this.pnUopDatafile.Controls.Add(this.nud_pnUopDatafile_Files);
            this.pnUopDatafile.Controls.Add(this.lbl_pnUopDatafile_Files);
            this.pnUopDatafile.Controls.Add(this.txt_pnUopDatafile_Offset);
            this.pnUopDatafile.Controls.Add(this.lbl_pnUopDatafile_Offset);
            this.pnUopDatafile.Enabled = false;
            this.pnUopDatafile.Location = new System.Drawing.Point(6, 18);
            this.pnUopDatafile.Name = "pnUopDatafile";
            this.pnUopDatafile.Size = new System.Drawing.Size(300, 313);
            this.pnUopDatafile.TabIndex = 5;
            this.pnUopDatafile.Visible = false;
            // 
            // btn_pnUopDatafile_RecountFiles
            // 
            this.btn_pnUopDatafile_RecountFiles.AutoSize = true;
            this.btn_pnUopDatafile_RecountFiles.Location = new System.Drawing.Point(74, 165);
            this.btn_pnUopDatafile_RecountFiles.Name = "btn_pnUopDatafile_RecountFiles";
            this.btn_pnUopDatafile_RecountFiles.Size = new System.Drawing.Size(72, 13);
            this.btn_pnUopDatafile_RecountFiles.TabIndex = 4;
            this.btn_pnUopDatafile_RecountFiles.TabStop = true;
            this.btn_pnUopDatafile_RecountFiles.Text = "Recount Files";
            // 
            // nud_pnUopDatafile_Files
            // 
            this.nud_pnUopDatafile_Files.Location = new System.Drawing.Point(12, 163);
            this.nud_pnUopDatafile_Files.Name = "nud_pnUopDatafile_Files";
            this.nud_pnUopDatafile_Files.Size = new System.Drawing.Size(56, 20);
            this.nud_pnUopDatafile_Files.TabIndex = 3;
            // 
            // lbl_pnUopDatafile_Files
            // 
            this.lbl_pnUopDatafile_Files.AutoSize = true;
            this.lbl_pnUopDatafile_Files.Location = new System.Drawing.Point(9, 147);
            this.lbl_pnUopDatafile_Files.Name = "lbl_pnUopDatafile_Files";
            this.lbl_pnUopDatafile_Files.Size = new System.Drawing.Size(57, 13);
            this.lbl_pnUopDatafile_Files.TabIndex = 2;
            this.lbl_pnUopDatafile_Files.Text = "File Count:";
            // 
            // txt_pnUopDatafile_Offset
            // 
            this.txt_pnUopDatafile_Offset.Location = new System.Drawing.Point(12, 69);
            this.txt_pnUopDatafile_Offset.Name = "txt_pnUopDatafile_Offset";
            this.txt_pnUopDatafile_Offset.Size = new System.Drawing.Size(148, 20);
            this.txt_pnUopDatafile_Offset.TabIndex = 1;
            // 
            // lbl_pnUopDatafile_Offset
            // 
            this.lbl_pnUopDatafile_Offset.AutoSize = true;
            this.lbl_pnUopDatafile_Offset.Location = new System.Drawing.Point(9, 53);
            this.lbl_pnUopDatafile_Offset.Name = "lbl_pnUopDatafile_Offset";
            this.lbl_pnUopDatafile_Offset.Size = new System.Drawing.Size(63, 13);
            this.lbl_pnUopDatafile_Offset.TabIndex = 0;
            this.lbl_pnUopDatafile_Offset.Text = "Next Offset:";
            // 
            // btnDetailsUnpack
            // 
            this.btnDetailsUnpack.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnDetailsUnpack.Image = ((System.Drawing.Image)(resources.GetObject("btnDetailsUnpack.Image")));
            this.btnDetailsUnpack.Location = new System.Drawing.Point(6, 337);
            this.btnDetailsUnpack.Name = "btnDetailsUnpack";
            this.btnDetailsUnpack.Size = new System.Drawing.Size(79, 26);
            this.btnDetailsUnpack.TabIndex = 4;
            this.btnDetailsUnpack.Text = "Unpack";
            this.btnDetailsUnpack.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDetailsUnpack.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDetailsUnpack.UseVisualStyleBackColor = true;
            this.btnDetailsUnpack.Click += new System.EventHandler(this.btnDetailsUnpack_Click);
            // 
            // btnDetailsDelete
            // 
            this.btnDetailsDelete.Enabled = false;
            this.btnDetailsDelete.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnDetailsDelete.Image = ((System.Drawing.Image)(resources.GetObject("btnDetailsDelete.Image")));
            this.btnDetailsDelete.Location = new System.Drawing.Point(243, 337);
            this.btnDetailsDelete.Name = "btnDetailsDelete";
            this.btnDetailsDelete.Size = new System.Drawing.Size(64, 26);
            this.btnDetailsDelete.TabIndex = 3;
            this.btnDetailsDelete.Text = "Delete";
            this.btnDetailsDelete.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDetailsDelete.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDetailsDelete.UseVisualStyleBackColor = true;
            this.btnDetailsDelete.Click += new System.EventHandler(this.btnDetailsDelete_Click);
            // 
            // btnDetailsApply
            // 
            this.btnDetailsApply.Enabled = false;
            this.btnDetailsApply.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnDetailsApply.Image = ((System.Drawing.Image)(resources.GetObject("btnDetailsApply.Image")));
            this.btnDetailsApply.Location = new System.Drawing.Point(169, 337);
            this.btnDetailsApply.Name = "btnDetailsApply";
            this.btnDetailsApply.Size = new System.Drawing.Size(68, 26);
            this.btnDetailsApply.TabIndex = 2;
            this.btnDetailsApply.Text = "Apply";
            this.btnDetailsApply.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDetailsApply.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDetailsApply.UseVisualStyleBackColor = true;
            this.btnDetailsApply.Click += new System.EventHandler(this.btnDetailsApply_Click);
            // 
            // btnDetailsModify
            // 
            this.btnDetailsModify.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnDetailsModify.Image = ((System.Drawing.Image)(resources.GetObject("btnDetailsModify.Image")));
            this.btnDetailsModify.Location = new System.Drawing.Point(91, 337);
            this.btnDetailsModify.Name = "btnDetailsModify";
            this.btnDetailsModify.Size = new System.Drawing.Size(72, 26);
            this.btnDetailsModify.TabIndex = 1;
            this.btnDetailsModify.Text = " Modify";
            this.btnDetailsModify.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDetailsModify.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDetailsModify.UseVisualStyleBackColor = true;
            this.btnDetailsModify.Click += new System.EventHandler(this.btnDetailsModify_Click);
            // 
            // pnUopfile
            // 
            this.pnUopfile.Controls.Add(this.btn_pnlUopFile_RecountFiles);
            this.pnUopfile.Controls.Add(this.label3);
            this.pnUopfile.Controls.Add(this.lblFiles);
            this.pnUopfile.Controls.Add(this.lblManyData);
            this.pnUopfile.Controls.Add(this.txt_pnlUopFile_Header2);
            this.pnUopfile.Controls.Add(this.num_pnlUopFile_Files);
            this.pnUopfile.Controls.Add(this.txt_pnlUopFile_Header1);
            this.pnUopfile.Enabled = false;
            this.pnUopfile.Location = new System.Drawing.Point(6, 19);
            this.pnUopfile.Name = "pnUopfile";
            this.pnUopfile.Size = new System.Drawing.Size(301, 312);
            this.pnUopfile.TabIndex = 0;
            this.pnUopfile.Visible = false;
            // 
            // btn_pnlUopFile_RecountFiles
            // 
            this.btn_pnlUopFile_RecountFiles.AutoSize = true;
            this.btn_pnlUopFile_RecountFiles.Location = new System.Drawing.Point(103, 130);
            this.btn_pnlUopFile_RecountFiles.Name = "btn_pnlUopFile_RecountFiles";
            this.btn_pnlUopFile_RecountFiles.Size = new System.Drawing.Size(72, 13);
            this.btn_pnlUopFile_RecountFiles.TabIndex = 7;
            this.btn_pnlUopFile_RecountFiles.TabStop = true;
            this.btn_pnlUopFile_RecountFiles.Text = "Recount Files";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 196);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(54, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Header 2:";
            // 
            // lblFiles
            // 
            this.lblFiles.AutoSize = true;
            this.lblFiles.Location = new System.Drawing.Point(3, 112);
            this.lblFiles.Name = "lblFiles";
            this.lblFiles.Size = new System.Drawing.Size(82, 13);
            this.lblFiles.TabIndex = 4;
            this.lblFiles.Text = "Files Contained:";
            // 
            // lblManyData
            // 
            this.lblManyData.AutoSize = true;
            this.lblManyData.Location = new System.Drawing.Point(3, 33);
            this.lblManyData.Name = "lblManyData";
            this.lblManyData.Size = new System.Drawing.Size(54, 13);
            this.lblManyData.TabIndex = 3;
            this.lblManyData.Text = "Header 1:";
            // 
            // txt_pnlUopFile_Header2
            // 
            this.txt_pnlUopFile_Header2.Location = new System.Drawing.Point(3, 212);
            this.txt_pnlUopFile_Header2.Name = "txt_pnlUopFile_Header2";
            this.txt_pnlUopFile_Header2.Size = new System.Drawing.Size(295, 20);
            this.txt_pnlUopFile_Header2.TabIndex = 2;
            // 
            // num_pnlUopFile_Files
            // 
            this.num_pnlUopFile_Files.Location = new System.Drawing.Point(3, 128);
            this.num_pnlUopFile_Files.Name = "num_pnlUopFile_Files";
            this.num_pnlUopFile_Files.Size = new System.Drawing.Size(92, 20);
            this.num_pnlUopFile_Files.TabIndex = 1;
            // 
            // txt_pnlUopFile_Header1
            // 
            this.txt_pnlUopFile_Header1.Location = new System.Drawing.Point(3, 49);
            this.txt_pnlUopFile_Header1.Name = "txt_pnlUopFile_Header1";
            this.txt_pnlUopFile_Header1.Size = new System.Drawing.Size(295, 20);
            this.txt_pnlUopFile_Header1.TabIndex = 0;
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
            this.moveUpToolStripMenuItem,
            this.moveDownToolStripMenuItem,
            this.deleteToolStripMenuItem});
            this.ctxMenuNode.Name = "ctxMenuNode";
            this.ctxMenuNode.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.ctxMenuNode.ShowItemToolTips = false;
            this.ctxMenuNode.Size = new System.Drawing.Size(131, 70);
            // 
            // moveUpToolStripMenuItem
            // 
            this.moveUpToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("moveUpToolStripMenuItem.Image")));
            this.moveUpToolStripMenuItem.Name = "moveUpToolStripMenuItem";
            this.moveUpToolStripMenuItem.Size = new System.Drawing.Size(130, 22);
            this.moveUpToolStripMenuItem.Text = "Move Up";
            this.moveUpToolStripMenuItem.Click += new System.EventHandler(this.moveUpToolStripMenuItem_Click);
            // 
            // moveDownToolStripMenuItem
            // 
            this.moveDownToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("moveDownToolStripMenuItem.Image")));
            this.moveDownToolStripMenuItem.Name = "moveDownToolStripMenuItem";
            this.moveDownToolStripMenuItem.Size = new System.Drawing.Size(130, 22);
            this.moveDownToolStripMenuItem.Text = "Move Down";
            this.moveDownToolStripMenuItem.Click += new System.EventHandler(this.moveDownToolStripMenuItem_Click);
            // 
            // deleteToolStripMenuItem
            // 
            this.deleteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem.Image")));
            this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
            this.deleteToolStripMenuItem.Size = new System.Drawing.Size(130, 22);
            this.deleteToolStripMenuItem.Text = "Delete";
            this.deleteToolStripMenuItem.Click += new System.EventHandler(this.deleteToolStripMenuItem_Click);
            // 
            // ssStatusBar
            // 
            this.ssStatusBar.BackColor = System.Drawing.SystemColors.Control;
            this.ssStatusBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tslblStatus,
            this.tslblWorking});
            this.ssStatusBar.Location = new System.Drawing.Point(0, 408);
            this.ssStatusBar.Name = "ssStatusBar";
            this.ssStatusBar.Size = new System.Drawing.Size(672, 25);
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
            this.tslblStatus.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.tslblStatus.Name = "tslblStatus";
            this.tslblStatus.Size = new System.Drawing.Size(640, 20);
            this.tslblStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // tslblWorking
            // 
            this.tslblWorking.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.tslblWorking.Image = ((System.Drawing.Image)(resources.GetObject("tslblWorking.Image")));
            this.tslblWorking.Name = "tslblWorking";
            this.tslblWorking.Size = new System.Drawing.Size(16, 20);
            this.tslblWorking.Visible = false;
            // 
            // oFileDlgUopsave
            // 
            this.oFileDlgUopsave.Filter = "UOKR Uop (*.uop)|*.uop";
            // 
            // pnUopHeaderAndData
            // 
            this.pnUopHeaderAndData.Location = new System.Drawing.Point(6, 18);
            this.pnUopHeaderAndData.Name = "pnUopHeaderAndData";
            this.pnUopHeaderAndData.Size = new System.Drawing.Size(301, 313);
            this.pnUopHeaderAndData.TabIndex = 6;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(672, 433);
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
            this.gbSelectedData.ResumeLayout(false);
            this.pnUopDatafile.ResumeLayout(false);
            this.pnUopDatafile.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nud_pnUopDatafile_Files)).EndInit();
            this.pnUopfile.ResumeLayout(false);
            this.pnUopfile.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.num_pnlUopFile_Files)).EndInit();
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
        private System.Windows.Forms.Panel pnUopfile;
        private System.Windows.Forms.Button btnDetailsDelete;
        private System.Windows.Forms.Button btnDetailsApply;
        private System.Windows.Forms.Button btnDetailsModify;
        private System.Windows.Forms.TextBox txt_pnlUopFile_Header2;
        private System.Windows.Forms.NumericUpDown num_pnlUopFile_Files;
        private System.Windows.Forms.TextBox txt_pnlUopFile_Header1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblFiles;
        private System.Windows.Forms.Label lblManyData;
        private System.Windows.Forms.Button btnDetailsUnpack;
        private System.Windows.Forms.LinkLabel btn_pnlUopFile_RecountFiles;
        private System.Windows.Forms.Panel pnUopDatafile;
        private System.Windows.Forms.NumericUpDown nud_pnUopDatafile_Files;
        private System.Windows.Forms.Label lbl_pnUopDatafile_Files;
        private System.Windows.Forms.TextBox txt_pnUopDatafile_Offset;
        private System.Windows.Forms.Label lbl_pnUopDatafile_Offset;
        private System.Windows.Forms.LinkLabel btn_pnUopDatafile_RecountFiles;
        private System.Windows.Forms.ToolStripButton toolBtnDontFix;
        private System.Windows.Forms.Panel pnUopHeaderAndData;
    }
}

