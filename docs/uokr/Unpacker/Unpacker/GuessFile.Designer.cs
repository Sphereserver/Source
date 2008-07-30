namespace Unpacker
{
	partial class GuessFile
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
			this.FileName = new System.Windows.Forms.TextBox();
			this.Check = new System.Windows.Forms.Button();
			this.Cancel = new System.Windows.Forms.Button();
			this.Status = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// FileName
			// 
			this.FileName.Location = new System.Drawing.Point( 12, 12 );
			this.FileName.Name = "FileName";
			this.FileName.Size = new System.Drawing.Size( 350, 20 );
			this.FileName.TabIndex = 0;
			this.FileName.KeyDown += new System.Windows.Forms.KeyEventHandler( this.FileName_KeyDown );
			// 
			// Check
			// 
			this.Check.Image = global::Unpacker.Properties.Resources.guess;
			this.Check.Location = new System.Drawing.Point( 287, 38 );
			this.Check.Name = "Check";
			this.Check.Size = new System.Drawing.Size( 75, 25 );
			this.Check.TabIndex = 1;
			this.Check.Text = "Check";
			this.Check.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
			this.Check.UseVisualStyleBackColor = true;
			this.Check.Click += new System.EventHandler( this.Check_Click );
			// 
			// Cancel
			// 
			this.Cancel.Image = global::Unpacker.Properties.Resources.close;
			this.Cancel.Location = new System.Drawing.Point( 206, 38 );
			this.Cancel.Name = "Cancel";
			this.Cancel.Size = new System.Drawing.Size( 75, 25 );
			this.Cancel.TabIndex = 2;
			this.Cancel.Text = "Close";
			this.Cancel.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
			this.Cancel.UseVisualStyleBackColor = true;
			this.Cancel.Click += new System.EventHandler( this.Cancel_Click );
			// 
			// Status
			// 
			this.Status.AutoSize = true;
			this.Status.Location = new System.Drawing.Point( 12, 44 );
			this.Status.Name = "Status";
			this.Status.Size = new System.Drawing.Size( 0, 13 );
			this.Status.TabIndex = 3;
			// 
			// GuessFile
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size( 374, 68 );
			this.Controls.Add( this.Status );
			this.Controls.Add( this.Cancel );
			this.Controls.Add( this.Check );
			this.Controls.Add( this.FileName );
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Name = "GuessFile";
			this.ShowIcon = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Guess File Name";
			this.ResumeLayout( false );
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TextBox FileName;
		private System.Windows.Forms.Button Check;
		private System.Windows.Forms.Button Cancel;
		private System.Windows.Forms.Label Status;
	}
}