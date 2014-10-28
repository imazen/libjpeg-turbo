@echo off
rem thumbs wrapper for windows; see main file for details


rem find bash from git
rem assumes git is in [gitdir]\cmd
rem and msys in [gitdir]\bin

for %%i in (git.exe) do set gitexe=%%~$PATH:i
pushd "%gitexe%\..\..\bin"
set bashdir=%cd%
popd
set path=%bashdir%;%path%


rem copy all known env vars to bash

setlocal enableDelayedExpansion
set exports=

for %%i in (tbs_conf tbs_arch tbs_tools tbs_static_runtime) do (
  if not [!%%i!]==[] (
    set exports=!exports!export %%i=!%%i!;
  )
)

bash -c "%exports%./thumbs.sh %*"
