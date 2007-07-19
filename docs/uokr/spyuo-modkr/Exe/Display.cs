using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using SpyUO.Packets;

namespace SpyUO
{
	public class Display : Form, ICounterDisplay
	{
		public const string Title = "SpyUO 1.10 Mod KR";

		public static void Main()
		{
			try
			{
				SetupClientsConfig( "Clients.cfg" );
				SetupItemsTable( "Items.cfg" );
				Application.Run( new Display() );
			}
			catch ( Exception e )
			{
				MessageBox.Show( e.ToString(), "Application error", MessageBoxButtons.OK, MessageBoxIcon.Error );
			}
		}

		private static void SetupClientsConfig( string file )
		{
			try
			{
				PacketPump.ClientsConfig.AddToTable( file );
			}
			catch ( Exception e )
			{
				throw new Exception( "Error reading " + file, e );
			}
		}

		private static void SetupItemsTable( string file )
		{
			try
			{
				PacketRecorder.InitItemsTable( file );
			}
			catch ( Exception e )
			{
				throw new Exception( "Error reading " + file, e );
			}
		}

		private System.Windows.Forms.ListView lvPackets;
		private System.Windows.Forms.ColumnHeader chType;
		private System.Windows.Forms.ColumnHeader chMessage;
		private System.Windows.Forms.ColumnHeader chPacket;
		private System.Windows.Forms.ColumnHeader chTime;
		private System.Windows.Forms.MainMenu mainMenu;
		private System.Windows.Forms.MenuItem miProcess;
		private System.Windows.Forms.MenuItem miStart;
		private System.Windows.Forms.MenuItem miAttach;
		private System.Windows.Forms.MenuItem miDetach;
		private System.Windows.Forms.MenuItem miExitSeparator;
		private System.Windows.Forms.MenuItem miExit;
		private System.Windows.Forms.OpenFileDialog ofdStart;
		private System.Windows.Forms.MenuItem miOptions;
		private System.Windows.Forms.MenuItem miFilter;
		private System.Windows.Forms.MenuItem miOnTop;
		private System.Windows.Forms.MenuItem miAutoScrolling;
		private System.Windows.Forms.ColumnHeader chRelTime;
		private System.Windows.Forms.MenuItem miLogs;
		private System.Windows.Forms.MenuItem miClear;
		private System.Windows.Forms.MenuItem miPause;
		private System.Windows.Forms.MenuItem miSaveAs;
		private System.Windows.Forms.SaveFileDialog sfdSaveAs;
		private System.Windows.Forms.MenuItem miHelp;
		private System.Windows.Forms.MenuItem miAbout;
		private System.Windows.Forms.ColumnHeader chDifTime;
		private System.Windows.Forms.ColumnHeader chASCII;
		private System.Windows.Forms.MenuItem miTools;
		private System.Windows.Forms.SaveFileDialog sfdExtractItems;
		private System.Windows.Forms.MenuItem miExtractItems;
		private System.Windows.Forms.MenuItem miExtractGump;
		private System.Windows.Forms.SaveFileDialog sfdExtractGump;
		private System.Windows.Forms.MenuItem miSaveHexAs;
		private System.Windows.Forms.ColumnHeader chLength;
		private System.Windows.Forms.MenuItem miLoad;
		private System.Windows.Forms.OpenFileDialog ofdLoad;
		private System.Windows.Forms.MenuItem miExtractGumpSphere;
		private System.Windows.Forms.SaveFileDialog sfdExtractGumpSphere;
		private System.Windows.Forms.MenuItem miSetBaseTime;
		private System.Windows.Forms.MenuItem miHide;
		private System.Windows.Forms.MenuItem miShowHidden;
		private System.Windows.Forms.MenuItem miExtractBooks;
		private System.Windows.Forms.SaveFileDialog sfdExtractBook;
		private System.Windows.Forms.MenuItem miSaveBin;
		private System.Windows.Forms.SaveFileDialog sfdSaveBin;
		private System.Windows.Forms.MenuItem miLoadBin;
		private System.Windows.Forms.OpenFileDialog ofdLoadBin;

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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(Display));
			this.lvPackets = new System.Windows.Forms.ListView();
			this.chType = new System.Windows.Forms.ColumnHeader();
			this.chMessage = new System.Windows.Forms.ColumnHeader();
			this.chTime = new System.Windows.Forms.ColumnHeader();
			this.chRelTime = new System.Windows.Forms.ColumnHeader();
			this.chDifTime = new System.Windows.Forms.ColumnHeader();
			this.chPacket = new System.Windows.Forms.ColumnHeader();
			this.chASCII = new System.Windows.Forms.ColumnHeader();
			this.chLength = new System.Windows.Forms.ColumnHeader();
			this.mainMenu = new System.Windows.Forms.MainMenu();
			this.miProcess = new System.Windows.Forms.MenuItem();
			this.miStart = new System.Windows.Forms.MenuItem();
			this.miAttach = new System.Windows.Forms.MenuItem();
			this.miDetach = new System.Windows.Forms.MenuItem();
			this.miExitSeparator = new System.Windows.Forms.MenuItem();
			this.miExit = new System.Windows.Forms.MenuItem();
			this.miLogs = new System.Windows.Forms.MenuItem();
			this.miLoad = new System.Windows.Forms.MenuItem();
			this.miSaveAs = new System.Windows.Forms.MenuItem();
			this.miSaveHexAs = new System.Windows.Forms.MenuItem();
			this.miPause = new System.Windows.Forms.MenuItem();
			this.miClear = new System.Windows.Forms.MenuItem();
			this.miSetBaseTime = new System.Windows.Forms.MenuItem();
			this.miHide = new System.Windows.Forms.MenuItem();
			this.miShowHidden = new System.Windows.Forms.MenuItem();
			this.miOptions = new System.Windows.Forms.MenuItem();
			this.miFilter = new System.Windows.Forms.MenuItem();
			this.miOnTop = new System.Windows.Forms.MenuItem();
			this.miAutoScrolling = new System.Windows.Forms.MenuItem();
			this.miTools = new System.Windows.Forms.MenuItem();
			this.miExtractItems = new System.Windows.Forms.MenuItem();
			this.miExtractBooks = new System.Windows.Forms.MenuItem();
			this.miExtractGump = new System.Windows.Forms.MenuItem();
			this.miExtractGumpSphere = new System.Windows.Forms.MenuItem();
			this.miHelp = new System.Windows.Forms.MenuItem();
			this.miAbout = new System.Windows.Forms.MenuItem();
			this.ofdStart = new System.Windows.Forms.OpenFileDialog();
			this.sfdSaveAs = new System.Windows.Forms.SaveFileDialog();
			this.sfdExtractItems = new System.Windows.Forms.SaveFileDialog();
			this.sfdExtractGump = new System.Windows.Forms.SaveFileDialog();
			this.ofdLoad = new System.Windows.Forms.OpenFileDialog();
			this.sfdExtractGumpSphere = new System.Windows.Forms.SaveFileDialog();
			this.sfdExtractBook = new System.Windows.Forms.SaveFileDialog();
			this.miSaveBin = new System.Windows.Forms.MenuItem();
			this.sfdSaveBin = new System.Windows.Forms.SaveFileDialog();
			this.miLoadBin = new System.Windows.Forms.MenuItem();
			this.ofdLoadBin = new System.Windows.Forms.OpenFileDialog();
			this.SuspendLayout();
			// 
			// lvPackets
			// 
			this.lvPackets.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																						this.chType,
																						this.chMessage,
																						this.chTime,
																						this.chRelTime,
																						this.chDifTime,
																						this.chPacket,
																						this.chASCII,
																						this.chLength});
			this.lvPackets.Dock = System.Windows.Forms.DockStyle.Fill;
			this.lvPackets.Font = new System.Drawing.Font("Arial Unicode MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lvPackets.FullRowSelect = true;
			this.lvPackets.GridLines = true;
			this.lvPackets.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.lvPackets.Location = new System.Drawing.Point(0, 0);
			this.lvPackets.MultiSelect = false;
			this.lvPackets.Name = "lvPackets";
			this.lvPackets.Size = new System.Drawing.Size(552, 414);
			this.lvPackets.TabIndex = 0;
			this.lvPackets.View = System.Windows.Forms.View.Details;
			this.lvPackets.ItemActivate += new System.EventHandler(this.lvPackets_ItemActivate);
			this.lvPackets.SelectedIndexChanged += new System.EventHandler(this.lvPackets_SelectedIndexChanged);
			// 
			// chType
			// 
			this.chType.Text = "Type";
			this.chType.Width = 130;
			// 
			// chMessage
			// 
			this.chMessage.Text = "Message";
			this.chMessage.Width = 330;
			// 
			// chTime
			// 
			this.chTime.Text = "Time";
			this.chTime.Width = 75;
			// 
			// chRelTime
			// 
			this.chRelTime.Text = "Rel time";
			// 
			// chDifTime
			// 
			this.chDifTime.Text = "Dif time";
			// 
			// chPacket
			// 
			this.chPacket.Text = "Packet";
			this.chPacket.Width = 250;
			// 
			// chASCII
			// 
			this.chASCII.Text = "ASCII";
			this.chASCII.Width = 250;
			// 
			// chLength
			// 
			this.chLength.Text = "Length";
			this.chLength.Width = 50;
			// 
			// mainMenu
			// 
			this.mainMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					 this.miProcess,
																					 this.miLogs,
																					 this.miOptions,
																					 this.miTools,
																					 this.miHelp});
			// 
			// miProcess
			// 
			this.miProcess.Index = 0;
			this.miProcess.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.miStart,
																					  this.miAttach,
																					  this.miDetach,
																					  this.miExitSeparator,
																					  this.miExit});
			this.miProcess.Text = "&Process";
			// 
			// miStart
			// 
			this.miStart.Index = 0;
			this.miStart.Text = "&Start...";
			this.miStart.Click += new System.EventHandler(this.miStart_Click);
			// 
			// miAttach
			// 
			this.miAttach.Index = 1;
			this.miAttach.Text = "&Attach...";
			this.miAttach.Click += new System.EventHandler(this.miAttach_Click);
			// 
			// miDetach
			// 
			this.miDetach.Enabled = false;
			this.miDetach.Index = 2;
			this.miDetach.Text = "&Detach";
			this.miDetach.Click += new System.EventHandler(this.miDetach_Click);
			// 
			// miExitSeparator
			// 
			this.miExitSeparator.Index = 3;
			this.miExitSeparator.Text = "-";
			// 
			// miExit
			// 
			this.miExit.Index = 4;
			this.miExit.Text = "&Exit";
			this.miExit.Click += new System.EventHandler(this.miExit_Click);
			// 
			// miLogs
			// 
			this.miLogs.Index = 1;
			this.miLogs.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																				   this.miLoad,
																				   this.miSaveAs,
																				   this.miSaveHexAs,
																				   this.miLoadBin,
																				   this.miSaveBin,
																				   this.miPause,
																				   this.miClear,
																				   this.miSetBaseTime,
																				   this.miHide,
																				   this.miShowHidden});
			this.miLogs.Text = "&Logs";
			// 
			// miLoad
			// 
			this.miLoad.Index = 0;
			this.miLoad.Text = "&Load...";
			this.miLoad.Click += new System.EventHandler(this.miLoad_Click);
			// 
			// miSaveAs
			// 
			this.miSaveAs.Index = 1;
			this.miSaveAs.Text = "&Save as...";
			this.miSaveAs.Click += new System.EventHandler(this.miSaveAs_Click);
			// 
			// miSaveHexAs
			// 
			this.miSaveHexAs.Index = 2;
			this.miSaveHexAs.Text = "Save &hex as...";
			this.miSaveHexAs.Click += new System.EventHandler(this.miSaveHexAs_Click);
			// 
			// miPause
			// 
			this.miPause.Enabled = false;
			this.miPause.Index = 5;
			this.miPause.Text = "&Pause";
			this.miPause.Click += new System.EventHandler(this.miPause_Click);
			// 
			// miClear
			// 
			this.miClear.Index = 6;
			this.miClear.Text = "&Clear";
			this.miClear.Click += new System.EventHandler(this.miClear_Click);
			// 
			// miSetBaseTime
			// 
			this.miSetBaseTime.Enabled = false;
			this.miSetBaseTime.Index = 7;
			this.miSetBaseTime.Text = "Set &base time";
			this.miSetBaseTime.Click += new System.EventHandler(this.miSetBaseTime_Click);
			// 
			// miHide
			// 
			this.miHide.Enabled = false;
			this.miHide.Index = 8;
			this.miHide.Shortcut = System.Windows.Forms.Shortcut.Del;
			this.miHide.Text = "&Hide";
			this.miHide.Click += new System.EventHandler(this.miHide_Click);
			// 
			// miShowHidden
			// 
			this.miShowHidden.Enabled = false;
			this.miShowHidden.Index = 9;
			this.miShowHidden.Text = "Show hi&dden";
			this.miShowHidden.Click += new System.EventHandler(this.miShowHidden_Click);
			// 
			// miOptions
			// 
			this.miOptions.Index = 2;
			this.miOptions.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.miFilter,
																					  this.miOnTop,
																					  this.miAutoScrolling});
			this.miOptions.Text = "&Options";
			// 
			// miFilter
			// 
			this.miFilter.Index = 0;
			this.miFilter.Text = "&Filter...";
			this.miFilter.Click += new System.EventHandler(this.miFilter_Click);
			// 
			// miOnTop
			// 
			this.miOnTop.Index = 1;
			this.miOnTop.Text = "Always on &top";
			this.miOnTop.Click += new System.EventHandler(this.miOnTop_Click);
			// 
			// miAutoScrolling
			// 
			this.miAutoScrolling.Index = 2;
			this.miAutoScrolling.Text = "Auto &scrolling";
			this.miAutoScrolling.Click += new System.EventHandler(this.miAutoScrolling_Click);
			// 
			// miTools
			// 
			this.miTools.Index = 3;
			this.miTools.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					this.miExtractItems,
																					this.miExtractBooks,
																					this.miExtractGump,
																					this.miExtractGumpSphere});
			this.miTools.Text = "&Tools";
			// 
			// miExtractItems
			// 
			this.miExtractItems.Index = 0;
			this.miExtractItems.Text = "Extract &items...";
			this.miExtractItems.Click += new System.EventHandler(this.miExtractItems_Click);
			// 
			// miExtractBooks
			// 
			this.miExtractBooks.Index = 1;
			this.miExtractBooks.Text = "Extract &book...";
			this.miExtractBooks.Click += new System.EventHandler(this.miExtractBooks_Click);
			// 
			// miExtractGump
			// 
			this.miExtractGump.Enabled = false;
			this.miExtractGump.Index = 2;
			this.miExtractGump.Text = "Extract &gump (RunUO)...";
			this.miExtractGump.Click += new System.EventHandler(this.miExtractGump_Click);
			// 
			// miExtractGumpSphere
			// 
			this.miExtractGumpSphere.Enabled = false;
			this.miExtractGumpSphere.Index = 3;
			this.miExtractGumpSphere.Text = "Extract gump (&Sphere)...";
			this.miExtractGumpSphere.Click += new System.EventHandler(this.miExtractGumpSphere_Click);
			// 
			// miHelp
			// 
			this.miHelp.Index = 4;
			this.miHelp.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																				   this.miAbout});
			this.miHelp.Text = "&Help";
			// 
			// miAbout
			// 
			this.miAbout.Index = 0;
			this.miAbout.Text = "&About...";
			this.miAbout.Click += new System.EventHandler(this.miAbout_Click);
			// 
			// ofdStart
			// 
			this.ofdStart.Filter = "Exe files (*.exe)|*.exe";
			// 
			// sfdSaveAs
			// 
			this.sfdSaveAs.FileName = "SpyUO.log";
			this.sfdSaveAs.Filter = "Log files (*.log)|*.log";
			// 
			// sfdExtractItems
			// 
			this.sfdExtractItems.FileName = "SpyUO.cfg";
			this.sfdExtractItems.Filter = "Cfg files (*.cfg)|*.cfg";
			this.sfdExtractItems.Title = "Extract to";
			// 
			// sfdExtractGump
			// 
			this.sfdExtractGump.FileName = "SpyUOGump.cs";
			this.sfdExtractGump.Filter = "C# files (*.cs)|*.cs";
			this.sfdExtractGump.Title = "Extract to";
			// 
			// ofdLoad
			// 
			this.ofdLoad.FileName = "SpyUO.log";
			this.ofdLoad.Filter = "All files (*.*)|*.*";
			// 
			// sfdExtractGumpSphere
			// 
			this.sfdExtractGumpSphere.FileName = "SpyUOGump.scp";
			this.sfdExtractGumpSphere.Filter = "Scp files (*.scp)|*.scp";
			// 
			// sfdExtractBook
			// 
			this.sfdExtractBook.FileName = "SpyUOBook.cs";
			this.sfdExtractBook.Filter = "C# files (*.cs)|*.cs";
			this.sfdExtractBook.Title = "Extract to";
			// 
			// miSaveBin
			// 
			this.miSaveBin.Index = 4;
			this.miSaveBin.Text = "Save bi&n as...";
			this.miSaveBin.Click += new System.EventHandler(this.miSaveBin_Click);
			// 
			// sfdSaveBin
			// 
			this.sfdSaveBin.FileName = "SpyUO.bin";
			this.sfdSaveBin.Filter = "Bin files (*.bin)|*.bin";
			// 
			// miLoadBin
			// 
			this.miLoadBin.Index = 3;
			this.miLoadBin.Text = "Load b&in...";
			this.miLoadBin.Click += new System.EventHandler(this.miLoadBin_Click);
			// 
			// ofdLoadBin
			// 
			this.ofdLoadBin.FileName = "SpyUO.bin";
			this.ofdLoadBin.Filter = "Bin files (*.bin)|*.bin";
			// 
			// Display
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(552, 414);
			this.Controls.Add(this.lvPackets);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Menu = this.mainMenu;
			this.MinimumSize = new System.Drawing.Size(320, 240);
			this.Name = "Display";
			this.ResumeLayout(false);

		}
		#endregion

		private PacketFilter m_PacketFilter;
		private PacketRecorder m_PacketRecorder;
		private PacketPump m_PacketPump;
		private bool m_FilterShowAll;
		private bool m_AutoScrolling;
		private DateTime m_BaseTime;
		private DateTime m_LastTime;

		private PacketPump PacketPump
		{
			get
			{
				return m_PacketPump;
			}
			set
			{
				if ( m_PacketPump != null )
				{
					m_PacketPump.Dispose();
					Detached();
				}

				m_PacketPump = value;
				if ( m_PacketPump != null )
				{
					Attached();
					m_PacketPump.OnPacketPumpTerminated += new PacketPumpTerminatedHandler( PacketPumpTerminated );
				}
			}
		}

		public Display()
		{
			InitializeComponent();

			UpdateTitle( "" );

			m_LastTime = DateTime.MinValue;
			m_BaseTime = DateTime.Now;

			m_FilterShowAll = false;

			m_PacketFilter = new PacketFilter();

			m_PacketRecorder = new PacketRecorder( this );
			m_PacketRecorder.OnPacket += new OnPacket( OnPacket );

			Application.ApplicationExit += new EventHandler( OnExit );
		}

		public void UpdateTitle( string affix )
		{
			Text = Title + (m_PacketPump != null ? " " + affix : "");
		}

		private void OnPacket( TimePacket packet )
		{
			if ( m_PacketFilter.Filter( packet ) )
				AddPacket( packet );
		}

		private void AddPacket( TimePacket packet )
		{
			PacketListViewItem item = new PacketListViewItem( packet, m_BaseTime, m_LastTime );

			lvPackets.Items.Add( item );
			if ( m_AutoScrolling )
				lvPackets.EnsureVisible( lvPackets.Items.Count - 1 );

			m_LastTime = packet.Time;
		}


		private void UpdatePackets()
		{
			miSetBaseTime.Enabled = false;
			miHide.Enabled = false;
			miShowHidden.Enabled = false;
			miExtractGump.Enabled = false;
			miExtractGumpSphere.Enabled = false;

			m_LastTime = DateTime.MinValue;

			lvPackets.BeginUpdate();

			lvPackets.Items.Clear();
			foreach ( TimePacket packet in m_PacketRecorder.Packets )
			{
				if ( m_PacketFilter.Filter( packet ) )
					AddPacket( packet );
			}

			lvPackets.EndUpdate();
		}

		private void Attached()
		{
			miStart.Enabled = false;
			miAttach.Enabled = false;

			miDetach.Enabled = true;

			miPause.Enabled = true;
			miPause.Checked = false;
		}

		private void Detached()
		{
			miStart.Enabled = true;
			miAttach.Enabled = true;

			miDetach.Enabled = false;

			miPause.Enabled = false;
			miPause.Checked = false;
		}

		private void PacketPumpTerminated()
		{
			PacketPump = null;
		}

		private void OnExit( object sender, System.EventArgs e )
		{
			if ( PacketPump != null )
				PacketPump = null;

			if ( m_PacketRecorder != null )
				m_PacketRecorder.Dispose();
		}

		private void miStart_Click( object sender, System.EventArgs e )
		{
			if ( ofdStart.ShowDialog() == DialogResult.OK )
			{
				try
				{
					PacketPump pump = new PacketPump( this, new PacketPumpPacketHandler( m_PacketRecorder.PacketPumpDequeue ) );
					pump.Start( ofdStart.FileName );

					PacketPump = pump;
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Packet pump error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
			}
		}

		private void miAttach_Click( object sender, System.EventArgs e )
		{
			SelectProcess selProc = new SelectProcess();
			selProc.TopMost = TopMost;
			if ( selProc.ShowDialog() == DialogResult.OK )
			{
				Process process = selProc.GetSelectedProcess();

				if ( process != null )
				{
					try
					{
						PacketPump pump = new PacketPump( this, new PacketPumpPacketHandler( m_PacketRecorder.PacketPumpDequeue ) );
						pump.Start( process );

						PacketPump = pump;
					}
					catch ( Exception ex )
					{
						MessageBox.Show( ex.ToString(), "Packet pump error", MessageBoxButtons.OK, MessageBoxIcon.Error );
					}
				}
			}
		}

		private void miDetach_Click( object sender, System.EventArgs e )
		{
			PacketPump = null;
		}

		private void miExit_Click( object sender, System.EventArgs e )
		{
			Application.Exit();
		}

		private void miFilter_Click( object sender, System.EventArgs e )
		{
			Filter filter = new Filter( m_PacketFilter, m_FilterShowAll );
			filter.TopMost = TopMost;
			if ( filter.ShowDialog() == DialogResult.OK )
			{
				m_FilterShowAll = filter.ShowAll;

				UpdatePackets();
			}
		}

		private void miOnTop_Click( object sender, System.EventArgs e )
		{
			TopMost = !miOnTop.Checked;
			miOnTop.Checked = TopMost;
		}

		private void miAutoScrolling_Click( object sender, System.EventArgs e )
		{
			m_AutoScrolling = !m_AutoScrolling;
			miAutoScrolling.Checked = m_AutoScrolling;
		}

		private void miSetBaseTime_Click( object sender, System.EventArgs e )
		{
			ListView.SelectedListViewItemCollection sel = lvPackets.SelectedItems;
			if ( sel.Count > 0 )
			{
				PacketListViewItem item = (PacketListViewItem)sel[0];
				m_BaseTime = item.TimePacket.Time;

				foreach ( PacketListViewItem plvi in lvPackets.Items )
					plvi.UpdateRelTime( m_BaseTime );
			}
		}

		private void miHide_Click( object sender, System.EventArgs e )
		{
			if ( lvPackets.SelectedIndices.Count > 0 )
			{
				int index = lvPackets.SelectedIndices[0];
				lvPackets.Items.RemoveAt( index );

				if ( index < lvPackets.Items.Count )
				{
					PacketListViewItem item = (PacketListViewItem)lvPackets.Items[index];

					if ( index == 0 )
						item.UpdateDifTime( DateTime.MinValue );
					else
						item.UpdateDifTime( ((PacketListViewItem)lvPackets.Items[index - 1]).TimePacket.Time );
				}

				miShowHidden.Enabled = true;
			}
		}

		private void miShowHidden_Click( object sender, System.EventArgs e )
		{
			UpdatePackets();
		}

		private void lvPackets_ItemActivate( object sender, System.EventArgs e )
		{
			ListView.SelectedListViewItemCollection sel = lvPackets.SelectedItems;
			if ( sel.Count > 0 )
			{
				PacketListViewItem item = (PacketListViewItem)sel[0];
				PacketDetails pDetails = new PacketDetails( item );

				pDetails.Show();
			}
		}

		private void miClear_Click( object sender, System.EventArgs e )
		{
			m_PacketRecorder.Clear();

			UpdatePackets();
		}

		private void miPause_Click( object sender, System.EventArgs e )
		{
			if ( PacketPump != null )
				PacketPump.TogglePause();

			miPause.Checked = !miPause.Checked;
		}

		private void miLoad_Click( object sender, System.EventArgs e )
		{
			if ( ofdLoad.ShowDialog() == DialogResult.OK )
			{
				StreamReader reader = null;
				try
				{
					reader = File.OpenText( ofdLoad.FileName );
					m_PacketRecorder.Load( reader );
					UpdatePackets();
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Load error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( reader != null )
						reader.Close();
				}
			}
		}

		private void miSaveAs_Click( object sender, System.EventArgs e )
		{
			if ( sfdSaveAs.ShowDialog() == DialogResult.OK )
			{
				StreamWriter writer = null;
				try
				{
					writer = File.CreateText( sfdSaveAs.FileName );
					Save( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
				}
			}
		}

		public void Save( StreamWriter writer )
		{
			writer.WriteLine( "Logs created on {0} by SpyUO", DateTime.Now );
			writer.WriteLine();
			writer.WriteLine();

			foreach ( ListViewItem item in lvPackets.Items )
			{
				for ( int i = 0; i < lvPackets.Columns.Count; i++ )
					writer.WriteLine( "{0} - {1}", lvPackets.Columns[i].Text, item.SubItems[i].Text );

				writer.WriteLine();
			}
		}

		private void miSaveHexAs_Click( object sender, System.EventArgs e )
		{
			if ( sfdSaveAs.ShowDialog() == DialogResult.OK )
			{
				StreamWriter writer = null;
				try
				{
					writer = File.CreateText( sfdSaveAs.FileName );
					SaveHex( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
				}
			}
		}

		public void SaveHex( StreamWriter writer )
		{
			writer.WriteLine( "Logs created on {0} by SpyUO", DateTime.Now );
			writer.WriteLine();
			writer.WriteLine();

			foreach ( PacketListViewItem item in lvPackets.Items )
			{
				TimePacket tPacket = item.TimePacket;
				Packet packet = tPacket.Packet;
				byte[] data = packet.Data;

				writer.WriteLine( " {0} - {1} (command: 0x{2:X}, length: 0x{3:X})",
					packet.Send ? "Send" : "Recv", tPacket.Time.ToString( @"H\:mm\:ss.ff" ), packet.Cmd, data.Length );

				for ( int l = 0; l < data.Length; l += 0x10 )
				{
					writer.Write( "{0:X4}:", l );

					for ( int i = l; i < l + 0x10; i++ )
						writer.Write( " {0}", i < data.Length ? data[i].ToString( "X2" ) : "--" );

					writer.Write( "\t" );

					for ( int i = l; i < l + 0x10; i++ )
					{
						if ( i >= data.Length )
							break;

						byte b = data[i];
						char c;
						if ( b >= 0x20 && b < 0x80 )
							c = (char)b;
						else
							c = '.';

						writer.Write( c );
					}

					writer.WriteLine();
				}

				writer.WriteLine();
			}
		}

		private void miSaveBin_Click( object sender, System.EventArgs e )
		{
			if ( sfdSaveBin.ShowDialog() == DialogResult.OK )
			{
				FileStream stream = null;
				BinaryWriter writer = null;
				try
				{
					stream = File.Create( sfdSaveBin.FileName );
					writer = new BinaryWriter( stream );

					SaveBin( writer );
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

		public void SaveBin( BinaryWriter writer )
		{
			writer.Write( (int) 0 ); // version

			writer.Write( (int) m_PacketRecorder.Packets.Count );

			foreach ( TimePacket tPacket in m_PacketRecorder.Packets )
			{
				Packet packet = tPacket.Packet;

				byte[] data = packet.Data;

				writer.Write( (bool) packet.Send );
				writer.Write( (long) tPacket.Time.Ticks );

				if ( packet.Cmd == 0x80 ) // AccountLogin
					data = new byte[] { 0x80 };
				else if ( packet.Cmd == 0x91 ) // GameLogin
					data = new byte[] { 0x91 };

				writer.Write( (int) data.Length );
				writer.Write( (byte[]) data );
			}
		}

		private void miLoadBin_Click( object sender, System.EventArgs e )
		{
			if ( ofdLoadBin.ShowDialog() == DialogResult.OK )
			{
				FileStream stream = null;
				BinaryReader reader = null;
				try
				{
					stream = File.OpenRead( ofdLoadBin.FileName );
					reader = new BinaryReader( stream );
					m_PacketRecorder.LoadBin( reader );
					UpdatePackets();
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Load error", MessageBoxButtons.OK, MessageBoxIcon.Error );
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

		private void miAbout_Click( object sender, System.EventArgs e )
		{
			About about = new About();
			about.TopMost = TopMost;
			about.ShowDialog();
		}

		private void miExtractItems_Click( object sender, System.EventArgs e )
		{
			if ( sfdExtractItems.ShowDialog() == DialogResult.OK )
			{
				StreamWriter writer = null;
				try
				{
					writer = File.CreateText( sfdExtractItems.FileName );
					m_PacketRecorder.ExtractItems( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
				}
			}
		}

		private void lvPackets_SelectedIndexChanged( object sender, System.EventArgs e )
		{
			if ( lvPackets.SelectedItems.Count > 0 )
			{
				miSetBaseTime.Enabled = true;
				miHide.Enabled = true;

				PacketListViewItem lvi = (PacketListViewItem)lvPackets.SelectedItems[0];
				Packet packet = lvi.TimePacket.Packet;
				if ( packet is BaseGump )
				{
					miExtractGump.Enabled = true;
					miExtractGumpSphere.Enabled = true;
					return;
				}
			}
			else
			{
				miSetBaseTime.Enabled = false;
				miHide.Enabled = false;
			}

			miExtractGump.Enabled = false;
			miExtractGumpSphere.Enabled = false;
		}

		private void miExtractGump_Click( object sender, System.EventArgs e )
		{
			if ( sfdExtractGump.ShowDialog() == DialogResult.OK )
			{
				PacketListViewItem lvi = (PacketListViewItem)lvPackets.SelectedItems[0];
				BaseGump gump = (BaseGump)lvi.TimePacket.Packet;

				StreamWriter writer = null;
				try
				{
					writer = File.CreateText( sfdExtractGump.FileName );
					gump.WriteRunUOClass( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
				}
			}
		}

		private void miExtractGumpSphere_Click( object sender, System.EventArgs e )
		{
			if ( sfdExtractGumpSphere.ShowDialog() == DialogResult.OK )
			{
				PacketListViewItem lvi = (PacketListViewItem)lvPackets.SelectedItems[0];
				BaseGump gump = (BaseGump)lvi.TimePacket.Packet;

				StreamWriter writer = null;
				try
				{
					writer = File.CreateText( sfdExtractGumpSphere.FileName );
					gump.WriteSphereGump( writer );
				}
				catch ( Exception ex )
				{
					MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
				}
				finally
				{
					if ( writer != null )
						writer.Close();
				}
			}
		}

		private delegate void TitleUpdater( string affix );

		public void DisplayCounter( int sentPackets, int sentPacketsSize, int recvPackets, int recvPacketsSize )
		{
			string title = string.Format( "(Packets/sec) s:{0} r:{1} T:{2} - (Bytes/sec) s:{3} r:{4} T:{5}",
				sentPackets, recvPackets, sentPackets + recvPackets, sentPacketsSize, recvPacketsSize, sentPacketsSize + recvPacketsSize );

			if ( Handle != IntPtr.Zero )
				BeginInvoke( new TitleUpdater( UpdateTitle ), new object[] { title } );
		}

		private void miExtractBooks_Click( object sender, System.EventArgs e )
		{
			SelectBook selection = new SelectBook( new ArrayList( m_PacketRecorder.Books.Values ) );
			if ( selection.ShowDialog() == DialogResult.OK )
			{
				BookInfo book = selection.GetSelectedBook();

				if ( book != null && sfdExtractBook.ShowDialog() == DialogResult.OK )
				{
					StreamWriter writer = null;
					try
					{
						writer = File.CreateText( sfdExtractBook.FileName );
						book.WriteRunUOClass( writer );
					}
					catch ( Exception ex )
					{
						MessageBox.Show( ex.ToString(), "Save error", MessageBoxButtons.OK, MessageBoxIcon.Error );
					}
					finally
					{
						if ( writer != null )
							writer.Close();
					}
				}
			}
		}
	}
}