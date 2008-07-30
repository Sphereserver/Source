using System;
using System.Collections.Generic;
using System.Text;

namespace MapTableCreator
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
        TOPLEFT=1,
        TOP=2,
        TOPRIGHT=7,
        RIGHT=3,
        BOTTOMRIGHT=4,
        BOTTOM=5,
        BOTTOMLEFT=6,
        LEFT=0
    }

    public class MapFileHeader
    {
        public byte facet = 0;
        public ushort FileID = 0;

        public string Dump()
        {
            return "MapFile Structure:" + Environment.NewLine + "Facet:" + this.facet + Environment.NewLine + "FileID:" + this.FileID + Environment.NewLine + "----------------" + Environment.NewLine;
        }
    }

    public class Landtile : GenericBlock
    {
        public ushort KRGraphic=0;
        public ushort MLGraphic=0;
        public byte Type3=0;

        public string Dump()
        {
            String result = "";
            result += result += "Landtile - Z" + this.z + Environment.NewLine;
            result += "KR Gr: " + this.KRGraphic + ", ML Gr: " + this.MLGraphic + Environment.NewLine;
            result += "Type3: " + this.Type3 + Environment.NewLine;
            return result;
        }

    }

    public class Delimiter : GenericBlock
    {
        public DelimiterDirection direction = DelimiterDirection.LEFT;
        public ushort graphic = 0;
        public byte unk = 0;

        public string Dump()
        {
            String result = "";
            result += "Delimiter - Z" + this.z + Environment.NewLine;
            result += "Dir: " + ((int)this.direction).ToString() + ", Gr: " + this.graphic + Environment.NewLine;
            result += "Unk: " + this.unk + Environment.NewLine + Environment.NewLine;
            return result;
        }
    }

    public class Static : GenericBlock
    {
        public ushort graphic = 0;
        public ushort color = 0;

        public string Dump()
        {
            String result = "";
            result += "Static - Z" + this.z + Environment.NewLine;
            result += "Gr: " + this.graphic + ", Col: " + this.color + Environment.NewLine + Environment.NewLine;
            return result;
        }
    }

    public abstract class GenericBlock
    {
        public sbyte z = 0;
    }

    public class MapPoint
    {
        public ushort x = 0;
        public ushort offsetx = 0;
        public ushort y = 0;
        public ushort offsety = 0;

        public Landtile lt=new Landtile();
        public List<Static> static_list = new List<Static>();
        
        public List<Delimiter> delimiter_list = new List<Delimiter>();

        public string Dump()
        {
            String result = "X " + this.x + " (" + this.offsetx + "), Y " + this.y + " (" + this.offsety + ")"+Environment.NewLine+lt.Dump();
            result += "Delimiters: ("+delimiter_list.Count+")" + Environment.NewLine;
            foreach (Delimiter d in delimiter_list)
            {
                result += d.Dump();
            }
            result += Environment.NewLine;
            return result;
        }
    }

    public class BlockFile : IComparer<BlockFile>, IComparable<BlockFile>
    {

        public MapFileHeader header = new MapFileHeader();
        public List<MapPoint> blocks = new List<MapPoint>(64*64);

        public String Name
        {
            get { return "Block "+header.FileID; }
        }

        public string  Dump()
        {
            String result = header.Dump();
            foreach (MapPoint p in blocks)
            {
                result += p.Dump();
            }
            return result;
        }

        public int CompareTo(BlockFile a)
        {
            return this.header.FileID.CompareTo(a.header.FileID);
        }

        public int Compare(BlockFile a, BlockFile b)
        {
            return a.header.FileID.CompareTo(b.header.FileID);
        }

    }

    public class Facet
    {
        public ushort facetId = 0;
        public LinkedList<BlockFile> blockfiles = new LinkedList<BlockFile>();
        public List<BlockFile> blockfileslist = new List<BlockFile>(112*64);
    }
}
