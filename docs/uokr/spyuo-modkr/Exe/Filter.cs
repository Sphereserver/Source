using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.IO;
using SpyUO.Packets;

namespace SpyUO
{
	public class Filter : System.Windows.Forms.Form
	{
		private class PacketItem
		{
			private Type m_PacketType;
			private byte m_Cmd;
			private string[] m_Props;
			private bool[] m_PropsHave;

			public Type PacketType { get { return m_PacketType; } }
			public byte Cmd { get { return m_Cmd; } }
			public string[] Props
			{
				get { return m_Props; }
				set { m_Props = value; }
			}
			public bool[] PropsHave
			{
				get { return m_PropsHave; }
				set { m_PropsHave = value; }
			}

			public PacketItem( Type packetType, byte cmd, string[] props, bool[] propsHave )
			{
				m_PacketType = packetType;
				m_Cmd = cmd;

				if ( props != null )
				{
					int length = props.Length;
					m_Props = new string[props.Length];
					m_PropsHave = new bool[props.Length];
					for ( int i = 0; i < length; i++ )
					{
						m_Props[i] = props[i];
						m_PropsHave[i] = propsHave[i];
					}
				}
				else
					m_Props = null;
			}

			public bool GetIndeterminate()
			{
				if ( m_Props != null )
				{
					int n = 0;
					foreach ( string s in m_Props )
					{
						if ( s != null )
							n++;
					}

					return n != 0;
				}
				else
					return false;
			}

			public override string ToString()
			{
				return string.Format( "{0} ({1:X2})", m_PacketType != null ? m_PacketType.Name : "Packet", m_Cmd );
			}
		}

