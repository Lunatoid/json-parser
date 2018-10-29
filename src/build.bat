@echo off

if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
  call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
)

set root=%~dp0..\
set src=%root%/src

IF NOT EXIST %root%\build\output mkdir %root%\build\output

pushd %root%\build\output
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -wd4172 -DWIN32_LEAN_AND_MEAN %src%/main.cpp -Fe..\json.exe -link -opt:ref -incremental:no
popd
