using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MapMULToUOP
{
    public class GraphicTableParser
    {
        private StreamReader sr=null;

        public GraphicTableParser(String filename)
        {
            try
            {
                sr = new StreamReader(filename);
            }
            catch (Exception)
            {
                sr = null;
            }
        }

        public Dictionary<ushort, ushort> Parse()
        {
            if (sr == null)
                return null;
            Dictionary<ushort, ushort> Values = new Dictionary<ushort, ushort>();
            String line;
            while ((line = sr.ReadLine()) != null)
            {
                line = line.Replace("->", " ").Trim();
                String[] valuesplit = line.Split(new char[] {' '});
                Values.Add(Convert.ToUInt16(valuesplit[0]), Convert.ToUInt16(valuesplit[1]));
            }
            sr.Close();
            return Values;
        }

    }
}
