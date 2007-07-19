using System;
using System.Collections.Generic;
using System.Text;

namespace SpyUO.Packets
{
    [PacketInfo(0xE4)]
    public class EncryptionReply : Packet
    {
        private uint m_LenData1;
        private byte[/*m_LenData1*/] m_Data1;

        [PacketProp(0)]
        public uint LengthData1 { get { return m_LenData1; } }

        [PacketProp(1)]
        public string Data1 { get { return Packet.ByteArrayToString(m_Data1); } }

        public EncryptionReply(PacketReader reader, bool send) : base(reader, send)
		{
            reader.ReadUInt16();

            m_LenData1 = reader.ReadUInt32();
            m_Data1 = new byte[m_LenData1];
            m_Data1 = reader.ReadBytes(m_LenData1);
        }
    }
}
