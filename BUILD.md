# Build Instructions

This document provides step-by-step instructions for building, testing, and running the Boza project.

## Prerequisites

### Required Tools

- **CMake** 3.24 or higher
- **C++23 compatible compiler**:
   - Windows: Visual Studio 2022 (17.0+) or MSVC
   - Linux: GCC 13+ or Clang 16+
   - macOS: Xcode 15+ or Clang 16+
- **vcpkg** (package manager)
- **Vulkan SDK** (required for Debug builds)

### Vulkan SDK Installation

**Important**: The Vulkan SDK is only required for Debug builds (for validation layers and debugging tools). Release builds use Vulkan headers and loader from vcpkg and do not require the SDK.

For Debug builds:
1. Download the Vulkan SDK from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/)
2. Install the SDK for your platform
3. The installer will automatically set up the required environment variables

For Release builds, you can skip this step as all Vulkan dependencies are managed by vcpkg.

---

## vcpkg Setup

vcpkg is required to install project dependencies. Choose one of the following options:

<details>
<summary><b>Option 1: Use CLion's Built-in vcpkg (CLion Users Only)</b></summary>

CLion comes with a built-in vcpkg installation. To use it:

1. Locate CLion's vcpkg installation (typically at `C:\Users\<username>\vcpkg-clion\vcpkg` on Windows and `~/.vcpkg-clion/vcpkg` on Linux/macOS)
2. Set the `VCPKG_ROOT` environment variable to point to this directory

**Windows:**
```cmd
setx VCPKG_ROOT "C:\Users\<username>\vcpkg-clion\vcpkg"
```

**Linux/macOS:**
```bash
echo 'export VCPKG_ROOT="$HOME/.vcpkg-clion/vcpkg"' >> ~/.bashrc
source ~/.bashrc
```

Note: Replace `<username>` with your actual username.

</details>

<details>
<summary><b>Option 2: Install vcpkg Manually</b></summary>

### Windows

1. **Clone vcpkg:**
   ```cmd
   cd C:\
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ```

2. **Bootstrap vcpkg:**
   ```cmd
   .\bootstrap-vcpkg.bat
   ```

3. **Set Environment Variable:**

   Open PowerShell as Administrator and run:
   ```powershell
   [System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', [System.EnvironmentVariableTarget]::User)
   ```

   Or using Command Prompt:
   ```cmd
   setx VCPKG_ROOT "C:\vcpkg"
   ```

4. **Integrate with Visual Studio (Optional but Recommended):**
   ```cmd
   .\vcpkg integrate install
   ```
   This command integrates vcpkg with Visual Studio, making all installed packages automatically available to MSBuild projects.

5. **Restart your terminal** or IDE for the changes to take effect.

### Linux

1. **Clone vcpkg:**
   ```bash
   cd ~
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ```

2. **Bootstrap vcpkg:**
   ```bash
   ./bootstrap-vcpkg.sh
   ```

3. **Set Environment Variable:**

   Add to your shell profile (`~/.bashrc`, `~/.zshrc`, or `~/.profile`):
   ```bash
   export VCPKG_ROOT="$HOME/vcpkg"
   export PATH="$VCPKG_ROOT:$PATH"
   ```

4. **Apply changes:**
   ```bash
   source ~/.bashrc  # or your shell's profile file
   ```

### macOS

1. **Clone vcpkg:**
   ```bash
   cd ~
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ```

2. **Bootstrap vcpkg:**
   ```bash
   ./bootstrap-vcpkg.sh
   ```

3. **Set Environment Variable:**

   Add to your shell profile (`~/.zshrc` for macOS Catalina and later):
   ```bash
   export VCPKG_ROOT="$HOME/vcpkg"
   export PATH="$VCPKG_ROOT:$PATH"
   ```

4. **Apply changes:**
   ```bash
   source ~/.zshrc
   ```

</details>

---

## Building with CLion

<details>
<summary><b>CLion Build Instructions</b></summary>

1. **Open the project in CLion:**
   - File → Open → Select the project root directory

2. **Configure CMake:**
   - CLion will automatically detect the CMakePresets.json
   - Select either `Debug` or `Release` profile from the dropdown in the top toolbar
   - Wait for CMake configuration to complete

3. **Build the project:**
   - Click the hammer icon (Build) or press `Ctrl+F9` (Windows/Linux) / `Cmd+F9` (macOS)
   - Or use menu: Build → Build Project

4. **Run the project:**
   - Select the run configuration from the dropdown
   - Click the play button or press `Shift+F10` (Windows/Linux) / `Ctrl+R` (macOS)

5. **Run tests:**
   - Open the CMake tool window (View → Tool Windows → CMake)
   - Right-click on a test preset (e.g., `unit:Debug`) and select "Run"
   - Or use the CTest tool window to run individual tests

**Note:** If CLion doesn't detect your vcpkg installation:
- Go to Settings → Build, Execution, Deployment → CMake
- Add `-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake` to CMake options

</details>

---

## Building with Visual Studio

<details>
<summary><b>Visual Studio Build Instructions</b></summary>

