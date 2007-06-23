using System;

namespace SpyUO.Packets
{
	public struct Point3D
	{
		public int X;
		public int Y;
		public int Z;

		public Point3D( int x, int y, int z )
		{
			X = x;
			Y = y;
			Z = z;
		}

		public override string ToString()
		{
			return string.Format( "({0} {1}, {2})", X, Y, Z );
		}
	}
}