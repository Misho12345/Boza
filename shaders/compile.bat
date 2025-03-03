@echo off
setlocal enabledelayedexpansion

rem Define directories and timestamp file
set "SHADER_DIR=%~dp0src"
set "OUTPUT_DIR=%~dp0spv"
set "TIMESTAMP_FILE=%TEMP%\boza_shader_timestamps.txt"

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

rem Build a temporary file with current shader file timestamps
set "TEMP_TIMESTAMP_FILE=%TEMP%\shader_timestamps_current.txt"
> "%TEMP_TIMESTAMP_FILE%" echo.

pushd "%SHADER_DIR%"
for %%f in (*.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss) do (
    for %%I in ("%%f") do (
        set "shader_time=%%~tI"
    )
    echo %%f !shader_time! >> "%TEMP_TIMESTAMP_FILE%"
)
popd

rem Loop through each shader file and compile if .spv is missing or if the shader file has a different timestamp
pushd "%SHADER_DIR%"
for %%f in (*.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss) do (
    set "output_file=%OUTPUT_DIR%\%%~nxf.spv"
    if not exist "!output_file!" (
        echo Compiling new shader: %%f
        "%~dp0glslc.exe" -I"%SHADER_DIR%" "%%f" -o "!output_file!"
    ) else (
        for %%I in ("%%f") do set "shader_time=%%~tI"
        for %%O in ("!output_file!") do set "output_time=%%~tO"
        if "!shader_time!" neq "!output_time!" (
            echo Recompiling changed shader: %%f
            "%~dp0glslc.exe" -I"%SHADER_DIR%" "%%f" -o "!output_file!"
        )
    )
)
popd

rem Update the stored timestamp file with the current timestamps
move /Y "%TEMP_TIMESTAMP_FILE%" "%TIMESTAMP_FILE%"

endlocal
