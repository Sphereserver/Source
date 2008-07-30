namespace Unpacker
{
	partial class SelectProcess
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
			this.ProcessList = new System.Windows.Forms.ListBox();
			this.ButtonCancel = new System.Windows.Forms.Button();
			this.ButtonOkay = new System.Windows.Forms.Button();
			this.ButtonRefresh = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// ProcessList
			// 
			this.ProcessList.Dock = System.Windows.Forms.DockStyle.Top;
			this.ProcessList.FormattingEnabled = true;
			this.ProcessList.Location = new System.Drawing.Point( 0, 0 );
			this.ProcessList.Name = "ProcessList";
			this.ProcessList.Size = new System.Drawing.Size( 192, 329 );
			this.ProcessList.TabIndex = 0;
			// 
			// ButtonCancel
			// 
			this.ButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ButtonCancel.Location = new System.Drawing.Point( 0, 335 );
			this.ButtonCancel.Name = "ButtonCancel";
			this.ButtonCancel.Size = new System.Drawing.Size( 60, 23 );
			this.ButtonCancel.TabIndex = 1;
			this.ButtonCancel.Text = "Cancel";
			this.ButtonCancel.UseVisualStyleBackColor = true;
			// 
			// ButtonOkay
			// 
			this.ButtonOkay.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ButtonOkay.Location = new System.Drawing.Point( 132, 335 );
			this.ButtonOkay.Name = "ButtonOkay";
			this.ButtonOkay.Size = new System.Drawing.Size( 60, 23 );
			this.ButtonOkay.TabIndex = 2;
			this.ButtonOkay.Text = "Okay";
			this.ButtonOkay.UseVisualStyleBackColor = true;
			// 
			// ButtonRefresh
			// 
			this.ButtonRefresh.Location = new System.Drawing.Point( 66, 335 );
			this.ButtonRefresh.Name = "ButtonRefresh";
			this.ButtonRefresh.Size = new System.Drawing.Size( 60, 23 );
			this.ButtonRefresh.TabIndex = 3;
			this.ButtonRefresh.Text = "Refresh";
			this.ButtonRefresh.UseVisualStyleBackColor = true;
			this.ButtonRefresh.Click += new System.EventHandler( this.ButtonRefresh_Click );
			// 
			// SelectProcess
			// 
			this.AcceptButton = this.ButtonOkay;
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.ButtonCancel;
			this.ClientSize = new System.Drawing.Size( 192, 366 );
			this.Controls.Add( this.ButtonRefresh );
			this.Controls.Add( this.ButtonOkay );
			this.Controls.Add( this.ButtonCancel );
			this.Controls.Add( this.ProcessList );
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Name = "SelectProcess";
			this.ShowIcon = false;
			this.Text = "Select Process";
			this.ResumeLayout( false );

		}

		#endregion

		private System.Windows.Forms.ListBox ProcessList;
		private System.Windows.Forms.Button ButtonCancel;
		private System.Windows.Forms.Button ButtonOkay;
		private System.Windows.Forms.Button ButtonRefresh;
	}
}