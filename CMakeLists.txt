cmake_minimum_required(VERSION 3.20)
project(Poker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.8
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
  FetchContent_Populate(stb)
endif()

set(IMGUI_DIR "${imgui_SOURCE_DIR}")
add_library(
  imgui STATIC
  "${IMGUI_DIR}/imgui.cpp"
  "${IMGUI_DIR}/imgui_draw.cpp"
  "${IMGUI_DIR}/imgui_tables.cpp"
  "${IMGUI_DIR}/imgui_widgets.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(
  imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends"
)
target_link_libraries(imgui PUBLIC glfw)

add_executable(
  poker
  src/main.cpp
  src/deck.cpp
  src/card_textures.cpp
  src/preflop_odds.cpp
)

target_include_directories(
  poker
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${stb_SOURCE_DIR}"
)

target_link_libraries(poker PRIVATE imgui)

target_compile_definitions(
  poker PRIVATE "POCKER_ASSETS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
)

if(APPLE)
  target_link_libraries(
    poker PRIVATE "-framework OpenGL" "-framework Cocoa"
                  "-framework IOKit" "-framework CoreVideo"
  )
elseif(WIN32)
  target_link_libraries(poker PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(poker PRIVATE OpenGL::GL)
endif()

target_compile_definitions(imgui PUBLIC GLFW_INCLUDE_NONE)

target_compile_options(
  poker PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wall -Wextra -Wpedantic>
                $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
)
