set(SELF_LIB pyzstream)

pybind11_add_module(${SELF_LIB} MODULE pyzstream.cpp)
target_link_libraries(${SELF_LIB} PUBLIC zstream)

set(SELF_SCRIPT zstream.py)

# Copy python libraries to build directory
# so that they share the same directory as pybind11 .so's;
# this is for the sake of consistency with install behavior.
#
# Use configure_file() for build dependency between source and destination
#
configure_file(
    ${SELF_SCRIPT}
    ${CMAKE_CURRENT_BINARY_DIR}/${SELF_SCRIPT}
    COPYONLY
)

# libs prepared via pybind11 will have architecture- and python-version- dependent name like:
#   pyzstream.cpython-310-x86_64-linux-gnu.so
# They will work even if they appear in the same directory as .so files from other python versions;
# python will take responsibility for loading only compatible libs.
#
install(
    TARGETS ${SELF_LIB}
#    EXPORT ${SELF_LIB}Targets
#    LIBRARY DESTINATION ${PYTHON_LIBRARY_PREFIX} COMPONENT Runtime
    LIBRARY DESTINATION ${PYTHON_LIBRARY_PREFIX} COMPONENT Runtime
)
