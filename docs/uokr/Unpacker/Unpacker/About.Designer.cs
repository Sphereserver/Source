namespace Unpacker
{
	partial class About
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager( typeof( About ) );
			this.Picture = new System.Windows.Forms.PictureBox();
			this.AuthorLabel = new System.Windows.Forms.Label();
			this.NameLabel = new System.Windows.Forms.Label();
			this.AppName = new System.Windows.Forms.Label();
			this.Author = new System.Windows.Forms.Label();
			this.CopyrightLabel = new System.Windows.Forms.Label();
			this.Copyright = new System.Windows.Forms.Label();
			this.Comments = new System.Windows.Forms.RichTextBox();
			this.CommentLabel = new System.Windows.Forms.Label();
			( (System.ComponentModel.ISupportInitialize) ( this.Picture ) ).BeginInit();
			this.SuspendLayout();
			// 
			// Picture
			// 
			this.Picture.Image = global::Unpacker.Properties.Resources.banner;
			this.Picture.InitialImage = global::Unpacker.Properties.Resources.banner;
			this.Picture.Location = new System.Drawing.Point( 12, 12 );
			this.Picture.Name = "Picture";
			this.Picture.Size = new System.Drawing.Size( 320, 80 );
			this.Picture.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
			this.Picture.TabIndex = 0;
			this.Picture.TabStop = false;
			// 
			// AuthorLabel
			// 
			this.AuthorLabel.AutoSize = true;
			this.AuthorLabel.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte) ( 0 ) ) );
			this.AuthorLabel.Location = new System.Drawing.Point( 12, 118 );
			this.AuthorLabel.Margin = new System.Windows.Forms.Padding( 3, 5, 3, 0 );
			this.AuthorLabel.Name = "AuthorLabel";
			this.AuthorLabel.Size = new System.Drawing.Size( 48, 13 );
			this.AuthorLabel.TabIndex = 1;
			this.AuthorLabel.Text = "Author:";
			// 
			// NameLabel
			// 
			this.NameLabel.AutoSize = true;
			this.NameLabel.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte) ( 0 ) ) );
			this.NameLabel.Location = new System.Drawing.Point( 12, 100 );
			this.NameLabel.Margin = new System.Windows.Forms.Padding( 3, 5, 3, 0 );
			this.NameLabel.Name = "NameLabel";
			this.NameLabel.Size = new System.Drawing.Size( 43, 13 );
			this.NameLabel.TabIndex = 2;
			this.NameLabel.Text = "Name:";
			// 
			// AppName
			// 
			this.AppName.AutoSize = true;
			this.AppName.Location = new System.Drawing.Point( 82, 100 );
			this.AppName.Name = "AppName";
			this.AppName.Size = new System.Drawing.Size( 204, 13 );
			this.AppName.TabIndex = 3;
			this.AppName.Text = "Ultima Online: Kingdom Reborn Unpacker";
			// 
			// Author
			// 
			this.Author.AutoSize = true;
			this.Author.Location = new System.Drawing.Point( 82, 118 );
			this.Author.Name = "Author";
			this.Author.Size = new System.Drawing.Size( 49, 13 );
			this.Author.TabIndex = 4;
			this.Author.Text = "Malganis";
			// 
			// CopyrightLabel
			// 
			this.CopyrightLabel.AutoSize = true;
			this.CopyrightLabel.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte) ( 0 ) ) );
			this.CopyrightLabel.Location = new System.Drawing.Point( 12, 136 );
			this.CopyrightLabel.Margin = new System.Windows.Forms.Padding( 3, 5, 3, 0 );
			this.CopyrightLabel.Name = "CopyrightLabel";
			this.CopyrightLabel.Size = new System.Drawing.Size( 64, 13 );
			this.CopyrightLabel.TabIndex = 5;
			this.CopyrightLabel.Text = "Copyright:";
			// 
			// Copyright
			// 
			this.Copyright.AutoSize = true;
			this.Copyright.Location = new System.Drawing.Point( 82, 136 );
			this.Copyright.Name = "Copyright";
			this.Copyright.Size = new System.Drawing.Size( 96, 13 );
			this.Copyright.TabIndex = 6;
			this.Copyright.Text = "© 2008 Everybody";
			// 
			// Comments
			// 
			this.Comments.Location = new System.Drawing.Point( 12, 173 );
			this.Comments.Margin = new System.Windows.Forms.Padding( 3, 5, 3, 3 );
			this.Comments.Name = "Comments";
			this.Comments.ReadOnly = true;
			this.Comments.Size = new System.Drawing.Size( 320, 91 );
			this.Comments.TabIndex = 7;
			this.Comments.Text = resources.GetString( "Comments.Text" );
			// 
			// CommentLabel
			// 
			this.CommentLabel.AutoSize = true;
			this.CommentLabel.Font = new System.Drawing.Font( "Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ( (byte) ( 0 ) ) );
			this.CommentLabel.Location = new System.Drawing.Point( 12, 155 );
			this.CommentLabel.Margin = new System.Windows.Forms.Padding( 3, 5, 3, 0 );
			this.CommentLabel.Name = "CommentLabel";
			this.CommentLabel.Size = new System.Drawing.Size( 68, 13 );
			this.CommentLabel.TabIndex = 8;
			this.CommentLabel.Text = "Comments:";
			// 
			// About
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size( 344, 276 );
			this.Controls.Add( this.CommentLabel );
			this.Controls.Add( this.Comments );
			this.Controls.Add( this.Copyright );
			this.Controls.Add( this.CopyrightLabel );
			this.Controls.Add( this.Author );
			this.Controls.Add( this.AppName );
			this.Controls.Add( this.NameLabel );
			this.Controls.Add( this.AuthorLabel );
			this.Controls.Add( this.Picture );
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "About";
			this.ShowIcon = false;
			this.Text = "About";
			( (System.ComponentModel.ISupportInitialize) ( this.Picture ) ).EndInit();
			this.ResumeLayout( false );
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.PictureBox Picture;
		private System.Windows.Forms.Label AuthorLabel;
		private System.Windows.Forms.Label NameLabel;
		private System.Windows.Forms.Label AppName;
		private System.Windows.Forms.Label Author;
		private System.Windows.Forms.Label CopyrightLabel;
		private System.Windows.Forms.Label Copyright;
		private System.Windows.Forms.RichTextBox Comments;
		private System.Windows.Forms.Label CommentLabel;
	}
}