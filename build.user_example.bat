@ECHO OFF
REM [Required for LAVFilters] MSYS2/MinGW paths:
SET "MPCHC_MSYS=C:\MSYS64"
SET "MPCHC_MINGW32=C:\MSYS64\mingw64"
SET "MPCHC_MINGW64=C:\MSYS64\mingw64"
SET "MSYSTEM=MINGW32"
SET "MSYS2_PATH_TYPE=inherit"
REM [Optional] Specify GIT location if it is not already set in %PATH%
SET "MPCHC_GIT=C:\Program Files\Git"
REM [Optional] If you plan to modify the translations, install Python 2.7 and set the variable to its path
SET "MPCHC_PYTHON=C:\Python27"
REM [Optional] If you want to customize the Windows SDK version used, set this variable
REM SET "MPCHC_WINSDK_VER=8.1"
REM SET "MPCHC_VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\"