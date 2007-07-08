using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MapFileDecryptor
{
    class UopParser
    {
        BinaryReader br;
        public UopParser(string filename)
        {
            br=null;
            try
            {
                br = new BinaryReader(File.OpenRead(filename));
            }
            catch { br = null; }
        }

        public bool Ready() 
        { 
            return br != null; 
        }

        static public UInt16 Flip(UInt16 input)
        {
            Byte p= (Byte)(input & 0xFF);
            Byte s = (Byte)((input >> 8) & 0xFF);
            return (UInt16)((UInt16)s + ((UInt16)p << 8));
        }

        public List<MapElem> Parse(out uint cells, out uint delims, out uint statics, out long UntilToEnd)
        {
            List<MapElem> res = new List<MapElem>();
            Byte Unk1, Type1, Type2;
            cells = 1;
            delims = 0;
            statics = 0;
            try
            {
                MapCell m = new MapCell();
                MapDelimiter d;
                MapStaticBlock s;
                m.Unk = br.ReadByte();
                m.Unk2 = br.ReadUInt16();
                m.Z = br.ReadSByte();
                m.NewGraphic = br.ReadUInt16();
                m.OldGraphic = Flip(br.ReadUInt16());
                res.Add(m);
                while (br.BaseStream.Position < br.BaseStream.Length-3)
                {
                    Unk1 = br.ReadByte();
                  /*  if (Unk1 != 0)
                        throw new Exception("Unk1!=0 Unk1=" + Unk1);*/
                    Type1 = br.ReadByte();
                    Type2 = br.ReadByte();
                    if (Type1 == 0 && Type2 == 0)  // cell
                    {
                        cells++;
                        br.BaseStream.Position -= 2;
                        m = new MapCell();
                        m.Unk = Unk1;
                        m.Unk2 = br.ReadUInt16();
                        m.Z = br.ReadSByte();
                        m.NewGraphic = br.ReadUInt16();
                        m.OldGraphic = Flip(br.ReadUInt16());
                        res.Add(m);
                    }
                    else if (Type1 != 0) // delimiter
                    {
                        delims++;
                        d = new MapDelimiter();
                        d.Unk = Unk1;
                        d.Direction = Type2;
                        d.Z = br.ReadSByte();
                        d.NewGraphic = br.ReadUInt16();
                        for (int i = 0; i < Type1 - 1; i++)
                        {
                            delims++;
                            MapDelimiter tmp = d.AppendNew();
                            tmp.Unk = br.ReadByte();
                            if (br.ReadByte() != 0)
                                throw new Exception("Not good on delimiter!");
                            tmp.Direction = br.ReadByte();
                            tmp.Z = br.ReadSByte();
                            tmp.NewGraphic = br.ReadUInt16();
                        }
                        res.Add(d);
                    }
                    else // static
                    {
                        statics++;
                        s = new MapStaticBlock();
                        s.Unk = Unk1;
                        s.SGraphic = br.ReadUInt16();
                        s.UnkS = br.ReadUInt16();
                        s.Z = br.ReadSByte();
                        s.Color = br.ReadUInt16();
                        br.BaseStream.Position -= 1;
                        for (int i = 0; i < Type2 - 1; i++)
                        {
                            statics++;
                            MapStaticBlock tmp = s.AppendNew();
                            tmp.Unk = br.ReadByte();
                            if (br.ReadUInt16() != 0)
                                throw new Exception("Not good on statics!");
                            tmp.SGraphic = br.ReadUInt16();
                            tmp.UnkS = br.ReadUInt16();
                            tmp.Z = br.ReadSByte();
                            tmp.Color = br.ReadUInt16();
                            br.BaseStream.Position-=1;
                        }
                        res.Add(s);
                    }
                }

            }
            catch(Exception e)
            {
                System.Windows.Forms.MessageBox.Show(e.Message);
            }
            UntilToEnd = br.BaseStream.Length-br.BaseStream.Position;
            GC.Collect();
            br.BaseStream.Position = 0;
            return res;
        }
    }
}
