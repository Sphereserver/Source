using System;
using System.IO;
using Zlib;

namespace Repacker.Mythic
{
    class R_MythicPackageData
    {
        private short m_Flag;
        private short m_DataOffset;
        private long m_DateTime;
        private byte[] m_data;
        private bool m_compressed;
        private int m_csize;
        private int m_usize;

        public short Flag
        {
            get { return m_Flag; }
            set { m_Flag = value; }
        }

        public short DataOffset
        {
            get { return m_DataOffset; }
            set { m_DataOffset = value; }
        }

        public long DateTime
        {
            get { return m_DateTime; }
            set { m_DateTime = value; }
        }

        public byte[] Data
        {
            get { return m_data; }
            set { m_data = value; }
        }

        public bool Compressed
        {
            get { return m_compressed; }
            set { m_compressed = value; }
        }

        public int C_size
        {
            get { return m_csize; }
            set { m_csize = value; }
        }

        public int U_size
        {
            get { return m_usize; }
            set { m_usize = value; }
        }

        internal static R_MythicPackageData Pack(string current_file)
        {
            /*
             * * [4] - Data Block/File (sizeof: 12+Lenght bytes) 
             * * WORD -> flag compressed ( 0008 )
             * * WORD -> offset  ( 0003 )
             * * QWORD -> FileTime
             * * BYTE(Lenght) 0xd88 -> compressed data 
             * * ...this is repeated until all Files from FileIndexes are processed 
             */
            FileStream reading = new FileStream(current_file, FileMode.Open);

            int usize = (int)reading.Length;            //Stream Size
            int csize = usize;                          //Compressed Size REF
            byte[] sourceData = new byte[usize];        //Stream to be compressed
            byte[] destData = new byte[csize];          //Stream compressed
            bool compressed = true;                     //Must say if datas are TO BE compressed or not.
            

            R_MythicPackageData final = new R_MythicPackageData();

            using (BinaryReader reader = new BinaryReader(reading))
            {
                reader.Read(sourceData, 0, usize);
            }
            if (sourceData.Length <= 10)//If file length is less than 10 bytes we do not compress it.
            {
                compressed = false;
            }

            if (compressed)
            {
                ZLibError error = Zlib.Zlib.Compress(destData, ref csize, sourceData, usize);

                if (error != ZLibError.Okay)
                    throw new Exception(String.Format("Error Compressing: {0}", error));
            }
            else
            {
                csize = usize;
                destData = sourceData;
            }

            final.Flag = 3;        //Is this fixed?
            final.DataOffset = 8;  //And this?
            final.DateTime = System.DateTime.Now.ToFileTime(); //To check
            byte[] compressedData = new byte[csize];
            for (int i = 0; i < csize; i++)
            {
                compressedData[i] = destData[i];
            }
            final.Data = compressedData;          //Ok
            final.Compressed = compressed;  //Ok
            final.C_size = csize;           //Ok
            final.U_size = usize;           //Ok

            return final;
        }

        internal static R_MythicPackageData Empty()
        {
            R_MythicPackageData d = new R_MythicPackageData();
            d.Flag = 0;
            d.Data = BitConverter.GetBytes(0) ;
            d.DataOffset = 0;
            d.DateTime = 0;
            return d;
        }
    }
}
