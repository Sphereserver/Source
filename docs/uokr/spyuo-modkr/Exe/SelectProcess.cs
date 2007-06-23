using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Diagnostics;

namespace SpyUO
{
	public class SelectProcess : System.Windows.Forms.Form
	{
		private class ProcessItem : IComparable
		{
			private Process m_Process;

			public Process Process { get { return m_Process; } }

			public ProcessItem( Process process )
			{
				m_Process = process;
			}

			public override string ToString()
			{
				return m_Process.ProcessName;
			}

			public int CompareTo( object obj )
			{
				ProcessItem pi = obj as ProcessItem;
				if ( pi == null )
					return -1;
				else
					return ToString().CompareTo( pi.ToString() );
			}
		}
		private System.Windows.Forms.Button bOk;
		private System.Windows.Forms.Button bCancel;
		private System.Windows.Forms.ListBox lbProcesses;

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
			this.bOk = new System.Windows.Forms.Button();
			this.bCancel = new System.Windows.Forms.Button();
			this.lbProcesses = new System.Windows.Forms.ListBox();
			this.SuspendLayout();
			// 
			// bOk
			// 
			this.bOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.bOk.Location = new System.Drawing.Point(88, 240);
			this.bOk.Name = "bOk";
			this.bOk.TabIndex = 1;
			this.bOk.Text = "Ok";
			// 
			// bCancel
			// 
			this.bCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.bCancel.Location = new System.Drawing.Point(8, 240);
			this.bCancel.Name = "bCancel";
			this.bCancel.Size = new System.Drawing.Size(72, 23);
			this.bCancel.TabIndex = 2;
			this.bCancel.Text = "Cancel";
			// 
			// lbProcesses
			// 
			this.lbProcesses.Name = "lbProcesses";
			this.lbProcesses.Size = new System.Drawing.Size(168, 238);
			this.lbProcesses.TabIndex = 3;
			// 
			// SelectProcess
			// 
			this.AcceptButton = this.bOk;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.bCancel;
			this.ClientSize = new System.Drawing.Size(168, 270);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.lbProcesses,
																		  this.bCancel,
																		  this.bOk});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "SelectProcess";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Select Process";
			this.ResumeLayout(false);

		}
		#endregion

		public SelectProcess()
		{
			InitializeComponent();

			Process[] processes = Process.GetProcesses();
			ArrayList list = new ArrayList( processes.Length );
			foreach ( Process process in processes )
				list.Add( new ProcessItem( process ) );
			list.Sort();

			lbProcesses.Items.AddRange( list.ToArray() );
		}

		public Process GetSelectedProcess()
		{
			ProcessItem pi = lbProcesses.SelectedItem as ProcessItem;

			if ( pi == null )
				return null;
			else
				return pi.Process;
		}
	}
}