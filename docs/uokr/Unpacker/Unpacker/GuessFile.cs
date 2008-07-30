using System;
using System.Windows.Forms;

using Unpacker.Mythic;

namespace Unpacker
{
	public partial class GuessFile : Form
	{
		private MythicPackageIndex m_File;

		public GuessFile( MythicPackageIndex file )
		{
			m_File = file;

			InitializeComponent();
		}

		private void Check_Click( object sender, EventArgs e )
		{
			CheckName();
		}

		private void Cancel_Click( object sender, EventArgs e )
		{
			Close();
		}

		private void FileName_KeyDown( object sender, KeyEventArgs e )
		{
			if ( e.KeyCode == Keys.Enter )
				CheckName();
		}

		private void CheckName()
		{
			if ( m_File != null )
			{
				if ( m_File.FileHash == HashDictionary.HashMeGently( FileName.Text.ToLower() ) )
				{
					m_File.FileName = FileName.Text.ToLower();
					HashDictionary.Set( m_File.FileHash, m_File.FileName );
					Close();
				}
				else
					Status.Text = "Wrong file name!";
			}
		}
	}
}
