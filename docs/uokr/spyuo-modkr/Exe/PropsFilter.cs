using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace SpyUO
{
	public class PropsFilter : System.Windows.Forms.Form
	{
		private System.Windows.Forms.CheckedListBox clbProps;
		private System.Windows.Forms.TextBox tbPropValue;
		private System.Windows.Forms.Button bOk;
		private System.Windows.Forms.Button bCancel;
		private System.Windows.Forms.Label lMust;
		private System.Windows.Forms.RadioButton rbHave;
		private System.Windows.Forms.RadioButton rbNotHave;
		private System.Windows.Forms.Label lValues;
		private System.Windows.Forms.TextBox tbFormat;

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
			this.clbProps = new System.Windows.Forms.CheckedListBox();
			this.tbPropValue = new System.Windows.Forms.TextBox();
			this.bOk = new System.Windows.Forms.Button();
			this.bCancel = new System.Windows.Forms.Button();
			this.lMust = new System.Windows.Forms.Label();
			this.rbHave = new System.Windows.Forms.RadioButton();
			this.rbNotHave = new System.Windows.Forms.RadioButton();
			this.lValues = new System.Windows.Forms.Label();
			this.tbFormat = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// clbProps
			// 
			this.clbProps.Location = new System.Drawing.Point(8, 8);
			this.clbProps.Name = "clbProps";
			this.clbProps.Size = new System.Drawing.Size(224, 124);
			this.clbProps.TabIndex = 0;
			this.clbProps.SelectedIndexChanged += new System.EventHandler(this.clbProps_SelectedIndexChanged);
			this.clbProps.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.clbProps_ItemCheck);
			// 
			// tbPropValue
			// 
			this.tbPropValue.AcceptsReturn = true;
			this.tbPropValue.AcceptsTab = true;
			this.tbPropValue.Enabled = false;
			this.tbPropValue.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.tbPropValue.Location = new System.Drawing.Point(8, 168);
			this.tbPropValue.Name = "tbPropValue";
			this.tbPropValue.Size = new System.Drawing.Size(136, 22);
			this.tbPropValue.TabIndex = 1;
			this.tbPropValue.Text = "";
			this.tbPropValue.TextChanged += new System.EventHandler(this.tbPropValue_TextChanged);
			// 
			// bOk
			// 
			this.bOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.bOk.Location = new System.Drawing.Point(144, 200);
			this.bOk.Name = "bOk";
			this.bOk.TabIndex = 3;
			this.bOk.Text = "Ok";
			// 
			// bCancel
			// 
			this.bCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.bCancel.Location = new System.Drawing.Point(24, 200);
			this.bCancel.Name = "bCancel";
			this.bCancel.TabIndex = 4;
			this.bCancel.Text = "Cancel";
			// 
			// lMust
			// 
			this.lMust.Enabled = false;
			this.lMust.Location = new System.Drawing.Point(8, 144);
			this.lMust.Name = "lMust";
			this.lMust.Size = new System.Drawing.Size(32, 16);
			this.lMust.TabIndex = 5;
			this.lMust.Text = "Must";
			// 
			// rbHave
			// 
			this.rbHave.Checked = true;
			this.rbHave.Enabled = false;
			this.rbHave.Location = new System.Drawing.Point(40, 136);
			this.rbHave.Name = "rbHave";
			this.rbHave.Size = new System.Drawing.Size(80, 16);
			this.rbHave.TabIndex = 6;
			this.rbHave.TabStop = true;
			this.rbHave.Text = "have one";
			this.rbHave.CheckedChanged += new System.EventHandler(this.rbHave_CheckedChanged);
			// 
			// rbNotHave
			// 
			this.rbNotHave.Enabled = false;
			this.rbNotHave.Location = new System.Drawing.Point(40, 152);
			this.rbNotHave.Name = "rbNotHave";
			this.rbNotHave.Size = new System.Drawing.Size(80, 16);
			this.rbNotHave.TabIndex = 7;
			this.rbNotHave.Text = "have none";
			this.rbNotHave.CheckedChanged += new System.EventHandler(this.rbNotHave_CheckedChanged);
			// 
			// lValues
			// 
			this.lValues.Enabled = false;
			this.lValues.Location = new System.Drawing.Point(120, 144);
			this.lValues.Name = "lValues";
			this.lValues.Size = new System.Drawing.Size(120, 16);
			this.lValues.TabIndex = 8;
			this.lValues.Text = "of these values (sep: |)";
			// 
			// tbFormat
			// 
			this.tbFormat.Location = new System.Drawing.Point(152, 168);
			this.tbFormat.Name = "tbFormat";
			this.tbFormat.ReadOnly = true;
			this.tbFormat.Size = new System.Drawing.Size(80, 20);
			this.tbFormat.TabIndex = 9;
			this.tbFormat.Text = "";
			this.tbFormat.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
			// 
			// PropsFilter
			// 
			this.AcceptButton = this.bOk;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.bCancel;
			this.ClientSize = new System.Drawing.Size(242, 232);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.tbFormat,
																		  this.lValues,
																		  this.rbNotHave,
																		  this.rbHave,
																		  this.lMust,
																		  this.bCancel,
																		  this.bOk,
																		  this.tbPropValue,
																		  this.clbProps});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "PropsFilter";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Property filter";
			this.ResumeLayout(false);

		}
		#endregion

		private string[] m_TypeNames;
		private string[] m_PropValues;
		private bool[] m_PropsHave;
		private string[] m_PropsFormat;

		public PropsFilter( string[] propNames, string[] typeNames, string[] propValues, bool[] propsHave, string[] propsFormat )
		{
			InitializeComponent();

			m_TypeNames = typeNames;
			m_PropsFormat = propsFormat;

			int length = propValues.Length;
			m_PropValues = new string[length];
			m_PropsHave = new bool[length];
			for ( int i = 0; i < length; i++ )
			{
				string propValue = propValues[i];
				m_PropValues[i] = propValue;

				clbProps.Items.Add( propNames[i], propValue != null );

				m_PropsHave[i] = propsHave[i];
			}
		}

		private void clbProps_SelectedIndexChanged( object sender, EventArgs e )
		{
			int index = clbProps.SelectedIndex;
			UpdatePropValue( clbProps.GetItemChecked( index ) );
		}

		private void clbProps_ItemCheck( object sender, ItemCheckEventArgs e )
		{
			UpdatePropValue( e.NewValue == CheckState.Checked );
		}

		private void UpdatePropValue( bool check )
		{
			int index = clbProps.SelectedIndex;

			if ( index >= 0 )
			{
				tbPropValue.Text = m_PropValues[index];
				rbHave.Checked = m_PropsHave[index];
				rbNotHave.Checked = !m_PropsHave[index];
				tbFormat.Text = m_PropsFormat[index].Replace( "{0", "{" + m_TypeNames[index] );

				tbPropValue.Enabled = check;
				lMust.Enabled = check;
				rbHave.Enabled = check;
				rbNotHave.Enabled = check;
				lValues.Enabled = check;
			}
			else
			{
				tbPropValue.Enabled = false;
				lMust.Enabled = false;
				rbHave.Enabled = false;
				rbNotHave.Enabled = false;
				lValues.Enabled = false;
			}
		}

		private void tbPropValue_TextChanged( object sender, System.EventArgs e )
		{
			int index = clbProps.SelectedIndex;
			m_PropValues[index] = tbPropValue.Text;
		}

		private void rbHave_CheckedChanged( object sender, System.EventArgs e )
		{
			HaveChanged();
		}

		private void rbNotHave_CheckedChanged( object sender, System.EventArgs e )
		{
			HaveChanged();
		}

		private void HaveChanged()
		{
			int index = clbProps.SelectedIndex;
			m_PropsHave[index] = rbHave.Checked;
		}

		public string[] GetPropValues()
		{
			int length = m_PropValues.Length;

			string[] ret = new string[length];
			for ( int i = 0; i < length; i++ )
			{
				if ( clbProps.GetItemChecked( i ) )
					ret[i] = m_PropValues[i] != null ? m_PropValues[i] : "";
				else
					ret[i] = null;
			}
			return ret;
		}

		public bool[] GetPropHaveValues()
		{
			int length = m_PropsHave.Length;

			bool[] ret = new bool[length];
			for ( int i = 0; i < length; i++ )
			{
				if ( clbProps.GetItemChecked( i ) )
					ret[i] = m_PropsHave[i];
				else
					ret[i] = true;
			}
			return ret;
		}
	}
}