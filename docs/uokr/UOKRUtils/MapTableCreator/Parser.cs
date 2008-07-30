using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace MapTableCreator
{
    class Parser
    {
        public static SortedDictionary<ushort, ushort> table = new SortedDictionary<ushort, ushort>();

        private static ushort AddToDictinary(ushort key, ushort value)
        {
            ushort valued;
            lock (table)
            {
                if (!table.TryGetValue(key, out valued))
                    table.Add(key, value);
                else
                {
                    if (value != valued)
                        return valued;
                }
            }
            return 0;
        }

        private BinaryReader br;
        private TextWriter err,output;

        private bool writeonlyfiles, tilecheck, staticcheck, delimitercheck;

        private const ushort BLOCK_WIDTH = 64;
        private const ushort BLOCK_HEIGHT = 64;

        private const ushort YBLOCKS = 64;

        private MapPoint mp;
        private ushort fileid;

        private void WriteFileErr(String towrite)
        {
            lock(err)
            {
                err.WriteLine(towrite);
            }
        }

        private void WriteFileOut(String towrite)
        {
            lock (output)
            {
                output.Write(towrite);
            }
        }

        public Parser(string filename,TextWriter err,TextWriter output, bool writeonlyfiles, bool delimitercheck,bool staticcheck,bool tilecheck)
        {
            br = new BinaryReader(File.OpenRead(filename));
            this.err =err;
            this.output=output;
            this.writeonlyfiles = writeonlyfiles;
            this.delimitercheck = delimitercheck;
            this.tilecheck = tilecheck;
            this.staticcheck = staticcheck;
        }

        private ushort AdjustX(ushort x)
        {
            ushort XBlock = (ushort)(fileid / YBLOCKS);
            return (ushort)(XBlock * YBLOCKS + x);
        }

        private ushort AdjustY(ushort y)
        {
            ushort YBlock = (ushort)(fileid / YBLOCKS);
            YBlock = (ushort)(fileid - YBlock * YBLOCKS);
            return (ushort)(YBlock * YBLOCKS + y);
        }


        private Landtile ReadLandtile()
        {
            Landtile l = new Landtile();
            l.z = br.ReadSByte();
            l.KRGraphic = br.ReadUInt16();
            l.Type3 = br.ReadByte();
            l.MLGraphic = br.ReadUInt16();
            ushort value = AddToDictinary(l.MLGraphic, l.KRGraphic);
            if (value!=0)
                WriteFileErr("FileID=" + fileid + ", Error X=" + mp.x + " (offset " + mp.offsetx + "), Y=" + mp.y + " (offset " + mp.offsety + "), KR Graphic: " + value + ", for ML: " + l.MLGraphic + " inserted another: " + l.KRGraphic);
            if(l.Type3!=0 && mp.x<6144)
                WriteFileErr("FileID="+fileid + ", Error X=" + mp.x + " (offset " + mp.offsetx + "), Y=" + mp.y + " (offset " + mp.offsety + "), Type3 is: " +l.Type3+", KR Graphic: "+l.KRGraphic+" ML:"+l.MLGraphic);
            
            return l;
        }

        private Static ReadStatic()
        {
            Static s = new Static();
            s.graphic = br.ReadUInt16();
            ushort zero=br.ReadUInt16();
            if (zero != 0)
                WriteFileErr("FileID=" + fileid + ", X=" + mp.x + " (offset " + mp.offsetx + "), Y=" + mp.y + " (offset " + mp.offsety + "), error static two zero byte is not zero: " + zero);
            s.z = br.ReadSByte();
            s.color = br.ReadUInt16();
            return s;
        }

        private Delimiter ReadDelimiter()
        {
            Delimiter d = new Delimiter();
            d.direction = (DelimiterDirection)br.ReadByte();
            d.z = br.ReadSByte();
            d.graphic = br.ReadUInt16();
            d.unk=br.ReadByte();

            if (mp.offsetx != 63 && mp.offsetx != 0 && mp.offsety != 63 && mp.offsety != 0)
            {
                WriteFileErr("FileID=" + fileid + ", X=" + mp.x + " (offset " + mp.offsetx + "), Y=" + mp.y + " (offset " + mp.offsety + "), error delimiter!");
            }

            return d;
        }



        public BlockFile Parse()
        {

            BlockFile result = new BlockFile();

            result.header.facet=br.ReadByte();
            result.header.FileID = br.ReadUInt16();

            fileid = result.header.FileID;

            if (output != null)
                WriteFileOut(result.header.Dump());

            mp = new MapPoint();
            mp.offsetx = 0;
            mp.x = AdjustX(0);
            mp.offsety = 0;
            mp.y = AdjustY(0);
            mp.lt = ReadLandtile();


            ushort currentx=0;
            ushort currenty=0;

            byte type1,type2;
            while (br.BaseStream.Position < br.BaseStream.Length - 3)
            {
                type1 = br.ReadByte();
                type2 = br.ReadByte();
                if (type1 == 0 && type2 == 0) //Landtile
                {
                    currenty++;
                    if (currenty == BLOCK_HEIGHT)
                    {
                        currentx++;
                        currenty = 0;
                        if (currentx >= BLOCK_WIDTH)
                            WriteFileErr("FileID=" + fileid + ", ERROR X is too huge: " + currentx);
                    }
                    if ((mp.delimiter_list.Count > 0 && delimitercheck) || (mp.static_list.Count > 0 && staticcheck) || (mp.delimiter_list.Count == 0 && mp.static_list.Count == 0 && tilecheck))
                    {
                        if(!writeonlyfiles)
                            result.blocks.Add(mp);
                        if(output!=null)
                            WriteFileOut(mp.Dump());
                    }
                    mp = new MapPoint();
                    mp.offsetx = currentx;
                    mp.x = AdjustX(currentx);
                    mp.offsety = currenty;
                    mp.y = AdjustY(currenty);
                    mp.lt=ReadLandtile();
                }
                else if (type1 == 0 && type2 != 0) //Static
                {
                    for (int i = 0; i < type2; i++)
                    {
                        if (i > 0)
                        {
                            ushort zero = br.ReadUInt16();
                            if (zero != 0)
                                WriteFileErr("FileID=" + fileid + ", X=" + AdjustX(currentx) + " (offset " + currentx + "), Y=" + AdjustY(currenty) + " (offset " + currenty + "), error static header placeholder zero byte is not zero: " + zero);
                        }
                        mp.static_list.Add(ReadStatic());
                    }
                }
                else if (type1 > 0)
                {
                    br.BaseStream.Position--;
                    for (int i = 0; i < type1; i++)
                    {
                        if (i > 0)
                        {
                            byte zero = br.ReadByte();
                            if (zero != 0)
                                WriteFileErr("FileID=" + fileid + ", X=" + AdjustX(currentx) + " (offset " + currentx + "), Y=" + AdjustY(currenty) + " (offset " + currenty + "), error delimiter type1 placeholder zero byte is not zero: " + zero);
                        }
                        mp.delimiter_list.Add(ReadDelimiter());
                    }
                }
                else
                    WriteFileErr("FileID=" + fileid + ", ERROR Type undefined, type1:" + type1 + ", type2:" + type2);
            }
            if ((mp.delimiter_list.Count > 0 && delimitercheck) || (mp.static_list.Count > 0 && staticcheck) || (mp.delimiter_list.Count == 0 && mp.static_list.Count == 0 && tilecheck))
            {
                if (!writeonlyfiles)
                    result.blocks.Add(mp);
                if (output != null)
                    WriteFileOut(mp.Dump());
            }
            br.Close();
            return result;
        }

    }
}
