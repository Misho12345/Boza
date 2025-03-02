@echo off
setlocal enabledelayedexpansion

set SHADER_DIR=%~dp0src
set OUTPUT_DIR=%~dp0spv
set TIMESTAMP_FILE=%TEMP%\boza_shader_timestamps.txt

if not exist "%OUTPUT_DIR%" (mkdir "%OUTPUT_DIR%")

set TEMP_TIMESTAMP_FILE=%TEMP%\shader_timestamps_current.txt
> "%TEMP_TIMESTAMP_FILE%" echo.

pushd "%SHADER_DIR%"

for %%f in (*.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss) do (
    set "shader_file=%%f"
    for %%I in ("%%f") do set "shader_time=%%~tI"
    echo !shader_file! !shader_time! >> "%TEMP_TIMESTAMP_FILE%"
)

popd

fc /b "%TEMP_TIMESTAMP_FILE%" "%TIMESTAMP_FILE%" >nul
if errorlevel 1 (
    pushd "%SHADER_DIR%"

    for %%f in (*.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss) do (
        set "shader_file=%%f"
        set "output_file=%OUTPUT_DIR%\%%~nxf.spv"

        if not exist "!output_file!" (
            echo Compiling new shader: %%f
            "%~dp0glslc.exe" -I"%SHADER_DIR%" "%%f" -o "!output_file!"
        ) else (
            for %%I in ("%%f") do set "shader_time=%%~tI"
            for %%O in ("!output_file!") do set "output_time=%%~tI"
            if "!shader_time!" neq "!output_time!" (
                echo Recompiling changed shader: %%f
                "%~dp0glslc.exe" -I"%SHADER_DIR%" "%%f" -o "!output_file!"
            )
        )
    )

    popd

    move /Y "%TEMP_TIMESTAMP_FILE%" "%TIMESTAMP_FILE%"
) else (
    del "%TEMP_TIMESTAMP_FILE%"
)

popd
endlocal
