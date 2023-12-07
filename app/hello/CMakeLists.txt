set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_include_directories(${SELF_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/compression/include)
target_link_libraries(${SELF_EXE} PUBLIC compression)
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)

install(TARGETS ${SELF_EXE}
    RUNTIME DESTINATION bin COMPONENT Runtime
    BUNDLE DESTINATION bin COMPONENT Runtime)
