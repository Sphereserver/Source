namespace MapMULToUOP
{
    partial class MainForm
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
            this.mainTab = new System.Windows.Forms.TabControl();
            this.MapPage = new System.Windows.Forms.TabPage();
            this.COnvGroup = new System.Windows.Forms.GroupBox();
            this.label2 = new System.Windows.Forms.Label();
            this.browseButton2 = new System.Windows.Forms.Button();
            this.UOFolderBox = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.browseButton = new System.Windows.Forms.Button();
            this.convBox = new System.Windows.Forms.TextBox();
            this.mapGroup = new System.Windows.Forms.GroupBox();
            this.tokunoButton = new System.Windows.Forms.RadioButton();
            this.britanniaButton = new System.Windows.Forms.RadioButton();
            this.malasButton = new System.Windows.Forms.RadioButton();
            this.britanniaAltButton = new System.Windows.Forms.RadioButton();
            this.ilshenarButton = new System.Windows.Forms.RadioButton();
            this.HuesPage = new System.Windows.Forms.TabPage();
            this.genButton = new System.Windows.Forms.Button();
            this.resButton = new System.Windows.Forms.Button();
            this.opf = new System.Windows.Forms.OpenFileDialog();
            this.fBD = new System.Windows.Forms.FolderBrowserDialog();
            this.mainTab.SuspendLayout();
            this.MapPage.SuspendLayout();
            this.COnvGroup.SuspendLayout();
            this.mapGroup.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainTab
            // 
            this.mainTab.Alignment = System.Windows.Forms.TabAlignment.Bottom;
            this.mainTab.Controls.Add(this.MapPage);
            this.mainTab.Controls.Add(this.HuesPage);
            this.mainTab.Location = new System.Drawing.Point(2, 2);
            this.mainTab.Name = "mainTab";
            this.mainTab.SelectedIndex = 0;
            this.mainTab.Size = new System.Drawing.Size(644, 185);
            this.mainTab.TabIndex = 0;
            // 
            // MapPage
            // 
            this.MapPage.Controls.Add(this.COnvGroup);
            this.MapPage.Controls.Add(this.mapGroup);
            this.MapPage.Location = new System.Drawing.Point(4, 4);
            this.MapPage.Name = "MapPage";
            this.MapPage.Padding = new System.Windows.Forms.Padding(3);
            this.MapPage.Size = new System.Drawing.Size(636, 159);
            this.MapPage.TabIndex = 0;
            this.MapPage.Text = "Map";
            this.MapPage.UseVisualStyleBackColor = true;
            // 
            // COnvGroup
            // 
            this.COnvGroup.Controls.Add(this.label2);
            this.COnvGroup.Controls.Add(this.browseButton2);
            this.COnvGroup.Controls.Add(this.UOFolderBox);
            this.COnvGroup.Controls.Add(this.label1);
            this.COnvGroup.Controls.Add(this.browseButton);
            this.COnvGroup.Controls.Add(this.convBox);
            this.COnvGroup.Location = new System.Drawing.Point(193, 11);
            this.COnvGroup.Name = "COnvGroup";
            this.COnvGroup.Size = new System.Drawing.Size(424, 134);
            this.COnvGroup.TabIndex = 6;
            this.COnvGroup.TabStop = false;
            this.COnvGroup.Text = "Input";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 65);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(75, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "UO 2D Folder:";
            // 
            // browseButton2
            // 
            this.browseButton2.Location = new System.Drawing.Point(341, 81);
            this.browseButton2.Name = "browseButton2";
            this.browseButton2.Size = new System.Drawing.Size(75, 23);
            this.browseButton2.TabIndex = 4;
            this.browseButton2.Text = "Browse";
            this.browseButton2.UseVisualStyleBackColor = true;
            this.browseButton2.Click += new System.EventHandler(this.browseButton2_Click);
            // 
            // UOFolderBox
            // 
            this.UOFolderBox.Location = new System.Drawing.Point(6, 83);
            this.UOFolderBox.Name = "UOFolderBox";
            this.UOFolderBox.Size = new System.Drawing.Size(329, 20);
            this.UOFolderBox.TabIndex = 3;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(122, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Graphic Conversion File:";
            // 
            // browseButton
            // 
            this.browseButton.Location = new System.Drawing.Point(341, 32);
            this.browseButton.Name = "browseButton";
            this.browseButton.Size = new System.Drawing.Size(75, 23);
            this.browseButton.TabIndex = 1;
            this.browseButton.Text = "Browse";
            this.browseButton.UseVisualStyleBackColor = true;
            this.browseButton.Click += new System.EventHandler(this.browseButton_Click);
            // 
            // convBox
            // 
            this.convBox.Location = new System.Drawing.Point(6, 34);
            this.convBox.Name = "convBox";
            this.convBox.Size = new System.Drawing.Size(329, 20);
            this.convBox.TabIndex = 0;
            // 
            // mapGroup
            // 
            this.mapGroup.Controls.Add(this.tokunoButton);
            this.mapGroup.Controls.Add(this.britanniaButton);
            this.mapGroup.Controls.Add(this.malasButton);
            this.mapGroup.Controls.Add(this.britanniaAltButton);
            this.mapGroup.Controls.Add(this.ilshenarButton);
            this.mapGroup.Location = new System.Drawing.Point(6, 6);
            this.mapGroup.Name = "mapGroup";
            this.mapGroup.Size = new System.Drawing.Size(158, 139);
            this.mapGroup.TabIndex = 5;
            this.mapGroup.TabStop = false;
            this.mapGroup.Text = "Map";
            // 
            // tokunoButton
            // 
            this.tokunoButton.AutoSize = true;
            this.tokunoButton.Location = new System.Drawing.Point(17, 111);
            this.tokunoButton.Name = "tokunoButton";
            this.tokunoButton.Size = new System.Drawing.Size(62, 17);
            this.tokunoButton.TabIndex = 4;
            this.tokunoButton.Text = "Tokuno";
            this.tokunoButton.UseVisualStyleBackColor = true;
            // 
            // britanniaButton
            // 
            this.britanniaButton.AutoSize = true;
            this.britanniaButton.Checked = true;
            this.britanniaButton.Location = new System.Drawing.Point(17, 19);
            this.britanniaButton.Name = "britanniaButton";
            this.britanniaButton.Size = new System.Drawing.Size(66, 17);
            this.britanniaButton.TabIndex = 0;
            this.britanniaButton.TabStop = true;
            this.britanniaButton.Text = "Britannia";
            this.britanniaButton.UseVisualStyleBackColor = true;
            // 
            // malasButton
            // 
            this.malasButton.AutoSize = true;
            this.malasButton.Location = new System.Drawing.Point(17, 88);
            this.malasButton.Name = "malasButton";
            this.malasButton.Size = new System.Drawing.Size(53, 17);
            this.malasButton.TabIndex = 3;
            this.malasButton.Text = "Malas";
            this.malasButton.UseVisualStyleBackColor = true;
            // 
            // britanniaAltButton
            // 
            this.britanniaAltButton.AutoSize = true;
            this.britanniaAltButton.Location = new System.Drawing.Point(17, 42);
            this.britanniaAltButton.Name = "britanniaAltButton";
            this.britanniaAltButton.Size = new System.Drawing.Size(81, 17);
            this.britanniaAltButton.TabIndex = 1;
            this.britanniaAltButton.Text = "Britannia Alt";
            this.britanniaAltButton.UseVisualStyleBackColor = true;
            // 
            // ilshenarButton
            // 
            this.ilshenarButton.AutoSize = true;
            this.ilshenarButton.Location = new System.Drawing.Point(17, 65);
            this.ilshenarButton.Name = "ilshenarButton";
            this.ilshenarButton.Size = new System.Drawing.Size(62, 17);
            this.ilshenarButton.TabIndex = 2;
            this.ilshenarButton.Text = "Ilshenar";
            this.ilshenarButton.UseVisualStyleBackColor = true;
            // 
            // HuesPage
            // 
            this.HuesPage.Location = new System.Drawing.Point(4, 4);
            this.HuesPage.Name = "HuesPage";
            this.HuesPage.Padding = new System.Windows.Forms.Padding(3);
            this.HuesPage.Size = new System.Drawing.Size(723, 174);
            this.HuesPage.TabIndex = 1;
            this.HuesPage.Text = "Hues";
            this.HuesPage.UseVisualStyleBackColor = true;
            // 
            // genButton
            // 
            this.genButton.Location = new System.Drawing.Point(237, 193);
            this.genButton.Name = "genButton";
            this.genButton.Size = new System.Drawing.Size(75, 23);
            this.genButton.TabIndex = 1;
            this.genButton.Text = "Generate";
            this.genButton.UseVisualStyleBackColor = true;
            this.genButton.Click += new System.EventHandler(this.genButton_Click);
            // 
            // resButton
            // 
            this.resButton.Location = new System.Drawing.Point(343, 193);
            this.resButton.Name = "resButton";
            this.resButton.Size = new System.Drawing.Size(75, 23);
            this.resButton.TabIndex = 2;
            this.resButton.Text = "Reset";
            this.resButton.UseVisualStyleBackColor = true;
            this.resButton.Click += new System.EventHandler(this.resButton_Click);
            // 
            // opf
            // 
            this.opf.Filter = "Tutti i file (*.*)|*.*";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(655, 225);
            this.Controls.Add(this.resButton);
            this.Controls.Add(this.genButton);
            this.Controls.Add(this.mainTab);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Multi2UOP Utility";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.mainTab.ResumeLayout(false);
            this.MapPage.ResumeLayout(false);
            this.COnvGroup.ResumeLayout(false);
            this.COnvGroup.PerformLayout();
            this.mapGroup.ResumeLayout(false);
            this.mapGroup.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl mainTab;
        private System.Windows.Forms.TabPage MapPage;
        private System.Windows.Forms.TabPage HuesPage;
        private System.Windows.Forms.Button genButton;
        private System.Windows.Forms.Button resButton;
        private System.Windows.Forms.GroupBox mapGroup;
        private System.Windows.Forms.RadioButton tokunoButton;
        private System.Windows.Forms.RadioButton britanniaButton;
        private System.Windows.Forms.RadioButton malasButton;
        private System.Windows.Forms.RadioButton britanniaAltButton;
        private System.Windows.Forms.RadioButton ilshenarButton;
        private System.Windows.Forms.GroupBox COnvGroup;
        private System.Windows.Forms.Button browseButton;
        private System.Windows.Forms.TextBox convBox;
        private System.Windows.Forms.OpenFileDialog opf;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button browseButton2;
        private System.Windows.Forms.TextBox UOFolderBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.FolderBrowserDialog fBD;
    }
}

