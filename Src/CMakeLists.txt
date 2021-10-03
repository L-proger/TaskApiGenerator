

file(GLOB_RECURSE SRC_FILES    
    "*.cpp"
)

file(GLOB_RECURSE HEADER_FILES    
    "*.h"
)

target_sources(TaskApiGenerator PRIVATE ${SRC_FILES} ${HEADER_FILES})

target_include_directories(TaskApiGenerator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})