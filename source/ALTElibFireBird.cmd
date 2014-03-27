@echo off
if exist %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a (
  echo Schalte auf die NEUE FireBirdLib um!
  del %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a libFireBird.a
  ren %~dp0libFireBird.h ALTElibFireBird.h
) else (
  echo Schalte auf die ALTE FireBirdLib um!
  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a libFireBird_neu.a
  copy %TFRoot%\FireBirdLib\ALTElibFireBird.a %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  ren %~dp0ALTElibFireBird.h libFireBird.h
)
pause