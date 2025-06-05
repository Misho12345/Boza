# Boza 🎮

A modern C++23 game engine built with Vulkan, designed for high-performance graphics and flexible entity-component-system architecture.

## 🛠️ Getting Started

### Prerequisites

* **C++23 compatible compiler**
  - MSVC 2022 (17.3+) on Windows
  - Clang 15+ on Linux/macOS
  - GCC 14+ on Linux
* **[CMake](https://cmake.org/)** >= 3.20
* **[vcpkg](https://vcpkg.io/)** for dependency management
* **Vulkan SDK** - Download from [LunarG Vulkan SDK](https://vulkan.lunarg.com/)

---

## 📦 Dependencies & vcpkg Setup

Boza uses **vcpkg manifest mode** to declare dependencies in `vcpkg.json`, but requires **manual installation** of packages before building.

### Why Manual Installation?

While vcpkg manifest mode automatically lists our dependencies, this project uses traditional `find_package()` calls in CMake rather than CMake's integrated manifest support. This means dependencies must be pre-installed in your vcpkg triplet.

### Install vcpkg

If you don't have vcpkg installed:

```bash
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh    # Linux/macOS
# or
.\bootstrap-vcpkg.bat   # Windows
```

**Set environment variable (recommended):**

```bash
# Linux/macOS
export VCPKG_ROOT="$(pwd)"

# Windows (Command Prompt)
set VCPKG_ROOT=%CD%

# Windows (PowerShell)
$env:VCPKG_ROOT = $PWD
```

---

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/Misho12345/Boza.git
cd Boza
```

### 2. Install Dependencies

The project's `vcpkg.json` declares all required dependencies:

```bash
# Install all dependencies for your platform
vcpkg install --triplet x64-windows     # Windows (MSVC)
vcpkg install --triplet x64-linux       # Linux
vcpkg install --triplet x64-osx         # macOS
```

**Available triplets:**
- `x64-windows` - Windows with MSVC
- `x64-windows-static` - Windows with static linking
- `x64-linux` - Linux with GCC/Clang
- `x64-osx` - macOS
- `arm64-osx` - Apple Silicon

### 3. Configure with CMake

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Or without environment variable:
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 4. Build the Project

```bash
cmake --build build --config Release
```

### 5. Run the Demo

```bash
# Linux/macOS
cd build
./BozaDemo

# Windows
cd build\Release
BozaDemo.exe
```

### Core Dependencies

- **Vulkan**: Modern graphics API
- **GLFW**: Window management and input
- **GLM**: Mathematics library
- **EnTT**: Entity-Component-System
- **VMA**: Vulkan Memory Allocator

### Audio & Assets (not yet used)

- **OpenAL Soft**: 3D audio
- **libsndfile**: Audio file loading
- **STB**: Image loading utilities

### Utilities

- **spdlog**: Fast logging
- **TaskFlow**: Parallel task execution
- **magic-enum**: Enum reflection
- **Boost.PFR**: Struct reflection

---

## 🔧 Build Configuration

### Debug Build

```bash
cmake --build build --config Debug
```

### Release Build

```bash
cmake --build build --config Release
```

### Custom Triplet

If you need a specific configuration:

```bash
# Install with custom triplet
vcpkg install --triplet x64-windows-static

# Configure CMake with triplet
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

---

## 🐛 Troubleshooting

### Common Issues

**CMake can't find vcpkg dependencies:**
- Ensure you ran `vcpkg install` with the correct triplet
- Verify `CMAKE_TOOLCHAIN_FILE` points to vcpkg's CMake toolchain
- Check that `VCPKG_ROOT` environment variable is set

**Vulkan not found:**
- Install the Vulkan SDK from LunarG
- Ensure `VULKAN_SDK` environment variable is set
- On Linux, install vulkan development packages: `sudo apt install vulkan-tools libvulkan-dev`

**Build fails with C++23 errors:**
- Update your compiler to support C++23
- On older systems, you may need to modify `CMakeLists.txt` to use C++20

**vcpkg install takes too long:**
- Use binary caching: `vcpkg install --binarysource=clear;x-azurl,<url>`
- Consider using `--triplet x64-windows-static` for faster linking

### Platform-Specific Notes

**Windows:**
- Use Visual Studio 2022 or later
- Ensure Windows SDK is installed
- Consider using vcpkg's Visual Studio integration

**Linux:**
- Install build essentials: `sudo apt install build-essential`
- Install Vulkan: `sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev`
- May need to install additional GL libraries

**macOS:**
- Install Xcode command line tools
- Vulkan support via MoltenVK (included in Vulkan SDK)
- Use Homebrew for additional dependencies if needed

---

> [!NOTE]
> You can also use CLion as a cross-platform IDE that has built-in integration with vcpkg and CMake. It would make compilation easy and fast.

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
