using System;
using System.IO;
using System.Windows.Forms;
using Repacker.Repack;

namespace Repacker.Mythic
{
    class R_MythicPackageIndex
    {		
        //private int m_Index;
		private R_MythicPackageData m_DataBlock;
		private long m_DataBlockOffset;
		private int m_HeadLength;
		private int m_CompressedSize;
		private int m_DecompressedSize;
		private long m_FileHash;
        private uint m_CRC;
        private short m_flag;

		/*public int Index
		{
			get{ return m_Index; }
			set{ m_Index = value; }
		}*/

		public R_MythicPackageData DataBlock
		{
			get{ return m_DataBlock; }
			set{ m_DataBlock = value; }
		}

		public long DataBlockOffset
		{
			get{ return m_DataBlockOffset; }
			set{ m_DataBlockOffset = value; }
		}

		public int HeadLength
		{
            get { return m_HeadLength; }
            set { m_HeadLength = value; }
		}

		public int CompressedSize
		{
			get{ return m_CompressedSize; }
			set{ m_CompressedSize = value; }
		}

		public int DecompressedSize
		{
			get{ return m_DecompressedSize; }
			set{ m_DecompressedSize = value; }
		}

		public long FileHash
		{
			get{ return m_FileHash; }
			set{ m_FileHash = value; }
		}

		public uint CRC
		{
            get { return m_CRC; }
            set { m_CRC = value; }
		}

        public short Flag
        {
            get { return m_flag; }
            set { m_flag = value; }
        }

        internal static R_MythicPackageIndex Pack(string current_file, ref bool error)
        {
            /*- Create index for each data (sizeof: 34bytes )
             * QWORD 0x34 -> Offset to start of Data Block of this file 
             * DWORD 0x3c -> Length of Data Block header (usually 0x0C) 
             * DWORD 0x40 -> Lenght of compressed data 
             * DWORD 0x44 -> Size of decompressed file 
             * QWORD 0x48 -> Filehash 
             * DWORD 0x50 -> Adler32 CRC of Data in littleEndian ( flag offset data ) 
             * WORD  0x54 -> Compressed Flag
             */
            R_MythicPackageData datas = new R_MythicPackageData();
            R_MythicPackageIndex final = new R_MythicPackageIndex();

            datas = R_MythicPackageData.Pack(current_file); //Pack current file datas

            string toHash = ""; //Initialized "" to prevent error in hashing
            /*error = false;      //Actually we does not want to get out of the cycle
            //We are looking fo Build folder, that is the root.
            int inde = current_file.IndexOf("build\\");
            if (inde != -1) //if found
                toHash = current_file.Substring(Form1.FolderName.Length+1);
                toHash = toHash.Replace("\\", "/");
                //toHash = current_file.Substring(inde, current_file.Length - inde);
            else
            {
                MessageBox.Show("NO BUILD FOLDER FOUND!");
                error = true;//We are going out from cycle
            }*/
            toHash = current_file.Substring(Form1.FolderName.Length+1);
            toHash = toHash.Replace("\\", "/");

            bool compr = datas.Compressed;  //Are datas compressed?

            final.DataBlockOffset = 0;  //Offset to start of Data Block of this file 
            final.HeadLength = 0x0C;    //Is this fixed? 
            final.CompressedSize = datas.C_size;   //Ok 
            final.DecompressedSize = datas.U_size; //Ok
            long hash = 0;
            Form1.Dictionary.TryGetValue(toHash, out hash);
            final.FileHash = hash; //FileHash To Check
            final.CRC = final.find_CRC(datas);                //See find_CRC
            final.Flag = compr ? (short)0x1 : (short)0x0;         //Flag Compressed data To check
            final.DataBlock = datas;                //Tocheck

            if (final.CRC == 0)
            {
                MessageBox.Show("Unable to find CRC!");
                error = true; //We are going out from cycle
            }
            return final;
        }

        internal uint find_CRC(R_MythicPackageData data)
        {
            AdlerChecksum crc = new AdlerChecksum();
            byte[] tocheck = new byte[ BitConverter.GetBytes(data.Flag).Length + BitConverter.GetBytes(data.DataOffset).Length + data.Data.Length];

            BitConverter.GetBytes(data.Flag).CopyTo(tocheck, 0); //tocheck position in array
            BitConverter.GetBytes(data.DataOffset).CopyTo(tocheck, BitConverter.GetBytes(data.Flag).Length); //tocheck position in array
            data.Data.CopyTo(tocheck, BitConverter.GetBytes(data.Flag).Length + BitConverter.GetBytes(data.DataOffset).Length);

            if(crc.MakeForBuff(tocheck))
                return crc.ChecksumValue;
            else
                return 0; //this will be Error
        }

        internal static R_MythicPackageIndex Empty()
        {
            R_MythicPackageIndex e = new R_MythicPackageIndex();
            e.CompressedSize = 0;
            e.CRC = 0;
            e.DataBlock = null;
            e.DataBlockOffset = 0;
            e.DecompressedSize = 0;
            e.FileHash = 0;
            e.Flag = 0;
            e.HeadLength = 0;
            return e;
        }
    }
}
