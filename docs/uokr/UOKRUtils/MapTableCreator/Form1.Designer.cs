namespace MapTableCreator
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
            this.opf = new System.Windows.Forms.OpenFileDialog();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.button1 = new System.Windows.Forms.Button();
            this.ParseButton = new System.Windows.Forms.Button();
            this.progressBar1 = new System.Windows.Forms.ProgressBar();
            this.fbd = new System.Windows.Forms.FolderBrowserDialog();
            this.extendedCheck = new System.Windows.Forms.CheckBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.writeOnlyCheck = new System.Windows.Forms.CheckBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.TileCheck = new System.Windows.Forms.CheckBox();
            this.DelimiterCheck = new System.Windows.Forms.CheckBox();
            this.StaticCheck = new System.Windows.Forms.CheckBox();
            this.numBlocks = new System.Windows.Forms.NumericUpDown();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.blocklist = new System.Windows.Forms.ListBox();
            this.button3 = new System.Windows.Forms.Button();
            this.blockbox = new System.Windows.Forms.TextBox();
            this.bw1 = new System.ComponentModel.BackgroundWorker();
            this.parseOnly = new System.Windows.Forms.CheckBox();
            this.groupBox1.SuspendLayout();
            this.groupBox3.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numBlocks)).BeginInit();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // opf
            // 
            this.opf.Filter = "File Dat (*.dat)|*.dat";
            // 
            // textBox2
            // 
            this.textBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox2.Location = new System.Drawing.Point(6, 20);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(721, 20);
            this.textBox2.TabIndex = 1;
            this.textBox2.Text = "C:\\Tiziano\\C++\\NET2005\\UOKR\\UOPUnpacker\\TestEncProjectCs\\bin\\Release\\Unpacked";
            // 
            // button1
            // 
            this.button1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button1.Location = new System.Drawing.Point(733, 19);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(61, 21);
            this.button1.TabIndex = 2;
            this.button1.Text = "Browse";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // ParseButton
            // 
            this.ParseButton.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.ParseButton.Location = new System.Drawing.Point(363, 94);
            this.ParseButton.Name = "ParseButton";
            this.ParseButton.Size = new System.Drawing.Size(75, 23);
            this.ParseButton.TabIndex = 3;
            this.ParseButton.Text = "Parse";
            this.ParseButton.UseVisualStyleBackColor = true;
            this.ParseButton.Click += new System.EventHandler(this.button2_Click);
            // 
            // progressBar1
            // 
            this.progressBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBar1.Location = new System.Drawing.Point(12, 510);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(799, 23);
            this.progressBar1.TabIndex = 4;
            // 
            // extendedCheck
            // 
            this.extendedCheck.AutoSize = true;
            this.extendedCheck.Location = new System.Drawing.Point(6, 54);
            this.extendedCheck.Name = "extendedCheck";
            this.extendedCheck.Size = new System.Drawing.Size(150, 17);
            this.extendedCheck.TabIndex = 5;
            this.extendedCheck.Text = "Generate extended output";
            this.extendedCheck.UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.parseOnly);
            this.groupBox1.Controls.Add(this.writeOnlyCheck);
            this.groupBox1.Controls.Add(this.groupBox3);
            this.groupBox1.Controls.Add(this.numBlocks);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.textBox2);
            this.groupBox1.Controls.Add(this.extendedCheck);
            this.groupBox1.Controls.Add(this.ParseButton);
            this.groupBox1.Controls.Add(this.button1);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(800, 123);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Input";
            // 
            // writeOnlyCheck
            // 
            this.writeOnlyCheck.AutoSize = true;
            this.writeOnlyCheck.Location = new System.Drawing.Point(6, 77);
            this.writeOnlyCheck.Name = "writeOnlyCheck";
            this.writeOnlyCheck.Size = new System.Drawing.Size(109, 17);
            this.writeOnlyCheck.TabIndex = 9;
            this.writeOnlyCheck.Text = "Write only on files";
            this.writeOnlyCheck.UseVisualStyleBackColor = true;
            // 
            // groupBox3
            // 
            this.groupBox3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox3.Controls.Add(this.TileCheck);
            this.groupBox3.Controls.Add(this.DelimiterCheck);
            this.groupBox3.Controls.Add(this.StaticCheck);
            this.groupBox3.Location = new System.Drawing.Point(494, 46);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(233, 41);
            this.groupBox3.TabIndex = 8;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Load only landtile with";
            // 
            // TileCheck
            // 
            this.TileCheck.AutoSize = true;
            this.TileCheck.Location = new System.Drawing.Point(146, 18);
            this.TileCheck.Name = "TileCheck";
            this.TileCheck.Size = new System.Drawing.Size(67, 17);
            this.TileCheck.TabIndex = 2;
            this.TileCheck.Text = "Only Tile";
            this.TileCheck.UseVisualStyleBackColor = true;
            // 
            // DelimiterCheck
            // 
            this.DelimiterCheck.AutoSize = true;
            this.DelimiterCheck.Checked = true;
            this.DelimiterCheck.CheckState = System.Windows.Forms.CheckState.Checked;
            this.DelimiterCheck.Location = new System.Drawing.Point(74, 18);
            this.DelimiterCheck.Name = "DelimiterCheck";
            this.DelimiterCheck.Size = new System.Drawing.Size(66, 17);
            this.DelimiterCheck.TabIndex = 1;
            this.DelimiterCheck.Text = "Delimiter";
            this.DelimiterCheck.UseVisualStyleBackColor = true;
            // 
            // StaticCheck
            // 
            this.StaticCheck.AutoSize = true;
            this.StaticCheck.Location = new System.Drawing.Point(15, 18);
            this.StaticCheck.Name = "StaticCheck";
            this.StaticCheck.Size = new System.Drawing.Size(53, 17);
            this.StaticCheck.TabIndex = 0;
            this.StaticCheck.Text = "Static";
            this.StaticCheck.UseVisualStyleBackColor = true;
            // 
            // numBlocks
            // 
            this.numBlocks.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.numBlocks.Location = new System.Drawing.Point(344, 57);
            this.numBlocks.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.numBlocks.Name = "numBlocks";
            this.numBlocks.Size = new System.Drawing.Size(78, 20);
            this.numBlocks.TabIndex = 7;
            // 
            // label1
            // 
            this.label1.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.SystemColors.Control;
            this.label1.Location = new System.Drawing.Point(248, 55);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(90, 26);
            this.label1.TabIndex = 6;
            this.label1.Text = "Load only blocks:\r\n(0=All)\r\n";
            // 
            // groupBox2
            // 
            this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox2.Controls.Add(this.blocklist);
            this.groupBox2.Controls.Add(this.button3);
            this.groupBox2.Controls.Add(this.blockbox);
            this.groupBox2.Location = new System.Drawing.Point(10, 156);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(801, 342);
            this.groupBox2.TabIndex = 7;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Output";
            // 
            // blocklist
            // 
            this.blocklist.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.blocklist.FormattingEnabled = true;
            this.blocklist.Location = new System.Drawing.Point(8, 18);
            this.blocklist.Name = "blocklist";
            this.blocklist.Size = new System.Drawing.Size(257, 290);
            this.blocklist.TabIndex = 3;
            this.blocklist.SelectedIndexChanged += new System.EventHandler(this.blocklist_SelectedIndexChanged);
            // 
            // button3
            // 
            this.button3.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.button3.Location = new System.Drawing.Point(360, 313);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(81, 22);
            this.button3.TabIndex = 2;
            this.button3.Text = "Grid View";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // blockbox
            // 
            this.blockbox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.blockbox.Location = new System.Drawing.Point(271, 18);
            this.blockbox.Multiline = true;
            this.blockbox.Name = "blockbox";
            this.blockbox.ReadOnly = true;
            this.blockbox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.blockbox.Size = new System.Drawing.Size(524, 289);
            this.blockbox.TabIndex = 1;
            // 
            // bw1
            // 
            this.bw1.DoWork += new System.ComponentModel.DoWorkEventHandler(this.bw1_DoWork);
            this.bw1.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.bw1_RunWorkerCompleted);
            // 
            // parseOnly
            // 
            this.parseOnly.AutoSize = true;
            this.parseOnly.Checked = true;
            this.parseOnly.CheckState = System.Windows.Forms.CheckState.Checked;
            this.parseOnly.Location = new System.Drawing.Point(6, 100);
            this.parseOnly.Name = "parseOnly";
            this.parseOnly.Size = new System.Drawing.Size(132, 17);
            this.parseOnly.TabIndex = 10;
            this.parseOnly.Text = "Parse only num blocks";
            this.parseOnly.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(824, 545);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.progressBar1);
            this.Name = "Form1";
            this.Text = "UOKR Map Decoder";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numBlocks)).EndInit();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.OpenFileDialog opf;
        private System.Windows.Forms.TextBox textBox2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button ParseButton;
        private System.Windows.Forms.ProgressBar progressBar1;
        private System.Windows.Forms.FolderBrowserDialog fbd;
        private System.Windows.Forms.CheckBox extendedCheck;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox blockbox;
        private System.Windows.Forms.ListBox blocklist;
        private System.Windows.Forms.Button button3;
        private System.ComponentModel.BackgroundWorker bw1;
        private System.Windows.Forms.NumericUpDown numBlocks;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.CheckBox DelimiterCheck;
        private System.Windows.Forms.CheckBox StaticCheck;
        private System.Windows.Forms.CheckBox TileCheck;
        private System.Windows.Forms.CheckBox writeOnlyCheck;
        private System.Windows.Forms.CheckBox parseOnly;
    }
}