cmake_minimum_required(VERSION 3.14)
project(MemoryAllocatorSim)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent to download dependencies automatically
include(FetchContent)

# -----------------------------------------------------------
# 1. Fetch GLFW
# -----------------------------------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
# Disable GLFW tests and examples to speed up the build
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# -----------------------------------------------------------
# 2. Fetch Dear ImGui
# -----------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90 # Or the latest release tag
)
FetchContent_MakeAvailable(imgui)

# -----------------------------------------------------------
# 3. Find System OpenGL (Built into Windows)
# -----------------------------------------------------------
find_package(OpenGL REQUIRED)

# -----------------------------------------------------------
# 4. Set up our Executable
# -----------------------------------------------------------
add_executable(${PROJECT_NAME} main_gui.cpp)

# Add ImGui core files and the specific GLFW/OpenGL backends
target_sources(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Tell our project where to find the ImGui headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Link everything together
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)