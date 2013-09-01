@echo off
"%~dp0\ia32\bin\pin.exe" -p32 "%~dp0\ia32\bin\pin.exe" -follow_execv -t "%~dp0\x86-windows-ts_pin.dll"   -short_name %* 

