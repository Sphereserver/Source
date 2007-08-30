using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace UoKRUnpacker
{
    static class Program
    {
        /// <summary>
        /// Punto di ingresso principale dell'applicazione.
        /// </summary>
        [STAThread]
        static void Main()
        {
            if (CommandLine.DoCommandLine(Environment.GetCommandLineArgs()))
            {
                //Console.Title = Application.ProductName + "-" + (new Random()).Next(Int32.MaxValue);
                //setConsoleWindowVisibility(false, Console.Title);

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Form1());

                //setConsoleWindowVisibility(true, Console.Title);
            }
        }

        //public static void setConsoleWindowVisibility(bool visible, string title)
        //{
        //    // below is Brandon's code           
        //    //Sometimes System.Windows.Forms.Application.ExecutablePath works for the caption depending on the system you are running under.          
        //    IntPtr hWnd = FindWindow(null, title);

        //    if (hWnd != IntPtr.Zero)
        //    {
        //        if (!visible)
        //            //Hide the window                   
        //            ShowWindow(hWnd, 0); // 0 = SW_HIDE               
        //        else
        //            //Show window again                   
        //            ShowWindow(hWnd, 1); //1 = SW_SHOWNORMA          
        //    }
        //}

        //[DllImport("user32.dll")]
        //public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        //[DllImport("user32.dll")]
        //static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    }
}