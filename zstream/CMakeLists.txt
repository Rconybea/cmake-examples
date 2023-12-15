set(SELF_LIB zstream)

add_library(${SELF_LIB} INTERFACE)
target_include_directories(${SELF_LIB} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${SELF_LIB} INTERFACE compression)

target_compile_options(${SELF_LIB} INTERFACE ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_LIB} INTERFACE ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_LIB} INTERFACE ${zlib_LIBRARIES})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/zstream
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

install(
    TARGETS ${SELF_LIB}
    PUBLIC_HEADER DESTINATION include COMPONENT Development)
