using System;
using System.Collections.Generic;
using System.Threading;
using System.Windows.Forms;

namespace UoKRLoader
{
    static class Program
    {
        /// <summary>
        /// Punto di ingresso principale dell'applicazione.
        /// </summary>
        [STAThread]
        static void Main()
        {
            bool bIsRunning;
            Mutex mRunner = new Mutex(true, Application.ProductName, out bIsRunning);
            if (!bIsRunning)
            {
                MessageBox.Show("Another instance is already running.", Application.ProductName + " Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());

            GC.KeepAlive(mRunner);
        }
    }
}