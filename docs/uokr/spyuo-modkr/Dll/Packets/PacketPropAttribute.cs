using System;

namespace SpyUO.Packets
{
	[AttributeUsage( AttributeTargets.Property )]
	public class PacketPropAttribute : Attribute, IComparable
	{
		private int m_Position;
		private string m_Format;

		public int Position { get { return m_Position; } }
		public string Format { get { return m_Format; } }

		public PacketPropAttribute() : this( int.MaxValue, "{0}" )
		{
		}

		public PacketPropAttribute( int position ) : this( position, "{0}" )
		{
		}

		public PacketPropAttribute( string format ) : this( int.MaxValue, format )
		{
		}

		public PacketPropAttribute( int position, string format )
		{
			m_Position = position;
			m_Format = format;
		}

		public int CompareTo( object obj )
		{
			PacketPropAttribute pp = obj as PacketPropAttribute;

			if ( pp == null )
				return -1;
			else
				return m_Position.CompareTo( pp.m_Position );
		}
	}
}