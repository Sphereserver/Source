using System;

namespace SpyUO.Packets
{
	[PacketInfo( 0xBF )]
	public class ExtendedCommand: Packet
	{
		private ushort m_Length;
		private ushort m_Type;
		private byte[/* m_Length - 5 */] m_Data;

		private string m_Name;

		[PacketProp( 0, "0x{0:X}" )]
		public ushort Type { get { return m_Type; } }

		[PacketProp( 1 )]
		public string Name { get { return m_Name; } }

		public ExtendedCommand( PacketReader reader, bool send ) : base( reader, send )
		{
			m_Length = reader.ReadUInt16();
			m_Type = reader.ReadUInt16();
			m_Data = reader.ReadBytes(m_Length - 5);

			switch(m_Type)
			{
				case 0x01:
					m_Name = "FastWalk"; break;
				case 0x02:
					m_Name = "WalkKeyAdd"; break;
				case 0x04:
					m_Name = "GumpClose"; break;
				case 0x05:
					m_Name = "ScreenSize"; break;
				case 0x06:
					m_Name = "PartyPacket"; break;
				case 0x07:
					m_Name = "QuestArrow"; break;
				case 0x08:
					m_Name = "MapChange"; break;
				case 0x09:
					m_Name = "DisarmRequest"; break;
				case 0x0a:
					m_Name = "StunRequest"; break;
				case 0x0b:
					m_Name = "LanguageSet"; break;
				case 0x0c:
					m_Name = "StatusClose"; break;
				case 0x0e:
					m_Name = "Animate"; break;
				case 0x0f:
					m_Name = "Unknown_Empty"; break;
				case 0x10:
					m_Name = "ObjectPropertyListOld"; break;
				case 0x13:
					m_Name = "ContextMenuRequest"; break;
				case 0x14:
					m_Name = "ContextMenuDisplay"; break;
				case 0x15:
					m_Name = "ContextMenuResponse"; break;
				case 0x17:
					m_Name = "DisplayHelp"; break;
				case 0x18:
					m_Name = "MapDiffEnable"; break;
				case 0x19:
					m_Name = "MiscellaneousStatus"; break;
				case 0x1a:
					m_Name = "StatLockChange"; break;
				case 0x1b:
					m_Name = "NewSpellbookContent"; break;
				case 0x1c:
					m_Name = "CastSpell"; break;
				case 0x1d:
					m_Name = "HouseDesignVersion"; break;
				case 0x1e:
					m_Name = "HouseDesignDetailed"; break;
				case 0x20:
					m_Name = "HouseCustomization"; break;
				case 0x21:
					m_Name = "WeaponAbilityClear"; break;
				case 0x22:
					m_Name = "DamagePacketOld"; break;
				case 0x24:
					m_Name = "Unknown_CheatDetector"; break;
				case 0x25:
					m_Name = "SESpellIconToggle"; break;
				case 0x26:
					m_Name = "SpeedModeSet"; break;
				default:
					m_Name = "Unknown";	break;
			}
		}
	}
}