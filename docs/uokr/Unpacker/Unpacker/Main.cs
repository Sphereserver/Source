using System;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.ComponentModel;
using System.Collections.Generic;
using System.Windows.Forms;

using Unpacker.Mythic;
using Unpacker.Spying;

namespace Unpacker
{
	public partial class Main : Form
	{
		public Main()
		{			
			InitializeComponent();
			Initialize();
		}
		
		#region Main Menu
		private void MainMenuFileOpen_Click( object sender, EventArgs e )
		{
			OpenFileDialog.ShowDialog();
		}

		private void OpenFileDialog_FileOk( object sender, CancelEventArgs e )
		{
			if ( !Worker.IsBusy )
			{
				List<string> files = new List<string>();

				foreach ( string fileName in OpenFileDialog.FileNames )
				{
					if ( !AlreadyOpen( fileName ) )
						files.Add( fileName );
				}

				StatusLabel.Text = "Loading Mythic packages";
				Worker.RunWorkerAsync( new MythicLoadArgs( files.ToArray() ) );
			}
		}

		private void MainMenuFileClose_Click( object sender, EventArgs e )
		{
			TreeNode c;

			if ( TreeView.SelectedNode != null && TreeView.SelectedNode.Parent != null )
				c = TreeView.SelectedNode.Parent;
			else
				c = TreeView.SelectedNode;

			if ( c != null )
			{
				MythicPackage p = c.Tag as MythicPackage;

				if ( p != null )
					p.Dispose();

				ListBox.Items.Clear();
				ClearDetails();
				c.Remove();
			}
		}

		private void MainMenuFileExit_Click( object sender, EventArgs e )
		{
			if ( !HashDictionary.Saved )
			{
				DialogResult result = MessageBox.Show( "Save changes to dictionary?", "Exit", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question );
			
				if ( result != DialogResult.Cancel )
				{
					if ( result == DialogResult.Yes )
						SaveDictionary( HashDictionary.FileName );

					Application.Exit();
				}
			}
			else
				Application.Exit();
		}

		private void MainMenuDictionaryLoad_Click( object sender, EventArgs e )
		{
			if ( !HashDictionary.Saved )
			{
				DialogResult result = MessageBox.Show( "Save changes to dictionary?", "Save", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question );
			
				if ( result != DialogResult.Cancel )
				{
					if ( result == DialogResult.Yes )
						SaveDictionary( HashDictionary.FileName );
					
					OpenDictionary.Tag = DictionaryArgs.LOAD;
					OpenDictionary.ShowDialog();
				}
			}
			else
			{
				OpenDictionary.Tag = DictionaryArgs.LOAD;		
				OpenDictionary.ShowDialog();
			}
		}

		private void MainMenuDictionarySave_Click( object sender, EventArgs e )
		{
			SaveDictionary( HashDictionary.DICTIONARY );
		}

		private void MainMenuDictionaryMerge_Click( object sender, EventArgs e )
		{
			OpenDictionary.Tag = DictionaryArgs.MERGE;
			OpenDictionary.ShowDialog();
		}

		private void OpenDictionary_FileOk( object sender, CancelEventArgs e )
		{
			if ( (int) OpenDictionary.Tag == DictionaryArgs.LOAD )
				LoadDictionary( OpenDictionary.FileName );
			else if ( (int) OpenDictionary.Tag == DictionaryArgs.MERGE )
				MergeDictionary( OpenDictionary.FileName );
		}

		private void MainMenuSettingsPath_Click( object sender, EventArgs e )
		{
			if ( SelectFolder.ShowDialog() == DialogResult.OK )
			{
				UnpackArgs.Path = SelectFolder.SelectedPath;
			}
		}

		#region Spy
		private Spy m_Spy;

		private void MainMenuSpyStart_Click( object sender, EventArgs e )
		{
			if ( m_Spy == null )
				m_Spy = new Spy();

			OpenExeDialog.ShowDialog();
		}

