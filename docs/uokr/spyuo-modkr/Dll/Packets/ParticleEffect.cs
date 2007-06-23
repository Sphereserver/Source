using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xC7 )]
	public class ParticleEffect : Packet
	{
		private byte m_Type;
		private uint m_From;
		private uint m_To;
		private ushort m_ItemId;
		private Point3D m_FromPoint;
		private Point3D m_ToPoint;
		private byte m_Speed;
		private byte m_Duration;
		private bool m_FixedDirection;
		private bool m_Explode;
		private uint m_Hue;
		private uint m_RenderMode;
		private ushort m_Effect;
		private ushort m_ExplodeEffect;
		private ushort m_ExplodeSound;
		private uint m_Serial;
		private byte m_Layer;
		private short m_Unknown;

		[PacketProp( 0 )]
		public byte Type { get { return m_Type; } }

		[PacketProp( 1, "0x{0:X}" )]
		public uint From { get { return m_From; } }

		[PacketProp( 2, "0x{0:X}" )]
		public uint To { get { return m_To; } }

		[PacketProp( 3, "0x{0:X}" )]
		public ushort ItemId { get { return m_ItemId; } }

		[PacketProp( 4 )]
		public string ItemIdName
		{
			get
			{
				try
				{
					return Ultima.TileData.ItemTable[m_ItemId].Name;
				}
				catch
				{
					return null;
				}
			} 
		}

		[PacketProp( 5 )]
		public Point3D FromLocation { get { return m_FromPoint; } }

		[PacketProp( 6 )]
		public Point3D ToLocation { get { return m_ToPoint; } }

		[PacketProp( 7 )]
		public byte Speed { get { return m_Speed; } }

		[PacketProp( 8 )]
		public byte Duration { get { return m_Duration; } }

		[PacketProp( 9 )]
		public bool FixedDirection { get { return m_FixedDirection; } }

		[PacketProp( 10 )]
		public bool Explode { get { return m_Explode; } }

		[PacketProp( 11, "0x{0:X}" )]
		public uint Hue { get { return m_Hue; } }

		[PacketProp( 12, "0x{0:X}" )]
		public uint RenderMode { get { return m_RenderMode; } }

		[PacketProp( 13, "0x{0:X}" )]
		public ushort Effect { get { return m_Effect; } }

		[PacketProp( 14, "0x{0:X}" )]
		public ushort ExplodeEffect { get { return m_ExplodeEffect; } }

		[PacketProp( 15, "0x{0:X}" )]
		public ushort ExplodeSound { get { return m_ExplodeSound; } }

		[PacketProp( 16, "0x{0:X}" )]
		public uint Serial { get { return m_Serial; } }

		[PacketProp( 17 )]
		public byte Layer { get { return m_Layer; } }

		[PacketProp( 18, "0x{0:X}" )]
		public short Unknown { get { return m_Unknown; } }

		public ParticleEffect( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Type = reader.ReadByte();
			m_From = reader.ReadUInt32();
			m_To = reader.ReadUInt32();
			m_ItemId = reader.ReadUInt16();
			m_FromPoint = new Point3D( reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadSByte() );
			m_ToPoint = new Point3D( reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadSByte() );
			m_Speed = reader.ReadByte();
			m_Duration = reader.ReadByte();

			reader.ReadByte();
			reader.ReadByte();

			m_FixedDirection = reader.ReadBoolean();
			m_Explode = reader.ReadBoolean();
			m_Hue = reader.ReadUInt32();
			m_RenderMode = reader.ReadUInt32();
			m_Effect = reader.ReadUInt16();
			m_ExplodeEffect = reader.ReadUInt16();
			m_ExplodeSound = reader.ReadUInt16();
			m_Serial = reader.ReadUInt32();
			m_Layer = reader.ReadByte();
			m_Unknown = reader.ReadInt16();
		}
	}
}