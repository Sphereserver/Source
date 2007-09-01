using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace UoKRUnpacker
{
    class CommandLine
    {
        private static bool bHelp = false;
        private static bool bUnpack = false;
        private static bool bEnablelog = false;

        private static bool bInputfile = false;
        private static string sInputpath = null;

        private static bool bOutputfile = false;
        private static string sOutputpath = null;
        
        private static bool bPatch = false;
        private static Stack<PatchFiles> patchList = new Stack<PatchFiles>();

        private static bool bFullpatch = false;
        private static string sFullpatchPath = null;

        private static bool bDontfix = false;

        private sealed class PatchFiles
        {
            public int index;
            public int subindex;
            public int compress;
            public string file;
        }

        private const string CMD_HELP = "--help";
        private const string CMD_LOG = "--log-error";
        private const string CMD_INPUT = "--input";
        private const string CMD_OUTPUT = "--output";
        private const string CMD_PATCH = "--patch";
        private const string CMD_FULLPATCH = "--full-patch";
        private const string CMD_UNPACK = "--unpack";
        private const string CMD_DONTFIX = "--dont-fix";

        private static readonly string MESSAGEBOX_HELP = "Command Line Help\n" +
                                                 "\n" +
                                                 CMD_HELP + " : Show this help" + "\n" +
                                                 CMD_LOG + " : Print error log to error.log" + "\n" +
                                                 CMD_INPUT + " <filename> : Input uop file" + "\n" +
                                                 CMD_OUTPUT + " <filename> : Output uop file (optional)" + "\n" +
                                                 CMD_PATCH + " <filename,index,subindex,uncompress> : Patch filename into index,subindex compressed (uncompress = 0) (you can use multiple " + CMD_PATCH + " command)" + "\n" +
                                                 CMD_FULLPATCH + " <path> : Read from path the files to patch the uop (must be in format uopname-index_subindex.[dat|raw])"  + "\n" +
                                                 CMD_UNPACK + " : Unpack the uop file" + "\n";

        public static bool DoCommandLine(string[] args)
        {
            if (args.Length <= 1)
                return true;

            patchList.Clear();
            System.Text.StringBuilder errorOutput = new System.Text.StringBuilder();
            errorOutput.AppendLine("-- ERROR LOG --");

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i].Equals(CMD_HELP, StringComparison.OrdinalIgnoreCase))
                {
                    bHelp = true;
                    break;
                }
                else if ((!bEnablelog) && args[i].Equals(CMD_LOG, StringComparison.OrdinalIgnoreCase))
                {
                    bEnablelog = true;
                }
                else if ((!bInputfile) && args[i].Equals(CMD_INPUT, StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        sInputpath = args[i + 1];
                        sInputpath = sInputpath.Trim();

                        if (sInputpath.Length > 0)
                            bInputfile = true;
                    }
                    else
                    {
                        errorOutput.AppendLine(CMD_INPUT + " switch without argument");
                    }
                }
                else if ((!bOutputfile) && args[i].Equals(CMD_OUTPUT, StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        sOutputpath = args[i + 1];
                        sOutputpath = sOutputpath.Trim();

                        if (sOutputpath.Length > 0)
                            bOutputfile = true;
                    }
                    else
                    {
                        errorOutput.AppendLine(CMD_OUTPUT + " switch without argument");
                    }
                }
                else if ((!bUnpack) && args[i].Equals(CMD_UNPACK, StringComparison.OrdinalIgnoreCase))
                {
                    bUnpack = true;
                }
                else if ((!bDontfix) && args[i].Equals(CMD_DONTFIX, StringComparison.OrdinalIgnoreCase))
                {
                    bDontfix = true;
                }
                else if (args[i].Equals(CMD_PATCH, StringComparison.OrdinalIgnoreCase))
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
                            errorOutput.AppendLine(CMD_PATCH + " switch with incorrect argument (\"" + args[i + 1] + "\")");
                        }
                    }
                    else
                    {
                        errorOutput.AppendLine(CMD_PATCH + " switch without argument");
                    }
                }
                else if ((!bFullpatch) && args[i].Equals(CMD_FULLPATCH, StringComparison.OrdinalIgnoreCase))
                {
                    if ((i + 1) < args.Length)
                    {
                        sFullpatchPath = args[i + 1];
                        sFullpatchPath = sFullpatchPath.Trim();

                        if (sFullpatchPath.Length > 0)
                            bFullpatch = true;
                    }
                    else
                    {
                        errorOutput.AppendLine(CMD_FULLPATCH + " switch without argument");
                    }
                }
            }

            if (bHelp)
            {
                MessageBox.Show(MESSAGEBOX_HELP, "UO:KR Uop Dumper - Patch Help", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else if (bInputfile)
            {
                if (UopManager.getIstance().Load(sInputpath))
                {
                    if (bUnpack)
                    {
                        string thePath = Application.StartupPath + StaticData.UNPACK_DIR; ;
                        if (!Directory.Exists(thePath))
                            Directory.CreateDirectory(thePath);

                        if (!UopManager.getIstance().UnPack(thePath))
                        {
                            errorOutput.AppendLine("Error while unpacking the uop file to (\"" + thePath + "\")");
                        }
                    }
                    else if (bFullpatch)
                    {
                        UopManager.UopPatchError upeError = UopManager.UopPatchError.Okay;

                        if (Directory.Exists(sFullpatchPath))
                        {
                            try
                            {
                                string uopName = Path.GetFileNameWithoutExtension(UopManager.getIstance().UopPath);
                                string[] filesFound = Directory.GetFiles(sFullpatchPath, uopName + "*.*", SearchOption.TopDirectoryOnly);
                                foreach (string currentFile in filesFound)
                                {
                                    string strippedName = Path.GetFileName(currentFile);
                                    if (!strippedName.StartsWith(uopName + "-", StringComparison.OrdinalIgnoreCase))
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName  + "\".");
                                        continue;
                                    }

                                    string parseName = strippedName.Substring(uopName.Length + 1);
                                    if (parseName.IndexOf('_') == -1)
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\".");
                                        continue;
                                    }

                                    int indexA = -1, indexB = -1;
                                    if (!Int32.TryParse(parseName.Substring(0, parseName.IndexOf('_')), out indexA))
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\".");
                                        continue;
                                    }

                                    parseName = parseName.Substring(parseName.IndexOf('_') + 1);
                                    if (!Int32.TryParse(parseName.Substring(0, parseName.IndexOf('.')), out indexB))
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\".");
                                        continue;
                                    }

                                    if ((indexA == -1) || (indexB == -1))
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\".");
                                        continue;
                                    }

                                    if (parseName.EndsWith("." + StaticData.UNPACK_EXT_COMP))
                                    {
                                        upeError = UopManager.getIstance().Replace(currentFile, indexA, indexB, false);
                                    }
                                    else if (parseName.EndsWith("." + StaticData.UNPACK_EXT_UCOMP))
                                    {
                                        upeError = UopManager.getIstance().Replace(currentFile, indexA, indexB, true);
                                    }
                                    else
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\".");
                                        continue;
                                    }

                                    if (upeError != UopManager.UopPatchError.Okay)
                                    {
                                        errorOutput.AppendLine("Error while patching \"" + strippedName + "\" (" + upeError.ToString() + ").");
                                        continue;
                                    }
                                }

                                if (!bOutputfile)
                                {
                                    sOutputpath = Utility.GetPathForSave(UopManager.getIstance().UopPath);
                                }

                                if (!bDontfix)
                                {
                                    UopManager.getIstance().FixOffsets(0, 0);
                                }

                                if (!UopManager.getIstance().Write(sOutputpath))
                                {
                                    errorOutput.AppendLine("Error while writing the new uop file (\"" + sOutputpath + "\")");
                                }
                            }
                            catch
                            {
                                errorOutput.AppendLine("Error while full-patching \"" + UopManager.getIstance().UopPath + "\" from \"" + sFullpatchPath + "\".");
                            }
                        }
                        else
                        {
                            errorOutput.AppendLine("Error while opening non-accessible directory \"" + sFullpatchPath + "\"");
                        }
                    }
                    else if (bPatch)
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
                            if (!bOutputfile)
                            {
                                sOutputpath = Utility.GetPathForSave(UopManager.getIstance().UopPath);
                            }

                            if (bDontfix)
                            {
                                UopManager.getIstance().FixOffsets(iMinIndex, iMinSubindex);
                            }

                            if (!UopManager.getIstance().Write(sOutputpath))
                            {
                                errorOutput.AppendLine("Error while writing the new uop file (\"" + sOutputpath + "\")");
                            }
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

            if (bEnablelog)
            {
                try
                {
                    File.WriteAllText(Application.StartupPath + @"\error.log", errorOutput.ToString());
                }
                catch { }
            }

            return false;
        }
    }
}
