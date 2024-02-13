set(SELF_LIB pyzstream)

pybind11_add_module(${SELF_LIB} MODULE pyzstream.cpp)

target_link_libraries(${SELF_LIB} PUBLIC zstream)

install(
    TARGETS ${SELF_LIB}
#    EXPORT ${SELF_LIB}Targets
    LIBRARY DESTINATION lib COMPONENT Runtime
)