		private void OpenExeDialog_FileOk( object sender, CancelEventArgs e )
		{
			if ( !Worker.IsBusy )
			{
				MainMenuSpyStart.Enabled = false;
				MainMenuSpyAttach.Enabled = false;
				MainMenuSpyDetach.Enabled = true;
				StatusLabel.Text = String.Format( "Spying on {0}", OpenExeDialog.FileName );
				Worker.RunWorkerAsync( new SpyPathArgs( OpenExeDialog.FileName ) );
			}
		}

		private void MainMenuSpyAttach_Click( object sender, EventArgs e )
		{
			if ( Worker.IsBusy )
				return;

			if ( m_Spy == null )
				m_Spy = new Spy();

			SelectProcess select = new SelectProcess();

			if ( select.ShowDialog() == DialogResult.OK )
			{				
				Process process = select.GetSelectedProcess();

				if ( process != null )
				{
					MainMenuSpyStart.Enabled = false;
					MainMenuSpyAttach.Enabled = false;
					MainMenuSpyDetach.Enabled = true;
					StatusLabel.Text = String.Format( "Spying on {0}", process.ProcessName );
					Worker.RunWorkerAsync( new SpyProcessArgs( process ) );
				}
			}
		}

		private void MainMenuSpyDetach_Click( object sender, EventArgs e )
		{
			MainMenuSpyStart.Enabled = true;
			MainMenuSpyAttach.Enabled = true;
			MainMenuSpyDetach.Enabled = false;

			if ( m_Spy != null )
				m_Spy.Dispose();

			UpdateFileNames();
		}
		#endregion

		#endregion

		private void Main_FormClosing( object sender, FormClosingEventArgs e )
		{
			if ( !HashDictionary.Saved )
			{
				DialogResult result = MessageBox.Show( "Save changes to dictionary?", "Save", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question );
			
				if ( result != DialogResult.Cancel )
				{
					if ( result == DialogResult.Yes )
						SaveDictionary( HashDictionary.FileName );					
				}
			}		
		}

		private void CopyMenuStripButton_Click( object sender, EventArgs e )
		{
			Clipboard.SetText( ((Label) CopyMenuStrip.Tag).Text, TextDataFormat.Text );
		}

		private void Label_MouseClick( object sender, MouseEventArgs e )
		{
			if ( e.Button == MouseButtons.Right )
			{
				Label label = (Label) sender;

				CopyMenuStrip.Tag = label;
				CopyMenuStrip.Show( label, e.Location );
			}
			else if ( e.Button == MouseButtons.Left )
			{
				
				if ( ListBox.SelectedIndex > -1 )
				{
					MythicPackageIndex idx = ListBox.SelectedItem as MythicPackageIndex;

					if ( idx != null && idx.FileName == null )
					{
						new GuessFile( idx ).ShowDialog();
					}
				}
			}
		}
		
		private void ListBox_SelectedIndexChanged( object sender, EventArgs e )
		{
			if ( ListBox.SelectedItem is MythicPackageIndex )
				ChangeIndex( (MythicPackageIndex) ListBox.SelectedItem );
			else 
				ClearIndex();
		}

		private void TreeView_AfterSelect( object sender, TreeViewEventArgs e )
		{
			if ( e.Node.Tag is MythicPackage )
				ChangePackage( (MythicPackage) e.Node.Tag );
			else if ( e.Node.Tag is MythicPackageBlock )
				ChangeBlock( (MythicPackageBlock) e.Node.Tag );

			ListBox.SelectedIndex = -1;
		}

		private void TreeView_NodeMouseClick( object sender, TreeNodeMouseClickEventArgs e )
		{			
			ListBox.SelectedIndex = -1;

			if ( TreeView.SelectedNode != null && TreeView.SelectedNode.Tag is MythicPackage )
				ClearBlock();
		}

