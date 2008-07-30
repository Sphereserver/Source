using System;
using System.Collections.Generic;
using System.Text;
using Ultima;
using System.IO;

namespace MapMULToUOP
{
    //1     2      7
    //  ----------
    //  |        |
    //  |   Map  |
    //0 |  Block | 3
    //  |        |
    //  |        |
    //  ----------
    //6     5      4

    public enum DelimiterDirection
    {
        TOPLEFT = 1,
        TOP = 2,
        TOPRIGHT = 7,
        RIGHT = 3,
        BOTTOMRIGHT = 4,
        BOTTOM = 5,
        BOTTOMLEFT = 6,
        LEFT = 0
    }

    public class Delimiter
    {
        public DelimiterDirection direction = DelimiterDirection.LEFT;
        public ushort graphic = 0;
        public byte unk = 0;
        public sbyte z;
    }


    public class BlockWriter
    {
        private string apppath;
        private Map mappa;
        private int facetId;
        private Dictionary<ushort, ushort> grMap;
        private int mapwidth, mapheight;
        private StreamWriter sw;

        public BlockWriter(int facetId, Map mappa, String apppath, Dictionary<ushort, ushort> grMap)
        {
            sw = new StreamWriter(apppath.Replace("Temp\\","") + "error_graphic.log");
            this.facetId = facetId;
            this.apppath = apppath;
            this.mappa = mappa;
            this.grMap = grMap;
            this.mapheight = mappa.Height;
            this.mapwidth = ((facetId == 0 || facetId == 1) ? 7168 : mappa.Width);
        }

        private String FileId(uint blockid)
        {
            blockid++;
            uint fid=blockid/100;
            uint sid=blockid%100;
            return "_"+fid+"_"+sid;
        }

        private sbyte WriteLandtile(BinaryWriter bw, uint X, uint Y, out ushort Graphic)
        {
            if (X != 0 || Y != 0)
                bw.Write((ushort)0);
            Tile tile=mappa.Tiles.GetLandTile((int)X, (int)Y);
            bw.Write((sbyte)tile.Z);
            ushort KRGraphic=0;
            ushort MLGraphic = (ushort)tile.ID;
            if (!grMap.TryGetValue(MLGraphic, out KRGraphic))
                sw.WriteLine("X="+X+" Y="+Y+", Graphic not founded in conversion table: " + MLGraphic);
            bw.Write(KRGraphic);
            bw.Write((byte)0);
            bw.Write(MLGraphic);
            Graphic = KRGraphic;
            return (sbyte)tile.Z;
        }

        private void WriteStatic(BinaryWriter bw, HuedTile stat)
        {
            bw.Write((ushort)stat.ID);
            bw.Write((ushort)0);
            bw.Write((sbyte)stat.Z);
            bw.Write((ushort)stat.Hue);
        }

        private void WriteDelimiters(BinaryWriter bw, uint OffsetX, uint OffsetY, uint RealX, uint RealY, sbyte Z, ushort KRGraphic)
        {
            List<Delimiter> delimiters = new List<Delimiter>();
            if (OffsetX == 0 && RealX>0)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.LEFT;
                delimiters.Add(d);
            }
            if (OffsetX == 63 && RealX<mapwidth-1)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.RIGHT;
                delimiters.Add(d);
            }
            if (OffsetY == 0 && RealY>0)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.TOP;
                delimiters.Add(d);
            }
            if (OffsetY == 63 && RealX<mapheight-1)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.BOTTOM;
                delimiters.Add(d);
            }
            if ((OffsetX == 0) && (OffsetY == 0) && RealX > 0 && RealY > 0)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.TOPLEFT;
                delimiters.Add(d);
            }
            if ((OffsetX == 0) && (OffsetY == 63) && RealX > 0 && RealX < mapheight - 1)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.BOTTOMLEFT;
                delimiters.Add(d);
            }
            if ((OffsetX == 63) && (OffsetY == 0) && RealX < mapwidth - 1 && RealY > 0)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction = DelimiterDirection.TOPRIGHT;
                delimiters.Add(d);
            }
            if ((OffsetX == 63) && (OffsetY == 63) && RealX < mapwidth - 1 && RealX < mapheight - 1)
            {
                Delimiter d = new Delimiter();
                d.z = Z;
                d.graphic = KRGraphic;
                d.direction=DelimiterDirection.BOTTOMRIGHT;
                delimiters.Add(d);
            }
            if (delimiters.Count > 0)
            {
                Delimiter[] delmiterarr = delimiters.ToArray();
                uint i;
                for (i = 0; i<delmiterarr.Length; i++)
                {
                    if (i == 0)
                        bw.Write((byte)delmiterarr.Length);
                    else
                        bw.Write((byte)0);
                    bw.Write((byte)delmiterarr[i].direction);
                    bw.Write(delmiterarr[i].z);
                    bw.Write(delmiterarr[i].graphic);
                    bw.Write((byte)delmiterarr[i].unk);
                }                    
            }
            
        }

        private void WriteBlock(uint blockid)
        {
            BinaryWriter bw = new BinaryWriter(File.OpenWrite(apppath + "facet" + facetId + FileId(blockid) + ".dat"));
            uint XBlock = blockid / 64;
            uint YBlock = blockid - XBlock * 64;
            uint StartOffsetX = XBlock * 64;
            uint StartOffsetY = YBlock * 64;
            uint X, Y;
            uint i;
            for (X = 0; X < 64; X++)
            {
                for (Y = 0; Y < 64; Y++)
                {
                    ushort KRGraphic;
                    sbyte Z = WriteLandtile(bw, StartOffsetX + X, StartOffsetY + Y, out KRGraphic);
                    HuedTile[] Tiles = mappa.Tiles.GetStaticTiles((int)(StartOffsetX+X), (int)(StartOffsetY+Y));
                    for (i = 0; i < Tiles.Length; i++)
                    {
                        bw.Write((byte)0);
                        if (i == 0)
                            bw.Write((byte)Tiles.Length);
                        else
                            bw.Write((byte)0);
                        WriteStatic(bw, Tiles[i]);
                    }
                    WriteDelimiters(bw, X, Y, StartOffsetX + X, StartOffsetY + Y, Z, KRGraphic);
                }
            }
            bw.Write(new byte[] {0,0,0});
            bw.Close();
        }

        public void Write()
        {
            int blocknums = ((int)(mappa.Width / 64)) * ((int)(mappa.Height / 64));
            uint i;
            BinaryWriter bw = new BinaryWriter(File.OpenWrite(apppath + "facet" + facetId + "_0_0.raw"));
            bw.Write((ushort)mappa.Height);
            bw.Write((ushort)0);
            bw.Write((ushort)((facetId == 0 || facetId == 1) ? 7168 : mappa.Width));
            bw.Write((ushort)0);
            bw.Close();
            for (i = 0; i < blocknums; i++)
            {
                WriteBlock(i);
            }
            sw.Close();
        }
    }
}
