namespace Repacker
{
    partial class Form1
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.load_btn = new System.Windows.Forms.Button();
            this.pack_btn = new System.Windows.Forms.Button();
            this.load_folder = new System.Windows.Forms.FolderBrowserDialog();
            this.display_box = new System.Windows.Forms.RichTextBox();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.Worker = new System.ComponentModel.BackgroundWorker();
            this.About_btn = new System.Windows.Forms.Button();
            this.Hashpanel = new System.Windows.Forms.Panel();
            this.hide_btn = new System.Windows.Forms.Button();
            this.Hash_Grid = new System.Windows.Forms.DataGridView();
            this.Main_panel = new System.Windows.Forms.Panel();
            this.seehash_btn = new System.Windows.Forms.Button();
            this.Column1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column2 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Hashpanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.Hash_Grid)).BeginInit();
            this.Main_panel.SuspendLayout();
            this.SuspendLayout();
            // 
            // load_btn
            // 
            this.load_btn.Location = new System.Drawing.Point(14, 2);
            this.load_btn.Name = "load_btn";
            this.load_btn.Size = new System.Drawing.Size(75, 23);
            this.load_btn.TabIndex = 0;
            this.load_btn.Text = "Load Folder";
            this.load_btn.UseVisualStyleBackColor = true;
            this.load_btn.Click += new System.EventHandler(this.load_btn_Click);
            // 
            // pack_btn
            // 
            this.pack_btn.Location = new System.Drawing.Point(184, 2);
            this.pack_btn.Name = "pack_btn";
            this.pack_btn.Size = new System.Drawing.Size(75, 23);
            this.pack_btn.TabIndex = 1;
            this.pack_btn.Text = "Pack it!";
            this.pack_btn.UseVisualStyleBackColor = true;
            this.pack_btn.Click += new System.EventHandler(this.pack_btn_Click);
            // 
            // load_folder
            // 
            this.load_folder.Description = "Select Unpacked files folder";
            this.load_folder.RootFolder = System.Environment.SpecialFolder.MyComputer;
            this.load_folder.SelectedPath = "E:\\RUO\\Unpacker\\unpack\\";
            this.load_folder.ShowNewFolderButton = false;
            // 
            // display_box
            // 
            this.display_box.Location = new System.Drawing.Point(14, 31);
            this.display_box.Name = "display_box";
            this.display_box.ReadOnly = true;
            this.display_box.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.display_box.Size = new System.Drawing.Size(267, 132);
            this.display_box.TabIndex = 4;
            this.display_box.Text = "Welcome, select \"Load Folder\" to start";
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(14, 169);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(267, 13);
            this.progressBar.TabIndex = 5;
            // 
            // Worker
            // 
            this.Worker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.Worker_DoWork);
            this.Worker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.Worker_RunWorkerCompleted);
            // 
            // About_btn
            // 
            this.About_btn.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.About_btn.Location = new System.Drawing.Point(265, 2);
            this.About_btn.Name = "About_btn";
            this.About_btn.Size = new System.Drawing.Size(20, 23);
            this.About_btn.TabIndex = 6;
            this.About_btn.Text = "?";
            this.About_btn.UseVisualStyleBackColor = true;
            this.About_btn.Click += new System.EventHandler(this.About_btn_Click);
            // 
            // Hashpanel
            // 
            this.Hashpanel.Controls.Add(this.Hash_Grid);
            this.Hashpanel.Controls.Add(this.hide_btn);
            this.Hashpanel.Location = new System.Drawing.Point(1, 2);
            this.Hashpanel.Name = "Hashpanel";
            this.Hashpanel.Size = new System.Drawing.Size(500, 400);
            this.Hashpanel.TabIndex = 8;
            this.Hashpanel.Visible = false;
            // 
            // hide_btn
            // 
            this.hide_btn.Location = new System.Drawing.Point(3, 374);
            this.hide_btn.Name = "hide_btn";
            this.hide_btn.Size = new System.Drawing.Size(75, 23);
            this.hide_btn.TabIndex = 8;
            this.hide_btn.Text = "Hide";
            this.hide_btn.UseVisualStyleBackColor = true;
            this.hide_btn.Click += new System.EventHandler(this.button1_Click);
            // 
            // Hash_Grid
            // 
            this.Hash_Grid.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.Hash_Grid.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.Column1,
            this.Column2});
            this.Hash_Grid.Location = new System.Drawing.Point(0, 0);
            this.Hash_Grid.Name = "Hash_Grid";
            this.Hash_Grid.RowTemplate.Height = 20;
            this.Hash_Grid.Size = new System.Drawing.Size(480, 370);
            this.Hash_Grid.TabIndex = 7;
            this.Hash_Grid.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView1_CellEndEdit);
            // 
            // Main_panel
            // 
            this.Main_panel.Controls.Add(this.seehash_btn);
            this.Main_panel.Controls.Add(this.load_btn);
            this.Main_panel.Controls.Add(this.About_btn);
            this.Main_panel.Controls.Add(this.pack_btn);
            this.Main_panel.Controls.Add(this.progressBar);
            this.Main_panel.Controls.Add(this.display_box);
            this.Main_panel.Location = new System.Drawing.Point(0, 0);
            this.Main_panel.Name = "Main_panel";
            this.Main_panel.Size = new System.Drawing.Size(295, 215);
            this.Main_panel.TabIndex = 9;
            // 
            // seehash_btn
            // 
            this.seehash_btn.Location = new System.Drawing.Point(14, 188);
            this.seehash_btn.Name = "seehash_btn";
            this.seehash_btn.Size = new System.Drawing.Size(155, 21);
            this.seehash_btn.TabIndex = 7;
            this.seehash_btn.Text = "See and change Hashes";
            this.seehash_btn.UseVisualStyleBackColor = true;
            this.seehash_btn.Click += new System.EventHandler(this.seehash_btn_Click);
            // 
            // Column1
            // 
            this.Column1.Frozen = true;
            this.Column1.HeaderText = "File name";
            this.Column1.Name = "Column1";
            this.Column1.ReadOnly = true;
            this.Column1.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column1.Width = 287;
            // 
            // Column2
            // 
            this.Column2.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.AllCellsExceptHeader;
            this.Column2.HeaderText = "Hash";
            this.Column2.Name = "Column2";
            this.Column2.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column2.Width = 21;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(516, 481);
            this.Controls.Add(this.Main_panel);
            this.Controls.Add(this.Hashpanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Opacity = 0.95;
            this.Text = ".Uop Packer v1.1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.Hashpanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.Hash_Grid)).EndInit();
            this.Main_panel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button load_btn;
        private System.Windows.Forms.Button pack_btn;
        private System.Windows.Forms.FolderBrowserDialog load_folder;
        private System.Windows.Forms.RichTextBox display_box;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.ComponentModel.BackgroundWorker Worker;
        private System.Windows.Forms.Button About_btn;
        private System.Windows.Forms.Panel Hashpanel;
        private System.Windows.Forms.Button hide_btn;
        private System.Windows.Forms.DataGridView Hash_Grid;
        private System.Windows.Forms.Panel Main_panel;
        private System.Windows.Forms.Button seehash_btn;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column1;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column2;
    }
}

