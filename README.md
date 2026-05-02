# Memory Allocation Simulator

A C++ educational project that simulates dynamic memory allocation using **Segmentation**. This tool visualizes how an operating system manages memory, allocates segments for active processes, and merges free spaces (holes) upon deallocation.

The simulator features both a standard **console interface** and an interactive, real-time **graphical user interface (GUI)** built with Dear ImGui.

## Features

- **Dynamic Memory Setup:** Define total memory size and inject custom initial free holes.
- **Process Segmentation:** Allocate processes that require multiple memory segments of varying sizes (e.g., Code: 50, Data: 200, Stack: 100).
- **Allocation Algorithms:**
  - **First-Fit:** Allocates the first available hole that is large enough for the segment.
  - **Best-Fit:** Allocates the smallest available hole that is large enough, minimizing leftover wasted space.
- **Dynamic Deallocation:** Free up processes and automatically merge adjacent free memory holes.
- **Error Handling:** Automatically rejects and reports processes if one or more of their segments cannot fit into the available memory.
- **Visualization:**
  - Real-time interactive memory map / Gantt-style layout.
  - Live updates of the Segment Page Tables.

## Screenshots
*(Note: Add your screenshots to a `docs/` or `assets/` folder and update these paths once you push them to GitHub)*

> **[ Placeholder: Insert an image of your GUI memory map here ]**
> `![GUI Screenshot](assets/gui_screenshot.png)`

## Dependencies

CMake **does not** install these natively; you must have them on your machine before building.

| Tool | Purpose | Download |
| :--- | :--- | :--- |
| **CMake** | Generates build files and drives the compilation | [cmake.org](https://cmake.org/download/) |
| **Git** | Used by CMake (`FetchContent`) to download GUI libraries | [git-scm.com](https://git-scm.com/downloads) |
| **C++ Toolchain** | Compiles C++17 code | **Windows:** Visual Studio or MSYS2/MinGW <br> **Linux:** GCC/Clang <br> **macOS:** Xcode |
| **OpenGL** | Rendering backend for the GUI | Usually pre-installed on Windows/macOS. Linux may need `libgl1-mesa-dev`. |

**Libraries fetched automatically by CMake during the build process:**
- [GLFW 3.4](https://github.com/glfw/glfw) — Window and OpenGL context creation.
- [Dear ImGui v1.91.9](https://github.com/ocornut/imgui) — Bloat-free graphical user interface.

## Build and Run Instructions

### 1. Clone the Repository
```bash
git clone [https://github.com/mina-emad-2099/MemomryProject.git](https://github.com/mina-emad-2099/MemomryProject.git)
cd MemomryProject
```

### 2. Configure and Build (CMake)
From the root of the project, run the following commands to generate the build files and compile the executables:

```bash
cmake -B build
cmake --build build --config Release
```

### 3. Run the Application

**To run the GUI version:**
Navigate to your build directory and execute the GUI binary.
```bash
# On Windows
.\build\Release\MemoryAllocatorGUI.exe

# On Linux/macOS
./build/MemoryAllocatorGUI
```

**To run the Console version:**
```bash
# On Windows
.\build\Release\MemoryAllocatorConsole.exe

# On Linux/macOS
./build/MemoryAllocatorConsole
```

## Project Structure

- `main_gui.cpp`: The complete graphical interface implementation utilizing Dear ImGui and OpenGL3.
- `main.cpp`: The console-based terminal interface for the simulation.
- `CMakeLists.txt`: Build configuration file handling both console and GUI targets, including dependency fetching.s