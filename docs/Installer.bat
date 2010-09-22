@Echo On
ECHO Sphere Core File Creator >> InstallerLog.txt
ECHO Created by Khaos. >> InstallerLog.txt
ECHO Idea was derived from Sub-Zero's version. >> InstallerLog.txt
ECHO Copyrighted by Menasoft >> InstallerLog.txt
ECHO Version: 56 series >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Sphere Core File Creator Initiated:- >> InstallerLog.txt
ECHO Checking for Missing Base Files and Folders to Run Sphere >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for accounts folder:- >> InstallerLog.txt
IF NOT EXIST accounts (
MD accounts
ECHO Created the Accounts Folder. >> InstallerLog.txt
) ELSE ( 
ECHO The Folder Already Existed In Your Sphere Folder >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for sphereacct.scp:- >> InstallerLog.txt
IF NOT EXIST accounts\sphereacct.scp (
ECHO [EOF] > accounts\sphereacct.scp
ECHO Created sphereacct.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Accounts Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for sphereaccu.scp:- >> InstallerLog.txt
IF NOT EXIST accounts\sphereaccu.scp (
ECHO [Admin] >> accounts\sphereaccu.scp
ECHO PLEVEL=7 >> accounts\sphereaccu.scp
ECHO PRIV=048 >> accounts\sphereaccu.scp
ECHO PASSWORD=Admin >> accounts\sphereaccu.scp
ECHO [EOF] >> accounts\sphereaccu.scp
ECHO Created sphereaccu.scp >> InstallerLog.txt
ECHO **An Owner account was created as well.  Login: Owner Password: Pass** >> InstallerLog.txt
ECHO **You should change the password as soon as possible.** >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Accounts Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a Check for Logs Folder:- >> InstallerLog.txt
IF NOT EXIST logs (
MD logs
ECHO Created the Logs Folder. >> InstallerLog.txt
) ELSE (
ECHO The Folder Already Existed In Your Sphere Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a Check for Muls Folder:- >> InstallerLog.txt
IF NOT EXIST muls (
MD muls
ECHO Created Muls Folder. >> InstallerLog.txt
) ELSE (
ECHO The Folder Already Existed In Your Sphere Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a Check for Save Folder:- >> InstallerLog.txt
IF NOT EXIST save (
MD save
ECHO Created the Save Folder >> InstallerLog.txt
) ELSE (
ECHO The Folder Already Existed In Your Sphere Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for spherechars.scp:- >> InstallerLog.txt
IF NOT EXIST save\spherechars.scp (
ECHO [EOF] > save\spherechars.scp
ECHO Created spherechars.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Save Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a Check for spheredata.scp:- >> InstallerLog.txt
IF NOT EXIST save\spheredata.scp (
ECHO [EOF] > save\spheredata.scp
ECHO Created spheredata.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Save Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for spheremultis.scp:- >> InstallerLog.txt
IF NOT EXIST save\spheremultis.scp (
ECHO [EOF] > save\spheremultis.scp
ECHO Created spheremultis.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Save Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for spherestatics.scp:- >> InstallerLog.txt
IF NOT EXIST save\spherestatics.scp (
ECHO [EOF] > save\spherestatics.scp
ECHO Created spherestatics.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Save Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Performing a check for spherechars.scp:- >> InstallerLog.txt
IF NOT EXIST save\sphereworld.scp (
ECHO [EOF] > save\sphereworld.scp
ECHO Created sphereworld.scp >> InstallerLog.txt
) ELSE (
ECHO The File Already Existed In Your Save Folder. >> InstallerLog.txt
)
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Sphere Mul Dependencies:- >> InstallerLog.txt
ECHO The following files are needed if you do not have Ultima Online installed. >> InstallerLog.txt
ECHO These files should be placed in the "Muls" folder. >> InstallerLog.txt
ECHO Sphere will need the following MUL files. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO multi.mul >> InstallerLog.txt
ECHO multi.idx >> InstallerLog.txt
ECHO map0.mul >> InstallerLog.txt
ECHO map1.mul >> InstallerLog.txt
ECHO map2.mul >> InstallerLog.txt
ECHO map3.mul >> InstallerLog.txt
ECHO map4.mul >> InstallerLog.txt
ECHO map5.mul >> InstallerLog.txt
ECHO tiledata.mul >> InstallerLog.txt
ECHO staidx0.mul >> InstallerLog.txt
ECHO staidx1.mul >> InstallerLog.txt
ECHO staidx2.mul >> InstallerLog.txt
ECHO staidx3.mul >> InstallerLog.txt
ECHO staidx4.mul >> InstallerLog.txt
ECHO staidx5.mul >> InstallerLog.txt
ECHO statics0.mul >> InstallerLog.txt
ECHO statics1.mul >> InstallerLog.txt
ECHO statics2.mul >> InstallerLog.txt
ECHO statics3.mul >> InstallerLog.txt
ECHO statics4.mul >> InstallerLog.txt
ECHO statics5.mul >> InstallerLog.txt
ECHO verdata.mul (If you have one) >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO Sphere will need the following txt files if you use custom housing in addition to the base. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO doors.txt >> InstallerLog.txt
ECHO misc.txt >> InstallerLog.txt
ECHO floors.txt >> InstallerLog.txt
ECHO teleports.txt >> InstallerLog.txt
ECHO roof.txt >> InstallerLog.txt
ECHO walls.txt >> InstallerLog.txt
ECHO stairs.txt >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO Sphere will need the following MUL files if you use mapdiffs in addition to the base. Basically if you use custom map and static files. They will also be needed. These are the normal edited files. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO stadif0.mul >> InstallerLog.txt
ECHO stadif1.mul >> InstallerLog.txt
ECHO stadif2.mul >> InstallerLog.txt
ECHO stadifi0.mul >> InstallerLog.txt
ECHO stadifi1.mul >> InstallerLog.txt
ECHO stadifi2.mul >> InstallerLog.txt
ECHO stadifl0.mul >> InstallerLog.txt
ECHO stadifl1.mul >> InstallerLog.txt
ECHO stadifl2.mul >> InstallerLog.txt
ECHO mapdif0.mul >> InstallerLog.txt
ECHO mapdif1.mul >> InstallerLog.txt
ECHO mapdif2.mul >> InstallerLog.txt
ECHO mapdifl0.mul >> InstallerLog.txt
ECHO mapdifl1.mul >> InstallerLog.txt
ECHO mapdifl2.mul >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO. >> InstallerLog.txt

ECHO -:Additional Notes:- >> InstallerLog.txt
ECHO Do not forget to add AGREE=1 in your sphere.ini >> InstallerLog.txt
ECHO This should be placed somewhere under the [SPHERE] block, but above anything to do with file locations. >> InstallerLog.txt
ECHO To new users, do not forget to set your flags for additional content beyond UO:R. >> InstallerLog.txt
ECHO This is located under the Server Mechanics section in your sphere.ini file. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO Most of the information you might need is explained within the sphere.ini, revisions.txt, the manual.txt >> InstallerLog.txt
ECHO or on the sphereserver.net forums. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO If you need help beyond this, please post on the forums. >> InstallerLog.txt
ECHO Thank you for downloading Sphere. >> InstallerLog.txt
ECHO. >> InstallerLog.txt
ECHO WARNING: DO NOT FORGET ABOUT THIS >> InstallerLog.txt
ECHO Please do not forget, if an accounts file was set up, there will be an owner account made there. >> InstallerLog.txt
ECHO We advise in your best interest to edit the account name and the password. >> InstallerLog.txt

REM A log was created. Please refer to it for all information regarding your install. Also if account files were set up, there was an account made with a password which you will need to edit as well as the account name. Thank you again for downloading and using Sphere.

Pause