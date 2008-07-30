using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;

namespace Unpacker.Mythic
{
	public static class HashDictionary
	{
		public const string DICTIONARY = "Dictionary.dic";
		public const uint SEED = 0xDEADBEEF;

		private static Dictionary<long,string> m_Dictionary = new Dictionary<long,string>();
		private static string m_FileName;
		private static int m_NewUnknowns;
		private static int m_NewFileNames;

		public static string FileName{ get{ return m_FileName; } }
		public static int NewUnknowns{ get{ return m_NewUnknowns; } }
		public static int NewFileNames{ get{ return m_NewFileNames; } }

		public static bool Saved
		{
			get
			{
				if ( m_NewUnknowns > 0 || m_NewFileNames > 0 )
					return false;

				return true;
			}
		}

		public static long HashMeGently( string s )
		{
			uint eax, ecx, edx, ebx, esi, edi;

			eax = ecx = edx = ebx = esi = edi = 0;
			ebx = edi = esi = (uint) s.Length + SEED;

			int i = 0;

			for ( i = 0; i + 12 < s.Length; i += 12 )
			{
				edi = (uint) ( ( s[ i + 7 ] << 24 ) | ( s[ i + 6 ] << 16 ) | ( s[ i + 5 ] << 8 ) | s[ i + 4 ] ) + edi;
				esi = (uint) ( ( s[ i + 11 ] << 24 ) | ( s[ i + 10 ] << 16 ) | ( s[ i + 9 ] << 8 ) | s[ i + 8 ] ) + esi;
				edx = (uint) ( ( s[ i + 3 ] << 24 ) | ( s[ i + 2 ] << 16 ) | ( s[ i + 1 ] << 8 ) | s[ i ] ) - esi;

				edx = ( edx + ebx ) ^ ( esi >> 28 ) ^ ( esi << 4 );
				esi += edi;
				edi = ( edi - edx ) ^ ( edx >> 26 ) ^ ( edx << 6 );
				edx += esi;
				esi = ( esi - edi ) ^ ( edi >> 24 ) ^ ( edi << 8 );
				edi += edx;
				ebx = ( edx - esi ) ^ ( esi >> 16 ) ^ ( esi << 16 );
				esi += edi;
				edi = ( edi - ebx ) ^ ( ebx >> 13 ) ^ ( ebx << 19 );
				ebx += esi;
				esi = ( esi - edi ) ^ ( edi >> 28 ) ^ ( edi << 4 );
				edi += ebx;
			}

			if ( s.Length - i > 0 )
			{
				switch ( s.Length - i )
				{
					case 12:
						esi += (uint) s[ i + 11 ] << 24;
						goto case 11;
					case 11:
						esi += (uint) s[ i + 10 ] << 16;
						goto case 10;
					case 10:
						esi += (uint) s[ i + 9 ] << 8;
						goto case 9;
					case 9:
						esi += (uint) s[ i + 8 ];
						goto case 8;
					case 8:
						edi += (uint) s[ i + 7 ] << 24;
						goto case 7;
					case 7:
						edi += (uint) s[ i + 6 ] << 16;
						goto case 6;
					case 6:
						edi += (uint) s[ i + 5 ] << 8;
						goto case 5;
					case 5:
						edi += (uint) s[ i + 4 ];
						goto case 4;
					case 4:
						ebx += (uint) s[ i + 3 ] << 24;
						goto case 3;
					case 3:
						ebx += (uint) s[ i + 2 ] << 16;
						goto case 2;
					case 2:
						ebx += (uint) s[ i + 1 ] << 8;
						goto case 1;
					case 1:		
						ebx += (uint) s[ i ];
						break;			
				}

				esi = ( esi ^ edi ) - ( ( edi >> 18 ) ^ ( edi << 14 ) );
				ecx = ( esi ^ ebx ) - ( ( esi >> 21 ) ^ ( esi << 11 ) );
				edi = ( edi ^ ecx ) - ( ( ecx >> 7 ) ^ ( ecx << 25 ) );
				esi = ( esi ^ edi ) - ( ( edi >> 16 ) ^ ( edi << 16 ) );
				edx = ( esi ^ ecx ) - ( ( esi >> 28 ) ^ ( esi << 4 ) );
				edi = ( edi ^ edx ) - ( ( edx >> 18 ) ^ ( edx << 14 ) );
				eax = ( esi ^ edi ) - ( ( edi >> 8 ) ^ ( edi << 24 ) );

				return ( (long) edi << 32 ) | eax;
			}

			return ( (long) esi << 32 ) | eax;
		}

