using System;
using System.Reflection;

namespace SpyUO.Packets
{
	public class PacketProp : IComparable
	{
		private PropertyInfo m_PropInfo;
		private PacketPropAttribute m_Attribute;
		private object m_Value;

		public PropertyInfo PropInfo { get { return m_PropInfo; } }
		public PacketPropAttribute Attribute { get { return m_Attribute; } }
		public object Value { get { return m_Value; } }

		public PacketProp( PropertyInfo propInfo, PacketPropAttribute attribute, object value )
		{
			m_PropInfo = propInfo;
			m_Attribute = attribute;
			m_Value = value;
		}

		public PacketProp( PacketProp packetProp, object newValue )
		{
			m_PropInfo = packetProp.m_PropInfo;
			m_Attribute = packetProp.m_Attribute;
			m_Value = newValue;
		}

		public string GetStringValue()
		{
			if ( m_Value != null )
				return string.Format( m_Attribute.Format, m_Value );
			else
				return "null";
		}

		public int CompareTo( object obj )
		{
			PacketProp pp = obj as PacketProp;

			if ( pp == null )
				return -1;
			else
				return m_Attribute.CompareTo( pp.m_Attribute );
		}
	}
}