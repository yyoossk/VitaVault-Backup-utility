@echo off
title VitaVault - Download Backup from PS Vita
setlocal enabledelayedexpansion

set VITA_IP=192.168.1.18
set FTP_PORT=1337
set LOCAL_DIR=C:\VitaVault_Backups
set REMOTE_DIR=/ux0:data/VitaVault
set USB_REL_PATH=data\VitaVault

:MENU
cls
echo ============================================
echo    VitaVault - Download Backup from PS Vita
echo ============================================
echo.
echo Vita IP : %VITA_IP%
echo Port    : %FTP_PORT%
echo Remote  : %REMOTE_DIR%
echo USB path: %USB_REL_PATH%  (on Vita drive letter)
echo Save to : %LOCAL_DIR%
echo.
echo --- FTP (Wi-Fi) ---
echo 1) Download ALL backups
echo 2) Download today's backup only
echo 3) Download specific backup (enter name)
echo 4) Download with Windows FTP
echo.
echo --- USB (Mass Storage) ---
echo 5) Copy backup from USB drive
echo 6) Copy ALL backups from USB drive
echo.
echo --- Settings ---
echo 7) Change Vita IP
echo 8) Change remote folder (Vita path)
echo 9) Change local folder (PC path)
echo A) Change USB relative path
echo 0) Exit
echo.
set /p CHOICE=Choose: 

if /i "%CHOICE%"=="1" goto ALL
if /i "%CHOICE%"=="2" goto TODAY
if /i "%CHOICE%"=="3" goto SPECIFIC
if /i "%CHOICE%"=="4" goto FTPMAN
if /i "%CHOICE%"=="5" goto USBCOPY
if /i "%CHOICE%"=="6" goto USBALL
if /i "%CHOICE%"=="7" goto IP
if /i "%CHOICE%"=="8" goto REMOTE
if /i "%CHOICE%"=="9" goto LOCAL
if /i "%CHOICE%"=="A" goto USBPATH
if /i "%CHOICE%"=="a" goto USBPATH
if "%CHOICE%"=="0" exit /b
goto MENU

:IP
cls
echo.
set /p VITA_IP=Enter Vita IP: 
goto MENU

:REMOTE
cls
echo.
echo Current remote folder: %REMOTE_DIR%
echo Enter the remote backup folder path (e.g. /ux0:data/VitaVault)
echo If you changed destination on Vita, enter it here.
echo.
set /p REMOTE_DIR=Remote path: 
call :SYNC_USB_PATH
goto MENU

:USBPATH
cls
echo.
echo Current USB relative path: %USB_REL_PATH%
echo This is the folder on the Vita USB drive after Mass Storage mode.
echo Example for ux0:data/VitaVault -^> data\VitaVault
echo.
set /p USB_REL_PATH=USB relative path: 
if "%USB_REL_PATH%"=="" goto MENU
goto MENU

:SYNC_USB_PATH
set USB_REL_PATH=%REMOTE_DIR:/=%
set USB_REL_PATH=%USB_REL_PATH:ux0=%
set USB_REL_PATH=%USB_REL_PATH:uma0=%
if "%USB_REL_PATH:~0,1%"=="\" set USB_REL_PATH=%USB_REL_PATH:~1%
if "%USB_REL_PATH:~0,1%"=="/" set USB_REL_PATH=%USB_REL_PATH:~1%
exit /b

:LOCAL
cls
echo.
echo Current local folder: %LOCAL_DIR%
echo Enter the local folder path on your PC.
echo You can use any drive, e.g.:
echo   D:\VitaVault_Backups
echo   E:\Backup
echo   F:\
echo.
set /p LOCAL_DIR=Local path: 
if "%LOCAL_DIR%"=="" goto MENU
goto MENU

:USBCOPY
cls
echo ============================================
echo    Copy backup from USB Mass Storage
echo ============================================
echo.
echo On Vita: Settings -^> USB Mass Storage, or press [] after backup.
echo Connect USB cable. Open This PC and note the Vita drive letter.
echo.
set /p VITA_DRIVE=Drive letter (e.g. E): 
if "%VITA_DRIVE%"=="" goto MENU
set VITA_DRIVE=%VITA_DRIVE::=%
echo.
echo Enter backup folder name (timestamp folder), e.g. 2026-05-31_18-00-00
set /p BACKUP_NAME=Folder name: 
if "%BACKUP_NAME%"=="" goto MENU
if not exist "%VITA_DRIVE%:\%USB_REL_PATH%\%BACKUP_NAME%" (
    echo.
    echo Folder not found: %VITA_DRIVE%:\%USB_REL_PATH%\%BACKUP_NAME%
    echo Check drive letter and folder name.
    pause
    goto MENU
)
if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"
echo.
echo Copying to %LOCAL_DIR%\%BACKUP_NAME% ...
xcopy /E /I /H /Y "%VITA_DRIVE%:\%USB_REL_PATH%\%BACKUP_NAME%" "%LOCAL_DIR%\%BACKUP_NAME%\"
echo.
echo Done. Exit code: %ERRORLEVEL%
pause
goto MENU

