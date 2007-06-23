using System;
using System.IO;
using SpyUO.Packets;

namespace SpyUO
{
	public class BookInfo : IComparable
	{
		private uint m_Serial;
		private WorldItem m_Item;
		private string m_Title;
		private string m_Author;
		private bool m_Writable;
		private string[][] m_Lines;

		public uint Serial{ get{ return m_Serial; } }
		public WorldItem Item{ get{ return m_Item; } set{ m_Item = value; } }
		public string Title{ get{ return m_Title; } set{ m_Title = value; } }
		public string Author{ get{ return m_Author; } set{ m_Author = value; } }
		public bool Writable{ get{ return m_Writable; } set{ m_Writable = value; } }
		public string[][] Lines{ get{ return m_Lines; } set{ m_Lines = value; } }

		public bool IsCompleted
		{
			get
			{
				for ( int i = 0; i < m_Lines.Length; i++ )
				{
					if ( m_Lines[i] == null )
						return false;
				}

				return true;
			}
		}

		public BookInfo( uint serial, string title, string author, bool writable, int pagesCount )
		{
			m_Serial = serial;
			m_Title = title;
			m_Author = author;
			m_Writable = writable;
			m_Lines = new string[pagesCount][];
		}

		public void WriteRunUOClass( StreamWriter writer )
		{
			string baseClass;
			ushort itemId;
			uint hue;
			bool guess;
			if ( m_Item == null )
			{
				baseClass = "BaseBook";
				itemId = 0xFF2; // Blue book
				hue = 0;
				guess = true;
			}
			else
			{
				itemId = m_Item.ItemId;
				hue = m_Item.Hue;

				if ( m_Item.ItemId == 0xFF2 )
					baseClass = "BlueBook";
				else if ( m_Item.ItemId == 0xFEF )
					baseClass = "BrownBook";
				else if ( m_Item.ItemId == 0xFF1 )
					baseClass = "RedBook";
				else if ( m_Item.ItemId == 0xFF0 )
					baseClass = "TanBook";
				else
					baseClass = "BaseBook";

				guess = false;
			}

			writer.WriteLine( "using System;" );
			writer.WriteLine( "using Server;" );

			writer.WriteLine();

			writer.WriteLine( "namespace Server.Items" );
			writer.WriteLine( "{" );

			if ( guess )
				writer.WriteLine( "\t// Base class, ItemID and Hue may be wrong" );

			writer.WriteLine( "\tpublic class SpyUOBook : {0}", baseClass );
			writer.WriteLine( "\t{" );

			writer.WriteLine( "\t\tpublic static readonly BookContent Content = new BookContent" );
			writer.WriteLine( "\t\t\t(" );
			writer.WriteLine( "\t\t\t\t\"{0}\", \"{1}\",", Fix( m_Title ), Fix( m_Author ) );

			for ( int i = 0; i < m_Lines.Length; i++ )
			{
				if ( m_Lines[i] == null )
				{
					writer.Write( "\t\t\t\tnew BookPageInfo()" );

					if ( i + 1 < m_Lines.Length )
						writer.Write( "," );

					writer.WriteLine( " // Page {0} not logged", i + 1 );
				}
				else
				{
					int linesCount = 0;
					for ( int j = 0; j < m_Lines[i].Length; j++ )
					{
						if ( m_Lines[i][j].Trim() != "" )
							linesCount = j + 1;
					}

					if ( linesCount == 0 )
					{
						writer.Write( "\t\t\t\tnew BookPageInfo()" );
					}
					else
					{
						writer.WriteLine( "\t\t\t\tnew BookPageInfo" );
						writer.WriteLine( "\t\t\t\t(" );

						for ( int j = 0; ; )
						{
							writer.Write( "\t\t\t\t\t\"{0}\"", Fix( m_Lines[i][j] ) );

							if ( ++j < linesCount )
							{
								writer.WriteLine( "," );
							}
							else
							{
								writer.WriteLine();
								break;
							}
						}

						writer.Write( "\t\t\t\t)" );
					}

					if ( i + 1 < m_Lines.Length )
						writer.WriteLine( "," );
					else
						writer.WriteLine();
				}
			}

			writer.WriteLine( "\t\t\t);" );

			writer.WriteLine();
			writer.WriteLine( "\t\tpublic override BookContent DefaultContent{ get{ return Content; } }" );
			writer.WriteLine();

			writer.WriteLine( "\t\t[Constructable]" );
			if ( baseClass != "BaseBook" )
				writer.WriteLine( "\t\tpublic SpyUOBook() : base( {0} )", m_Writable ? "true" : "false" );
			else
				writer.WriteLine( "\t\tpublic SpyUOBook() : base( 0x{0:X}, {1}, {2} )", itemId, m_Lines.Length, m_Writable ? "true" : "false" );
			writer.WriteLine( "\t\t{" );

			if ( hue != 0 )
			{
				writer.WriteLine( "Hue = 0x{0:X};", hue );
				writer.WriteLine();
			}

			writer.WriteLine( "\t\t}" );

			writer.WriteLine();

			writer.WriteLine( "\t\tpublic SpyUOBook( Serial serial ) : base( serial )" );
			writer.WriteLine( "\t\t{" );
			writer.WriteLine( "\t\t}" );

			writer.WriteLine();

			writer.WriteLine( "\t\tpublic override void Serialize( GenericWriter writer )" );
			writer.WriteLine( "\t\t{" );
			writer.WriteLine( "\t\t\tbase.Serialize( writer );" );
			writer.WriteLine();
			writer.WriteLine( "\t\t\twriter.WriteEncodedInt( 0 ); // version" );
			writer.WriteLine( "\t\t}" );

			writer.WriteLine();
			writer.WriteLine( "\t\tpublic override void Deserialize( GenericReader reader )" );
			writer.WriteLine( "\t\t{" );
			writer.WriteLine( "\t\t\tbase.Deserialize( reader );" );
			writer.WriteLine();
			writer.WriteLine( "\t\t\tint version = reader.ReadEncodedInt();" );
			writer.WriteLine( "\t\t}" );

			writer.WriteLine( "\t}" );

			writer.Write( "}" );
		}

		private static string Fix( string s )
		{
			return s.TrimEnd().Replace( @"\", @"\\" ).Replace( "\"", "\\\"" );
		}

		public override string ToString()
		{
			return m_Title + " - 0x" + m_Serial.ToString( "X" ) + (!IsCompleted ? " (incomplete)" : "") + (m_Item == null ? " (item not logged)" : "");
		}

		public int CompareTo( object obj )
		{
			BookInfo book = obj as BookInfo;
			if ( book == null )
				return -1;
			else
				return ToString().CompareTo( book.ToString() );
		}
	}
}