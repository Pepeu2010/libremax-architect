@echo off
echo LibreMax Architect - Teste do notebook
echo Feche outros programas. O teste abre o editor e cria tres imagens pequenas.
echo Aguarde ate aparecer o resultado. Seus projetos ficam preservados.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0share\libremax\scripts\benchmark-windows.ps1" -Application "%~dp0bin\libremax-architect.exe"
if errorlevel 1 (
  echo O teste encontrou uma falha. Os relatorios foram preservados.
)
pause
