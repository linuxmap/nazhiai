::@echo off

set _task=wscript.exe

:checkstart
SET status=1 
(TASKLIST|FIND /I "%_task%"||SET status=0) 2>nul 1>nul
::ECHO %status%
IF %status% EQU 1 (goto stopdamon ) ELSE (goto nullok)

:stopdamon
taskkill /F /T /IM %_task%

:nullok
