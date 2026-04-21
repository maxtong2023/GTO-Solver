cmake_minimum_required(VERSION 3.20)
project(Poker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(poker
  src/main.cpp
  src/deck.cpp
)

target_include_directories(poker PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(poker PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wall -Wextra -Wpedantic>
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
)