### Prerequisites
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake component installed (included in the C++ workload)

### Steps

1. **Open the project:**
   - Launch Visual Studio 2022
   - File → Open → CMake → Select `CMakeLists.txt` from the project root

2. **Select Configuration:**
   - Use the configuration dropdown in the toolbar to select either `Debug` or `Release`

3. **Configure CMake:**
   - Visual Studio will automatically run CMake configuration
   - Wait for "CMake generation finished" in the output window

4. **Build:**
   - Build → Build All or press `Ctrl+Shift+B`
   - Or right-click on CMakeLists.txt in Solution Explorer → Build

5. **Run:**
   - Select the executable from the "Select Startup Item" dropdown
   - Click the play button or press `F5` (with debugging) / `Ctrl+F5` (without debugging)

6. **Run tests:**
   - Test → Run All Tests or press `Ctrl+R, A`
   - View results in the Test Explorer window

**Troubleshooting:**
- If vcpkg is not detected, ensure `VCPKG_ROOT` environment variable is set and restart Visual Studio
- Check CMake Settings (Project → CMake Settings) to verify the toolchain file path

</details>

---

## Building from Terminal

<details>
<summary><b>Windows (Command Prompt/PowerShell)</b></summary>

### Using CMake Presets

**Configure and Build (Debug):**
```cmd
cmake --preset Debug
cmake --build --preset Debug
```

**Configure and Build (Release):**
```cmd
cmake --preset Release
cmake --build --preset Release
```

**Build specific target (Game only):**
```cmd
cmake --build --preset game:Debug
```

### Running Tests

**Run all tests (Debug):**
```cmd
ctest --preset unit:Debug
```

**Run all tests (Release):**
```cmd
ctest --preset unit:Release
```

**Run tests with verbose output:**
```cmd
ctest --preset unit:Debug --verbose
```

**Run specific test:**
```cmd
ctest --preset unit:Debug -R boza_core_tests
```

### Manual Configuration (Alternative)

**Debug Build:**
```cmd
mkdir build\Debug
cd build\Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ../..
cmake --build . --config Debug
cd ..\..
```

**Release Build:**
```cmd
mkdir build\Release
cd build\Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ../..
cmake --build . --config Release
cd ..\..
```

### Running the Application

```cmd
# Debug
.\build\Debug\bin\boza_game.exe

# Release
.\build\Release\bin\boza_game.exe
```

</details>

<details>
<summary><b>Linux</b></summary>

### Using CMake Presets

**Configure and Build (Debug):**
```bash
cmake --preset Debug
cmake --build --preset Debug
```

**Configure and Build (Release):**
```bash
cmake --preset Release
cmake --build --preset Release
```

**Build specific target (Game only):**
```bash
cmake --build --preset game:Debug
```

### Running Tests

**Run all tests (Debug):**
```bash
ctest --preset unit:Debug
```

**Run all tests (Release):**
```bash
ctest --preset unit:Release
```

**Run tests with verbose output:**
```bash
ctest --preset unit:Debug --verbose
```

**Run specific test:**
```bash
ctest --preset unit:Debug -R boza_core_tests
```

**Run individual test executables:**
```bash
# Run core tests
./build/Debug/bin/boza_core_tests

# Run platform tests
./build/Debug/bin/boza_platform_tests

# Run RHI tests
./build/Debug/bin/boza_rhi_tests

# Run AHI tests
./build/Debug/bin/boza_ahi_tests

# Run engine tests
./build/Debug/bin/boza_engine_tests
```

### Manual Configuration (Alternative)

**Debug Build:**
```bash
mkdir -p build/Debug
cd build/Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ../..
cmake --build .
cd ../..
```

**Release Build:**
```bash
mkdir -p build/Release
cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ../..
cmake --build .
cd ../..
```

### Running the Application

```bash
# Debug
./build/Debug/bin/boza_game

# Release
./build/Release/bin/boza_game
```

</details>

<details>
<summary><b>macOS</b></summary>

### Using CMake Presets

**Configure and Build (Debug):**
```bash
cmake --preset Debug
cmake --build --preset Debug
```

**Configure and Build (Release):**
```bash
cmake --preset Release
cmake --build --preset Release
```

**Build specific target (Game only):**
```bash
cmake --build --preset game:Debug
```

### Running Tests

**Run all tests (Debug):**
```bash
ctest --preset unit:Debug
```

**Run all tests (Release):**
```bash
ctest --preset unit:Release
```

**Run tests with verbose output:**
```bash
ctest --preset unit:Debug --verbose
```

**Run specific test:**
```bash
ctest --preset unit:Debug -R boza_core_tests
```

**Run individual test executables:**
```bash
# Run core tests
./build/Debug/bin/boza_core_tests

# Run platform tests
./build/Debug/bin/boza_platform_tests

# Run RHI tests
./build/Debug/bin/boza_rhi_tests

# Run AHI tests
./build/Debug/bin/boza_ahi_tests

# Run engine tests
./build/Debug/bin/boza_engine_tests
```