		private System.Windows.Forms.CheckedListBox clbPackets;
		private System.Windows.Forms.Button bOk;
		private System.Windows.Forms.Button bCancel;
		private System.Windows.Forms.Button bDetails;
		private System.Windows.Forms.CheckBox cbShowAll;
		private System.Windows.Forms.Button bSelectAll;
		private System.Windows.Forms.Button bUnselectAll;
		private System.Windows.Forms.Button bLoad;
		private System.Windows.Forms.Button bSave;
		private System.Windows.Forms.SaveFileDialog sfdSave;
		private System.Windows.Forms.OpenFileDialog ofdLoad;

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
			this.clbPackets = new System.Windows.Forms.CheckedListBox();
			this.bOk = new System.Windows.Forms.Button();
			this.bCancel = new System.Windows.Forms.Button();
			this.bDetails = new System.Windows.Forms.Button();
			this.cbShowAll = new System.Windows.Forms.CheckBox();
			this.bSelectAll = new System.Windows.Forms.Button();
			this.bUnselectAll = new System.Windows.Forms.Button();
			this.bLoad = new System.Windows.Forms.Button();
			this.bSave = new System.Windows.Forms.Button();
			this.sfdSave = new System.Windows.Forms.SaveFileDialog();
			this.ofdLoad = new System.Windows.Forms.OpenFileDialog();
			this.SuspendLayout();
			// 
			// clbPackets
			// 
			this.clbPackets.Location = new System.Drawing.Point(8, 56);
			this.clbPackets.Name = "clbPackets";
			this.clbPackets.Size = new System.Drawing.Size(232, 229);
			this.clbPackets.TabIndex = 0;
			this.clbPackets.SelectedIndexChanged += new System.EventHandler(this.clbPackets_SelectedIndexChanged);
			this.clbPackets.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.clbPackets_ItemCheck);
			// 
			// bOk
			// 
			this.bOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.bOk.Location = new System.Drawing.Point(136, 336);
			this.bOk.Name = "bOk";
			this.bOk.TabIndex = 1;
			this.bOk.Text = "Ok";
			this.bOk.Click += new System.EventHandler(this.bOk_Click);
			// 
			// bCancel
			// 
			this.bCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.bCancel.Location = new System.Drawing.Point(32, 336);
			this.bCancel.Name = "bCancel";
			this.bCancel.TabIndex = 2;
			this.bCancel.Text = "Cancel";
			// 
			// bDetails
			// 
			this.bDetails.Enabled = false;
			this.bDetails.Location = new System.Drawing.Point(160, 296);
			this.bDetails.Name = "bDetails";
			this.bDetails.TabIndex = 3;
			this.bDetails.Text = "Details...";
			this.bDetails.Click += new System.EventHandler(this.bDetails_Click);
			// 
			// cbShowAll
			// 
			this.cbShowAll.Location = new System.Drawing.Point(16, 296);
			this.cbShowAll.Name = "cbShowAll";
			this.cbShowAll.Size = new System.Drawing.Size(72, 24);
			this.cbShowAll.TabIndex = 4;
			this.cbShowAll.Text = "Show all";
			this.cbShowAll.CheckedChanged += new System.EventHandler(this.cbShowAll_CheckedChanged);
			// 
			// bSelectAll
			// 
			this.bSelectAll.Location = new System.Drawing.Point(24, 40);
			this.bSelectAll.Name = "bSelectAll";
			this.bSelectAll.Size = new System.Drawing.Size(75, 16);
			this.bSelectAll.TabIndex = 5;
			this.bSelectAll.Text = "select all";
			this.bSelectAll.Click += new System.EventHandler(this.bSelectAll_Click);
			// 
			// bUnselectAll
			// 
			this.bUnselectAll.Location = new System.Drawing.Point(152, 40);
			this.bUnselectAll.Name = "bUnselectAll";
			this.bUnselectAll.Size = new System.Drawing.Size(75, 16);
			this.bUnselectAll.TabIndex = 6;
			this.bUnselectAll.Text = "unselect all";
			this.bUnselectAll.Click += new System.EventHandler(this.bUnselectAll_Click);
			// 
			// bLoad
			// 
			this.bLoad.Location = new System.Drawing.Point(8, 8);
			this.bLoad.Name = "bLoad";
			this.bLoad.Size = new System.Drawing.Size(104, 23);
			this.bLoad.TabIndex = 7;
			this.bLoad.Text = "Load...";
			this.bLoad.Click += new System.EventHandler(this.bLoad_Click);
			// 
			// bSave
			// 
			this.bSave.Location = new System.Drawing.Point(136, 8);
			this.bSave.Name = "bSave";
			this.bSave.Size = new System.Drawing.Size(104, 23);
			this.bSave.TabIndex = 8;
			this.bSave.Text = "Save...";
			this.bSave.Click += new System.EventHandler(this.bSave_Click);
			// 
			// sfdSave
			// 
			this.sfdSave.FileName = "Filter.flt";
			this.sfdSave.Filter = "Filter files (*.ftl)|*.flt";
			// 
			// ofdLoad
			// 
			this.ofdLoad.FileName = "Filter.flt";
			this.ofdLoad.Filter = "Filter files (*.ftl)|*.flt";
			// 
			// Filter
			// 
			this.AcceptButton = this.bOk;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.bCancel;
			this.ClientSize = new System.Drawing.Size(250, 368);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.bSave,
																		  this.bLoad,
																		  this.bUnselectAll,
																		  this.bSelectAll,
																		  this.cbShowAll,
																		  this.bDetails,
																		  this.bCancel,
																		  this.bOk,
																		  this.clbPackets});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "Filter";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Filter";
			this.ResumeLayout(false);

		}
		#endregion

		private PacketFilter m_Filter;
		private PacketItem[] m_PacketItems;
		private bool[] m_Checked;

		public bool ShowAll { get { return cbShowAll.Checked; } }

		public Filter( PacketFilter filter, bool showAll )
		{
			InitializeComponent();

			m_PacketItems = new PacketItem[0x100];
			m_Checked = new bool[0x100];
			for ( int i = 0; i < 0x100; i++ )
			{
				m_PacketItems[i] = new PacketItem( Packet.Table[i], (byte)i, filter.PropsTable[i], filter.PropsHave[i] );
				m_Checked[i] = filter.Table[i];
			}

			m_Filter = filter;
			cbShowAll.Checked = showAll;

			UpdatePackets();
		}

		private CheckState GetCheckState( PacketItem pi )
		{
			if ( !m_Checked[pi.Cmd] )
				return CheckState.Unchecked;
			else if ( pi.GetIndeterminate() )
				return CheckState.Indeterminate;
			else
				return CheckState.Checked;
		}

		private void UpdatePackets()
		{
			clbPackets.BeginUpdate();

			clbPackets.Items.Clear();

			for ( int i = 0; i < m_PacketItems.Length; i++ )
			{
				PacketItem pi = m_PacketItems[i];
				if ( ShowAll || pi.PacketType != null )
				{
					CheckState state = GetCheckState( pi );
					clbPackets.Items.Add( pi, state );
				}
			}

			clbPackets.EndUpdate();
		}

		private void UpdateDetails( CheckState check )
		{
			int index = clbPackets.SelectedIndex;
			PacketItem pi = clbPackets.SelectedItem as PacketItem;
			if ( pi != null && pi.Props != null && check != CheckState.Unchecked )
				bDetails.Enabled = true;
			else
				bDetails.Enabled = false;
		}

		private void cbShowAll_CheckedChanged( object sender, System.EventArgs e )
		{
			UpdatePackets();
		}

		private void clbPackets_SelectedIndexChanged( object sender, System.EventArgs e )
		{
			int index = clbPackets.SelectedIndex;
			UpdateDetails( clbPackets.GetItemCheckState( index ) );
		}

		private void SetAllProps( PacketItem pi )
		{
			if ( pi.Props != null )
			{
				for ( int i = 0; i < pi.Props.Length; i++ )
				{
					pi.Props[i] = null;
					pi.PropsHave[i] = true;
				}
			}
		}

		private void clbPackets_ItemCheck( object sender, ItemCheckEventArgs e )
		{
			PacketItem pi = clbPackets.SelectedItem as PacketItem;
			if ( pi != null )
			{
				m_Checked[pi.Cmd] = e.NewValue != CheckState.Unchecked;

				if ( e.NewValue == CheckState.Checked )
					SetAllProps( pi );
			}

			UpdateDetails( e.NewValue );
		}

		private void bDetails_Click( object sender, System.EventArgs e )
		{
			PacketItem pi = clbPackets.SelectedItem as PacketItem;
			if ( pi != null )
			{
				PacketProp[] props = Packet.PropsTable[pi.Cmd];

				string[] names = new string[props.Length];
				string[] typeNames = new String[props.Length];
				string[] formats = new string[props.Length];
				for ( int i = 0; i < props.Length; i++ )
				{
					names[i] = props[i].PropInfo.Name;
					typeNames[i] = props[i].PropInfo.PropertyType.Name;
					formats[i] = props[i].Attribute.Format;
				}

				PropsFilter pFilter = new PropsFilter( names, typeNames, pi.Props, pi.PropsHave, formats );
				pFilter.TopMost = TopMost;
				if ( pFilter.ShowDialog() == DialogResult.OK )
				{
					pi.Props = pFilter.GetPropValues();
					pi.PropsHave = pFilter.GetPropHaveValues();

					clbPackets.SetItemCheckState( clbPackets.SelectedIndex, GetCheckState( pi ) );
				}
			}
		}

		private void bOk_Click( object sender, EventArgs e )
		{
			for ( int i = 0; i < 0x100; i++ )
			{
				PacketItem pi = m_PacketItems[i];

				bool check = m_Checked[i];
				m_Filter.Table[i] = check;

				if ( check )
				{
					m_Filter.PropsTable[i] = pi.Props;
					m_Filter.PropsHave[i] = pi.PropsHave;
				}
			}
		}

		private void bSelectAll_Click( object sender, System.EventArgs e )
		{
			for ( int i = 0; i < m_Checked.Length; i++ )
			{
				m_Checked[i] = true;
				SetAllProps( m_PacketItems[i] );
			}

			cbShowAll.Checked = true;

			UpdatePackets();
		}

		private void bUnselectAll_Click( object sender, System.EventArgs e )
		{
			for ( int i = 0; i < m_Checked.Length; i++ )
				m_Checked[i] = false;

			UpdatePackets();
		}

		public void SaveFilter( BinaryWriter writer )
		{
			writer.Write( (int)0 ); // Version

			writer.Write( cbShowAll.Checked );

			for ( int i = 0; i < 0x100; i++ )
			{
				bool check = m_Checked[i];
				writer.Write( check );

				if ( check )
				{
					PacketItem pi = m_PacketItems[i];
					if ( pi.Props == null )
					{
						writer.Write( (int)0 );
					}
					else
					{
						int length = pi.Props.Length;

						writer.Write( length );
						for ( int j = 0; j < length; j++ )
						{
							if ( pi.Props[j] != null )
							{
								writer.Write( true );

								writer.Write( pi.Props[j] );
								writer.Write( pi.PropsHave[j] );
							}
							else
							{
								writer.Write( false );
							}
						}
					}
				}
			}
		}

		public void LoadFilter( BinaryReader reader )
		{
			int version = reader.ReadInt32();

			cbShowAll.Checked = reader.ReadBoolean();

			for ( int i = 0; i < 0x100; i++ )
			{
				bool check = reader.ReadBoolean();
				m_Checked[i] = check;

				if ( check )
				{
					PacketItem pi = m_PacketItems[i];

					int length = reader.ReadInt32();

					for ( int j = 0; j < length; j++ )
					{
						if ( reader.ReadBoolean() )
						{
							string s = reader.ReadString();
							bool b = reader.ReadBoolean();
							if ( pi.Props != null && j < pi.Props.Length )
							{
								pi.Props[j] = s;
								pi.PropsHave[j] = b;
							}
						}
						else if ( pi.Props != null && j < pi.Props.Length )
						{
							pi.Props[j] = null;
							pi.PropsHave[j] = true;
						}
					}
				}
			}

			UpdatePackets();
		}

		private void bSave_Click( object sender, System.EventArgs e )
		{
			if ( sfdSave.ShowDialog() == DialogResult.OK )
			{
				FileStream stream = null;
				BinaryWriter writer = null;
				try
				{
					stream = File.Create( sfdSave.FileName );
					writer = new BinaryWriter( stream );

					SaveFilter( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
					if ( stream != null )
						stream.Close();
				}
			}
		}

		private void bLoad_Click( object sender, System.EventArgs e )
		{
			if ( ofdLoad.ShowDialog() == DialogResult.OK )
			{
				FileStream stream = null;
				BinaryReader reader = null;
				try
				{
					stream = File.OpenRead( ofdLoad.FileName );
					reader = new BinaryReader( stream );

					LoadFilter( reader );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( reader != null )
						reader.Close();
					if ( stream != null )
						stream.Close();
				}
			}
		}
	}
}