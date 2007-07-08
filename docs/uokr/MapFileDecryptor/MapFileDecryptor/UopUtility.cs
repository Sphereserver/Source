using System;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Text;

namespace MapFileDecryptor
{

    class MapElem
    {
        public Byte Unk;
        public SByte Z;
        public UInt16 NewGraphic;
        public MapElem()
        {
            Unk = 0;
            Z = 0;
            NewGraphic = 0;
        }
    }

    class MapCell : MapElem
    {
        public UInt16 Unk2;
        public UInt16 OldGraphic;

        public MapCell()
        {
            OldGraphic = 0;
            Unk2 = 0;
        }
    }

    class MapDelimiter : MapElem
    {
        public Byte Direction;
        public MapDelimiter Next;

        public MapDelimiter()
        {
            Direction = 0;
            Next = null;
        }

        public Byte Count()
        {
            Byte c = 1;
            MapDelimiter mp=this;
            while (mp.Next != null)
            {
                c++;
                mp = mp.Next;
            }
            return c;
        }

        public MapDelimiter AppendNew()
        {
            MapDelimiter mp = this;
            while (mp.Next != null)
                mp = mp.Next;
            mp.Next = new MapDelimiter();
            return mp.Next;
        }

    }

    class MapStaticBlock : MapElem
    {
        public UInt16 SGraphic;
        public UInt16 UnkS;
        public UInt16 Color;
        public MapStaticBlock Next;

        public MapStaticBlock()
        {
            SGraphic = 0;
            UnkS = 0;
            Next = null;
        }

        public Byte Count()
        {
            Byte c = 1;
            MapStaticBlock mp = this;
            while (mp.Next != null)
            {
                c++;
                mp = mp.Next;
            }
            return c;
        }

        public MapStaticBlock AppendNew()
        {
            MapStaticBlock mp = this;
            while (mp.Next != null)
                mp = mp.Next;
            mp.Next = new MapStaticBlock();
            return mp.Next;
        }
    }

    class TransTable
    {
        public ArrayList trans_table;
        private Random r;

        public int this[int i]
        {
            get
            {
                try
                {
                    int[] arr = (int[])trans_table[i];
                    if (arr.Length == 1)
                        return arr[0];
                    else
                        return arr[r.Next(arr.Length)];
                }
                catch
                {
                    throw new Exception("Error on " + i + "graphic");
                }
            }
        }


