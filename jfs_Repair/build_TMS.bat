@echo off
cd /d %~dp0%
set TFROOT=C:\sw\prgm\Topfield
set PATH=%TFROOT%\gccForTMS\crosstool\bin;C:\sw\OS\cygwin\bin
bash -i -c make
