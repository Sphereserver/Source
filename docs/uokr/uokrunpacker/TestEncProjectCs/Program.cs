using System;
using System.IO;
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
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if ( DoCommandLine( Environment.GetCommandLineArgs() ) )
            {
                Application.Run(new Form1());
            }
        }

        static bool DoCommandLine(string[] args)
        {
            if (args.Length <= 1)
                return true;

            bool bHelp = false, bPatch = false, bInputFile = false, bOutputFile = false, bLog = false;
            string inputFile = null, outputFile = null;
            Stack<PatchFiles> patchList = new Stack<PatchFiles>();
            System.Text.StringBuilder errorOutput = new System.Text.StringBuilder();
            errorOutput.AppendLine("-- ERROR LOG --");

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i].Equals("--help", StringComparison.OrdinalIgnoreCase))
                {
                    bHelp = true;
                    break;
                }
                else if ((!bLog) && args[i].Equals("--errorlog", StringComparison.OrdinalIgnoreCase))
                {
                    bLog = true;
                }
                else if ((!bInputFile) && args[i].Equals("--input", StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        inputFile = args[i + 1];
                        bInputFile = true;
                    }
                    else
                    {
                        errorOutput.AppendLine("--input switch without argument");
                    }
                }
                else if ((!bOutputFile) && args[i].Equals("--output", StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        outputFile = args[i + 1];
                        bOutputFile = true;
                    }
                    else
                    {
                        errorOutput.AppendLine("--output switch without argument");
                    }
                }
                else if ((bInputFile) && args[i].Equals("--patch", StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        try
                        {
                            string[] patchParams = args[i + 1].Split(new char[] { ',' }, 4);
                            PatchFiles pfTemp = new PatchFiles();
                            pfTemp.file = patchParams[0];
                            pfTemp.index = Int32.Parse(patchParams[1]);
                            pfTemp.subindex = Int32.Parse(patchParams[2]);
                            pfTemp.compress = Int32.Parse(patchParams[3]);
                            patchList.Push(pfTemp);
                            bPatch = true;
                        }
                        catch 
                        {
                            errorOutput.AppendLine("--patch switch with incorrect argument (\"" + args[i + 1] + "\")");
                        }
                    }
                    else
                    {
                        errorOutput.AppendLine("--patch switch without argument");
                    }
                }
            }

            if (bHelp)
            {
                MessageBox.Show(MESSAGEBOX_HELP, "Patch Help", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else if (bPatch && bInputFile)
            {
                if (UopManager.getIstance().Load(inputFile))
                {
                    UopManager.UopPatchError upeError = UopManager.UopPatchError.Okay;
                    int iMinIndex = Int32.MaxValue, iMinSubindex = Int32.MaxValue;

                    foreach (PatchFiles pfCurrent in patchList)
                    {
                        iMinIndex = Math.Min(iMinIndex, pfCurrent.index);
                        iMinSubindex = Math.Min(iMinSubindex, pfCurrent.subindex);
                        upeError = UopManager.getIstance().Replace(pfCurrent.file, pfCurrent.index, pfCurrent.subindex, pfCurrent.compress > 0);

                        if (upeError != UopManager.UopPatchError.Okay)
                        {
                            errorOutput.AppendLine("Error (" + upeError.ToString() + ") while patching file \"" + pfCurrent.file + "\" to " + pfCurrent.index.ToString() + "," + pfCurrent.subindex.ToString() + " (" + ((pfCurrent.compress > 0) ? "UN" : "") + "compressed)");
                            break;
                        }
                    }

                    if (upeError == UopManager.UopPatchError.Okay)
                    {
                        if (!bOutputFile)
                        {
                            int iStartName = UopManager.getIstance().UopPath.LastIndexOf('\\') + 1;

                            if (iStartName != -1)
                            {
                                outputFile = UopManager.getIstance().UopPath.Substring(iStartName, UopManager.getIstance().UopPath.Length - iStartName);
                            }
                            else
                            {
                                outputFile = UopManager.getIstance().UopPath;
                            }

                            outputFile = Application.StartupPath + @"\" + "NEW-" + outputFile;
                        }

                        UopManager.getIstance().FixOffsets(iMinIndex, iMinSubindex);
                        if (!UopManager.getIstance().Write(outputFile))
                        {
                            errorOutput.AppendLine("Error while writing the new uop file (\"" + outputFile + "\")");
                        }
                    }
                }
                else
                {
                    errorOutput.AppendLine("Error while opening the uop file (\"" + UopManager.getIstance().UopPath + "\")");
                }
            }
            else
            {
                errorOutput.AppendLine("ERROR: No action defined.");
            }

            if (bLog)
            {
                try
                {
                    File.WriteAllText(Application.StartupPath + @"\error.log", errorOutput.ToString());
                }
                catch { }
            }

            return false;
        }

        private sealed class PatchFiles
        {
            public int index;
            public int subindex;
            public int compress;
            public string file;
        }

        private static readonly string MESSAGEBOX_HELP = "Command Line Help\n" +
                                                         "\n" +
                                                         "--help : Show this help" + "\n" +
                                                         "--printlog : Print error log to error.log" + "\n" +
                                                         "--input <filename> : Input uop file" + "\n" +
                                                         "--output <filename> : Output uop file (optional)" + "\n" +
                                                         "--patch <filename,index,subindex,uncompress> : Patch filename into index,subindex compressed (uncompress = 0) (you can use multiple --patch instructions)";
    }
}