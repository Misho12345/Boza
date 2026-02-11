# Build Instructions

This document provides step-by-step instructions for building, testing, and running the Boza project.

## Prerequisites

> **Note:** The project currently can be compiled only on Windows using the MSVC toolchain, because Clang and GCC module support is not good enough yet.

### Required Tools

- **Latest possible version of Visual Studio with the features for Desktop development with C++ installed**
- **vcpkg**
- **Vulkan SDK** (required for Debug builds)

### Vulkan SDK Installation

> **Note**: The Vulkan SDK is only required for Debug builds (for validation layers and debugging tools). For Release builds, you can skip this step as all other Vulkan dependencies are managed by vcpkg.

1. Download the Vulkan SDK from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/)
2. Install the SDK for your platform
3. The installer will automatically set up the required environment variables


---

## vcpkg Setup

vcpkg is required to install project dependencies. Choose one of the following options:

<details>
<summary><b>Option 1: Use CLion's Built-in vcpkg</b></summary>

CLion comes with a built-in vcpkg installation. To use it:

1. Locate CLion's vcpkg installation (typically at `C:\Users\<username>\vcpkg-clion\vcpkg`)
2. Create a `VCPKG_ROOT` environment variable that points to this directory

```cmd
setx VCPKG_ROOT "C:\Users\<username>\vcpkg-clion\vcpkg"
```


</details>

<details>
<summary><b>Option 2: Install vcpkg Manually</b></summary>

1. **Clone vcpkg:**
   ```cmd
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ```

2. **Bootstrap vcpkg:**
   ```cmd
   .\bootstrap-vcpkg.bat
   ```

3. **Set Environment Variable:**
   ```cmd
   setx VCPKG_ROOT "<the-path-to-the-cloned-vcpkg>"
   ```
4. **Restart your terminal** or IDE for the changes to take effect.

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

3. **Build and run the project:**

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

4. **Build and run the project**

**Troubleshooting:**
- If vcpkg is not detected, ensure `VCPKG_ROOT` environment variable is set and restart Visual Studio
- Check CMake Settings (Project → CMake Settings) to verify the toolchain file path

</details>

---

## Building from Terminal

<details>
<summary><b>Build instructions (Command Prompt/PowerShell)</b></summary>

> **Note:** Make sure you're running the following commands in a Developer PowerShell or Command Prompt so all MSVC-related tools needed for the build are accessible.

**Configure, Build and Run (Debug):**
```cmd
cmake --preset Debug
cmake --build --preset Debug
cd build\Debug\bin
.\whatever.exe
```

**Configure, Build and Run (Release):**
```cmd
cmake --preset Release
cmake --build --preset Release
cd build\Release\bin
.\whatever.exe
```
</details>

---

## Project Dependencies

### The project uses the following libraries:

| Library                   | Description                                                    |
|---------------------------|----------------------------------------------------------------|
| `glfw3`                   | Window creation and input handling                             |
| `volk`                    | Meta-loader for Vulkan function pointers                       |
| `vulkan-headers`          | Official Vulkan API headers and registry files                 |
| `vulkan-memory-allocator` | High-level GPU memory management for Vulkan                    |
| `shaderc`                 | GLSL/HLSL to SPIR-V compiler frontend                          |
| `spirv-cross`             | Cross-compile SPIR-V to high-level shading languages           |
| `spirv-tools`             | SPIR-V validation, optimization, and inspection tools          |
| `glm`                     | Math library with structs and utilities commonly used by games |
| `flecs`                   | Fast ECS framework with rich runtime features                  |
| `stb`                     | Single-file utilities for images and many other                |
| `spdlog`                  | Fast logging library                                           |
| `gtl`                     | Generic template utilities and helpers                         |
| `nlohmann-json`           | JSON serialization library                                     |
| `mimalloc`                | High-performance memory allocator                              |


> **Note**: Due to the large number of dependencies, it may take a while to complete.