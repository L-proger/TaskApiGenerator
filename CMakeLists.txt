cmake_minimum_required (VERSION 3.15.0)

project(TaskApiGenerator LANGUAGES CXX)

add_executable(TaskApiGenerator)

set_property(TARGET TaskApiGenerator PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(TARGET TaskApiGenerator PROPERTY CXX_STANDARD 17)
add_subdirectory(Dependencies)
add_subdirectory(Src)

target_link_libraries(TaskApiGenerator PRIVATE CommandLine)
target_link_libraries(TaskApiGenerator PRIVATE libComInterfaceGenerator)

install(TARGETS TaskApiGenerator
        CONFIGURATIONS Debug
        RUNTIME DESTINATION TaskApiGenerator/bin)