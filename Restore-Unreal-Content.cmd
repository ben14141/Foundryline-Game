@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Release\Sync-FoundrylineContent.ps1" %*
if errorlevel 1 pause
