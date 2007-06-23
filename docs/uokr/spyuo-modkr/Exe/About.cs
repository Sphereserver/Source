using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Diagnostics;

namespace SpyUO
{
	public class About : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label lEditor;
		private System.Windows.Forms.Label lPhenos;
		private System.Windows.Forms.Label lEmail;
		private System.Windows.Forms.LinkLabel llUodreams;

		private System.ComponentModel.Container components = null;

		public About()
		{
			InitializeComponent();

			Text = Display.Title;
		}

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
			this.lEditor = new System.Windows.Forms.Label();
			this.lPhenos = new System.Windows.Forms.Label();
			this.lEmail = new System.Windows.Forms.Label();
			this.llUodreams = new System.Windows.Forms.LinkLabel();
			this.SuspendLayout();
			// 
			// lEditor
			// 
			this.lEditor.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lEditor.Location = new System.Drawing.Point(8, 8);
			this.lEditor.Name = "lEditor";
			this.lEditor.Size = new System.Drawing.Size(80, 16);
			this.lEditor.TabIndex = 0;
			this.lEditor.Text = "Written by :";
			this.lEditor.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// lPhenos
			// 
			this.lPhenos.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lPhenos.Location = new System.Drawing.Point(96, 8);
			this.lPhenos.Name = "lPhenos";
			this.lPhenos.Size = new System.Drawing.Size(168, 16);
			this.lPhenos.TabIndex = 1;
			this.lPhenos.Text = "Lorenzo \"Phenos\" Castelli";
			// 
			// lEmail
			// 
			this.lEmail.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lEmail.Location = new System.Drawing.Point(8, 32);
			this.lEmail.Name = "lEmail";
			this.lEmail.Size = new System.Drawing.Size(72, 16);
			this.lEmail.TabIndex = 2;
			this.lEmail.Text = "E-Mail :";
			this.lEmail.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// llUodreams
			// 
			this.llUodreams.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.llUodreams.LinkArea = new System.Windows.Forms.LinkArea(0, 22);
			this.llUodreams.LinkBehavior = System.Windows.Forms.LinkBehavior.HoverUnderline;
			this.llUodreams.Location = new System.Drawing.Point(88, 32);
			this.llUodreams.Name = "llUodreams";
			this.llUodreams.Size = new System.Drawing.Size(168, 16);
			this.llUodreams.TabIndex = 3;
			this.llUodreams.TabStop = true;
			this.llUodreams.Text = "gcastelli@racine.ra.it";
			this.llUodreams.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.llUodreams_LinkClicked);
			// 
			// About
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(274, 56);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.llUodreams,
																		  this.lEmail,
																		  this.lPhenos,
																		  this.lEditor});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "About";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.ResumeLayout(false);

		}
		#endregion

		private void llUodreams_LinkClicked( object sender, System.Windows.Forms.LinkLabelLinkClickedEventArgs e )
		{
			ProcessStartInfo info = new ProcessStartInfo( "\"mailto:gcastelli@racine.ra.it\"" );
			info.UseShellExecute = true;
			info.ErrorDialog = true;
			System.Diagnostics.Process.Start( info );
		}
	}
}
