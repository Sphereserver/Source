using System;
using System.Collections.Generic;
using System.Text;

namespace SpyUO.Packets
{
    [PacketInfo(0xED)]
    public class MacroUnequipItem : Packet
    {
        private byte m_LayerCount;
        private List<byte> m_LayerIds = new List<byte>();

        [PacketProp(0)]
        public byte LayerCount { get { return m_LayerCount; } }

        [PacketProp(1)]
        public string LayerIds
        {
            get
            {
                if (m_LayerIds.Count == 0)
                    return "Empty";

                StringBuilder sb = new StringBuilder();
                int i = 0;
                while (true)
                {
                    sb.AppendFormat("{0}", m_LayerIds[i]);

                    if (++i < m_LayerIds.Count)
                        sb.Append(" - ");
                    else
                        break;
                }

                return sb.ToString();
            }
        }

        public MacroUnequipItem(PacketReader reader, bool send) : base(reader, send)
		{
			reader.ReadUInt16();

            m_LayerCount = reader.ReadByte();
            for (int i = 0; i < (int)(m_LayerCount); i++)
			{
                m_LayerIds.Add(reader.ReadByte());
			}
		}
    }
}
