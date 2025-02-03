add_library(glad STATIC
    src/gl.c
    include/KHR/khrplatform.h
    include/glad/gl.h
)

create_target_directory_groups(glad)
target_include_directories(glad PUBLIC "include/")

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    target_link_libraries(glad PRIVATE dl)
endif()
