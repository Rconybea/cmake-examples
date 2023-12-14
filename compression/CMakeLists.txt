set(SELF_LIB compression)
set(SELF_SRCS compression.cpp inflate_zstream.cpp deflate_zstream.cpp buffered_inflate_zstream.cpp buffered_deflate_zstream.cpp)
set(SELF_VERSION 2)
set(SELF_SOVERSION 2.3)

add_library(${SELF_LIB} SHARED ${SELF_SRCS})
set_target_properties(${SELF_LIB} PROPERTIES VERSION ${SELF_VERSION} SOVERSION ${SELF_SOVERSION})
target_include_directories(${SELF_LIB} PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_options(${SELF_LIB} PRIVATE ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_LIB} PRIVATE ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_LIB} PRIVATE ${zlib_LIBRARIES})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/compression
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

install(
    TARGETS ${SELF_LIB}
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    PUBLIC_HEADER DESTINATION include COMPONENT Development)
