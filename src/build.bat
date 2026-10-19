@echo off
setlocal
echo ==========================================
echo Compilando Algoritmo Genetico Hibrido FTTx
echo ==========================================

if exist "C:\Program Files\CodeBlocks\MinGW\bin" (
    set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"
)

where gcc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    gcc -O3 ag_ftth_hibrido.c -o ag_ftth_hibrido.exe -lm
    if %ERRORLEVEL% equ 0 (
        echo [SUCESSO] Executavel gerado com sucesso: ag_ftth_hibrido.exe
        exit /b 0
    ) else (
        echo [ERRO] Falha durante a compilacao.
        exit /b %ERRORLEVEL%
    )
) else (
    echo [ERRO] Compilador GCC nao encontrado no sistema.
    exit /b 1
)
