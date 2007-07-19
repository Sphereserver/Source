using System;
using System.Collections.Generic;
using System.Text;

namespace SpyUO.Packets
{
    [PacketInfo(0xE3)]
    public class EncryptionSet : Packet
    {
        private uint m_LenData1;
        private byte[/*m_LenData1*/] m_Data1;
        private uint m_LenData2;
        private byte[/*m_LenData2*/] m_Data2;
        private uint m_LenData3;
        private byte[/*m_LenData3*/] m_Data3;
        private uint m_Unknown;
        private uint m_LenDataIV;
        private byte[/*m_LenDataIV*/] m_DataIV;

        [PacketProp(0)]
        public uint LengthData1 { get { return m_LenData1; } }

        [PacketProp(1)]
        public string Data1 { get { return Packet.ByteArrayToString(m_Data1); } }

        [PacketProp(2)]
        public uint LengthData2 { get { return m_LenData2; } }

        [PacketProp(3)]
        public string Data2 { get { return Packet.ByteArrayToString(m_Data2); } }

        [PacketProp(4)]
        public uint LengthData3 { get { return m_LenData3; } }

        [PacketProp(5)]
        public string Data3 { get { return Packet.ByteArrayToString(m_Data3); } }

        [PacketProp(6)]
        public uint Unknown { get { return m_Unknown; } }

        [PacketProp(7)]
        public uint LengthDataIV { get { return m_LenDataIV; } }

        [PacketProp(8)]
        public string DataIV { get { return Packet.ByteArrayToString(m_DataIV); } }

        public EncryptionSet(PacketReader reader, bool send) : base(reader, send)
		{
            reader.ReadUInt16();

            m_LenData1 = reader.ReadUInt32();
            m_Data1 = new byte[m_LenData1];
            m_Data1 = reader.ReadBytes(m_LenData1);
            m_LenData2 = reader.ReadUInt32();
            m_Data2 = new byte[m_LenData2];
            m_Data2 = reader.ReadBytes(m_LenData2);
            m_LenData3 = reader.ReadUInt32();
            m_Data3 = new byte[m_LenData3];
            m_Data3 = reader.ReadBytes(m_LenData3);
            m_Unknown = reader.ReadUInt32();
            m_LenDataIV = reader.ReadUInt32();
            m_DataIV = new byte[m_LenDataIV];
            m_DataIV = reader.ReadBytes(m_LenDataIV);
		}
    }
}
