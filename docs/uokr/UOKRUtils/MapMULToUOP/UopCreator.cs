using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Collections;

namespace MapMULToUOP
{
    class MapFileComparer : IComparer
    {
        private int GetValue(String s)
        {
            int indb = s.LastIndexOf("\\");
            if (indb == -1)
                indb = 0;
            int ind = s.IndexOf("_", indb);
            int ind2 = s.IndexOf("_", ind + 1);
            int ind3 = s.IndexOf(".");
            String num1 = s.Substring(ind + 1, ind2 - ind - 1);
            String num2 = s.Substring(ind2 + 1, ind3 - ind2 - 1);
            return Convert.ToInt32(num1) * 100 + Convert.ToInt32(num2);
        }

        int IComparer.Compare(Object a, Object b)
        {
            return GetValue((String)a).CompareTo(GetValue((String)b));
        }
    }

    public class MapParameter : GenericParameter
    {
        public int facetId;

    }

    public abstract class GenericParameter
    {
        public String temppath;
    }

    public class UopCreator   
    {

        GenericParameter p;

        StreamWriter err;

        BinaryWriter uopfile;

        public UopCreator(GenericParameter p)
        {
            this.p = p;
            err = new StreamWriter(p.temppath.Replace("Temp\\", "") + "error_uop.log");
        }

        public bool Write()
        {
            String[] files=null;
            String uopname="";
            if (this.p is MapParameter)
            {
                IComparer icomp=new MapFileComparer();
                files = Directory.GetFiles(p.temppath, "facet"+((MapParameter)p).facetId+"*.*");
                Array.Sort(files,icomp);
                uopname="facet"+((MapParameter)p).facetId+".uop";

            }
            foreach (String filename in files)
            {   
                byte[] source=File.ReadAllBytes(filename);
                byte[] dest=new byte[(int)(source.Length*1.1+12)];
                int destlen=dest.Length;
                ZLibError res=Compressor.Compress(dest, ref destlen, source, source.Length);
                if (res != ZLibError.Okay)
                {
                    err.WriteLine("Error compressing " + filename + ", error code:" + (int)res);
                    err.Close();
                    return false;
                }
                if(destlen!=dest.Length)
                {
                    byte[] desttemp=new byte[destlen];
                    Array.Copy(dest,desttemp,destlen);
                    dest=desttemp;
                }
                File.WriteAllBytes(p.temppath + filename + ".zlib", dest);
            }
            File.Delete(p.temppath.Replace("Temp\\", "") + uopname);
            uopfile = new BinaryWriter(File.OpenWrite(p.temppath.Replace("Temp\\", "") + uopname));

            // Header

            uopfile.Write(new byte[] { (byte)'M',(byte)'Y',(byte)'P',0 });
            uopfile.Write((uint)4); //Version
            uopfile.Write((uint)0xFD23EC43); //File Time
            uopfile.Write((uint)40); //Header size
            uopfile.Write((uint)0); // Empty
            uopfile.Write((uint)0x64); //Block size
            uopfile.Write((uint)files.Length);
            uopfile.Write((ulong)0); // Unk
            uopfile.Write((uint)0); //Unk

            int numIndex = files.Length / 100;
            if ((numIndex * 100) < files.Length)
                numIndex++;

            int realindex;
            FileInfo compressed,uncompressed;
            int i,j;
          
            for (i = 0; i < numIndex; i++)
            {
                List<UOPFileData> datas = new List<UOPFileData>();
                List<UOPFileIndexDef> indexes = new List<UOPFileIndexDef>();

                ulong totalLengthData = 0;
                for(j=0;j<((numIndex==i-1)?files.Length-((numIndex-1)*100):100);j++)
                {
                    realindex = i * 100 + j;
                    compressed = new FileInfo(files[realindex] + ".zlib");
                    uncompressed = new FileInfo(files[realindex]);
                    UOPFileData filedata=new UOPFileData();
                    filedata.m_CompressedData = File.ReadAllBytes(files[realindex] + ".zlib");
                    filedata.m_FileTime = compressed.LastAccessTime.ToFileTime();
                    UOPFileIndexDef def = new UOPFileIndexDef();
                    def.m_OffsetOfDataBlock = (ulong)(uopfile.BaseStream.Position)+((ulong)(UOPFileIndexDef.SIZE * 100)) + totalLengthData;
                    def.m_LenghtCompressed = (uint)compressed.Length;
                    def.m_LenghtUncompressed = (uint)uncompressed.Length;
                    def.adlerCRC32 = filedata.Hash();
                    def.fileHash = FileNameHasher.HashFileName(uncompressed.Name); // Controllare questo
                    datas.Add(filedata);
                    indexes.Add(def);
                    totalLengthData += filedata.GetLength();
                }
            }

            uopfile.Close();
            err.Close();
            return true;
        }
    }
}
