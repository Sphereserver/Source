namespace Unpacker
{
	partial class Main
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose( bool disposing )
		{
			if ( disposing && ( components != null ) )
			{
				components.Dispose();
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Main));
            this.MainMenu = new System.Windows.Forms.MenuStrip();
            this.MainMenuFile = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuFileOpen = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuFileClose = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuFileSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.MainMenuFileExit = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuDictionary = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuDictionaryLoad = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuDictionarySave = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuDictionaryMerge = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuDictionarySeparator = new System.Windows.Forms.ToolStripSeparator();
            this.MainMenuDictionaryUpdate = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuSpyStart = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuSpyAttach = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuSpyDetach = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuSettings = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuSettingsPath = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.OpenFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.TreeView = new System.Windows.Forms.TreeView();
            this.ListBox = new System.Windows.Forms.ListBox();
            this.Status = new System.Windows.Forms.StatusStrip();
            this.StatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.Worker = new System.ComponentModel.BackgroundWorker();
            this.CopyMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.CopyMenuStripButton = new System.Windows.Forms.ToolStripMenuItem();
            this.FileDetails = new System.Windows.Forms.TabControl();
            this.DetailsPackage = new System.Windows.Forms.TabPage();
            this.PackageCompleteInfo = new System.Windows.Forms.Label();
            this.PackageCompleteLabel = new System.Windows.Forms.Label();
            this.PackageSizeInfo = new System.Windows.Forms.Label();
            this.PackageSizeLabel = new System.Windows.Forms.Label();
            this.PackageCreationInfo = new System.Windows.Forms.Label();
            this.PackageCreationLabel = new System.Windows.Forms.Label();
            this.PackageAttributesInfo = new System.Windows.Forms.Label();
            this.PackageAttributesLabel = new System.Windows.Forms.Label();
            this.PackageFullNameInfo = new System.Windows.Forms.Label();
            this.PackageFullNameLabel = new System.Windows.Forms.Label();
            this.PackageGeneralLabel = new System.Windows.Forms.Label();
            this.PackageFileCountInfo = new System.Windows.Forms.Label();
            this.PackageFileCountLabel = new System.Windows.Forms.Label();
            this.PackageBlockSizeInfo = new System.Windows.Forms.Label();
            this.PackageBlockSizeLabel = new System.Windows.Forms.Label();
            this.PackageHeaderSizeInfo = new System.Windows.Forms.Label();
            this.PackageHeaderSizeLabel = new System.Windows.Forms.Label();
            this.PackageMiscInfo = new System.Windows.Forms.Label();
            this.PackageMiscLabel = new System.Windows.Forms.Label();
            this.PackageVersionInfo = new System.Windows.Forms.Label();
            this.PackageVersionLabel = new System.Windows.Forms.Label();
            this.PackageHeader = new System.Windows.Forms.Label();
            this.DetailsBlock = new System.Windows.Forms.TabPage();
            this.BlockCompleteInfo = new System.Windows.Forms.Label();
            this.BlockCompleteLabel = new System.Windows.Forms.Label();
            this.BlockNextBlockInfo = new System.Windows.Forms.Label();
            this.BlockNextBlockLabel = new System.Windows.Forms.Label();
            this.BlockHeader = new System.Windows.Forms.Label();
            this.BlockFileCountInfo = new System.Windows.Forms.Label();
            this.BlockFileCountLabel = new System.Windows.Forms.Label();
            this.DetailsFile = new System.Windows.Forms.TabPage();
            this.FileFlagInfo = new System.Windows.Forms.Label();
            this.FileDataOffsetInfo = new System.Windows.Forms.Label();
            this.FileUnknown2Info = new System.Windows.Forms.Label();
            this.FileUnknown2Label = new System.Windows.Forms.Label();
            this.FileDataOffsetLabel = new System.Windows.Forms.Label();
            this.FileFlagLabel = new System.Windows.Forms.Label();
            this.FileData = new System.Windows.Forms.Label();
            this.FileCompressed = new System.Windows.Forms.CheckBox();
            this.FileCompression = new System.Windows.Forms.Label();
            this.FileGeneral = new System.Windows.Forms.Label();
            this.FileDecompressedInfo = new System.Windows.Forms.Label();
            this.FileFileNameInfo = new System.Windows.Forms.Label();
            this.FileDecompressedLabel = new System.Windows.Forms.Label();
            this.FileFileNameLabel = new System.Windows.Forms.Label();
            this.FileCompressedInfo = new System.Windows.Forms.Label();
            this.FileHashLabel = new System.Windows.Forms.Label();
            this.FileCompressedLabel = new System.Windows.Forms.Label();
            this.FileHashInfo = new System.Windows.Forms.Label();
            this.FileUnknownLabel = new System.Windows.Forms.Label();
            this.FileUnknownInfo = new System.Windows.Forms.Label();
            this.GeneralLabel = new System.Windows.Forms.Label();
            this.UncompressedInfo = new System.Windows.Forms.Label();
            this.UncompressedLabel = new System.Windows.Forms.Label();
            this.CompressedInfo = new System.Windows.Forms.Label();
            this.CompressedLabel = new System.Windows.Forms.Label();
            this.SizesLabel = new System.Windows.Forms.Label();
            this.DataOffsetInfo = new System.Windows.Forms.Label();
            this.DataOffsetLabel = new System.Windows.Forms.Label();
            this.UnknownInfo = new System.Windows.Forms.Label();
            this.UnknownLabel = new System.Windows.Forms.Label();
            this.HashInfo = new System.Windows.Forms.Label();
            this.HashLabel = new System.Windows.Forms.Label();
            this.FilenameInfo = new System.Windows.Forms.Label();
            this.FilenameLabel = new System.Windows.Forms.Label();
            this.OpenExeDialog = new System.Windows.Forms.OpenFileDialog();
            this.OpenDictionary = new System.Windows.Forms.OpenFileDialog();
            this.SearchBox = new System.Windows.Forms.TextBox();
            this.SelectFolder = new System.Windows.Forms.FolderBrowserDialog();
            this.Search = new System.Windows.Forms.Button();
            this.Unpack = new System.Windows.Forms.Button();
            this.MainMenu.SuspendLayout();
            this.Status.SuspendLayout();
            this.CopyMenuStrip.SuspendLayout();
            this.FileDetails.SuspendLayout();
            this.DetailsPackage.SuspendLayout();
            this.DetailsBlock.SuspendLayout();
            this.DetailsFile.SuspendLayout();
            this.SuspendLayout();
            // 
            // MainMenu
            // 
            this.MainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuFile,
            this.MainMenuDictionary,
            this.MainMenuSettings,
            this.MainMenuAbout});
            this.MainMenu.Location = new System.Drawing.Point(0, 0);
            this.MainMenu.Name = "MainMenu";
            this.MainMenu.Size = new System.Drawing.Size(677, 24);
            this.MainMenu.TabIndex = 0;
            this.MainMenu.Text = "Menu";
            // 
            // MainMenuFile
            // 
            this.MainMenuFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuFileOpen,
            this.MainMenuFileClose,
            this.MainMenuFileSeparator,
            this.MainMenuFileExit});
            this.MainMenuFile.Name = "MainMenuFile";
            this.MainMenuFile.Size = new System.Drawing.Size(37, 20);
            this.MainMenuFile.Text = "File";
            // 
            // MainMenuFileOpen
            // 
            this.MainMenuFileOpen.Image = global::Unpacker.Properties.Resources.open;
            this.MainMenuFileOpen.Name = "MainMenuFileOpen";
            this.MainMenuFileOpen.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.MainMenuFileOpen.Size = new System.Drawing.Size(146, 22);
            this.MainMenuFileOpen.Text = "Open";
            this.MainMenuFileOpen.Click += new System.EventHandler(this.MainMenuFileOpen_Click);
            // 
            // MainMenuFileClose
            // 
            this.MainMenuFileClose.Image = global::Unpacker.Properties.Resources.close;
            this.MainMenuFileClose.Name = "MainMenuFileClose";
            this.MainMenuFileClose.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
            this.MainMenuFileClose.Size = new System.Drawing.Size(146, 22);
            this.MainMenuFileClose.Text = "Close";
            this.MainMenuFileClose.Click += new System.EventHandler(this.MainMenuFileClose_Click);
            // 
            // MainMenuFileSeparator
            // 
            this.MainMenuFileSeparator.Name = "MainMenuFileSeparator";
            this.MainMenuFileSeparator.Size = new System.Drawing.Size(143, 6);
            // 
            // MainMenuFileExit
            // 
            this.MainMenuFileExit.Name = "MainMenuFileExit";
            this.MainMenuFileExit.Size = new System.Drawing.Size(146, 22);
            this.MainMenuFileExit.Text = "Exit";
            this.MainMenuFileExit.Click += new System.EventHandler(this.MainMenuFileExit_Click);
            // 
            // MainMenuDictionary
            // 
            this.MainMenuDictionary.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuDictionaryLoad,
            this.MainMenuDictionarySave,
            this.MainMenuDictionaryMerge,
            this.MainMenuDictionarySeparator,
            this.MainMenuDictionaryUpdate});
            this.MainMenuDictionary.Name = "MainMenuDictionary";
            this.MainMenuDictionary.Size = new System.Drawing.Size(73, 20);
            this.MainMenuDictionary.Text = "Dictionary";
            // 
            // MainMenuDictionaryLoad
            // 
            this.MainMenuDictionaryLoad.Image = global::Unpacker.Properties.Resources.open;
            this.MainMenuDictionaryLoad.Name = "MainMenuDictionaryLoad";
            this.MainMenuDictionaryLoad.Size = new System.Drawing.Size(112, 22);
            this.MainMenuDictionaryLoad.Text = "Load";
            this.MainMenuDictionaryLoad.Click += new System.EventHandler(this.MainMenuDictionaryLoad_Click);
            // 
            // MainMenuDictionarySave
            // 
            this.MainMenuDictionarySave.Image = global::Unpacker.Properties.Resources.save;
            this.MainMenuDictionarySave.Name = "MainMenuDictionarySave";
            this.MainMenuDictionarySave.Size = new System.Drawing.Size(112, 22);
            this.MainMenuDictionarySave.Text = "Save";
            this.MainMenuDictionarySave.Click += new System.EventHandler(this.MainMenuDictionarySave_Click);
            // 
            // MainMenuDictionaryMerge
            // 
            this.MainMenuDictionaryMerge.Image = global::Unpacker.Properties.Resources.merge;
            this.MainMenuDictionaryMerge.Name = "MainMenuDictionaryMerge";
            this.MainMenuDictionaryMerge.Size = new System.Drawing.Size(112, 22);
            this.MainMenuDictionaryMerge.Text = "Merge";
            this.MainMenuDictionaryMerge.Click += new System.EventHandler(this.MainMenuDictionaryMerge_Click);
            // 
            // MainMenuDictionarySeparator
            // 
            this.MainMenuDictionarySeparator.Name = "MainMenuDictionarySeparator";
            this.MainMenuDictionarySeparator.Size = new System.Drawing.Size(109, 6);
            // 
            // MainMenuDictionaryUpdate
            // 
            this.MainMenuDictionaryUpdate.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuSpyStart,
            this.MainMenuSpyAttach,
            this.MainMenuSpyDetach});
            this.MainMenuDictionaryUpdate.Name = "MainMenuDictionaryUpdate";
            this.MainMenuDictionaryUpdate.Size = new System.Drawing.Size(112, 22);
            this.MainMenuDictionaryUpdate.Text = "Update";
            // 
            // MainMenuSpyStart
            // 
            this.MainMenuSpyStart.Name = "MainMenuSpyStart";
            this.MainMenuSpyStart.Size = new System.Drawing.Size(131, 22);
            this.MainMenuSpyStart.Text = "Spy Start";
            this.MainMenuSpyStart.Click += new System.EventHandler(this.MainMenuSpyStart_Click);
            // 
            // MainMenuSpyAttach
            // 
            this.MainMenuSpyAttach.Name = "MainMenuSpyAttach";
            this.MainMenuSpyAttach.Size = new System.Drawing.Size(131, 22);
            this.MainMenuSpyAttach.Text = "Spy Attach";
            this.MainMenuSpyAttach.Click += new System.EventHandler(this.MainMenuSpyAttach_Click);
            // 
            // MainMenuSpyDetach
            // 
            this.MainMenuSpyDetach.Enabled = false;
            this.MainMenuSpyDetach.Name = "MainMenuSpyDetach";
            this.MainMenuSpyDetach.Size = new System.Drawing.Size(131, 22);
            this.MainMenuSpyDetach.Text = "Detach";
            this.MainMenuSpyDetach.Click += new System.EventHandler(this.MainMenuSpyDetach_Click);
            // 
            // MainMenuSettings
            // 
            this.MainMenuSettings.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuSettingsPath});
            this.MainMenuSettings.Name = "MainMenuSettings";
            this.MainMenuSettings.Size = new System.Drawing.Size(61, 20);
            this.MainMenuSettings.Text = "Settings";
            // 
            // MainMenuSettingsPath
            // 
            this.MainMenuSettingsPath.Image = global::Unpacker.Properties.Resources.path;
            this.MainMenuSettingsPath.Name = "MainMenuSettingsPath";
            this.MainMenuSettingsPath.Size = new System.Drawing.Size(160, 22);
            this.MainMenuSettingsPath.Text = "Set Unpack Path";
            this.MainMenuSettingsPath.Click += new System.EventHandler(this.MainMenuSettingsPath_Click);
            // 
            // MainMenuAbout
            // 
            this.MainMenuAbout.Name = "MainMenuAbout";
            this.MainMenuAbout.Size = new System.Drawing.Size(52, 20);
            this.MainMenuAbout.Text = "About";
            this.MainMenuAbout.Click += new System.EventHandler(this.MainMenuAbout_Click);
            // 
            // OpenFileDialog
            // 
            this.OpenFileDialog.Filter = "UOP files|*.uop";
            this.OpenFileDialog.Multiselect = true;
            this.OpenFileDialog.RestoreDirectory = true;
            this.OpenFileDialog.Title = "Open Mythic Package";
            this.OpenFileDialog.FileOk += new System.ComponentModel.CancelEventHandler(this.OpenFileDialog_FileOk);
            // 
            // TreeView
            // 
            this.TreeView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.TreeView.Location = new System.Drawing.Point(12, 27);
            this.TreeView.Name = "TreeView";
            this.TreeView.Size = new System.Drawing.Size(150, 433);
            this.TreeView.TabIndex = 1;
            this.TreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.TreeView_AfterSelect);
            this.TreeView.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.TreeView_NodeMouseClick);
            // 
            // ListBox
            // 
            this.ListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.ListBox.FormattingEnabled = true;
            this.ListBox.Location = new System.Drawing.Point(168, 27);
            this.ListBox.Name = "ListBox";
            this.ListBox.Size = new System.Drawing.Size(170, 433);
            this.ListBox.TabIndex = 2;
            this.ListBox.SelectedIndexChanged += new System.EventHandler(this.ListBox_SelectedIndexChanged);
            // 
            // Status
            // 
            this.Status.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.StatusLabel});
            this.Status.Location = new System.Drawing.Point(0, 462);
            this.Status.Name = "Status";
            this.Status.Size = new System.Drawing.Size(677, 22);
            this.Status.TabIndex = 4;
            // 
            // StatusLabel
            // 
            this.StatusLabel.Margin = new System.Windows.Forms.Padding(5, 3, 0, 2);
            this.StatusLabel.Name = "StatusLabel";
            this.StatusLabel.Size = new System.Drawing.Size(0, 17);
            // 
            // Worker
            // 
            this.Worker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.Worker_DoWork);
            this.Worker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.Worker_RunWorkerCompleted);
            // 
            // CopyMenuStrip
            // 
            this.CopyMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.CopyMenuStripButton});
            this.CopyMenuStrip.Name = "CopyMenuStrip";
            this.CopyMenuStrip.ShowImageMargin = false;
            this.CopyMenuStrip.Size = new System.Drawing.Size(147, 26);
            // 
            // CopyMenuStripButton
            // 
            this.CopyMenuStripButton.Name = "CopyMenuStripButton";
            this.CopyMenuStripButton.Size = new System.Drawing.Size(146, 22);
            this.CopyMenuStripButton.Text = "Copy to Clipboard";
            this.CopyMenuStripButton.Click += new System.EventHandler(this.CopyMenuStripButton_Click);
            // 
            // FileDetails
            // 
            this.FileDetails.Controls.Add(this.DetailsPackage);
            this.FileDetails.Controls.Add(this.DetailsBlock);
            this.FileDetails.Controls.Add(this.DetailsFile);
            this.FileDetails.Location = new System.Drawing.Point(344, 27);
            this.FileDetails.Name = "FileDetails";
            this.FileDetails.Padding = new System.Drawing.Point(10, 3);
            this.FileDetails.SelectedIndex = 0;
            this.FileDetails.Size = new System.Drawing.Size(321, 403);
            this.FileDetails.TabIndex = 8;
            // 
            // DetailsPackage
            // 
            this.DetailsPackage.BackColor = System.Drawing.SystemColors.Control;
            this.DetailsPackage.Controls.Add(this.PackageCompleteInfo);
            this.DetailsPackage.Controls.Add(this.PackageCompleteLabel);
            this.DetailsPackage.Controls.Add(this.PackageSizeInfo);
            this.DetailsPackage.Controls.Add(this.PackageSizeLabel);
            this.DetailsPackage.Controls.Add(this.PackageCreationInfo);
            this.DetailsPackage.Controls.Add(this.PackageCreationLabel);
            this.DetailsPackage.Controls.Add(this.PackageAttributesInfo);
            this.DetailsPackage.Controls.Add(this.PackageAttributesLabel);
            this.DetailsPackage.Controls.Add(this.PackageFullNameInfo);
            this.DetailsPackage.Controls.Add(this.PackageFullNameLabel);
            this.DetailsPackage.Controls.Add(this.PackageGeneralLabel);
            this.DetailsPackage.Controls.Add(this.PackageFileCountInfo);
            this.DetailsPackage.Controls.Add(this.PackageFileCountLabel);
            this.DetailsPackage.Controls.Add(this.PackageBlockSizeInfo);
            this.DetailsPackage.Controls.Add(this.PackageBlockSizeLabel);
            this.DetailsPackage.Controls.Add(this.PackageHeaderSizeInfo);
            this.DetailsPackage.Controls.Add(this.PackageHeaderSizeLabel);
            this.DetailsPackage.Controls.Add(this.PackageMiscInfo);
            this.DetailsPackage.Controls.Add(this.PackageMiscLabel);
            this.DetailsPackage.Controls.Add(this.PackageVersionInfo);
            this.DetailsPackage.Controls.Add(this.PackageVersionLabel);
            this.DetailsPackage.Controls.Add(this.PackageHeader);
            this.DetailsPackage.Location = new System.Drawing.Point(4, 22);
            this.DetailsPackage.Name = "DetailsPackage";
            this.DetailsPackage.Padding = new System.Windows.Forms.Padding(3);
            this.DetailsPackage.Size = new System.Drawing.Size(313, 377);
            this.DetailsPackage.TabIndex = 0;
            this.DetailsPackage.Text = "Package Details";
            // 
            // PackageCompleteInfo
            // 
            this.PackageCompleteInfo.BackColor = System.Drawing.Color.White;
            this.PackageCompleteInfo.Location = new System.Drawing.Point(74, 114);
            this.PackageCompleteInfo.Name = "PackageCompleteInfo";
            this.PackageCompleteInfo.Size = new System.Drawing.Size(100, 13);
            this.PackageCompleteInfo.TabIndex = 21;
            this.PackageCompleteInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageCompleteInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageCompleteLabel
            // 
            this.PackageCompleteLabel.AutoSize = true;
            this.PackageCompleteLabel.Location = new System.Drawing.Point(16, 114);
            this.PackageCompleteLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageCompleteLabel.Name = "PackageCompleteLabel";
            this.PackageCompleteLabel.Size = new System.Drawing.Size(54, 13);
            this.PackageCompleteLabel.TabIndex = 20;
            this.PackageCompleteLabel.Text = "Complete:";
            // 
            // PackageSizeInfo
            // 
            this.PackageSizeInfo.BackColor = System.Drawing.Color.White;
            this.PackageSizeInfo.Location = new System.Drawing.Point(74, 96);
            this.PackageSizeInfo.Name = "PackageSizeInfo";
            this.PackageSizeInfo.Size = new System.Drawing.Size(100, 13);
            this.PackageSizeInfo.TabIndex = 19;
            this.PackageSizeInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageSizeInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageSizeLabel
            // 
            this.PackageSizeLabel.AutoSize = true;
            this.PackageSizeLabel.Location = new System.Drawing.Point(16, 96);
            this.PackageSizeLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageSizeLabel.Name = "PackageSizeLabel";
            this.PackageSizeLabel.Size = new System.Drawing.Size(30, 13);
            this.PackageSizeLabel.TabIndex = 18;
            this.PackageSizeLabel.Text = "Size:";
            // 
            // PackageCreationInfo
            // 
            this.PackageCreationInfo.BackColor = System.Drawing.Color.White;
            this.PackageCreationInfo.Location = new System.Drawing.Point(74, 78);
            this.PackageCreationInfo.Name = "PackageCreationInfo";
            this.PackageCreationInfo.Size = new System.Drawing.Size(100, 13);
            this.PackageCreationInfo.TabIndex = 17;
            this.PackageCreationInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageCreationInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageCreationLabel
            // 
            this.PackageCreationLabel.AutoSize = true;
            this.PackageCreationLabel.Location = new System.Drawing.Point(16, 78);
            this.PackageCreationLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageCreationLabel.Name = "PackageCreationLabel";
            this.PackageCreationLabel.Size = new System.Drawing.Size(49, 13);
            this.PackageCreationLabel.TabIndex = 16;
            this.PackageCreationLabel.Text = "Creation:";
            // 
            // PackageAttributesInfo
            // 
            this.PackageAttributesInfo.BackColor = System.Drawing.Color.White;
            this.PackageAttributesInfo.Location = new System.Drawing.Point(74, 60);
            this.PackageAttributesInfo.Name = "PackageAttributesInfo";
            this.PackageAttributesInfo.Size = new System.Drawing.Size(100, 13);
            this.PackageAttributesInfo.TabIndex = 15;
            this.PackageAttributesInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageAttributesInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageAttributesLabel
            // 
            this.PackageAttributesLabel.AutoSize = true;
            this.PackageAttributesLabel.Location = new System.Drawing.Point(16, 60);
            this.PackageAttributesLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageAttributesLabel.Name = "PackageAttributesLabel";
            this.PackageAttributesLabel.Size = new System.Drawing.Size(54, 13);
            this.PackageAttributesLabel.TabIndex = 14;
            this.PackageAttributesLabel.Text = "Attributes:";
            // 
            // PackageFullNameInfo
            // 
            this.PackageFullNameInfo.BackColor = System.Drawing.Color.White;
            this.PackageFullNameInfo.Cursor = System.Windows.Forms.Cursors.IBeam;
            this.PackageFullNameInfo.Location = new System.Drawing.Point(74, 28);
            this.PackageFullNameInfo.Name = "PackageFullNameInfo";
            this.PackageFullNameInfo.Size = new System.Drawing.Size(233, 26);
            this.PackageFullNameInfo.TabIndex = 13;
            this.PackageFullNameInfo.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.PackageFullNameInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageFullNameLabel
            // 
            this.PackageFullNameLabel.AutoSize = true;
            this.PackageFullNameLabel.Location = new System.Drawing.Point(16, 28);
            this.PackageFullNameLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageFullNameLabel.Name = "PackageFullNameLabel";
            this.PackageFullNameLabel.Size = new System.Drawing.Size(55, 13);
            this.PackageFullNameLabel.TabIndex = 12;
            this.PackageFullNameLabel.Text = "Full name:";
            // 
            // PackageGeneralLabel
            // 
            this.PackageGeneralLabel.AutoSize = true;
            this.PackageGeneralLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PackageGeneralLabel.Location = new System.Drawing.Point(6, 10);
            this.PackageGeneralLabel.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.PackageGeneralLabel.Name = "PackageGeneralLabel";
            this.PackageGeneralLabel.Size = new System.Drawing.Size(51, 13);
            this.PackageGeneralLabel.TabIndex = 11;
            this.PackageGeneralLabel.Text = "General";
            // 
            // PackageFileCountInfo
            // 
            this.PackageFileCountInfo.BackColor = System.Drawing.Color.White;
            this.PackageFileCountInfo.Location = new System.Drawing.Point(85, 227);
            this.PackageFileCountInfo.Name = "PackageFileCountInfo";
            this.PackageFileCountInfo.Size = new System.Drawing.Size(75, 13);
            this.PackageFileCountInfo.TabIndex = 10;
            this.PackageFileCountInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageFileCountInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageFileCountLabel
            // 
            this.PackageFileCountLabel.AutoSize = true;
            this.PackageFileCountLabel.Location = new System.Drawing.Point(13, 227);
            this.PackageFileCountLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageFileCountLabel.Name = "PackageFileCountLabel";
            this.PackageFileCountLabel.Size = new System.Drawing.Size(56, 13);
            this.PackageFileCountLabel.TabIndex = 9;
            this.PackageFileCountLabel.Text = "File count:";
            // 
            // PackageBlockSizeInfo
            // 
            this.PackageBlockSizeInfo.BackColor = System.Drawing.Color.White;
            this.PackageBlockSizeInfo.Location = new System.Drawing.Point(85, 209);
            this.PackageBlockSizeInfo.Name = "PackageBlockSizeInfo";
            this.PackageBlockSizeInfo.Size = new System.Drawing.Size(75, 13);
            this.PackageBlockSizeInfo.TabIndex = 8;
            this.PackageBlockSizeInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageBlockSizeInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageBlockSizeLabel
            // 
            this.PackageBlockSizeLabel.AutoSize = true;
            this.PackageBlockSizeLabel.Location = new System.Drawing.Point(13, 209);
            this.PackageBlockSizeLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageBlockSizeLabel.Name = "PackageBlockSizeLabel";
            this.PackageBlockSizeLabel.Size = new System.Drawing.Size(58, 13);
            this.PackageBlockSizeLabel.TabIndex = 7;
            this.PackageBlockSizeLabel.Text = "Block size:";
            // 
            // PackageHeaderSizeInfo
            // 
            this.PackageHeaderSizeInfo.BackColor = System.Drawing.Color.White;
            this.PackageHeaderSizeInfo.Location = new System.Drawing.Point(85, 191);
            this.PackageHeaderSizeInfo.Name = "PackageHeaderSizeInfo";
            this.PackageHeaderSizeInfo.Size = new System.Drawing.Size(75, 13);
            this.PackageHeaderSizeInfo.TabIndex = 6;
            this.PackageHeaderSizeInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageHeaderSizeInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageHeaderSizeLabel
            // 
            this.PackageHeaderSizeLabel.AutoSize = true;
            this.PackageHeaderSizeLabel.Location = new System.Drawing.Point(13, 191);
            this.PackageHeaderSizeLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageHeaderSizeLabel.Name = "PackageHeaderSizeLabel";
            this.PackageHeaderSizeLabel.Size = new System.Drawing.Size(66, 13);
            this.PackageHeaderSizeLabel.TabIndex = 5;
            this.PackageHeaderSizeLabel.Text = "Header size:";
            // 
            // PackageMiscInfo
            // 
            this.PackageMiscInfo.BackColor = System.Drawing.Color.White;
            this.PackageMiscInfo.Location = new System.Drawing.Point(85, 173);
            this.PackageMiscInfo.Name = "PackageMiscInfo";
            this.PackageMiscInfo.Size = new System.Drawing.Size(75, 13);
            this.PackageMiscInfo.TabIndex = 4;
            this.PackageMiscInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageMiscInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageMiscLabel
            // 
            this.PackageMiscLabel.AutoSize = true;
            this.PackageMiscLabel.Location = new System.Drawing.Point(13, 173);
            this.PackageMiscLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageMiscLabel.Name = "PackageMiscLabel";
            this.PackageMiscLabel.Size = new System.Drawing.Size(32, 13);
            this.PackageMiscLabel.TabIndex = 3;
            this.PackageMiscLabel.Text = "Misc:";
            // 
            // PackageVersionInfo
            // 
            this.PackageVersionInfo.BackColor = System.Drawing.Color.White;
            this.PackageVersionInfo.Location = new System.Drawing.Point(85, 155);
            this.PackageVersionInfo.Name = "PackageVersionInfo";
            this.PackageVersionInfo.Size = new System.Drawing.Size(75, 13);
            this.PackageVersionInfo.TabIndex = 2;
            this.PackageVersionInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.PackageVersionInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // PackageVersionLabel
            // 
            this.PackageVersionLabel.AutoSize = true;
            this.PackageVersionLabel.Location = new System.Drawing.Point(13, 155);
            this.PackageVersionLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.PackageVersionLabel.Name = "PackageVersionLabel";
            this.PackageVersionLabel.Size = new System.Drawing.Size(45, 13);
            this.PackageVersionLabel.TabIndex = 1;
            this.PackageVersionLabel.Text = "Version:";
            // 
            // PackageHeader
            // 
            this.PackageHeader.AutoSize = true;
            this.PackageHeader.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PackageHeader.Location = new System.Drawing.Point(3, 137);
            this.PackageHeader.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.PackageHeader.Name = "PackageHeader";
            this.PackageHeader.Size = new System.Drawing.Size(48, 13);
            this.PackageHeader.TabIndex = 0;
            this.PackageHeader.Text = "Header";
            // 
            // DetailsBlock
            // 
            this.DetailsBlock.BackColor = System.Drawing.SystemColors.Control;
            this.DetailsBlock.Controls.Add(this.BlockCompleteInfo);
            this.DetailsBlock.Controls.Add(this.BlockCompleteLabel);
            this.DetailsBlock.Controls.Add(this.BlockNextBlockInfo);
            this.DetailsBlock.Controls.Add(this.BlockNextBlockLabel);
            this.DetailsBlock.Controls.Add(this.BlockHeader);
            this.DetailsBlock.Controls.Add(this.BlockFileCountInfo);
            this.DetailsBlock.Controls.Add(this.BlockFileCountLabel);
            this.DetailsBlock.Location = new System.Drawing.Point(4, 22);
            this.DetailsBlock.Name = "DetailsBlock";
            this.DetailsBlock.Padding = new System.Windows.Forms.Padding(3);
            this.DetailsBlock.Size = new System.Drawing.Size(313, 377);
            this.DetailsBlock.TabIndex = 1;
            this.DetailsBlock.Text = "Block Details";
            // 
            // BlockCompleteInfo
            // 
            this.BlockCompleteInfo.BackColor = System.Drawing.Color.White;
            this.BlockCompleteInfo.Location = new System.Drawing.Point(87, 64);
            this.BlockCompleteInfo.Name = "BlockCompleteInfo";
            this.BlockCompleteInfo.Size = new System.Drawing.Size(75, 13);
            this.BlockCompleteInfo.TabIndex = 17;
            this.BlockCompleteInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // BlockCompleteLabel
            // 
            this.BlockCompleteLabel.AutoSize = true;
            this.BlockCompleteLabel.Location = new System.Drawing.Point(15, 64);
            this.BlockCompleteLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.BlockCompleteLabel.Name = "BlockCompleteLabel";
            this.BlockCompleteLabel.Size = new System.Drawing.Size(54, 13);
            this.BlockCompleteLabel.TabIndex = 16;
            this.BlockCompleteLabel.Text = "Complete:";
            // 
            // BlockNextBlockInfo
            // 
            this.BlockNextBlockInfo.BackColor = System.Drawing.Color.White;
            this.BlockNextBlockInfo.Location = new System.Drawing.Point(87, 46);
            this.BlockNextBlockInfo.Name = "BlockNextBlockInfo";
            this.BlockNextBlockInfo.Size = new System.Drawing.Size(120, 13);
            this.BlockNextBlockInfo.TabIndex = 15;
            this.BlockNextBlockInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.BlockNextBlockInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // BlockNextBlockLabel
            // 
            this.BlockNextBlockLabel.AutoSize = true;
            this.BlockNextBlockLabel.Location = new System.Drawing.Point(15, 46);
            this.BlockNextBlockLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.BlockNextBlockLabel.Name = "BlockNextBlockLabel";
            this.BlockNextBlockLabel.Size = new System.Drawing.Size(61, 13);
            this.BlockNextBlockLabel.TabIndex = 14;
            this.BlockNextBlockLabel.Text = "Next block:";
            // 
            // BlockHeader
            // 
            this.BlockHeader.AutoSize = true;
            this.BlockHeader.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.BlockHeader.Location = new System.Drawing.Point(6, 10);
            this.BlockHeader.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.BlockHeader.Name = "BlockHeader";
            this.BlockHeader.Size = new System.Drawing.Size(48, 13);
            this.BlockHeader.TabIndex = 13;
            this.BlockHeader.Text = "Header";
            // 
            // BlockFileCountInfo
            // 
            this.BlockFileCountInfo.BackColor = System.Drawing.Color.White;
            this.BlockFileCountInfo.Location = new System.Drawing.Point(87, 28);
            this.BlockFileCountInfo.Name = "BlockFileCountInfo";
            this.BlockFileCountInfo.Size = new System.Drawing.Size(75, 13);
            this.BlockFileCountInfo.TabIndex = 12;
            this.BlockFileCountInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.BlockFileCountInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // BlockFileCountLabel
            // 
            this.BlockFileCountLabel.AutoSize = true;
            this.BlockFileCountLabel.Location = new System.Drawing.Point(15, 28);
            this.BlockFileCountLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.BlockFileCountLabel.Name = "BlockFileCountLabel";
            this.BlockFileCountLabel.Size = new System.Drawing.Size(56, 13);
            this.BlockFileCountLabel.TabIndex = 11;
            this.BlockFileCountLabel.Text = "File count:";
            // 
            // DetailsFile
            // 
            this.DetailsFile.BackColor = System.Drawing.SystemColors.Control;
            this.DetailsFile.Controls.Add(this.FileFlagInfo);
            this.DetailsFile.Controls.Add(this.FileDataOffsetInfo);
            this.DetailsFile.Controls.Add(this.FileUnknown2Info);
            this.DetailsFile.Controls.Add(this.FileUnknown2Label);
            this.DetailsFile.Controls.Add(this.FileDataOffsetLabel);
            this.DetailsFile.Controls.Add(this.FileFlagLabel);
            this.DetailsFile.Controls.Add(this.FileData);
            this.DetailsFile.Controls.Add(this.FileCompressed);
            this.DetailsFile.Controls.Add(this.FileCompression);
            this.DetailsFile.Controls.Add(this.FileGeneral);
            this.DetailsFile.Controls.Add(this.FileDecompressedInfo);
            this.DetailsFile.Controls.Add(this.FileFileNameInfo);
            this.DetailsFile.Controls.Add(this.FileDecompressedLabel);
            this.DetailsFile.Controls.Add(this.FileFileNameLabel);
            this.DetailsFile.Controls.Add(this.FileCompressedInfo);
            this.DetailsFile.Controls.Add(this.FileHashLabel);
            this.DetailsFile.Controls.Add(this.FileCompressedLabel);
            this.DetailsFile.Controls.Add(this.FileHashInfo);
            this.DetailsFile.Controls.Add(this.FileUnknownLabel);
            this.DetailsFile.Controls.Add(this.FileUnknownInfo);
            this.DetailsFile.Location = new System.Drawing.Point(4, 22);
            this.DetailsFile.Name = "DetailsFile";
            this.DetailsFile.Padding = new System.Windows.Forms.Padding(3);
            this.DetailsFile.Size = new System.Drawing.Size(313, 377);
            this.DetailsFile.TabIndex = 2;
            this.DetailsFile.Text = "File Details";
            // 
            // FileFlagInfo
            // 
            this.FileFlagInfo.BackColor = System.Drawing.Color.White;
            this.FileFlagInfo.Location = new System.Drawing.Point(81, 159);
            this.FileFlagInfo.Name = "FileFlagInfo";
            this.FileFlagInfo.Size = new System.Drawing.Size(60, 13);
            this.FileFlagInfo.TabIndex = 23;
            this.FileFlagInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.FileFlagInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileDataOffsetInfo
            // 
            this.FileDataOffsetInfo.BackColor = System.Drawing.Color.White;
            this.FileDataOffsetInfo.Location = new System.Drawing.Point(81, 177);
            this.FileDataOffsetInfo.Name = "FileDataOffsetInfo";
            this.FileDataOffsetInfo.Size = new System.Drawing.Size(60, 13);
            this.FileDataOffsetInfo.TabIndex = 22;
            this.FileDataOffsetInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.FileDataOffsetInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileUnknown2Info
            // 
            this.FileUnknown2Info.BackColor = System.Drawing.Color.White;
            this.FileUnknown2Info.Location = new System.Drawing.Point(81, 195);
            this.FileUnknown2Info.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.FileUnknown2Info.Name = "FileUnknown2Info";
            this.FileUnknown2Info.Size = new System.Drawing.Size(107, 13);
            this.FileUnknown2Info.TabIndex = 21;
            this.FileUnknown2Info.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileUnknown2Label
            // 
            this.FileUnknown2Label.AutoSize = true;
            this.FileUnknown2Label.Location = new System.Drawing.Point(13, 195);
            this.FileUnknown2Label.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileUnknown2Label.Name = "FileUnknown2Label";
            this.FileUnknown2Label.Size = new System.Drawing.Size(49, 13);
            this.FileUnknown2Label.TabIndex = 20;
            this.FileUnknown2Label.Text = "FileTime:";
            // 
            // FileDataOffsetLabel
            // 
            this.FileDataOffsetLabel.AutoSize = true;
            this.FileDataOffsetLabel.Location = new System.Drawing.Point(13, 177);
            this.FileDataOffsetLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileDataOffsetLabel.Name = "FileDataOffsetLabel";
            this.FileDataOffsetLabel.Size = new System.Drawing.Size(62, 13);
            this.FileDataOffsetLabel.TabIndex = 19;
            this.FileDataOffsetLabel.Text = "Data offset:";
            // 
            // FileFlagLabel
            // 
            this.FileFlagLabel.AutoSize = true;
            this.FileFlagLabel.Location = new System.Drawing.Point(13, 159);
            this.FileFlagLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileFlagLabel.Name = "FileFlagLabel";
            this.FileFlagLabel.Size = new System.Drawing.Size(30, 13);
            this.FileFlagLabel.TabIndex = 18;
            this.FileFlagLabel.Text = "Flag:";
            // 
            // FileData
            // 
            this.FileData.AutoSize = true;
            this.FileData.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FileData.Location = new System.Drawing.Point(6, 141);
            this.FileData.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.FileData.Name = "FileData";
            this.FileData.Size = new System.Drawing.Size(34, 13);
            this.FileData.TabIndex = 17;
            this.FileData.Text = "Data";
            // 
            // FileCompressed
            // 
            this.FileCompressed.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.FileCompressed.AutoSize = true;
            this.FileCompressed.Checked = true;
            this.FileCompressed.CheckState = System.Windows.Forms.CheckState.Indeterminate;
            this.FileCompressed.Enabled = false;
            this.FileCompressed.Location = new System.Drawing.Point(187, 106);
            this.FileCompressed.Name = "FileCompressed";
            this.FileCompressed.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this.FileCompressed.Size = new System.Drawing.Size(87, 17);
            this.FileCompressed.TabIndex = 16;
            this.FileCompressed.Text = "Compressed:";
            this.FileCompressed.UseVisualStyleBackColor = true;
            // 
            // FileCompression
            // 
            this.FileCompression.AutoSize = true;
            this.FileCompression.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FileCompression.Location = new System.Drawing.Point(6, 82);
            this.FileCompression.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.FileCompression.Name = "FileCompression";
            this.FileCompression.Size = new System.Drawing.Size(78, 13);
            this.FileCompression.TabIndex = 14;
            this.FileCompression.Text = "Compression";
            // 
            // FileGeneral
            // 
            this.FileGeneral.AutoSize = true;
            this.FileGeneral.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FileGeneral.Location = new System.Drawing.Point(6, 10);
            this.FileGeneral.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.FileGeneral.Name = "FileGeneral";
            this.FileGeneral.Size = new System.Drawing.Size(51, 13);
            this.FileGeneral.TabIndex = 13;
            this.FileGeneral.Text = "General";
            // 
            // FileDecompressedInfo
            // 
            this.FileDecompressedInfo.BackColor = System.Drawing.Color.White;
            this.FileDecompressedInfo.Location = new System.Drawing.Point(121, 118);
            this.FileDecompressedInfo.Name = "FileDecompressedInfo";
            this.FileDecompressedInfo.Size = new System.Drawing.Size(60, 13);
            this.FileDecompressedInfo.TabIndex = 12;
            this.FileDecompressedInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.FileDecompressedInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileFileNameInfo
            // 
            this.FileFileNameInfo.AutoEllipsis = true;
            this.FileFileNameInfo.BackColor = System.Drawing.Color.White;
            this.FileFileNameInfo.Location = new System.Drawing.Point(65, 28);
            this.FileFileNameInfo.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.FileFileNameInfo.Name = "FileFileNameInfo";
            this.FileFileNameInfo.Size = new System.Drawing.Size(242, 26);
            this.FileFileNameInfo.TabIndex = 1;
            this.FileFileNameInfo.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.FileFileNameInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileDecompressedLabel
            // 
            this.FileDecompressedLabel.AutoSize = true;
            this.FileDecompressedLabel.Location = new System.Drawing.Point(13, 118);
            this.FileDecompressedLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileDecompressedLabel.Name = "FileDecompressedLabel";
            this.FileDecompressedLabel.Size = new System.Drawing.Size(102, 13);
            this.FileDecompressedLabel.TabIndex = 11;
            this.FileDecompressedLabel.Text = "Decompressed size:";
            // 
            // FileFileNameLabel
            // 
            this.FileFileNameLabel.AutoSize = true;
            this.FileFileNameLabel.Location = new System.Drawing.Point(13, 28);
            this.FileFileNameLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileFileNameLabel.Name = "FileFileNameLabel";
            this.FileFileNameLabel.Size = new System.Drawing.Size(52, 13);
            this.FileFileNameLabel.TabIndex = 0;
            this.FileFileNameLabel.Text = "Filename:";
            // 
            // FileCompressedInfo
            // 
            this.FileCompressedInfo.BackColor = System.Drawing.Color.White;
            this.FileCompressedInfo.Location = new System.Drawing.Point(121, 100);
            this.FileCompressedInfo.Name = "FileCompressedInfo";
            this.FileCompressedInfo.Size = new System.Drawing.Size(60, 13);
            this.FileCompressedInfo.TabIndex = 10;
            this.FileCompressedInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.FileCompressedInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileHashLabel
            // 
            this.FileHashLabel.AutoSize = true;
            this.FileHashLabel.Location = new System.Drawing.Point(13, 59);
            this.FileHashLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileHashLabel.Name = "FileHashLabel";
            this.FileHashLabel.Size = new System.Drawing.Size(35, 13);
            this.FileHashLabel.TabIndex = 2;
            this.FileHashLabel.Text = "Hash:";
            // 
            // FileCompressedLabel
            // 
            this.FileCompressedLabel.AutoSize = true;
            this.FileCompressedLabel.Location = new System.Drawing.Point(13, 100);
            this.FileCompressedLabel.Margin = new System.Windows.Forms.Padding(10, 5, 3, 0);
            this.FileCompressedLabel.Name = "FileCompressedLabel";
            this.FileCompressedLabel.Size = new System.Drawing.Size(89, 13);
            this.FileCompressedLabel.TabIndex = 9;
            this.FileCompressedLabel.Text = "Compressed size:";
            // 
            // FileHashInfo
            // 
            this.FileHashInfo.BackColor = System.Drawing.Color.White;
            this.FileHashInfo.Location = new System.Drawing.Point(65, 59);
            this.FileHashInfo.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.FileHashInfo.Name = "FileHashInfo";
            this.FileHashInfo.Size = new System.Drawing.Size(107, 13);
            this.FileHashInfo.TabIndex = 3;
            this.FileHashInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // FileUnknownLabel
            // 
            this.FileUnknownLabel.AutoSize = true;
            this.FileUnknownLabel.Location = new System.Drawing.Point(178, 59);
            this.FileUnknownLabel.Name = "FileUnknownLabel";
            this.FileUnknownLabel.Size = new System.Drawing.Size(32, 13);
            this.FileUnknownLabel.TabIndex = 4;
            this.FileUnknownLabel.Text = "CRC:";
            // 
            // FileUnknownInfo
            // 
            this.FileUnknownInfo.BackColor = System.Drawing.Color.White;
            this.FileUnknownInfo.Location = new System.Drawing.Point(216, 59);
            this.FileUnknownInfo.Name = "FileUnknownInfo";
            this.FileUnknownInfo.Size = new System.Drawing.Size(91, 13);
            this.FileUnknownInfo.TabIndex = 5;
            this.FileUnknownInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.FileUnknownInfo.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Label_MouseClick);
            // 
            // GeneralLabel
            // 
            this.GeneralLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GeneralLabel.Location = new System.Drawing.Point(6, 26);
            this.GeneralLabel.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.GeneralLabel.Name = "GeneralLabel";
            this.GeneralLabel.Size = new System.Drawing.Size(309, 13);
            this.GeneralLabel.TabIndex = 13;
            this.GeneralLabel.Text = "General";
            // 
            // UncompressedInfo
            // 
            this.UncompressedInfo.BackColor = System.Drawing.Color.White;
            this.UncompressedInfo.Location = new System.Drawing.Point(93, 146);
            this.UncompressedInfo.Name = "UncompressedInfo";
            this.UncompressedInfo.Size = new System.Drawing.Size(60, 13);
            this.UncompressedInfo.TabIndex = 12;
            this.UncompressedInfo.Text = "999 KB";
            this.UncompressedInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // UncompressedLabel
            // 
            this.UncompressedLabel.AutoSize = true;
            this.UncompressedLabel.Location = new System.Drawing.Point(6, 146);
            this.UncompressedLabel.Margin = new System.Windows.Forms.Padding(30, 8, 3, 0);
            this.UncompressedLabel.Name = "UncompressedLabel";
            this.UncompressedLabel.Size = new System.Drawing.Size(81, 13);
            this.UncompressedLabel.TabIndex = 11;
            this.UncompressedLabel.Text = "Uncompressed:";
            // 
            // CompressedInfo
            // 
            this.CompressedInfo.BackColor = System.Drawing.Color.White;
            this.CompressedInfo.Location = new System.Drawing.Point(93, 125);
            this.CompressedInfo.Name = "CompressedInfo";
            this.CompressedInfo.Size = new System.Drawing.Size(60, 13);
            this.CompressedInfo.TabIndex = 10;
            this.CompressedInfo.Text = "999 KB";
            this.CompressedInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // CompressedLabel
            // 
            this.CompressedLabel.AutoSize = true;
            this.CompressedLabel.Location = new System.Drawing.Point(6, 125);
            this.CompressedLabel.Margin = new System.Windows.Forms.Padding(30, 8, 3, 0);
            this.CompressedLabel.Name = "CompressedLabel";
            this.CompressedLabel.Size = new System.Drawing.Size(68, 13);
            this.CompressedLabel.TabIndex = 9;
            this.CompressedLabel.Text = "Compressed:";
            // 
            // SizesLabel
            // 
            this.SizesLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.SizesLabel.Location = new System.Drawing.Point(6, 104);
            this.SizesLabel.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.SizesLabel.Name = "SizesLabel";
            this.SizesLabel.Size = new System.Drawing.Size(309, 13);
            this.SizesLabel.TabIndex = 8;
            this.SizesLabel.Text = "Sizes";
            // 
            // DataOffsetInfo
            // 
            this.DataOffsetInfo.AutoSize = true;
            this.DataOffsetInfo.BackColor = System.Drawing.Color.White;
            this.DataOffsetInfo.Location = new System.Drawing.Point(93, 218);
            this.DataOffsetInfo.Name = "DataOffsetInfo";
            this.DataOffsetInfo.Size = new System.Drawing.Size(36, 13);
            this.DataOffsetInfo.TabIndex = 7;
            this.DataOffsetInfo.Text = "ADAS";
            // 
            // DataOffsetLabel
            // 
            this.DataOffsetLabel.AutoSize = true;
            this.DataOffsetLabel.Location = new System.Drawing.Point(6, 218);
            this.DataOffsetLabel.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.DataOffsetLabel.Name = "DataOffsetLabel";
            this.DataOffsetLabel.Size = new System.Drawing.Size(62, 13);
            this.DataOffsetLabel.TabIndex = 6;
            this.DataOffsetLabel.Text = "Data offset:";
            // 
            // UnknownInfo
            // 
            this.UnknownInfo.BackColor = System.Drawing.Color.White;
            this.UnknownInfo.Location = new System.Drawing.Point(239, 81);
            this.UnknownInfo.Name = "UnknownInfo";
            this.UnknownInfo.Size = new System.Drawing.Size(76, 13);
            this.UnknownInfo.TabIndex = 5;
            this.UnknownInfo.Text = "B307A07F";
            this.UnknownInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // UnknownLabel
            // 
            this.UnknownLabel.AutoSize = true;
            this.UnknownLabel.Location = new System.Drawing.Point(177, 81);
            this.UnknownLabel.Name = "UnknownLabel";
            this.UnknownLabel.Size = new System.Drawing.Size(56, 13);
            this.UnknownLabel.TabIndex = 4;
            this.UnknownLabel.Text = "Unknown:";
            // 
            // HashInfo
            // 
            this.HashInfo.BackColor = System.Drawing.Color.White;
            this.HashInfo.Location = new System.Drawing.Point(64, 81);
            this.HashInfo.Name = "HashInfo";
            this.HashInfo.Size = new System.Drawing.Size(107, 13);
            this.HashInfo.TabIndex = 3;
            this.HashInfo.Text = "897E9F7AB307A07F";
            // 
            // HashLabel
            // 
            this.HashLabel.AutoSize = true;
            this.HashLabel.Location = new System.Drawing.Point(6, 81);
            this.HashLabel.Name = "HashLabel";
            this.HashLabel.Size = new System.Drawing.Size(35, 13);
            this.HashLabel.TabIndex = 2;
            this.HashLabel.Text = "Hash:";
            // 
            // FilenameInfo
            // 
            this.FilenameInfo.AutoEllipsis = true;
            this.FilenameInfo.BackColor = System.Drawing.Color.White;
            this.FilenameInfo.Location = new System.Drawing.Point(64, 47);
            this.FilenameInfo.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.FilenameInfo.Name = "FilenameInfo";
            this.FilenameInfo.Size = new System.Drawing.Size(251, 26);
            this.FilenameInfo.TabIndex = 1;
            this.FilenameInfo.Text = "data/interface/interfacecore/fonts/neuehammerunzialeltstd.ttf";
            this.FilenameInfo.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // FilenameLabel
            // 
            this.FilenameLabel.AutoSize = true;
            this.FilenameLabel.Location = new System.Drawing.Point(6, 47);
            this.FilenameLabel.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.FilenameLabel.Name = "FilenameLabel";
            this.FilenameLabel.Size = new System.Drawing.Size(52, 13);
            this.FilenameLabel.TabIndex = 0;
            this.FilenameLabel.Text = "Filename:";
            // 
            // OpenExeDialog
            // 
            this.OpenExeDialog.Filter = "Executable files|*.exe";
            this.OpenExeDialog.RestoreDirectory = true;
            this.OpenExeDialog.Title = "Open Kingdom Reborn Executable";
            this.OpenExeDialog.FileOk += new System.ComponentModel.CancelEventHandler(this.OpenExeDialog_FileOk);
            // 
            // OpenDictionary
            // 
            this.OpenDictionary.Filter = "Dictionary files|*.dic";
            this.OpenDictionary.RestoreDirectory = true;
            this.OpenDictionary.Title = "Open Dictionary";
            this.OpenDictionary.FileOk += new System.ComponentModel.CancelEventHandler(this.OpenDictionary_FileOk);
            // 
            // SearchBox
            // 
            this.SearchBox.Location = new System.Drawing.Point(506, 438);
            this.SearchBox.MaxLength = 256;
            this.SearchBox.Name = "SearchBox";
            this.SearchBox.Size = new System.Drawing.Size(159, 20);
            this.SearchBox.TabIndex = 9;
            this.SearchBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.SearchBox_KeyPress);
            // 
            // SelectFolder
            // 
            this.SelectFolder.Description = "Select Folder";
            this.SelectFolder.RootFolder = System.Environment.SpecialFolder.DesktopDirectory;
            // 
            // Search
            // 
            this.Search.Image = global::Unpacker.Properties.Resources.search;
            this.Search.Location = new System.Drawing.Point(425, 436);
            this.Search.Name = "Search";
            this.Search.Size = new System.Drawing.Size(75, 23);
            this.Search.TabIndex = 7;
            this.Search.Text = "Search";
            this.Search.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.Search.UseVisualStyleBackColor = true;
            this.Search.Click += new System.EventHandler(this.Search_Click);
            // 
            // Unpack
            // 
            this.Unpack.Image = global::Unpacker.Properties.Resources.unpack;
            this.Unpack.Location = new System.Drawing.Point(344, 436);
            this.Unpack.Name = "Unpack";
            this.Unpack.Size = new System.Drawing.Size(75, 23);
            this.Unpack.TabIndex = 6;
            this.Unpack.Text = "Unpack";
            this.Unpack.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.Unpack.UseVisualStyleBackColor = true;
            this.Unpack.Click += new System.EventHandler(this.Unpack_Click);
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(677, 484);
            this.Controls.Add(this.FileDetails);
            this.Controls.Add(this.Status);
            this.Controls.Add(this.SearchBox);
            this.Controls.Add(this.ListBox);
            this.Controls.Add(this.TreeView);
            this.Controls.Add(this.Search);
            this.Controls.Add(this.MainMenu);
            this.Controls.Add(this.Unpack);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.MainMenu;
            this.Name = "Main";
            this.Text = "Ultima Online: Kingdom Reborn™ Unpacker";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Main_FormClosing);
            this.MainMenu.ResumeLayout(false);
            this.MainMenu.PerformLayout();
            this.Status.ResumeLayout(false);
            this.Status.PerformLayout();
            this.CopyMenuStrip.ResumeLayout(false);
            this.FileDetails.ResumeLayout(false);
            this.DetailsPackage.ResumeLayout(false);
            this.DetailsPackage.PerformLayout();
            this.DetailsBlock.ResumeLayout(false);
            this.DetailsBlock.PerformLayout();
            this.DetailsFile.ResumeLayout(false);
            this.DetailsFile.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.MenuStrip MainMenu;
		private System.Windows.Forms.ToolStripMenuItem MainMenuFile;
		private System.Windows.Forms.ToolStripMenuItem MainMenuFileOpen;
		private System.Windows.Forms.ToolStripSeparator MainMenuFileSeparator;
		private System.Windows.Forms.ToolStripMenuItem MainMenuFileExit;
		private System.Windows.Forms.OpenFileDialog OpenFileDialog;
		private System.Windows.Forms.TreeView TreeView;
		private System.Windows.Forms.ListBox ListBox;
		private System.Windows.Forms.StatusStrip Status;
		private System.ComponentModel.BackgroundWorker Worker;
		private System.Windows.Forms.ToolStripStatusLabel StatusLabel;
		private System.Windows.Forms.Button Unpack;
		private System.Windows.Forms.Button Search;
		private System.Windows.Forms.ContextMenuStrip CopyMenuStrip;
		private System.Windows.Forms.ToolStripMenuItem CopyMenuStripButton;
		private System.Windows.Forms.ToolStripMenuItem MainMenuDictionary;
		private System.Windows.Forms.ToolStripMenuItem MainMenuDictionaryUpdate;
		private System.Windows.Forms.TabControl FileDetails;
		private System.Windows.Forms.TabPage DetailsPackage;
		private System.Windows.Forms.TabPage DetailsBlock;
		private System.Windows.Forms.TabPage DetailsFile;
		private System.Windows.Forms.Label FileDecompressedInfo;
		private System.Windows.Forms.Label FileDecompressedLabel;
		private System.Windows.Forms.Label FileCompressedInfo;
		private System.Windows.Forms.Label FileCompressedLabel;
		private System.Windows.Forms.Label FileUnknownInfo;
		private System.Windows.Forms.Label FileUnknownLabel;
		private System.Windows.Forms.Label FileHashInfo;
		private System.Windows.Forms.Label FileHashLabel;
		private System.Windows.Forms.Label FileFileNameInfo;
		private System.Windows.Forms.Label FileFileNameLabel;
		private System.Windows.Forms.Label GeneralLabel;
		private System.Windows.Forms.Label UncompressedInfo;
		private System.Windows.Forms.Label UncompressedLabel;
		private System.Windows.Forms.Label CompressedInfo;
		private System.Windows.Forms.Label CompressedLabel;
		private System.Windows.Forms.Label SizesLabel;
		private System.Windows.Forms.Label DataOffsetInfo;
		private System.Windows.Forms.Label DataOffsetLabel;
		private System.Windows.Forms.Label UnknownInfo;
		private System.Windows.Forms.Label UnknownLabel;
		private System.Windows.Forms.Label HashInfo;
		private System.Windows.Forms.Label HashLabel;
		private System.Windows.Forms.Label FilenameInfo;
		private System.Windows.Forms.Label FilenameLabel;
		private System.Windows.Forms.Label PackageHeader;
		private System.Windows.Forms.Label PackageVersionLabel;
		private System.Windows.Forms.Label PackageVersionInfo;
		private System.Windows.Forms.Label PackageMiscLabel;
		private System.Windows.Forms.Label PackageMiscInfo;
		private System.Windows.Forms.Label PackageHeaderSizeLabel;
		private System.Windows.Forms.Label PackageHeaderSizeInfo;
		private System.Windows.Forms.Label PackageBlockSizeInfo;
		private System.Windows.Forms.Label PackageBlockSizeLabel;
		private System.Windows.Forms.Label PackageFileCountLabel;
		private System.Windows.Forms.Label PackageFileCountInfo;
		private System.Windows.Forms.Label PackageGeneralLabel;
		private System.Windows.Forms.Label PackageFullNameLabel;
		private System.Windows.Forms.Label PackageFullNameInfo;
		private System.Windows.Forms.Label PackageAttributesInfo;
		private System.Windows.Forms.Label PackageAttributesLabel;
		private System.Windows.Forms.Label PackageCreationLabel;
		private System.Windows.Forms.Label PackageCreationInfo;
		private System.Windows.Forms.Label PackageSizeLabel;
		private System.Windows.Forms.Label PackageSizeInfo;
		private System.Windows.Forms.Label BlockHeader;
		private System.Windows.Forms.Label BlockFileCountInfo;
		private System.Windows.Forms.Label BlockFileCountLabel;
		private System.Windows.Forms.Label BlockNextBlockInfo;
		private System.Windows.Forms.Label BlockNextBlockLabel;
		private System.Windows.Forms.Label FileGeneral;
		private System.Windows.Forms.Label FileCompression;
		private System.Windows.Forms.CheckBox FileCompressed;
		private System.Windows.Forms.Label FileData;
		private System.Windows.Forms.Label FileFlagLabel;
		private System.Windows.Forms.Label FileDataOffsetLabel;
		private System.Windows.Forms.Label FileUnknown2Label;
		private System.Windows.Forms.Label FileFlagInfo;
		private System.Windows.Forms.Label FileDataOffsetInfo;
		private System.Windows.Forms.Label FileUnknown2Info;
		private System.Windows.Forms.Label PackageCompleteLabel;
		private System.Windows.Forms.Label PackageCompleteInfo;
		private System.Windows.Forms.ToolStripMenuItem MainMenuSpyStart;
		private System.Windows.Forms.ToolStripMenuItem MainMenuSpyAttach;
		private System.Windows.Forms.OpenFileDialog OpenExeDialog;
		private System.Windows.Forms.ToolStripMenuItem MainMenuDictionarySave;
		private System.Windows.Forms.ToolStripMenuItem MainMenuSpyDetach;
		private System.Windows.Forms.ToolStripMenuItem MainMenuFileClose;
		private System.Windows.Forms.ToolStripMenuItem MainMenuDictionaryLoad;
		private System.Windows.Forms.ToolStripMenuItem MainMenuDictionaryMerge;
		private System.Windows.Forms.OpenFileDialog OpenDictionary;
		private System.Windows.Forms.TextBox SearchBox;
		private System.Windows.Forms.ToolStripSeparator MainMenuDictionarySeparator;
		private System.Windows.Forms.Label BlockCompleteLabel;
		private System.Windows.Forms.Label BlockCompleteInfo;
		private System.Windows.Forms.ToolStripMenuItem MainMenuSettings;
		private System.Windows.Forms.ToolStripMenuItem MainMenuSettingsPath;
		private System.Windows.Forms.FolderBrowserDialog SelectFolder;
		private System.Windows.Forms.ToolStripMenuItem MainMenuAbout;
	}
}

