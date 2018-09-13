@echo off

set _task=StreamService.exe

:checkstart
SET status=1 
(TASKLIST|FIND /I "%_task%"||SET status=0) 2>nul 1>nul
::ECHO %status%
IF %status% EQU 1 (goto checkag ) ELSE (goto startsvr)

:startsvr
start %_task%

:checkag
::echo %time% StreamService.exe is running
echo Wscript.Sleep WScript.Arguments(0) > tmp\delay.vbs 
cscript //b //nologo tmp\delay.vbs 500 
goto checkstart