		private void Unpack_Click( object sender, EventArgs e )
		{
			if ( Worker.IsBusy )
				return;

			if ( UnpackArgs.Path == null )
			{
				if ( SelectFolder.ShowDialog() == DialogResult.OK )
					UnpackArgs.Path = SelectFolder.SelectedPath;
				else
					return;
			}

			UnpackArgs args = null;
			MythicPackage p = null;
			MythicPackageBlock b = null;
			MythicPackageIndex idx = null;
			
			if ( TreeView.SelectedNode != null )
			{
				if ( TreeView.SelectedNode.Tag is MythicPackage )
				{
					p = (MythicPackage) TreeView.SelectedNode.Tag;

					if (  ListBox.SelectedIndex == -1 )
						args = new UnpackArgs( p );
				}
				else if ( TreeView.SelectedNode.Tag is MythicPackageBlock )
				{
					p = (MythicPackage) TreeView.SelectedNode.Parent.Tag;
					b = (MythicPackageBlock) TreeView.SelectedNode.Tag;

					if (  ListBox.SelectedIndex == -1 )
						args = new UnpackArgs( p, b );
				}
			}

			if ( ListBox.SelectedItem is MythicPackageIndex )
			{
				idx = (MythicPackageIndex) ListBox.SelectedItem;

				if ( p != null && b != null )
						args = new UnpackArgs( p, b, idx );
			}

			if ( args != null)
			{
				StatusLabel.Text = String.Format( "Unpacking in {0}", UnpackArgs.Path );
				Unpack.Enabled = false;
				Worker.RunWorkerAsync( args );
			}
		}

		private void Search_Click( object sender, EventArgs e )
		{
			SearchExpression( SearchBox.Text );
		}

		private void SearchBox_KeyPress( object sender, KeyPressEventArgs e )
		{
			if ( (Keys) e.KeyChar == Keys.Enter )
				SearchExpression( SearchBox.Text );
		}

		#region Threads
		private void Worker_DoWork( object sender, DoWorkEventArgs e )
		{
			if ( e.Argument is DictionaryArgs )
			{
				DictionaryArgs args = (DictionaryArgs) e.Argument;

				if ( args.Load )
					HashDictionary.LoadDictionary( args.Name );
				else if ( args.Save )
					HashDictionary.SaveDictionary( args.Name );
				else if ( args.Merge )
					HashDictionary.MergeDictionary( args.Name );
			}
			else if ( e.Argument is MythicLoadArgs )
			{
				MythicLoadArgs args = (MythicLoadArgs) e.Argument;
				MythicPackage[] packs = new MythicPackage[ args.Names.Length ];

				for ( int i = 0; i < args.Names.Length; i ++ )
					packs[ i ] = MythicPackage.Read( args.Names[ i ] );

				args.Result = packs;
			}
			else if ( e.Argument is SpyProcessArgs )
			{
				SpyProcessArgs args = (SpyProcessArgs) e.Argument;

				m_Spy.Init( args.Process );
				m_Spy.MainLoop();
			}
			else if ( e.Argument is SpyPathArgs )
			{
				SpyPathArgs args = (SpyPathArgs) e.Argument;

				m_Spy.Init( args.Path );
				m_Spy.MainLoop();
			}
			else if ( e.Argument is UnpackArgs )
			{
				UnpackArgs args = (UnpackArgs) e.Argument;

				if ( args.IsPackage )
				{
					args.Package.Decompress( UnpackArgs.Path, args.Package.Info.Name );
				}
				else if ( args.IsBlock )
				{
					using ( FileStream stream = File.OpenRead( args.Package.Info.FullName ) )
					{
						using ( BinaryReader reader = new BinaryReader( stream ) )
							args.Block.Decompress( reader, UnpackArgs.Path, args.Package.Info.Name );
					}
				}
				else if ( args.IsFile )
				{
					using ( FileStream stream = File.OpenRead( args.Package.Info.FullName ) )
					{
						using ( BinaryReader reader = new BinaryReader( stream ) )
							args.File.Decompress( reader, UnpackArgs.Path, String.Format( "{0}_{1}", args.Package.Info.Name, args.Block.Index ) );
					}
				}
			}
			else if ( e.Argument is SearchArgs )
			{
				SearchArgs args = (SearchArgs) e.Argument;		
		
				args.Found = 0;

				for ( int i = args.From; i < args.To; i++ )
				{
					if ( SearchKeyword( String.Format( "{0}{1}{2}", args.Before, i.ToString( "D" + args.Length ), args.After ), true ) )
						args.Found += 1;
				}
			}
			
			e.Result = e.Argument;
		}

