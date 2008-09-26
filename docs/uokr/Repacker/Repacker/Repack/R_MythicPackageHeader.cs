using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Repacker.Mythic
{
    class R_MythicPackageHeader
    {
        private int m_Version;
        private uint m_Misc;
        private int m_HeaderSize;
        private int m_BlockSize;
        private int m_FileCount;
        private int m_Zeros;
        private List<R_MythicPackageBlock> m_Blocks;
        private byte[] m_start = new byte[4];
        private int m_Fill;

        public int Version
        {
            get { return m_Version; }
            set { m_Version = value; }
        }

        public uint Misc
        {
            get { return m_Misc; }
            set { m_Misc = value; }
        }

        public int HeaderSize
        {
            get { return m_HeaderSize; }
            set { m_HeaderSize = value; }
        }

        public int BlockSize
        {
            get { return m_BlockSize; }
            set { m_BlockSize = value; }
        }

        public int FileCount
        {
            get { return m_FileCount; }
            set { m_FileCount = value; }
        }

        public List<R_MythicPackageBlock> Blocks
        {
            get { return m_Blocks; }
            set { m_Blocks = value; }
        }

        public byte[] Start
        {
            get { return m_start; }
            set { m_start = value; }
        }

        public int Zeros
        {
            get { return m_Zeros; }
            set { m_Zeros = value; }
        }
        public int Fill
        {
            get { return m_Fill; }
            set { m_Fill = value; }
        }

        internal static R_MythicPackageHeader Create( List<R_MythicPackageBlock> blocks )
        {
            /*
             * [1] - General Format Header (sizeof: 40bytes ) 
             * Byte(23) 0x0 - 0x17 -> Containing general file headers (Version etc.) 
             * DWORD? 0x18 -> Amount of contained files/indexes 
             * byte(12) -> Unknown gibberish 
             * 
             * DWORD MYTH 	-> Start of every Myth package
             * DWORD 		-> Version
             * DWORD 0xFD	-> Misc ( 0xFD23EC43 )
             * DWORD		-> Header Size
             * DWORD 0x00	-> Empty bytes
             * DWORD 		-> Block size
             * DWORD 		-> File count
             */

            R_MythicPackageHeader package = new R_MythicPackageHeader();

            int counted = 0;    //We count all files number.
            for (int i = 0; i < blocks.Count; i++)
            {
                counted += blocks[i].FileCount;
            }

            //Whole Size must be 40bytes
            package.Start[0] = (BitConverter.GetBytes('M'))[0];
            package.Start[1] = (BitConverter.GetBytes('Y'))[0];
            package.Start[2] = (BitConverter.GetBytes('P'))[0];
            package.Start[3] = (BitConverter.GetBytes(0))[0];
            package.Version = 4;                    //Version
            package.Misc = 0xFD23EC43;              //Misc ( 0xFD23EC43 )
            package.HeaderSize = 40;                //Size ( fixed! )
            package.Zeros = 0;                      //DWORD empty bytes
            package.BlockSize = 100;                //Block size
            package.FileCount = counted;            //Number of files
            package.Blocks = blocks;                //Should copy all blocks into new list.
            package.Fill = (package.HeaderSize - 28);

            return package;
        }
    }
}
