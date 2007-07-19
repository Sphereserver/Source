using System;
using System.Collections.Generic;
using System.Text;

namespace SpyUO.Packets
{
    [PacketInfo(0xFF)]
    public class EncryptionStart : Packet
    {
        private byte[] m_Ffs;

        [PacketProp(0, "0x{0:X}")]
        public uint CryptHeader { get { return (uint)((m_Ffs[2] * 0x1000000) + (m_Ffs[1] * 0x10000) + (m_Ffs[0] * 0x100) + base.Cmd); } }

        public EncryptionStart(PacketReader reader, bool send) : base(reader, send)
		{
            m_Ffs = new byte[3];
            m_Ffs = reader.ReadBytes(3);
		}
    }
}