### Manual Configuration (Alternative)

**Debug Build:**
```bash
mkdir -p build/Debug
cd build/Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ../..
cmake --build .
cd ../..
```

**Release Build:**
```bash
mkdir -p build/Release
cd build/Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ../..
cmake --build .
cd ../..
```

### Running the Application

```bash
# Debug
./build/Debug/bin/boza_game

# Release
./build/Release/bin/boza_game
```

</details>

---

## Testing

The project includes comprehensive unit tests for all engine components.

### Test Organization

Tests are organized by component:
- **boza_core_tests** - Core engine functionality tests
- **boza_platform_tests** - Platform abstraction layer tests
- **boza_rhi_tests** - Rendering Hardware Interface tests
- **boza_ahi_tests** - Audio Hardware Interface tests
- **boza_engine_tests** - High-level engine integration tests

### Running Tests

#### Quick Start

**Run all tests (recommended):**
```bash
# Debug
ctest --preset unit:Debug

# Release
ctest --preset unit:Release
```

#### Advanced Test Options

**Run tests in parallel:**
```bash
ctest --preset unit:Debug -j 4
```

**Run tests with detailed output:**
```bash
ctest --preset unit:Debug --verbose
```

**Run only failed tests:**
```bash
ctest --preset unit:Debug --rerun-failed
```

**Run specific test suite:**
```bash
# Run only core tests
ctest --preset unit:Debug -R boza_core_tests

# Run only RHI tests
ctest --preset unit:Debug -R boza_rhi_tests
```

**List all available tests:**
```bash
ctest --preset unit:Debug -N
```

#### Test Presets

The project provides the following test presets:

- **unit:Debug** - Run all unit tests in Debug mode with output on failure
- **unit:Release** - Run all unit tests in Release mode with output on failure
- **all:Debug** - Run all tests sequentially without stopping on failure (Debug)
- **all:Release** - Run all tests sequentially without stopping on failure (Release)

### Continuous Integration

Tests are automatically run in CI/CD pipelines. See `.github/workflows/ci.yml` for the complete CI configuration.

---

## Build Output

After a successful build, you'll find the compiled binaries in:

- **Debug builds:** `build/Debug/bin/`
- **Release builds:** `build/Release/bin/`

Libraries will be located in the corresponding `lib/` directories.

Test executables are also placed in the `bin/` directory.

---

## CMake Presets Reference

The project uses CMake presets for standardized build configurations.

### Configure Presets
- **Debug** - Debug build with symbols and assertions
- **Release** - Optimized release build

### Build Presets
- **Debug** - Build all targets (Debug)
- **Release** - Build all targets (Release)
- **game:Debug** - Build only the game executable (Debug)
- **game:Release** - Build only the game executable (Release)

### Test Presets
- **unit:Debug** - Run unit tests (Debug)
- **unit:Release** - Run unit tests (Release)
- **all:Debug** - Run all tests without stopping on failure (Debug)
- **all:Release** - Run all tests without stopping on failure (Release)

### Usage Examples

```bash
# Configure
cmake --preset Debug

# Build all
cmake --build --preset Debug

# Build game only
cmake --build --preset game:Debug

# Run tests
ctest --preset unit:Debug

# All in one
cmake --preset Debug && cmake --build --preset Debug && ctest --preset unit:Debug
```

---

## Troubleshooting

### Common Issues

**vcpkg not found:**
- Verify `VCPKG_ROOT` environment variable is set correctly
- Restart your terminal/IDE after setting the environment variable
- Run `echo %VCPKG_ROOT%` (Windows) or `echo $VCPKG_ROOT` (Linux/macOS) to verify

**CMake configuration fails:**
- Ensure CMake 3.24+ is installed: `cmake --version`
- Check that vcpkg is properly bootstrapped
- Delete the build directory and try again

**Compilation errors:**
- Verify you have a C++23 compatible compiler
- Ensure all dependencies are installed via vcpkg
- For Debug builds, verify Vulkan SDK is installed (Release builds don't need it)

**Missing Vulkan SDK (Debug builds only):**
- The Vulkan SDK is only required for Debug builds
- Release builds use Vulkan dependencies from vcpkg
- Download and install from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/)
- Restart your terminal/IDE after installation

**Tests fail to run:**
- Ensure the project is built before running tests
- Check that test executables exist in the build output directory
- Run tests with `--verbose` flag for detailed error messages

**Test discovery fails:**
- Increase CTest discovery timeout in CMakeLists.txt
- Verify GoogleTest is properly installed via vcpkg
- Check test executable dependencies

---

## Project Dependencies

The project uses the following libraries (managed by vcpkg):

- glfw3, volk, vulkan-headers, vulkan-memory-allocator
- shaderc, spirv-cross, spirv-tools
- libsndfile, openal-soft
- glm, entt, stb
- spdlog, nlohmann-json, magic-enum
- boost-pfr, boost-preprocessor
- tracy, gtest

These are automatically installed by vcpkg during the CMake configuration phase.
<br>
Due to the large number of dependencies, it may take a while to complete.