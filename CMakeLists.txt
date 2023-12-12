cmake_minimum_required (VERSION 3.5)
project (RN-Praxis)
set (CMAKE_C_STANDARD 11)

find_package(OpenSSL REQUIRED)

add_executable(webserver src/webserver.c
        src/webserver.h
        src/lib/socket.c
        src/lib/socket.h
        src/lib/utils.c
        src/lib/utils.h
        src/lib/http.c
        src/lib/http.h
        src/lib/filesystem/filesystem.c
        src/lib/filesystem/filesystem.h
        src/lib/filesystem/operations.c
        src/lib/filesystem/operations.h
)

target_compile_options (webserver PRIVATE -g -Wall -Wextra -Wpedantic)
target_link_libraries(webserver PRIVATE ${OPENSSL_LIBRARIES} -lm)

# Packaging
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  ${CMAKE_BINARY_DIR}
  /\\..*$
)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
