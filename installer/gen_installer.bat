REM @echo off
REM Espace utilisateur a remplir avant chaque generation d'installeur
REM ------------------------------------------------------
set version=1.1.0
set date=2026-07-21

REM ------------------------------------------------------

set DATA_DIR=.\packages\com.vendor.root\data
mkdir %DATA_DIR%\tmp\
setlocal enabledelayedexpansion

:: Définir le chemin du fichier XML
set "xmlFile=packages\com.vendor.root\meta\package.xml"

:: Vérifier si le fichier existe
if not exist "%xmlFile%" (
    echo Le fichier %xmlFile% n'existe pas.
    exit /b 1
)

:: Lire et modifier la version à l'aide de PowerShell
powershell -Command ^
    "(Get-Content -Path '%xmlFile%' -Raw) -replace '<Version>.*?</Version>', '<Version>%version%</Version>' | Set-Content -Path '%xmlFile%'"
:: Lire et modifier la releasedate à l'aide de PowerShell
powershell -Command ^
    "(Get-Content -Path '%xmlFile%' -Raw) -replace '<ReleaseDate>.*?</ReleaseDate>', '<ReleaseDate>%date%</ReleaseDate>' | Set-Content -Path '%xmlFile%'"

echo La version a été mise à jour avec succès.




del %DATA_DIR%\micro_brico_app.exe
copy  .\..\build\Desktop_Qt_6_11_2_MinGW_64_bit_Release\micro_brico_app.exe %DATA_DIR%\tmp\micro_brico_app.exe

call windeployqt %DATA_DIR%\tmp\micro_brico_app.exe
copy .\qsqlmysql.dll_Qt_SQL_driver_6.11.2_MinGW_64-bit\libmysql.dll %DATA_DIR%\tmp\libmysql.dll
copy .\qsqlmysql.dll_Qt_SQL_driver_6.11.2_MinGW_64-bit\libssl-3-x64.dll %DATA_DIR%\tmp\libssl-3-x64.dll
copy .\qsqlmysql.dll_Qt_SQL_driver_6.11.2_MinGW_64-bit\libcrypto-3-x64.dll %DATA_DIR%\tmp\libcrypto-3-x64.dll
xcopy /Y /s /F  .\qsqlmysql.dll_Qt_SQL_driver_6.11.2_MinGW_64-bit\sqldrivers\ %DATA_DIR%\tmp\sqldrivers\

REM del %DATA_DIR%\microBricoArch.zipq
call archivegen %DATA_DIR%\microBricoArch.zip %DATA_DIR%\tmp\* -f zip

rmdir /s /Q %DATA_DIR%\tmp\

call binarycreator.exe --offline-only -p ./packages -c config/config.xml micro_brico_installer_%version%

pause