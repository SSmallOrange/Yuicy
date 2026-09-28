@echo off
setlocal
cd /d "%~dp0"

rem premake5 is downloaded on first use (pinned version + SHA256), see premake\premake.ps1
call :premake vs2022 || goto :fail
rem Generate compile_commands.json for clangd / LSP tools
call :premake export-compile-commands || goto :fail

pause
exit /b 0

:premake
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0premake\premake.ps1" %*
exit /b %errorlevel%

:fail
echo.
echo Project generation failed.
pause
exit /b 1
