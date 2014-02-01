@echo off
set TFROOT=C:\sw\prgm\Topfield
set PATH=%TFROOT%\gccForTMS\crosstool\bin;C:\sw\OS\cygwin\bin
bash -i -c make
move *.tap ..
