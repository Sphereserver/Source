using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace SpyUO
{
	public class SelectBook : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button bCancel;
		private System.Windows.Forms.ListBox lbBooks;
		private System.Windows.Forms.Button bExctract;

		private System.ComponentModel.Container components = null;

		protected override void Dispose( bool disposing )
		{
			if ( disposing )
			{
				if ( components != null )
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Metodo necessario per il supporto della finestra di progettazione. Non modificare
		/// il contenuto del metodo con l'editor di codice.
		/// </summary>
		private void InitializeComponent()
		{
			this.bExctract = new System.Windows.Forms.Button();
			this.bCancel = new System.Windows.Forms.Button();
			this.lbBooks = new System.Windows.Forms.ListBox();
			this.SuspendLayout();
			// 
			// bExctract
			// 
			this.bExctract.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.bExctract.Location = new System.Drawing.Point(136, 240);
			this.bExctract.Name = "bExctract";
			this.bExctract.TabIndex = 1;
			this.bExctract.Text = "Extract...";
			// 
			// bCancel
			// 
			this.bCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.bCancel.Location = new System.Drawing.Point(32, 240);
			this.bCancel.Name = "bCancel";
			this.bCancel.Size = new System.Drawing.Size(72, 23);
			this.bCancel.TabIndex = 2;
			this.bCancel.Text = "Cancel";
			// 
			// lbBooks
			// 
			this.lbBooks.HorizontalScrollbar = true;
			this.lbBooks.Name = "lbBooks";
			this.lbBooks.Size = new System.Drawing.Size(248, 238);
			this.lbBooks.TabIndex = 3;
			// 
			// SelectBook
			// 
			this.AcceptButton = this.bExctract;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.bCancel;
			this.ClientSize = new System.Drawing.Size(250, 270);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.lbBooks,
																		  this.bCancel,
																		  this.bExctract});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "SelectBook";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Select Book";
			this.ResumeLayout(false);

		}
		#endregion

		public SelectBook( ArrayList books )
		{
			InitializeComponent();

			books.Sort();

			lbBooks.Items.AddRange( books.ToArray() );
		}

		public BookInfo GetSelectedBook()
		{
			BookInfo book = lbBooks.SelectedItem as BookInfo;

			if ( book == null )
				return null;
			else
				return book;
		}
	}
}