		public static void LoadDictionary( string fileName )
		{
			if ( !File.Exists( fileName ) )
				throw new Exception( String.Format( "Cannot find {0}!", Path.GetFileName( fileName ) ) );

			using ( FileStream stream = new FileStream( fileName, FileMode.Open ) )
			{
				using ( BinaryReader reader = new BinaryReader( stream ) )
                {
					byte[] id = reader.ReadBytes( 4 );					

					if ( id[ 0 ] != 'D' || id[ 1 ] != 'I' || id[ 2 ] != 'C' || id[ 3 ] != 0 )
						throw new Exception( String.Format( "{0} is not a Dictionary file!", Path.GetFileName( fileName ) ) );

					m_Dictionary.Clear();

					try
					{
						int version = reader.ReadByte();

						while ( true )
						{
							long hash = reader.ReadInt64();
							string name = null;

							if ( reader.ReadByte() == 1 )
								name = reader.ReadString();

							if ( !m_Dictionary.ContainsKey( hash ) )
								m_Dictionary.Add( hash, name );
						}
					}
					catch ( EndOfStreamException )
					{
					}

					m_FileName = fileName;
					m_NewUnknowns = 0;
					m_NewFileNames = 0;

					reader.Close();
					stream.Close();
				}
			}
		}

		public static void SaveDictionary( string path )
		{
			using ( FileStream stream = new FileStream( path, FileMode.Create, FileAccess.Write ) )
			{
				using ( BinaryWriter writer = new BinaryWriter( stream ) )
                {
					writer.Write( 'D' );
					writer.Write( 'I' );
					writer.Write( 'C' );
					writer.Write( (byte) 0 );

					writer.Write( (byte) 1 ); // version
					
					foreach ( KeyValuePair<long,string> kvp in m_Dictionary )
					{
						writer.Write( (long) kvp.Key );

						if ( kvp.Value != null )
						{
							writer.Write( (byte) 1 );
							writer.Write( (string) kvp.Value );
						}
						else
							writer.Write( (byte) 0 );
					}

					m_NewUnknowns = 0;
					m_NewFileNames = 0;

					writer.Close();
					stream.Close();
				}
			}
		}

		public static void MergeDictionary( string fileName )
		{
			if ( !File.Exists( fileName ) )
				throw new Exception( String.Format( "Cannot find {0}!", Path.GetFileName( fileName ) ) );

			using ( FileStream stream = new FileStream( fileName, FileMode.Open ) )
			{
				using ( BinaryReader reader = new BinaryReader( stream ) )
                {
					byte[] id = reader.ReadBytes( 4 );					

					if ( id[ 0 ] != 'D' || id[ 1 ] != 'I' || id[ 2 ] != 'C' || id[ 3 ] != 0 )
						throw new Exception( String.Format( "{0} is not a Dictionary file!", Path.GetFileName( fileName ) ) );

					try
					{
						int version = reader.ReadByte();

						while ( true )
						{
							long hash = reader.ReadInt64();
							string name = null;

							if ( reader.ReadByte() == 1 )
								name = reader.ReadString();

							if ( m_Dictionary.ContainsKey( hash ) )
							{
								if ( m_Dictionary[ hash ] == null )
								{
									m_Dictionary[ hash ] = name;

									if ( name != null )
										m_NewFileNames += 1;
								}
							}
							else
							{
								m_NewUnknowns += 1;

								if ( name != null )
									m_NewFileNames += 1;

								m_Dictionary.Add( hash, name );	
							}
						}
					}
					catch ( EndOfStreamException )
					{
					}

					reader.Close();
					stream.Close();
				}
			}
		}

		public static bool Contains( long hash )
		{
			if ( m_Dictionary.ContainsKey( hash ) )
				return true;

			return false;
		}

		public static string Get( long hash )
		{
			string name;

			if ( m_Dictionary.ContainsKey( hash ) )
			{
				if ( m_Dictionary.TryGetValue( hash, out name ) )
					return name;
			}
			else
			{
				m_NewUnknowns += 1;
				m_Dictionary.Add( hash, null );
				WriteUnknown( hash );
			}

			return null;
		}

		public static void Set( long hash, string name )
		{
			if ( m_Dictionary.ContainsKey( hash ) )
			{
				string value = m_Dictionary[ hash ];
				
				if ( value == null )
				{
					if ( hash == HashMeGently( name ) )
					{
						m_NewFileNames += 1;
						m_Dictionary[ hash ] = name;
						WriteFound( hash, name );
					}
				}
			}				
		}

		#region Unknown hashes
		private static StreamWriter m_Unknowns;
		private static StreamWriter m_Found;

		public static void InitFiles()
		{
			try
			{
				m_Unknowns = new StreamWriter( "Unknown Hashes.txt", true );
				m_Found = new StreamWriter( "Found Hashes.txt", true );
			}
			catch
			{
			}
		}

		public static void CloseFiles()
		{
			try
			{
				if ( m_Unknowns != null )
				{
					m_Unknowns.Flush();
					m_Unknowns.Close();
				}

				if ( m_Found != null )
				{
					m_Found.Flush();
					m_Found.Close();
				}
			}
			catch
			{
			}
		}

		public static void WriteUnknown( long hash )
		{
			try
			{
				m_Unknowns.WriteLine( hash.ToString( "X16" ) );
			}
			catch
			{
			}
		}

		public static void WriteFound( long hash, string name )
		{
			try
			{
				m_Found.WriteLine( String.Format( "{0} - {1}", hash.ToString( "X16" ), name ) );
			}
			catch
			{
			}
		}
		#endregion
	}
}
