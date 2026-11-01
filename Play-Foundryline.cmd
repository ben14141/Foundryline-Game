@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Release\Play-Foundryline.ps1" %*
if errorlevel 1 pause