        public TransTable()
        {
            r = new Random();
            trans_table = new ArrayList();
            for (int i = 0; i < 764; i++)
                trans_table.Add(null);

            trans_table[0] = new int[] { 2, 3, 151 };
            trans_table[1] = new int[] { 0, 2, 3, 24 };
            trans_table[2] = new int[] { 0, 2, 3, 24, 151 };
            trans_table[3] = new int[] { 1, 2, 3, 24, 58, 151 };
            trans_table[4] = new int[] { 1, 2, 6, 24, 60 };
            trans_table[5] = new int[] { 1, 2, 6, 59 };
            trans_table[6] = new int[] { 1, 6, 11 };
            trans_table[7] = new int[] { 6, 11 };
            trans_table[8] = new int[] { 3, 11 };
            trans_table[9] = new int[] { 3, 4, 11, 20, 64, 151 };
            trans_table[10] = new int[] { 3, 4, 12, 20, 151 };
            trans_table[11] = new int[] { 4, 12, 20, 151 };
            trans_table[12] = new int[] { 3, 4, 8, 12, 20, 151 };
            trans_table[13] = new int[] { 3, 8, 12, 20, 151, 160 };
            trans_table[14] = new int[] { 8, 20, 22, 49, 151, 161 };
            trans_table[15] = new int[] { 6, 8, 20, 22, 49, 162 };
            trans_table[16] = new int[] { 6, 20, 22, 163 };
            trans_table[17] = new int[] { 4, 6, 20, 22 };
            trans_table[18] = new int[] { 6, 20, 23 };
            trans_table[19] = new int[] { 6, 20, 23 };
            trans_table[20] = new int[] { 6, 8, 20, 23 };
            trans_table[21] = new int[] { 3, 4, 8, 20, 23 };
            trans_table[22] = new int[] { 3, 4, 7, 8, 94 };
            trans_table[23] = new int[] { 4, 7, 8, 95 };
            trans_table[24] = new int[] { 3, 4, 7, 96 };
            trans_table[25] = new int[] { 4, 7, 97 };
            trans_table[26] = new int[] { 4, 54, 93 };
            trans_table[27] = new int[] { 4, 56, 90 };
            trans_table[28] = new int[] { 4, 55, 91 };
            trans_table[29] = new int[] { 6, 49, 55, 92 };
            trans_table[30] = new int[] { 6, 7, 49, 57, 89 };
            trans_table[31] = new int[] { 6, 7, 57, 73, 146 };
            trans_table[32] = new int[] { 6, 7, 55, 78, 146 };
            trans_table[33] = new int[] { 7, 56, 79, 146 };
            trans_table[34] = new int[] { 6, 54, 80, 146 };
            trans_table[35] = new int[] { 2, 6, 54, 81, 146 };
            trans_table[36] = new int[] { 2, 6, 56, 75, 146 };
            trans_table[37] = new int[] { 2, 6, 56, 76, 146 };
            trans_table[38] = new int[] { 2, 54, 77, 146 };
            trans_table[39] = new int[] { 2, 54, 74, 146 };
            trans_table[40] = new int[] { 56, 88, 146 };
            trans_table[41] = new int[] { 2, 56, 98, 146 };
            trans_table[42] = new int[] { 2, 7, 49, 54, 99, 146 };
            trans_table[43] = new int[] { 2, 7, 49, 54, 100, 146 };
            trans_table[44] = new int[] { 2, 7, 49, 101, 146 };
            trans_table[45] = new int[] { 2, 7, 49, 57, 102, 146 };
            trans_table[46] = new int[] { 49, 55, 103, 146 };
            trans_table[47] = new int[] { 2, 49, 55, 72, 146 };
            trans_table[48] = new int[] { 2, 49, 55, 82, 146 };
            trans_table[49] = new int[] { 2, 49, 56, 83 };
            trans_table[50] = new int[] { 49, 54, 84 };
            trans_table[51] = new int[] { 2, 7, 49, 85 };
            trans_table[52] = new int[] { 2, 7, 45, 49, 86 };
            trans_table[53] = new int[] { 2, 6, 7, 45, 49, 87 };
            trans_table[54] = new int[] { 5, 6, 7, 25, 45, 49 };
            trans_table[55] = new int[] { 2, 5, 6, 7, 25, 45, 49 };
            trans_table[56] = new int[] { 2, 6, 7, 25, 45, 49 };
            trans_table[57] = new int[] { 2, 7, 25, 45, 49 };
            trans_table[58] = new int[] { 2, 7, 26 };
            trans_table[59] = new int[] { 6, 7, 13, 26, 169 };
            trans_table[60] = new int[] { 7, 13, 26 };
            trans_table[61] = new int[] { 2, 6, 7, 13, 26 };
            trans_table[62] = new int[] { 2, 6, 7, 13, 27 };
            trans_table[63] = new int[] { 2, 27 };
            trans_table[64] = new int[] { 2, 27 };
            trans_table[65] = new int[] { 2, 27 };
            trans_table[66] = new int[] { 1, 2, 7, 28 };
            trans_table[67] = new int[] { 1, 2, 7, 28 };
            trans_table[68] = new int[] { 2, 7, 29, 56 };
            trans_table[69] = new int[] { 2, 13, 29, 54 };
            trans_table[70] = new int[] { 2, 13, 30, 54 };
            trans_table[71] = new int[] { 2, 13, 30 };
            trans_table[72] = new int[] { 0, 2, 13, 30, 55 };
            trans_table[73] = new int[] { 0, 2, 13, 30, 57 };
            trans_table[74] = new int[] { 2, 6, 31, 57, 171 };
            trans_table[75] = new int[] { 0, 2, 7, 31, 55, 172 };
            trans_table[76] = new int[] { 2, 7, 31, 52, 173 };
            trans_table[77] = new int[] { 2, 7, 31, 52, 174 };
            trans_table[78] = new int[] { 2, 32, 52, 175 };
            trans_table[79] = new int[] { 2, 4, 7, 32, 52, 176 };
            trans_table[80] = new int[] { 4, 7, 21, 32, 52, 177 };
            trans_table[81] = new int[] { 4, 7, 21, 32, 52, 178 };
            trans_table[82] = new int[] { 4, 7, 21, 33, 52, 179 };
            trans_table[83] = new int[] { 2, 4, 7, 21, 33, 52, 180 };
            trans_table[84] = new int[] { 2, 7, 21, 33, 52, 181 };
            trans_table[85] = new int[] { 2, 4, 8, 21, 33, 52, 182 };
            trans_table[86] = new int[] { 2, 21, 30, 52, 183 };
            trans_table[87] = new int[] { 2, 8, 21, 30, 52, 188 };
            trans_table[88] = new int[] { 2, 8, 21, 30, 52 };
            trans_table[89] = new int[] { 7, 21, 30, 52, 189 };
            trans_table[90] = new int[] { 7, 16, 30, 52 };
            trans_table[91] = new int[] { 6, 7, 16, 30, 52 };
            trans_table[92] = new int[] { 6, 7, 15, 30, 52 };
            trans_table[93] = new int[] { 4, 16, 30, 52 };
            trans_table[94] = new int[] { 4, 16, 30, 52 };
            trans_table[95] = new int[] { 4, 16, 30, 52 };
            trans_table[96] = new int[] { 4, 16, 30, 52 };
            trans_table[97] = new int[] { 4, 16, 30, 52 };
            trans_table[98] = new int[] { 4, 17, 30, 52, 184 };
            trans_table[99] = new int[] { 4, 30, 52, 185 };
            trans_table[100] = new int[] { 4, 30, 52, 186 };
            trans_table[101] = new int[] { 4, 30, 52, 187 };
            trans_table[102] = new int[] { 4, 14, 30, 52 };
            trans_table[103] = new int[] { 2, 4, 14, 30 };
            trans_table[104] = new int[] { 2, 4, 15, 30, 52 };
            trans_table[105] = new int[] { 2, 14, 30, 52, 147 };
            trans_table[106] = new int[] { 2, 14, 30, 52, 147 };
            trans_table[107] = new int[] { 2, 16, 52, 147 };
            trans_table[108] = new int[] { 2, 16, 30, 52 };
            trans_table[109] = new int[] { 2, 16, 30, 52 };
            trans_table[110] = new int[] { 2, 3, 30, 52, 64, 149 };
            trans_table[111] = new int[] { 1, 2, 3, 30, 52, 147 };
            trans_table[112] = new int[] { 1, 2, 3, 30, 147 };
            trans_table[113] = new int[] { 1, 2, 3, 30, 155 };
            trans_table[114] = new int[] { 1, 2, 30, 155 };
            trans_table[115] = new int[] { 1, 2, 30, 148, 155 };
            trans_table[116] = new int[] { 1, 2, 30, 149, 155 };
            trans_table[117] = new int[] { 1, 2, 30, 147 };
            trans_table[118] = new int[] { 1, 2, 3, 30, 147 };
            trans_table[119] = new int[] { 2, 3, 30, 58 };
            trans_table[120] = new int[] { 2, 3, 30, 58 };
            trans_table[121] = new int[] { 2, 3, 30, 58, 62, 148 };
            trans_table[122] = new int[] { 1, 2, 3, 34, 58, 61 };
            trans_table[123] = new int[] { 1, 2, 3, 34, 60, 63, 149 };
            trans_table[124] = new int[] { 1, 2, 3, 34, 60, 147 };
            trans_table[125] = new int[] { 1, 3, 34, 60, 64 };
            trans_table[126] = new int[] { 1, 2, 35, 60, 64, 147 };
            trans_table[127] = new int[] { 0, 1, 2, 35, 59, 148 };
            trans_table[128] = new int[] { 1, 2, 36, 58 };
            trans_table[129] = new int[] { 0, 2, 3, 36, 59, 147 };
            trans_table[130] = new int[] { 1, 3, 7, 36, 59, 149 };
            trans_table[131] = new int[] { 1, 3, 36, 60, 149 };
            trans_table[132] = new int[] { 0, 3, 7, 35, 58, 149 };
            trans_table[133] = new int[] { 2, 3, 7, 35, 60, 148 };
            trans_table[134] = new int[] { 2, 3, 9, 37, 58 };
            trans_table[135] = new int[] { 2, 3, 37, 60 };
            trans_table[136] = new int[] { 2, 3, 37, 58 };
            trans_table[137] = new int[] { 1, 2, 37, 58, 149 };
            trans_table[138] = new int[] { 1, 2, 7, 58, 104, 149 };
            trans_table[139] = new int[] { 1, 2, 7, 58, 105 };
            trans_table[140] = new int[] { 2, 46, 58, 60, 106 };
            trans_table[141] = new int[] { 2, 3, 7, 58, 60, 107 };
            trans_table[142] = new int[] { 2, 3, 47, 58, 60, 108 };
            trans_table[143] = new int[] { 2, 3, 38, 58, 60, 149 };
            trans_table[144] = new int[] { 2, 3, 38, 58, 60, 150 };
            trans_table[145] = new int[] { 38, 58, 60 };
            trans_table[146] = new int[] { 38, 60, 147 };
            trans_table[147] = new int[] { 39, 60 };
            trans_table[148] = new int[] { 39, 58, 60, 149 };
            trans_table[149] = new int[] { 39, 46, 59, 150 };
            trans_table[150] = new int[] { 2, 19, 39, 58 };
            trans_table[151] = new int[] { 2, 58, 59, 68, 147 };
            trans_table[152] = new int[] { 2, 19, 58, 59, 147 };
            trans_table[153] = new int[] { 2, 19, 58, 60 };
            trans_table[154] = new int[] { 2, 19, 60, 149 };
            trans_table[155] = new int[] { 2, 19, 58, 60, 149 };
            trans_table[156] = new int[] { 2, 19, 60, 149 };
            trans_table[157] = new int[] { 2, 19, 60, 147 };
            trans_table[158] = new int[] { 2, 19, 58, 147 };
            trans_table[159] = new int[] { 2, 3, 19, 58, 147 };
            trans_table[160] = new int[] { 2, 19, 58, 147 };
            trans_table[161] = new int[] { 3, 19, 58 };
            trans_table[162] = new int[] { 2, 3, 19, 60, 148 };
            trans_table[163] = new int[] { 2, 19, 60, 147 };
            trans_table[164] = new int[] { 1, 2, 19, 58, 147 };
            trans_table[165] = new int[] { 1, 2, 19, 60, 147 };
            trans_table[166] = new int[] { 2, 3, 19, 60, 147 };
            trans_table[167] = new int[] { 1, 2, 19, 58, 147 };
            trans_table[168] = new int[] { 2, 5, 7, 68 };
            trans_table[169] = new int[] { 2, 5, 7, 68 };
            trans_table[170] = new int[] { 5, 7, 9, 68, 148 };
            trans_table[171] = new int[] { 5, 7, 9, 68, 147 };
            trans_table[172] = new int[] { 3, 9, 58, 68, 147 };
            trans_table[173] = new int[] { 3, 9, 58, 68 };
            trans_table[174] = new int[] { 3, 60, 68, 170 };
            trans_table[175] = new int[] { 0, 2, 3, 4, 60, 68, 147 };
            trans_table[176] = new int[] { 0, 2, 3, 4, 60, 68 };
            trans_table[177] = new int[] { 2, 4, 60, 68 };
            trans_table[178] = new int[] { 4, 60, 68 };
            trans_table[179] = new int[] { 3, 4, 60, 68, 109 };
            trans_table[180] = new int[] { 3, 4, 58, 68, 110 };
            trans_table[181] = new int[] { 1, 58, 68, 111, 147 };
            trans_table[182] = new int[] { 1, 3, 58, 68, 112, 147 };
            trans_table[183] = new int[] { 1, 3, 58, 68, 113, 147 };
            trans_table[184] = new int[] { 1, 58, 68, 114, 147 };
            trans_table[185] = new int[] { 1, 3, 7, 54, 58, 68, 115, 151 };
            trans_table[186] = new int[] { 1, 56, 60, 68, 116, 151 };
            trans_table[187] = new int[] { 4, 7, 19, 55, 60, 117, 151 };
            trans_table[188] = new int[] { 3, 4, 7, 55, 60, 119, 151 };
            trans_table[189] = new int[] { 3, 4, 7, 57, 60, 120, 151 };
            trans_table[190] = new int[] { 3, 4, 7, 57, 58, 121 };
            trans_table[191] = new int[] { 1, 3, 7, 55, 58, 122, 151 };
            trans_table[192] = new int[] { 1, 7, 56, 69, 123, 151 };
            trans_table[193] = new int[] { 1, 6, 124, 129, 151 };
            trans_table[194] = new int[] { 1, 6, 54, 125, 130 };
            trans_table[195] = new int[] { 1, 6, 56, 126, 148 };
            trans_table[196] = new int[] { 1, 4, 6, 48, 56, 127, 151 };
            trans_table[197] = new int[] { 1, 4, 54, 118, 151 };
            trans_table[198] = new int[] { 1, 4, 54, 128, 151 };
            trans_table[199] = new int[] { 4, 6, 7, 152 };
            trans_table[200] = new int[] { 4, 6, 7, 56 };
            trans_table[201] = new int[] { 4, 7, 54, 152 };
            trans_table[202] = new int[] { 4, 7, 54, 147 };
            trans_table[203] = new int[] { 1, 4, 151 };
            trans_table[204] = new int[] { 1, 4, 50, 57, 151 };
            trans_table[205] = new int[] { 0, 1, 4, 6, 50, 55 };
            trans_table[206] = new int[] { 1, 4, 50, 55, 151 };
            trans_table[207] = new int[] { 4, 6, 50, 55, 153 };
            trans_table[208] = new int[] { 4, 6, 56, 153 };
            trans_table[209] = new int[] { 4, 6, 54, 71, 153 };
            trans_table[210] = new int[] { 4, 6, 54, 151 };
            trans_table[211] = new int[] { 4, 6, 148, 154 };
            trans_table[212] = new int[] { 4, 56, 151, 154 };
            trans_table[213] = new int[] { 4, 147, 154 };
            trans_table[214] = new int[] { 4, 54, 147, 154 };
            trans_table[215] = new int[] { 4, 151, 154 };
            trans_table[216] = new int[] { 1, 151, 154 };
            trans_table[217] = new int[] { 1, 131, 151, 154 };
            trans_table[218] = new int[] { 1, 6, 148, 154 };
            trans_table[219] = new int[] { 1, 6, 132, 151 };
            trans_table[220] = new int[] { 2, 6, 133 };
            trans_table[221] = new int[] { 2, 6 };
            trans_table[222] = new int[] { 1, 2, 134 };
            trans_table[223] = new int[] { 1, 2, 135 };
            trans_table[224] = new int[] { 1, 2, 136 };
            trans_table[225] = new int[] { 1, 2, 137 };
            trans_table[226] = new int[] { 2, 138 };
            trans_table[227] = new int[] { 2, 139, 149 };
            trans_table[228] = new int[] { 2, 6, 140, 148 };
            trans_table[229] = new int[] { 2, 6, 58, 141, 149 };
            trans_table[230] = new int[] { 2, 6, 58, 142 };
            trans_table[231] = new int[] { 0, 2, 6, 58, 143 };
            trans_table[232] = new int[] { 2, 58, 144 };
            trans_table[233] = new int[] { 2, 9, 41, 60, 70 };
            trans_table[234] = new int[] { 2, 9, 41, 60, 70 };
            trans_table[235] = new int[] { 2, 6, 9, 42, 60, 70, 149 };
            trans_table[236] = new int[] { 2, 9, 42, 44, 60, 70, 149 };
            trans_table[237] = new int[] { 2, 6, 42, 44, 59, 149 };
            trans_table[238] = new int[] { 0, 2, 6, 42, 44, 58, 149 };
            trans_table[239] = new int[] { 0, 2, 4, 6, 43, 44, 59, 147 };
            trans_table[240] = new int[] { 0, 3, 4, 43, 44, 59, 145 };
            trans_table[241] = new int[] { 3, 4, 6, 43, 44, 60, 145 };
            trans_table[242] = new int[] { 0, 4, 6, 43, 60, 145, 148 };
            trans_table[243] = new int[] { 0, 2, 4, 58, 145, 148 };
            trans_table[244] = new int[] { 0, 4, 6, 10, 60, 145, 148 };
            trans_table[245] = new int[] { 2, 3, 6, 10, 60, 145 };
            trans_table[246] = new int[] { 2, 3, 6, 10, 58, 145 };
            trans_table[247] = new int[] { 2, 6, 10, 58, 145, 147 };
            trans_table[248] = new int[] { 2, 3, 4, 58, 145, 147, 190 };
            trans_table[249] = new int[] { 2, 4, 58, 147, 190 };
            trans_table[250] = new int[] { 2, 4, 53, 60, 147, 190 };
            trans_table[251] = new int[] { 2, 4, 6, 53, 60, 147, 190 };
            trans_table[252] = new int[] { 2, 6, 53, 58 };
            trans_table[253] = new int[] { 2, 6, 53, 60, 151 };
            trans_table[254] = new int[] { 2, 6, 53, 60 };
            trans_table[255] = new int[] { 2, 53, 58 };
            trans_table[262] = new int[] { 149 };
            trans_table[264] = new int[] { 149 };
            trans_table[266] = new int[] { 63 };
            trans_table[267] = new int[] { 63 };
            trans_table[274] = new int[] { 63 };
            trans_table[282] = new int[] { 8 };
            trans_table[283] = new int[] { 8 };
            trans_table[284] = new int[] { 8 };
            trans_table[285] = new int[] { 8 };
            trans_table[294] = new int[] { 7 };
            trans_table[295] = new int[] { 7 };
            trans_table[296] = new int[] { 7 };
            trans_table[297] = new int[] { 7 };
            trans_table[305] = new int[] { 1 };
            trans_table[306] = new int[] { 1 };
            trans_table[307] = new int[] { 1 };
            trans_table[308] = new int[] { 1 };
            trans_table[313] = new int[] { 1 };
            trans_table[314] = new int[] { 1 };
            trans_table[315] = new int[] { 1 };
            trans_table[316] = new int[] { 1 };
            trans_table[317] = new int[] { 1 };
            trans_table[318] = new int[] { 1 };
            trans_table[319] = new int[] { 1 };
            trans_table[320] = new int[] { 1 };
            trans_table[321] = new int[] { 1 };
            trans_table[324] = new int[] { 170 };
            trans_table[325] = new int[] { 8 };
            trans_table[327] = new int[] { 8 };
            trans_table[337] = new int[] { 8 };
            trans_table[344] = new int[] { 13 };
            trans_table[349] = new int[] { 8 };
            trans_table[351] = new int[] { 8 };
            trans_table[352] = new int[] { 8 };
            trans_table[365] = new int[] { 63 };
            trans_table[367] = new int[] { 62 };
            trans_table[368] = new int[] { 62 };
            trans_table[369] = new int[] { 63, 147 };
            trans_table[370] = new int[] { 63, 149 };
            trans_table[371] = new int[] { 61 };
            trans_table[375] = new int[] { 149 };
            trans_table[376] = new int[] { 149 };
            trans_table[380] = new int[] { 62 };
            trans_table[381] = new int[] { 147 };
            trans_table[383] = new int[] { 62 };
            trans_table[384] = new int[] { 63 };
            trans_table[385] = new int[] { 61 };
            trans_table[386] = new int[] { 61 };
            trans_table[387] = new int[] { 63 };
            trans_table[388] = new int[] { 63 };
            trans_table[389] = new int[] { 8, 61 };
            trans_table[390] = new int[] { 8, 61 };
            trans_table[391] = new int[] { 8, 63 };
            trans_table[392] = new int[] { 8, 63 };
            trans_table[393] = new int[] { 8, 61 };
            trans_table[394] = new int[] { 8, 61 };
            trans_table[395] = new int[] { 8 };
            trans_table[396] = new int[] { 8 };
            trans_table[401] = new int[] { 8 };
            trans_table[402] = new int[] { 8 };
            trans_table[404] = new int[] { 8 };
            trans_table[417] = new int[] { 8, 147 };
            trans_table[418] = new int[] { 8 };
            trans_table[419] = new int[] { 8 };
            trans_table[424] = new int[] { 147 };
            trans_table[425] = new int[] { 8, 147 };
            trans_table[426] = new int[] { 8 };
            trans_table[427] = new int[] { 8 };
            trans_table[428] = new int[] { 8 };
            trans_table[433] = new int[] { 147 };
            trans_table[444] = new int[] { 14 };
            trans_table[445] = new int[] { 14 };
            trans_table[446] = new int[] { 15 };
            trans_table[448] = new int[] { 16 };
            trans_table[449] = new int[] { 16 };
            trans_table[451] = new int[] { 69 };
            trans_table[452] = new int[] { 14, 69 };
            trans_table[453] = new int[] { 14, 69 };
            trans_table[454] = new int[] { 69 };
            trans_table[455] = new int[] { 14, 69 };
            trans_table[456] = new int[] { 16, 69, 152 };
            trans_table[457] = new int[] { 7, 69 };
            trans_table[458] = new int[] { 7, 16, 69 };
            trans_table[459] = new int[] { 7, 16, 69 };
            trans_table[460] = new int[] { 7 };
            trans_table[461] = new int[] { 7, 69 };
            trans_table[462] = new int[] { 7, 69 };
            trans_table[463] = new int[] { 69 };
            trans_table[464] = new int[] { 7, 69 };
            trans_table[465] = new int[] { 69 };
            trans_table[466] = new int[] { 1, 69 };
            trans_table[467] = new int[] { 1, 69 };
            trans_table[468] = new int[] { 1, 6, 69 };
            trans_table[469] = new int[] { 1, 69 };
            trans_table[470] = new int[] { 1, 69 };
            trans_table[471] = new int[] { 69 };
            trans_table[472] = new int[] { 1, 69 };
            trans_table[473] = new int[] { 1 };
            trans_table[474] = new int[] { 69 };
            trans_table[475] = new int[] { 154 };
            trans_table[476] = new int[] { 2 };
            trans_table[477] = new int[] { 2, 69 };
            trans_table[478] = new int[] { 2 };
            trans_table[479] = new int[] { 2 };
            trans_table[480] = new int[] { 2 };
            trans_table[481] = new int[] { 2 };
            trans_table[482] = new int[] { 2 };
            trans_table[483] = new int[] { 2 };
            trans_table[486] = new int[] { 151 };
            trans_table[487] = new int[] { 151 };
            trans_table[488] = new int[] { 151 };
            trans_table[492] = new int[] { 4 };
            trans_table[493] = new int[] { 4, 69 };
            trans_table[494] = new int[] { 4, 69 };
            trans_table[495] = new int[] { 69 };
            trans_table[496] = new int[] { 5, 69, 151 };
            trans_table[497] = new int[] { 69, 151 };
            trans_table[500] = new int[] { 2 };
            trans_table[504] = new int[] { 190 };
            trans_table[508] = new int[] { 3 };
            trans_table[509] = new int[] { 3 };
            trans_table[510] = new int[] { 3, 151 };
            trans_table[511] = new int[] { 3 };
            trans_table[512] = new int[] { 2 };
            trans_table[584] = new int[] { 2 };
            trans_table[586] = new int[] { 2 };
            trans_table[587] = new int[] { 2 };
            trans_table[588] = new int[] { 2 };
            trans_table[589] = new int[] { 2 };
            trans_table[590] = new int[] { 2 };
            trans_table[591] = new int[] { 2 };
            trans_table[596] = new int[] { 2 };
            trans_table[598] = new int[] { 2 };
            trans_table[615] = new int[] { 2 };
            trans_table[616] = new int[] { 2 };
            trans_table[617] = new int[] { 2 };
            trans_table[618] = new int[] { 2 };
            trans_table[619] = new int[] { 2 };
            trans_table[620] = new int[] { 2 };
            trans_table[621] = new int[] { 2 };
            trans_table[622] = new int[] { 2 };
            trans_table[623] = new int[] { 2 };
            trans_table[624] = new int[] { 2 };
            trans_table[625] = new int[] { 2 };
            trans_table[626] = new int[] { 2 };
            trans_table[627] = new int[] { 2 };
            trans_table[628] = new int[] { 2 };
            trans_table[631] = new int[] { 2 };
            trans_table[632] = new int[] { 2 };
            trans_table[633] = new int[] { 2 };
            trans_table[634] = new int[] { 2 };
            trans_table[635] = new int[] { 2 };
            trans_table[636] = new int[] { 2 };
            trans_table[645] = new int[] { 2 };
            trans_table[646] = new int[] { 2 };
            trans_table[647] = new int[] { 2 };
            trans_table[648] = new int[] { 2 };
            trans_table[649] = new int[] { 2 };
            trans_table[650] = new int[] { 2 };
            trans_table[651] = new int[] { 2 };
            trans_table[652] = new int[] { 2 };
            trans_table[666] = new int[] { 2 };
            trans_table[761] = new int[] { 2 };
            trans_table[763] = new int[] { 2 };                        
        }
    }

}
