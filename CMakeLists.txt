CMAKE_MINIMUM_REQUIRED(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(USE_SANITIZER "None" CACHE STRING "Santizer option to use")
set_property(CACHE USE_SANITIZER PROPERTY STRINGS None Address Thread Undefined Leak Memory)

if (USE_SANITIZER STREQUAL "Address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif (USE_SANITIZER STREQUAL "Thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
elseif (USE_SANITIZER STREQUAL "Undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif (USE_SANITIZER STREQUAL "Leak")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
elseif (USE_SANITIZER STREQUAL "Memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif()

project(awParticleEditor)

add_subdirectory(${AW_ENGINE_DIR} "awEngine")
include(../cmake/color.cmake)
include(FetchContent)
include(../cmake/loadDependency.cmake)

# set(YAML_CPP_INSTALL OFF)
# set(YAML_CPP_BUILD_TOOLS OFF)
# set(YAML_CPP_BUILD_TESTS OFF)
# set(YAML_CPP_BUILD_SHARED_LIBS ON)
# loadDependencyFromGit(yamlcpp https://github.com/jbeder/yaml-cpp yaml-cpp-0.6.3)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  src/gl.cpp
  src/main.cpp
  src/particleEditorState.cpp
  #IMGUI
  src/imgui/imgui.cpp
  src/imgui/imgui_draw.cpp
  src/imgui/imgui_widgets.cpp
  src/imgui/imgui_impl_sdl.cpp
  src/imgui/imgui_impl_opengl3.cpp
)

find_package(Threads)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads awEngine SDL2)