		private void Worker_RunWorkerCompleted( object sender, RunWorkerCompletedEventArgs e )
		{
			if ( e.Error != null )
			{
				StatusLabel.Text = e.Error.Message;
				MainMenuSpyStart.Enabled = true;
				MainMenuSpyAttach.Enabled = true;
				MainMenuSpyDetach.Enabled = false;
				MainMenuDictionary.Enabled = true;
				Unpack.Enabled = true;
			}
			else if ( e.Result is DictionaryArgs )
			{
				DictionaryArgs args = (DictionaryArgs) e.Result;

				if ( args.Load )
					StatusLabel.Text = "Loading finished.";
				else if ( args.Save )
					StatusLabel.Text = "Saving finished.";
				else if ( args.Merge )
					StatusLabel.Text = String.Format( "Merging finished. There is {0} new hashes and {1} new file names." );

				MainMenuDictionary.Enabled = true;
			}			
			else if ( e.Result is MythicLoadArgs )
			{
				MythicLoadArgs args = (MythicLoadArgs) e.Result;
				TreeNode parent, child;

				foreach ( MythicPackage p in args.Result )
				{
					parent = new TreeNode( p.Info.Name );
					parent.Tag = p;
					TreeView.Nodes.Add( parent );

					for ( int i = 0; i < p.Blocks.Count; i ++ )
					{
						child = new TreeNode( String.Format( "Block_{0}", i ) );
						child.Tag = p.Blocks[ i ];
						parent.Nodes.Add( child );
					}
				}				

				if ( args.Result.Length > 0 )
				{
					TreeView.SelectedImageIndex = 0;
					ChangePackage( args.Result[ 0 ] );
				}

				StatusLabel.Text = "Loading finished.";

				if ( HashDictionary.NewUnknowns > 0 )
				{
					if ( MessageBox.Show( String.Format( "Found {0} new hashes. Save dictionary?", HashDictionary.NewUnknowns ), "Save Dictionary", MessageBoxButtons.YesNo, MessageBoxIcon.Question ) == DialogResult.Yes )
						SaveDictionary( HashDictionary.DICTIONARY );
				}
			}
			else if ( e.Result is SpyPathArgs || e.Result is SpyProcessArgs )
			{
				StatusLabel.Text = "Process terminated.";
				MainMenuSpyStart.Enabled = true;
				MainMenuSpyAttach.Enabled = true;
				MainMenuSpyDetach.Enabled = false;

				if ( HashDictionary.NewFileNames > 0 )
				{
					UpdateFileNames();

					if ( MessageBox.Show( String.Format( "Found {0} new file names. Save dictionary?", HashDictionary.NewFileNames ), "Save Dictionary", MessageBoxButtons.YesNo, MessageBoxIcon.Question ) == DialogResult.Yes )
						SaveDictionary( HashDictionary.DICTIONARY );
				}
			}
			else if ( e.Result is UnpackArgs )
			{				
				StatusLabel.Text = "Unpacking finished.";
				Unpack.Enabled = true;
			}
			else if ( e.Result is SearchArgs )
			{
				SearchArgs args = (SearchArgs) e.Result;
				
				StatusLabel.Text = String.Format( "Found {0} file names", args.Found );

				if ( args.Found > 0 )
					UpdateFileNames();
			}
		}
		#endregion

		#region Dictionary
		private void LoadDictionary( string path )
		{			
			if ( !Worker.IsBusy )
			{
				MainMenuDictionary.Enabled = false;
				StatusLabel.Text = String.Format( "Loading {0}", Path.GetFileName( path ) );
				Worker.RunWorkerAsync( new DictionaryArgs( path, DictionaryArgs.LOAD ) );
			}
		}

		private void SaveDictionary( string path )
		{
			if ( !Worker.IsBusy )
			{
				MainMenuDictionary.Enabled = false;
				StatusLabel.Text = String.Format( "Saving {0}", Path.GetFileName( path ) );			
				Worker.RunWorkerAsync( new DictionaryArgs( path, DictionaryArgs.SAVE ) );
			}
		}

		private void MergeDictionary( string path )
		{
			if ( !Worker.IsBusy )
			{
				MainMenuDictionary.Enabled = false;
				StatusLabel.Text = String.Format( "Merging with {0}", Path.GetFileName( path ) );			
				Worker.RunWorkerAsync( new DictionaryArgs( path, DictionaryArgs.MERGE ) );
			}
		}
		#endregion

