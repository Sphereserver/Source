using System;
using System.Collections.Generic;
using System.Text;

namespace SpyUO.Packets
{
    [PacketInfo(0xEC)]
    public class MacroEquipItem : Packet
    {
        private byte m_ItemCount;
        private List<uint> m_ItemUids = new List<uint>();

        [PacketProp(0)]
        public byte ItemCount { get { return m_ItemCount; } }

        [PacketProp(1)]
        public string ItemUids
        {
            get
            {
                if (m_ItemUids.Count == 0)
                    return "Empty";

                StringBuilder sb = new StringBuilder();
                int i = 0;
                while (true)
                {
                    sb.AppendFormat("0x{0:X}", m_ItemUids[i]);

                    if (++i < m_ItemUids.Count)
                        sb.Append(" - ");
                    else
                        break;
                }

                return sb.ToString();
            }
        }

        public MacroEquipItem(PacketReader reader, bool send) : base(reader, send)
		{
			reader.ReadUInt16();

            m_ItemCount = reader.ReadByte();
            for (int i = 0; i < (int)(m_ItemCount); i++)
			{
                m_ItemUids.Add(reader.ReadUInt32());
			}
		}
    }
}
