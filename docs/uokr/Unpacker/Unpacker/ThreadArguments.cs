using System;
using System.Diagnostics;

using Unpacker.Mythic;

namespace Unpacker
{
	public class DictionaryArgs
	{
		public const int LOAD = 0;
		public const int SAVE = 1;
		public const int MERGE = 2;

		private string m_Name;
		private int m_Action;

		public string Name{ get{ return m_Name; } }
		public bool Load{ get{ return m_Action == 0; } }
		public bool Save{ get{ return m_Action == 1; } }
		public bool Merge{ get{ return m_Action == 2; } }

		public DictionaryArgs( string name, int action )
		{
			m_Name = name;
			m_Action = action;
		}
	}

	public class MythicLoadArgs
	{
		private string[] m_Names;
		private MythicPackage[] m_Result;

		public string[] Names{ get{ return m_Names; } }

		public MythicPackage[] Result
		{ 
			get{ return m_Result; } 
			set{ m_Result = value; }
		}

		public MythicLoadArgs( string[] names )
		{
			m_Names = names;
			m_Result = null;
		}
	}

	public class SpyProcessArgs
	{
		private Process m_Process;
		
		public Process Process{ get{ return m_Process; } }

		public SpyProcessArgs( Process process )
		{
			m_Process = process;
		}
	}

	public class SpyPathArgs
	{
		private string m_Path;
		
		public string Path{ get{ return m_Path; } }

		public SpyPathArgs( string path )
		{
			m_Path = path;
		}
	}

	public class SearchArgs
	{
		private string m_Before;
		private string m_After;
		private int m_From;
		private int m_To;
		private int m_Length;
		private int m_Found;
		
		public string Before{ get{ return m_Before; } }
		public string After{ get{ return m_After; } }
		public int From{ get{ return m_From; } }
		public int To{ get{ return m_To; } }
		public int Length{ get{ return m_Length; } }

		public int Found
		{ 
			get{ return m_Found; } 
			set{ m_Found = value; } 
		}

		public SearchArgs( string before, string after, int from, int to, int length )
		{
			m_Before = before;
			m_After = after;
			m_From = from;
			m_To = to;
			m_Length = length;
		}
	}

	public class UnpackArgs
	{
		public const int PACKAGE = 0;
		public const int BLOCK = 1;
		public const int FILE = 2;

		private static string m_Path;
		
		public static string Path
		{ 
			get{ return m_Path; } 
			set{ m_Path = value; }
		}

		private int m_Action;
		public bool IsPackage{ get{ return m_Action == 0; } }
		public bool IsBlock{ get{ return m_Action == 1; } }
		public bool IsFile{ get{ return m_Action == 2; } }

		private MythicPackage m_Package;
		private MythicPackageBlock m_Block;
		private MythicPackageIndex m_File;

		public MythicPackage Package{ get{ return m_Package; } }
		public MythicPackageBlock Block{ get{ return m_Block; } }
		public MythicPackageIndex File{ get{ return m_File; } }

		public UnpackArgs( MythicPackage p )
		{
			m_Action = PACKAGE;
			m_Package = p;
		}

		public UnpackArgs( MythicPackage p, MythicPackageBlock b )
		{
			m_Action = BLOCK;
			m_Package = p;
			m_Block = b;
		}

		public UnpackArgs( MythicPackage p, MythicPackageBlock b, MythicPackageIndex idx )
		{
			m_Action = FILE;
			m_Package = p;
			m_Block = b;
			m_File = idx;
		}
	}
}