		#region Private
		private void Initialize()
		{			
			HashDictionary.InitFiles();
			LoadDictionary( HashDictionary.DICTIONARY );
		}

		private void ClearDetails()
		{	
			ClearPackage();
			ClearBlock();
			ClearIndex();
		}

		public void ClearPackage()
		{
			PackageFullNameInfo.Text = null;
			PackageAttributesInfo.Text = null;
			PackageCreationInfo.Text = null;
			PackageSizeInfo.Text = null;
			PackageCompleteInfo.Text = null;
			PackageVersionInfo.Text = null;
			PackageMiscInfo.Text = null;
			PackageHeaderSizeInfo.Text = null;
			PackageBlockSizeInfo.Text = null;
			PackageFileCountInfo.Text = null;	
		}

		private void ChangePackage( MythicPackage p )
		{
			PackageFullNameInfo.Text = p.Info.FullName;
			PackageAttributesInfo.Text = p.Info.Attributes.ToString();
			PackageCreationInfo.Text = p.Info.CreationTimeUtc.ToLocalTime().ToString();
			PackageSizeInfo.Text = ConvertSize( p.Info.Length );
			PackageCompleteInfo.Text = String.Format( "{0} %", p.Complete.ToString( "F2" ) );
			PackageVersionInfo.Text = p.Header.Version.ToString();
			PackageMiscInfo.Text = p.Header.Misc.ToString( "X8" );
			PackageHeaderSizeInfo.Text = ConvertSize( p.Header.HeaderSize );
			PackageBlockSizeInfo.Text = p.Header.BlockSize.ToString();
			PackageFileCountInfo.Text = p.Header.FileCount.ToString();
			
			FileDetails.SelectedIndex = 0;
		}

		private void ClearBlock()
		{	
			BlockFileCountInfo.Text = null;
			BlockNextBlockInfo.Text = null;
			BlockCompleteInfo.Text = null;
		}

		private void ChangeBlock( MythicPackageBlock b )
		{
			BlockFileCountInfo.Text = b.FileCount.ToString();
			BlockNextBlockInfo.Text = b.NextBlock.ToString( "X16" );
			BlockCompleteInfo.Text = String.Format( "{0} %", b.Complete.ToString( "F2" ) );

			ListBox.Items.Clear();
			ListBox.Items.AddRange( b.Files.ToArray() );

			FileDetails.SelectedIndex = 1;			
		}

		private void ClearIndex()
		{
			FileFileNameInfo.Text = null;
			FileHashInfo.Text = null;
			FileUnknownInfo.Text = null;
			FileCompressedInfo.Text = null;
			FileDecompressedInfo.Text = null;
			FileFlagInfo.Text = null;
			FileDataOffsetInfo.Text = null;
			FileUnknown2Info.Text = null;
			FileCompressed.Checked = true;
			FileCompressed.CheckState = CheckState.Indeterminate;
		}

		private void ChangeIndex( MythicPackageIndex idx )
		{
			FileFileNameInfo.Text = idx.FileName;
			FileHashInfo.Text = idx.FileHash.ToString( "X16" );
			FileUnknownInfo.Text = idx.Unknown.ToString( "X8" );
			FileCompressedInfo.Text = ConvertSize( idx.CompressedSize );
			FileDecompressedInfo.Text = ConvertSize( idx.DecompressedSize );
			FileCompressed.Checked = idx.Compressed;

			FileFlagInfo.Text = idx.DataBlock.Flag.ToString( "X4" );
			FileDataOffsetInfo.Text = idx.DataBlock.DataOffset.ToString( "X4" );
			FileUnknown2Info.Text = idx.DataBlock.Unknown.ToString( "X16" );

			FileDetails.SelectedIndex = 2;			
		}

		private void UpdateFileNames()
		{
			foreach( TreeNode n in TreeView.Nodes )
			{
				MythicPackage p = (MythicPackage) n.Tag;
				p.UpdateFileNames();
			}
		}

