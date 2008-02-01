using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Reflection;

namespace SpyUO.Packets
{
	public abstract class BaseGump : Packet
	{
		private uint m_X;
		private uint m_Y;
		private uint m_Serial;
		private uint m_GumpId;
		private GumpEntry[] m_Layout;
		private string[] m_Text;

		[PacketProp( 0 )]
		public uint X { get { return m_X; } }

		[PacketProp( 1 )]
		public uint Y { get { return m_Y; } }

		[PacketProp( 2, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 3, "0x{0:X}" )]
		public uint GumpId { get { return m_GumpId; } }

		public GumpEntry[] Layout { get { return m_Layout; } }

		[PacketProp( 4 )]
		public string LayoutString
		{
			get
			{
				if ( m_Layout.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					sb.AppendFormat( "{0}", m_Layout[i] );

					if ( ++i < m_Layout.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		public string[] Text { get { return m_Text; } }

		[PacketProp( 5 )]
		public string TextString
		{
			get
			{
				if ( m_Text.Length == 0 )
					return "Empty";

				StringBuilder sb = new StringBuilder();
				int i = 0;
				while ( true )
				{
					sb.AppendFormat( "{0}. \"{1}\"", i, m_Text[i] );

					if ( ++i < m_Text.Length )
						sb.Append( " - " );
					else
						break;
				}

				return sb.ToString();
			}
		}

		public BaseGump( PacketReader reader, bool send ) : base( reader, send )
		{
		}

		protected void Init( uint serial, uint gumpId, uint x, uint y, string layout, string[] text )
		{
			m_Serial = serial;
			m_GumpId = gumpId;
			m_X = x;
			m_Y = y;
			m_Text = text;

			layout = layout.Replace( "}", "" );
			string[] splt = layout.Substring( 1 ).Split( '{' );
			ArrayList layoutList = new ArrayList();
			foreach ( string s in splt )
			{
				try
				{
					string[] commands = SplitCommands( s );

					ArrayList cmdList = new ArrayList();
					foreach ( string cmd in commands )
					{
						if ( cmd != "" )
							cmdList.Add( cmd );
					}

					GumpEntry entry = GumpEntry.Create( (string[])cmdList.ToArray( typeof( string ) ), this );

					layoutList.Add( entry );
				}
				catch { }
			}
			m_Layout = (GumpEntry[])layoutList.ToArray( typeof( GumpEntry ) );
		}

		private static string[] SplitCommands( string s )
		{
			s = s.Trim();

			ArrayList ret = new ArrayList();

			bool stringCmd = false;
			int start = 0;
			for ( int i = 0; i < s.Length; i++ )
			{
				char ch = s[i];

				if ( ch == ' ' || ch == '\t' )
				{
					if ( !stringCmd )
					{
						ret.Add( s.Substring( start, i - start ) );
						start = i + 1;
					}
				}
				else if ( ch == '@' )
				{
					stringCmd = !stringCmd;
				}
			}

			ret.Add( s.Substring( start, s.Length - start ) );

			return (string[]) ret.ToArray( typeof( string ) );
		}

		public void WriteRunUOClass( StreamWriter writer )
		{
			writer.WriteLine( "using System;" );
			writer.WriteLine( "using Server;" );
			writer.WriteLine( "using Server.Gumps;" );
			writer.WriteLine( "using Server.Network;" );

			writer.WriteLine();

			writer.WriteLine( "namespace Server.SpyUO" );
			writer.WriteLine( "{" );

			writer.WriteLine( "\tpublic class SpyUOGump : Gump" );
			writer.WriteLine( "\t{" );

			writer.WriteLine( "\t\tpublic SpyUOGump() : base( {0}, {1} )", m_X, m_Y );
			writer.WriteLine( "\t\t{" );
			for ( int i = 0; i < m_Layout.Length; i++ )
			{
				GumpEntry entry = m_Layout[i];
				bool space = entry is GumpPage;
				if ( space && i != 0 )
					writer.WriteLine();

				writer.Write( "\t\t\t{0}", entry.GetRunUOLine() );

				if ( entry is GumpHtmlLocalized )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalized)entry).Number] as string );
				else if ( entry is GumpHtmlLocalizedColor )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalizedColor)entry).Number] as string );
				else if ( entry is GumpHtmlLocalizedArgs )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalizedArgs)entry).Number] as string );
				else
					writer.WriteLine();

				if ( space && i < m_Layout.Length )
					writer.WriteLine();
			}
			writer.WriteLine( "\t\t}" );

			writer.WriteLine();

			writer.WriteLine( "\t\tpublic override void OnResponse( NetState sender, RelayInfo info )" );
			writer.WriteLine( "\t\t{" );
			writer.WriteLine( "\t\t}" );

			writer.WriteLine( "\t}" );

			writer.Write( "}" );
		}

		public void WriteSphereGump( StreamWriter writer )
		{
			writer.WriteLine( "[DIALOG d_SpyUO]" );
			writer.WriteLine( "{0},{1}", m_X, m_Y );
			foreach ( GumpEntry entry in m_Layout )
			{
				if ( entry is GumpPage )
					writer.WriteLine();

				int i = 0;
				while ( true )
				{
					writer.Write( entry.Commands[i] );
					if ( ++i < entry.Commands.Length )
						writer.Write( " " );
					else
						break;
				}

				if ( entry is GumpHtmlLocalized )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalized)entry).Number] as string );
				else if ( entry is GumpHtmlLocalizedColor )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalizedColor)entry).Number] as string );
				else if ( entry is GumpHtmlLocalizedArgs )
					writer.WriteLine( " // " + LocalizedList.List.Table[((GumpHtmlLocalizedArgs)entry).Number] as string );
				else
					writer.WriteLine();
			}
			writer.WriteLine();

			writer.WriteLine( "[DIALOG d_SpyUO TEXT]" );
			foreach ( string txt in Text )
			{
				writer.WriteLine( txt );
			}
			writer.WriteLine();

			writer.WriteLine( "[DIALOG d_SpyUO BUTTON]" );
			ArrayList list = new ArrayList();
			list.Add( 0 );
			foreach ( GumpEntry entry in m_Layout )
			{
				GumpButton button = entry as GumpButton;
				if ( button != null && button.Type != 0 && !list.Contains( button.ButtonId ) )
				{
					list.Add( button.ButtonId );
				}
			}
			list.Sort();
			foreach ( int b in list )
			{
				writer.WriteLine( "ON={0}", b );
			}

			writer.WriteLine();
			writer.Write( "[EOF]" );
		}
	}

	public abstract class GumpEntry
	{
		public static GumpEntry Create( string[] commands, BaseGump parent )
		{
			string command = commands[0].ToLower();
			switch ( command )
			{
				case "nomove":
					return new GumpNotDragable( commands, parent );
				case "noclose":
					return new GumpNotClosable( commands, parent );
				case "nodispose":
					return new GumpNotDisposable( commands, parent );
				case "noresize":
					return new GumpNotResizable( commands, parent );
				case "checkertrans":
					return new GumpAlphaRegion( commands, parent );
				case "resizepic":
					return new GumpBackground( commands, parent );
				case "button":
					return new GumpButton( commands, parent );
				case "checkbox":
					return new GumpCheck( commands, parent );
				case "group":
					return new GumpGroup( commands, parent );
				case "htmlgump":
					return new GumpHtml( commands, parent );
				case "xmfhtmlgump":
					return new GumpHtmlLocalized( commands, parent );
				case "xmfhtmlgumpcolor":
					return new GumpHtmlLocalizedColor( commands, parent );
				case "xmfhtmltok":
					return new GumpHtmlLocalizedArgs( commands, parent );
				case "gumppic":
					return new GumpImage( commands, parent );
				case "gumppictiled":
					return new GumpImageTiled( commands, parent );
				case "buttontileart":
					return new GumpImageTiledButton( commands, parent );
				case "tilepic":
					return new GumpItem( commands, parent );
				case "tilepichue":
					return new GumpItemColor( commands, parent );
				case "text":
					return new GumpLabel( commands, parent );
				case "croppedtext":
					return new GumpLabelCropped( commands, parent );
				case "page":
					return new GumpPage( commands, parent );
				case "radio":
					return new GumpRadio( commands, parent );
				case "textentry":
					return new GumpTextEntry( commands, parent );
				case "tooltip":
					return new GumpTooltip( commands, parent );

				default:
					throw new ArgumentException();
			}
		}

		private string[] m_Commands;
		private BaseGump m_Parent;

		public string[] Commands { get { return m_Commands; } }
		public BaseGump Parent { get { return m_Parent; } }

		public GumpEntry( string[] commands, BaseGump parent )
		{
			m_Commands = commands;
			m_Parent = parent;
		}

		public abstract string GetRunUOLine();

		public int GetInt32( int n )
		{
			return Int32.Parse( m_Commands[n] );
		}

		public bool GetBoolean( int n )
		{
			return GetInt32( n ) != 0;
		}

		public string GetString( int n )
		{
			string cmd = m_Commands[n];
			return cmd.Substring( 1, cmd.Length - 2 );
		}

		public string GetText( int n )
		{
			return m_Parent.Text[n];
		}

		public static string Format( bool b )
		{
			return b ? "true" : "false";
		}

		public static string Format( string s )
		{
			return s.Replace( "\t", "\\t" );
		}
	}

	public class GumpNotDragable : GumpEntry
	{
		public GumpNotDragable( string[] commands, BaseGump parent ) : base( commands, parent )
		{
		}

		public override string GetRunUOLine()
		{
			return "Dragable = false;";
		}

		public override string ToString()
		{
			return "Not Dragable";
		}
	}

	public class GumpNotClosable : GumpEntry
	{
		public GumpNotClosable( string[] commands, BaseGump parent ) : base( commands, parent )
		{
		}

		public override string GetRunUOLine()
		{
			return "Closable = false;";
		}

		public override string ToString()
		{
			return "Not Closable";
		}
	}

	public class GumpNotDisposable : GumpEntry
	{
		public GumpNotDisposable( string[] commands, BaseGump parent ) : base( commands, parent )
		{
		}

		public override string GetRunUOLine()
		{
			return "Disposable = false;";
		}

		public override string ToString()
		{
			return "Not Disposable";
		}
	}

	public class GumpNotResizable : GumpEntry
	{
		public GumpNotResizable( string[] commands, BaseGump parent ) : base( commands, parent )
		{
		}

		public override string GetRunUOLine()
		{
			return "Resizable = false;";
		}

		public override string ToString()
		{
			return "Not Resizable";
		}
	}

	public class GumpAlphaRegion : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }

		public GumpAlphaRegion( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddAlphaRegion( {0}, {1}, {2}, {3} );", m_X, m_Y, m_Width, m_Height );
		}

		public override string ToString()
		{
			return string.Format( "Alpha Region: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\"",
				m_X, m_Y, m_Width, m_Height );
		}
	}

	public class GumpBackground : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_GumpId;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int GumpId { get { return m_GumpId; } }

		public GumpBackground( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_GumpId = GetInt32( 3 );
			m_Width = GetInt32( 4 );
			m_Height = GetInt32( 5 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddBackground( {0}, {1}, {2}, {3}, 0x{4:X} );", m_X, m_Y, m_Width, m_Height, m_GumpId );
		}

		public override string ToString()
		{
			return string.Format( "Background: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", GumpId: \"0x{4:X}\"",
				m_X, m_Y, m_Width, m_Height, m_GumpId );
		}
	}

	public class GumpButton : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_NormalId;
		private int m_PressedId;
		private int m_ButtonId;
		private int m_Type;
		private int m_Param;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int NormalId { get { return m_NormalId; } }
		public int PressedId { get { return m_PressedId; } }
		public int ButtonId { get { return m_ButtonId; } }
		public int Type { get { return m_Type; } }
		public int Param { get { return m_Param; } }

		public GumpButton( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_NormalId = GetInt32( 3 );
			m_PressedId = GetInt32( 4 );
			m_Type = GetInt32( 5 );
			m_Param = GetInt32( 6 );
			m_ButtonId = GetInt32( 7 );
		}

		public override string GetRunUOLine()
		{
			string type = m_Type == 0 ? "GumpButtonType.Page" : "GumpButtonType.Reply";
			return string.Format( "AddButton( {0}, {1}, 0x{2:X}, 0x{3:X}, {4}, {5}, {6} );",
				m_X, m_Y, m_NormalId, m_PressedId, m_ButtonId, type, m_Param );
		}

		public override string ToString()
		{
			return string.Format( "Button: \"X: \"{0}\", Y: \"{1}\", NormalId: \"0x{2:X}\", PressedId: \"0x{3:X}\", ButtonId: \"{4}\", Type: \"{5}\", Param: \"{6}\"",
				m_X, m_Y, m_NormalId, m_PressedId, m_ButtonId, m_Type, m_Param );
		}
	}

	public class GumpCheck : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_InactiveId;
		private int m_ActiveId;
		private bool m_InitialState;
		private int m_SwitchId;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int InactiveId { get { return m_InactiveId; } }
		public int ActiveId { get { return m_ActiveId; } }
		public bool InitialState { get { return m_InitialState; } }
		public int SwitchId { get { return m_SwitchId; } }

		public GumpCheck( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_InactiveId = GetInt32( 3 );
			m_ActiveId = GetInt32( 4 );
			m_InitialState = GetBoolean( 5 );
			m_SwitchId = GetInt32( 6 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddCheck( {0}, {1}, 0x{2:X}, 0x{3:X}, {4}, {5} );",
				m_X, m_Y, m_InactiveId, m_ActiveId, Format( m_InitialState ), m_SwitchId );
		}

		public override string ToString()
		{
			return string.Format( "Check: X: \"{0}\", Y: \"{1}\", InactiveId: \"0x{2:X}\", ActiveId: \"0x{3:X}\", InitialState: \"{4}\", SwitchId: \"{5}\"",
				m_X, m_Y, m_InactiveId, m_ActiveId, m_InitialState, m_SwitchId );
		}
	}

	public class GumpGroup : GumpEntry
	{
		private int m_Group;

		public int Group { get { return m_Group; } }

		public GumpGroup( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_Group = GetInt32( 1 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddGroup( {0} );", m_Group );
		}

		public override string ToString()
		{
			return string.Format( "Group: \"{0}\"", m_Group );
		}
	}

	public class GumpHtml : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private string m_Text;
		private bool m_Background;
		private bool m_Scrollbar;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public string Text { get { return m_Text; } }
		public bool Background { get { return m_Background; } }
		public bool Scrollbar { get { return m_Scrollbar; } }

		public GumpHtml( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Text = GetText( GetInt32( 5 ) );
			m_Background = GetBoolean( 6 );
			m_Scrollbar = GetBoolean( 7 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddHtml( {0}, {1}, {2}, {3}, \"{4}\", {5}, {6} );",
				m_X, m_Y, m_Width, m_Height, Format( m_Text ), Format( m_Background ), Format( m_Scrollbar ) );
		}

		public override string ToString()
		{
			return string.Format( "Html: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Text: \"{4}\", Background: \"{5}\", Scrollbar: \"{6}\"",
				m_X, m_Y, m_Width, m_Height, m_Text, m_Background, m_Scrollbar );
		}
	}

	public class GumpHtmlLocalized : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_Number;
		private bool m_Background;
		private bool m_Scrollbar;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int Number { get { return m_Number; } }
		public bool Background { get { return m_Background; } }
		public bool Scrollbar { get { return m_Scrollbar; } }

		public GumpHtmlLocalized( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Number = GetInt32( 5 );

			if ( commands.Length < 8 )
			{
				m_Background = false;
				m_Scrollbar = false;
			}
			else
			{
				m_Background = GetBoolean( 6 );
				m_Scrollbar = GetBoolean( 7 );
			}
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddHtmlLocalized( {0}, {1}, {2}, {3}, {4}, {5}, {6} );",
				m_X, m_Y, m_Width, m_Height, m_Number, Format( m_Background ), Format( m_Scrollbar ) );
		}

		public override string ToString()
		{
			return string.Format( "HtmlLocalized: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Number: \"{4}\", Background: \"{5}\", Scrollbar: \"{6}\"",
				m_X, m_Y, m_Width, m_Height, m_Number, m_Background, m_Scrollbar );
		}
	}

	public class GumpHtmlLocalizedColor : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_Number;
		private int m_Color;
		private bool m_Background;
		private bool m_Scrollbar;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int Number { get { return m_Number; } }
		public int Color { get { return m_Color; } }
		public bool Background { get { return m_Background; } }
		public bool Scrollbar { get { return m_Scrollbar; } }

		public GumpHtmlLocalizedColor( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Number = GetInt32( 5 );
			m_Background = GetBoolean( 6 );
			m_Scrollbar = GetBoolean( 7 );
			m_Color = GetInt32( 8 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddHtmlLocalized( {0}, {1}, {2}, {3}, {4}, 0x{5:X}, {6}, {7} );",
				m_X, m_Y, m_Width, m_Height, m_Number, m_Color, Format( m_Background ), Format( m_Scrollbar ) );
		}

		public override string ToString()
		{
			return string.Format( "HtmlLocalizedColor: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Number: \"{4}\", Color: \"0x{5:X}\", \"Background: \"{6}\", Scrollbar: \"{7}\"",
				m_X, m_Y, m_Width, m_Height, m_Number, m_Color, m_Background, m_Scrollbar );
		}
	}

	public class GumpHtmlLocalizedArgs : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_Number;
		private string m_Args;
		private int m_Color;
		private bool m_Background;
		private bool m_Scrollbar;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int Number { get { return m_Number; } }
		public string Args{ get{ return m_Args; } }
		public int Color { get { return m_Color; } }
		public bool Background { get { return m_Background; } }
		public bool Scrollbar { get { return m_Scrollbar; } }

		public GumpHtmlLocalizedArgs( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Background = GetBoolean( 5 );
			m_Scrollbar = GetBoolean( 6 );
			m_Color = GetInt32( 7 );
			m_Number = GetInt32( 8 );
			m_Args = GetString( 9 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddHtmlLocalized( {0}, {1}, {2}, {3}, {4}, \"{5}\", 0x{6:X}, {7}, {8} );",
				m_X, m_Y, m_Width, m_Height, m_Number, Format( m_Args ), m_Color, Format( m_Background ), Format( m_Scrollbar ) );
		}

		public override string ToString()
		{
			return string.Format( "HtmlLocalizedArgs: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Number: \"{4}\", Args: \"{5}\", Color: \"0x{6:X}\", \"Background: \"{7}\", Scrollbar: \"{8}\"",
				m_X, m_Y, m_Width, m_Height, m_Number, m_Args, m_Color, m_Background, m_Scrollbar );
		}
	}

	public class GumpImage : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_GumpId;
		private int m_Color;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int GumpId { get { return m_GumpId; } }
		public int Color { get { return m_Color; } }

		public GumpImage( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_GumpId = GetInt32( 3 );

			if ( commands.Length > 4 )
				m_Color = Int32.Parse( commands[4].Substring( 4 ) );
			else
				m_Color = 0;
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddImage( {0}, {1}, 0x{2:X}{3} );",
				m_X, m_Y, m_GumpId, m_Color != 0 ? ", 0x" + m_Color.ToString( "X" ) : "" );
		}

		public override string ToString()
		{
			return string.Format( "Image: \"X: \"{0}\", Y: \"{1}\", GumpId: \"0x{2:X}\", Color: \"0x{3:X}\"",
				m_X, m_Y, m_GumpId, m_Color );
		}
	}

	public class GumpImageTiled : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_GumpId;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int GumpId { get { return m_GumpId; } }

		public GumpImageTiled( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_GumpId = GetInt32( 5 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddImageTiled( {0}, {1}, {2}, {3}, 0x{4:X} );",
				m_X, m_Y, m_Width, m_Height, m_GumpId );
		}

		public override string ToString()
		{
			return string.Format( "ImageTiled: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", GumpId: \"0x{4:X}\"",
				m_X, m_Y, m_Width, m_Height, m_GumpId );
		}
	}

	public class GumpImageTiledButton : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_NormalID;
		private int m_PressedID;
		private int m_ButtonID;
		private int m_Type;
		private int m_Param;

		private int m_ItemID;
		private int m_Hue;
		private int m_Width;
		private int m_Height;

		public int X { get { return m_X; } }
		public int Y { get { return m_Y; } }
		public int NormalID { get { return m_NormalID; } }
		public int PressedID { get { return m_PressedID; } }
		public int ButtonID { get { return m_ButtonID; } }
		public int Type { get { return m_Type; } }
		public int Param { get { return m_Param; } }

		public int ItemID { get { return m_ItemID; } }
		public int Hue { get { return m_Hue; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }

		public GumpImageTiledButton( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_NormalID = GetInt32( 3 );
			m_PressedID = GetInt32( 4 );
			m_Type = GetInt32( 5 );
			m_Param = GetInt32( 6 );
			m_ButtonID = GetInt32( 7 );
			m_ItemID = GetInt32( 8 );
			m_Hue = GetInt32( 9 );
			m_Width = GetInt32( 10 );
			m_Height = GetInt32( 11 );
		}

		public override string GetRunUOLine()
		{
			string type = ( m_Type == 0 ? "GumpButtonType.Page" : "GumpButtonType.Reply" );
			return String.Format( "AddImageTiledButton( {0}, {1}, 0x{2:X}, 0x{3:X}, 0x{4:X}, {5}, {6}, 0x{7:X}, 0x{8:X}, {9}, {10} );",
				m_X, m_Y, m_NormalID, m_PressedID, m_ButtonID, type, m_Param, m_ItemID, m_Hue, m_Width, m_Height );
		}

		public override string ToString()
		{
			return string.Format( "ImageTiledButton: \"X: \"{0}\", Y: \"{1}\", Id1: \"0x{2:X}\", Id2: \"0x{3:X}\", ButtonId: \"0x{4:X}\", Type: \"{5}\", Param: \"{6}\", ItemId: \"0x{7:X}\", Hue: \"0x{8:X}\", Width: \"{9}\", Height: \"{10}\"",
				m_X, m_Y, m_NormalID, m_PressedID, m_ButtonID, m_Type, m_Param, m_ItemID, m_Hue, m_Width, m_Height );
		}
	}

	public class GumpItem : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_GumpId;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int GumpId { get { return m_GumpId; } }

		public GumpItem( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_GumpId = GetInt32( 3 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddItem( {0}, {1}, 0x{2:X} );", m_X, m_Y, m_GumpId );
		}

		public override string ToString()
		{
			return string.Format( "Item: \"X: \"{0}\", Y: \"{1}\", GumpId: \"0x{2:X}\"",
				m_X, m_Y, m_GumpId );
		}
	}

	public class GumpItemColor : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_GumpId;
		private int m_Color;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int GumpId { get { return m_GumpId; } }
		public int Color { get { return m_Color; } }

		public GumpItemColor( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_GumpId = GetInt32( 3 );
			m_Color = GetInt32( 4 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddItem( {0}, {1}, 0x{2:X}, 0x{3:X} );",
				m_X, m_Y, m_GumpId, m_Color );
		}

		public override string ToString()
		{
			return string.Format( "ItemColor: \"X: \"{0}\", Y: \"{1}\", GumpId: \"0x{2:X}\", Color: \"0x{3:X}\"",
				m_X, m_Y, m_GumpId, m_Color );
		}
	}

	public class GumpLabel : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Color;
		private string m_Text;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Color { get { return m_Color; } }
		public string Text { get { return m_Text; } }

		public GumpLabel( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Color = GetInt32( 3 );
			m_Text = GetText( GetInt32( 4 ) );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddLabel( {0}, {1}, 0x{2:X}, \"{3}\" );",
				m_X, m_Y, m_Color, Format( m_Text ) );
		}

		public override string ToString()
		{
			return string.Format( "Label: \"X: \"{0}\", Y: \"{1}\", Color: \"0x{2:X}\", Text: \"{3}\"",
				m_X, m_Y, m_Color, m_Text );
		}
	}

	public class GumpLabelCropped : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_Color;
		private string m_Text;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Color { get { return m_Color; } }
		public string Text { get { return m_Text; } }

		public GumpLabelCropped( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Color = GetInt32( 5 );
			m_Text = GetText( GetInt32( 6 ) );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddLabelCropped( {0}, {1}, {2}, {3}, 0x{4:X}, \"{5}\" );",
				m_X, m_Y, m_Width, m_Height, m_Color, Format( m_Text ) );
		}

		public override string ToString()
		{
			return string.Format( "LabelCropped: \"X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Color: \"0x{4:X}\", Text: \"{5}\"",
				m_X, m_Y, m_Width, m_Height, m_Color, m_Text );
		}
	}

	public class GumpPage : GumpEntry
	{
		private int m_Page;

		public int Page { get { return m_Page; } }

		public GumpPage( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_Page = GetInt32( 1 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddPage( {0} );", m_Page );
		}

		public override string ToString()
		{
			return string.Format( "Page: \"{0}\"", m_Page );
		}
	}

	public class GumpRadio : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_InactiveId;
		private int m_ActiveId;
		private bool m_InitialState;
		private int m_SwitchId;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int InactiveId { get { return m_InactiveId; } }
		public int ActiveId { get { return m_ActiveId; } }
		public bool InitialState { get { return m_InitialState; } }
		public int SwitchId { get { return m_SwitchId; } }

		public GumpRadio( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_InactiveId = GetInt32( 3 );
			m_ActiveId = GetInt32( 4 );
			m_InitialState = GetBoolean( 5 );
			m_SwitchId = GetInt32( 6 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddRadio( {0}, {1}, 0x{2:X}, 0x{3:X}, {4}, {5} );",
				m_X, m_Y, m_InactiveId, m_ActiveId, Format( m_InitialState ), m_SwitchId );
		}

		public override string ToString()
		{
			return string.Format( "Radio: X: \"{0}\", Y: \"{1}\", InactiveId: \"0x{2:X}\", ActiveId: \"0x{3:X}\", InitialState: \"{4}\", SwitchId: \"{5}\"",
				m_X, m_Y, m_InactiveId, m_ActiveId, m_InitialState, m_SwitchId );
		}
	}

	public class GumpTextEntry : GumpEntry
	{
		private int m_X;
		private int m_Y;
		private int m_Width;
		private int m_Height;
		private int m_Color;
		private int m_EntryId;
		private string m_InitialText;

		public int X { get { return m_X; } }
		public int Y { get { return m_X; } }
		public int Width { get { return m_Width; } }
		public int Height { get { return m_Height; } }
		public int Color { get { return m_Color; } }
		public int EntryId { get { return m_EntryId; } }
		public string InitialText { get { return m_InitialText; } }

		public GumpTextEntry( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_X = GetInt32( 1 );
			m_Y = GetInt32( 2 );
			m_Width = GetInt32( 3 );
			m_Height = GetInt32( 4 );
			m_Color = GetInt32( 5 );
			m_EntryId = GetInt32( 6 );
			m_InitialText = GetText( GetInt32( 7 ) );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddTextEntry( {0}, {1}, {2}, {3}, 0x{4:X}, {5}, \"{6}\" );",
				m_X, m_Y, m_Width, m_Height, m_Color, m_EntryId, Format( m_InitialText ) );
		}

		public override string ToString()
		{
			return string.Format( "TextEntry: X: \"{0}\", Y: \"{1}\", Width: \"{2}\", Height: \"{3}\", Color: \"0x{4:X}\", EntryId: \"{5}\", Text: \"{6}\"",
				m_X, m_Y, m_Width, m_Height, m_Color, m_EntryId, m_InitialText );
		}
	}

	public class GumpTooltip : GumpEntry
	{
		private int m_Number;

		public int Number { get { return m_Number; } }

		public GumpTooltip( string[] commands, BaseGump parent ) : base( commands, parent )
		{
			m_Number = GetInt32( 1 );
		}

		public override string GetRunUOLine()
		{
			return string.Format( "AddTooltip( {0} );", m_Number );
		}

		public override string ToString()
		{
			return string.Format( "Tooltip: Number: \"{0}\"", m_Number );
		}

	}
}