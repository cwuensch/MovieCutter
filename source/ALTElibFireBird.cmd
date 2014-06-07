@echo off
if exist %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a (
  echo Schalte auf die NEUE FireBirdLib um!
  del %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a libFireBird.a
  ren %~dp0libFireBird.h ALTElibFireBird.h
  if exist %~dp0NEUElibFireBird.h ren %~dp0NEUElibFireBird.h libFireBird.h
) else (
  echo Schalte auf die ALTE FireBirdLib um!
  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a libFireBird_neu.a
  copy %TFRoot%\FireBirdLib\ALTElibFireBird.a %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  if exist %~dp0libFireBird.h ren %~dp0libFireBird.h NEUElibFireBird.h
  ren %~dp0ALTElibFireBird.h libFireBird.h
)
pause