:USBALL
cls
echo ============================================
echo    Copy ALL backups from USB Mass Storage
echo ============================================
echo.
set /p VITA_DRIVE=Drive letter (e.g. E): 
if "%VITA_DRIVE%"=="" goto MENU
set VITA_DRIVE=%VITA_DRIVE::=%
if not exist "%VITA_DRIVE%:\%USB_REL_PATH%" (
    echo.
    echo Path not found: %VITA_DRIVE%:\%USB_REL_PATH%
    pause
    goto MENU
)
if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"
echo.
echo Copying backup folders from USB (skips module/logs) ...
for /d %%D in ("%VITA_DRIVE%:\%USB_REL_PATH%\*") do (
    set "FOLDER=%%~nxD"
    echo !FOLDER! | findstr /R "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]" >nul
    if !ERRORLEVEL! equ 0 (
        echo Copying !FOLDER! ...
        xcopy /E /I /H /Y "%%D" "%LOCAL_DIR%\!FOLDER!\"
    )
)
echo.
echo Done.
pause
goto MENU

:TODAY
cls
echo ============================================
echo    Step 1: Check wget
echo ============================================
where wget >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo wget not found.
    echo Download from: https://eternallybored.org/misc/wget/
    pause
    goto MENU
)
echo wget found!
echo.

for /f %%i in ('powershell -Command "Get-Date -Format yyyy-MM-dd" 2^>nul') do set TODAY=%%i
if "%TODAY%"=="" (
    for /f "tokens=1-3 delims=/" %%a in ("%DATE%") do (
        set DAY=%%a
        set MONTH=%%b
        set YEAR=%%c
    )
    set TODAY=!YEAR!-!MONTH!-!DAY!
)

echo Today: %TODAY%
echo Looking for today's backup on Vita...
echo.

wget -q -O "%TEMP%\vita_list.txt" --timeout=10 --ftp-user=anonymous --ftp-password="" "ftp://%VITA_IP%:%FTP_PORT%%REMOTE_DIR%/" 2>nul

set LATEST=
for /f "tokens=*" %%a in ('type "%TEMP%\vita_list.txt" 2^>nul ^| findstr /R /C:"%TODAY%"') do set LATEST=%%a

if "%LATEST%"=="" (
    echo No backup found for today (%TODAY%).
    echo.
    type "%TEMP%\vita_list.txt" 2>nul
    echo.
    pause
    goto MENU
)

for /f "tokens=*" %%a in ("%LATEST%") do set LATEST=%%a
for %%b in (%LATEST%) do set LATEST=%%b

echo Found: %LATEST%
if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"

cls
echo ============================================
echo    Downloading: %LATEST%
echo ============================================
echo.
wget -r -nH --no-parent --progress=bar:force --timeout=15 --tries=2 --ftp-user=anonymous --ftp-password="" "ftp://%VITA_IP%:%FTP_PORT%%REMOTE_DIR%/%LATEST%/" -P "%LOCAL_DIR%"

echo.
echo Exit code: %ERRORLEVEL%
dir "%LOCAL_DIR%" /b 2>nul
echo.
pause
goto MENU

:SPECIFIC
cls
echo ============================================
echo    Download specific backup
echo ============================================
echo.

where wget >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo wget not found.
    pause
    goto MENU
)
echo wget found!
echo.
echo Enter backup folder name (e.g. 2026-05-29_18-00-00)
echo.
set /p LATEST=Name: 
if "%LATEST%"=="" goto MENU

if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"

cls
echo ============================================
echo    Downloading: %LATEST%
echo ============================================
echo.
wget -r -nH --no-parent --progress=bar:force --timeout=15 --tries=2 --ftp-user=anonymous --ftp-password="" "ftp://%VITA_IP%:%FTP_PORT%%REMOTE_DIR%/%LATEST%/" -P "%LOCAL_DIR%"

echo.
echo Exit code: %ERRORLEVEL%
dir "%LOCAL_DIR%" /b 2>nul
echo.
pause
goto MENU

:ALL
cls
echo ============================================
echo    Download ALL backups
echo ============================================
echo.

where wget >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo wget not found.
    pause
    goto MENU
)
echo wget found!

if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"

cls
echo ============================================
echo    Downloading ALL backups from %REMOTE_DIR%
echo ============================================
echo.
wget -r -nH --no-parent --progress=bar:force --timeout=15 --tries=2 --ftp-user=anonymous --ftp-password="" "ftp://%VITA_IP%:%FTP_PORT%%REMOTE_DIR%/" -P "%LOCAL_DIR%"

echo.
echo Exit code: %ERRORLEVEL%
dir "%LOCAL_DIR%" /b 2>nul
echo.
pause
goto MENU

:FTPMAN
cls
echo ============================================
echo    Windows FTP Client
echo ============================================
echo.
echo Commands:
echo   dir                         - list files
echo   cd %REMOTE_DIR%            - go to backups folder
echo   dir                         - list backups
echo   cd BACKUPNAME              - enter a backup
echo   binary                      - binary mode
echo   prompt                      - disable prompts
echo   mget *                      - download all
echo   quit                        - exit
echo.
pause
if not exist "%LOCAL_DIR%" mkdir "%LOCAL_DIR%"
(
echo open %VITA_IP% %FTP_PORT%
echo anonymous
echo anonymous
echo binary
echo prompt
echo lcd "%LOCAL_DIR%"
echo cd %REMOTE_DIR%
echo dir
) > "%TEMP%\ftpscript.txt"
start ftp -s:"%TEMP%\ftpscript.txt"
goto MENU