		private bool AlreadyOpen( string fileName )
		{
			foreach( TreeNode n in TreeView.Nodes )
			{
				MythicPackage p = (MythicPackage) n.Tag;

				if ( p.Info.FullName.Equals( fileName ) )
					return true;
			}

			return false;
		}

		private string ConvertSize( long bytes )
		{
			if ( bytes >= 1024 )
			{
				double value = bytes / (double) 1024;

				if ( value >= 1024 )
				{
					value /= 1024;
					return String.Format( "{0} MB", value.ToString( "F2" ) );
				}
				
				return String.Format( "{0} KB", value.ToString( "F2" ) );
			}

			return String.Format( "{0} B", bytes );
		}

		private bool ParseString( string str, out int val )
		{
			bool ret = Int32.TryParse( str, out val );

			if ( !ret )
				StatusLabel.Text = String.Format( "Not a number: {0}", str );

			return ret;
		}

		private void SearchExpression( string line )
		{			
			if ( !String.IsNullOrEmpty( line ) )
			{
				int s, e;

				s = line.IndexOf( '{' );

				if ( s < 0 )
				{					
					SearchKeyword( line, false );
					return;
				}

				e = line.IndexOf( '}' );

				if ( e < 0 )
				{
					StatusLabel.Text = "Missing '}'!";
					return;
				}

				string before, center, after;

				before = line.Substring( 0, s );
				center = line.Substring( s + 1, e - s - 1 );
				after = line.Substring( e + 1, line.Length - e - 1 );

				int dash = center.IndexOf( '-' );

				int from, to, len;

				if ( dash > 0 )
				{
					string a, b;

					a = center.Substring( 0, dash );
					b = center.Substring( dash + 1, center.Length - dash - 1 );

					if ( !ParseString( a, out from ) )
						return;
					
					if ( !ParseString( b, out to ) )
						return;

					if ( a.Length > b.Length )
						len = a.Length;
					else
						len = b.Length;
				}
				else
				{
					from = 0;
					len = center.Length;

					if ( !ParseString( center, out to ) )
						return;
				}

				StatusLabel.Text = "Searching! Warning: this may take a while!!!";

				if ( !Worker.IsBusy )
					Worker.RunWorkerAsync( new SearchArgs( before, after, from, to, len ) );
			}
			else				
				StatusLabel.Text = "Invalid search pattern!";
		}

		private bool SearchKeyword( string keyword, bool silent )
		{
			int pstart = 0;
			int bstart = 0;
			int istart = 0;

			if ( TreeView.SelectedNode != null )
			{
				if ( TreeView.SelectedNode.Tag is MythicPackage )
					pstart = TreeView.SelectedNode.Index;
				else if ( TreeView.SelectedNode.Tag is MythicPackageBlock )
				{
					pstart = TreeView.SelectedNode.Parent.Index;
					bstart = TreeView.SelectedNode.Index;
				}
			}

			if ( ListBox.SelectedIndex > -1 )
				istart = ListBox.SelectedIndex;

			int[] index = null;

			for ( int i = pstart; i < TreeView.Nodes.Count; i ++ )
			{
				MythicPackage p = TreeView.Nodes[ i ].Tag as MythicPackage;

				if ( p != null )
				{
					index = p.Contains( bstart, istart, keyword.ToLower() );

					if ( index != null )
					{
						if ( !silent )
						{
							TreeView.SelectedNode = TreeView.Nodes[ i ];
							TreeView.SelectedNode = TreeView.SelectedNode.Nodes[ index[ 0 ] ];
							ListBox.SelectedIndex = index[ 1 ];
						}

						return true;
					}
				}
			}

			if ( !silent )
				StatusLabel.Text = String.Format( "{0} not found!", keyword );

			return false;
		}

		private void ShowError( string error )
		{
			MessageBox.Show( error, "Error!", MessageBoxButtons.OK, MessageBoxIcon.Error );
		}

		private void ShowWarning( string warning )
		{
			MessageBox.Show( warning, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning );
		}

		private void ShowInfo( string info )
		{
			MessageBox.Show( info, "Info", MessageBoxButtons.OK, MessageBoxIcon.Information );
		}
		#endregion		

		private void MainMenuAbout_Click( object sender, EventArgs e )
		{
			new About().ShowDialog();
		}
	}
}
