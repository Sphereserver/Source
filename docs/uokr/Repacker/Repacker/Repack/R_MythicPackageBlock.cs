using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Repacker.Mythic
{
    class R_MythicPackageBlock
    {
        private int m_FileCount;
        private long m_NextBlock;
        private List<R_MythicPackageIndex> m_Files;

        public int FileCount
        {
            get { return m_FileCount; }
            set { m_FileCount = value; }
        }

        public long NextBlock
        {
            get { return m_NextBlock; }
            set { m_NextBlock = value; }
        }

        public List<R_MythicPackageIndex> Files
        {
            get { return m_Files; }
            set { m_Files = value; }
        }

        public R_MythicPackageIndex Adds(R_MythicPackageIndex index)
        {
            if (m_Files == null)
                m_Files = new List<R_MythicPackageIndex>();

            m_Files.Add(index);
            return index;
        }

        internal static R_MythicPackageBlock Pack(List<R_MythicPackageIndex> indexes, int k, ref int i)
        {
            /*
             * [2] - Index Block Header (sizeof: 24bytes) 
             * There can be multiple index blocks, they are splitted into chunks. 
             * DWORD 0x28 -> Amount of contained files in this index, max 100/0x64 
             * QWORD 0x2c -> Offset to the next index block header OR Zero 
             * When a index block doesn't contain 100 index definitions, it will be padded with nulls 
             */
            R_MythicPackageBlock final = new R_MythicPackageBlock();

            for (int j = 0; j < 100; j++)  //Should fill single block
            {
                if (k < indexes.Count)
                {
                    final.Adds(indexes[k]);
                    final.FileCount++;      //Amount of contained files in this index, max 100/0x64 
                    k++;
                    i = k;
                }
                else
                    final.Adds(R_MythicPackageIndex.Empty());
            }
            final.NextBlock = 0;    //Offset to the next index block header OR Zero 

            return final;
        }
    }
}
