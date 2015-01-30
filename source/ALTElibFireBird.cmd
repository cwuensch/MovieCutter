@echo off
if exist %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a (
  echo Schalte auf die NEUE FireBirdLib um!

  del %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  del %TFRoot%\API\TMS\include\libFireBird.h

  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird_neu.a libFireBird.a
  ren %TFRoot%\API\TMS\include\libFireBird_neu.h libFireBird.h

  ren %~dp0libFireBird.h ALTElibFireBird.h
  if exist %~dp0NEUElibFireBird.h ren %~dp0NEUElibFireBird.h libFireBird.h

  ren %~dp0FBLibWrapper.c ALTEFBLibWrapper.c
  echo. > %~dp0FBLibWrapper.c

) else (
  echo Schalte auf die ALTE FireBirdLib um!

  ren %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a libFireBird_neu.a
  ren %TFRoot%\API\TMS\include\libFireBird.h libFireBird_neu.h

  copy %TFRoot%\FireBirdLib\ALTElibFireBird.a %TFRoot%\gccForTMS\crosstool\lib\libFireBird.a
  copy %TFRoot%\FireBirdLib\ALTElibFireBird.h %TFRoot%\API\TMS\include\libFireBird.h

  if exist %~dp0libFireBird.h ren %~dp0libFireBird.h NEUElibFireBird.h
  ren %~dp0ALTElibFireBird.h libFireBird.h

  del %~dp0FBLibWrapper.c
  ren %~dp0ALTEFBLibWrapper.c FBLibWrapper.c
)
pause