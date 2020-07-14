
set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/mango)
set(DOXYGEN_OUT ${PROJECT_BINARY_DIR}/doc)

set(MANGO_PROJECT_NAME "Mango")
set(MANGO_PROJECT_VERSION "Version ")
set(MANGO_PROJECT_BRIEF "")

set(DOXYGEN_DOXYFILE_OUT ${DOXYGEN_OUT}/Doxyfile)
configure_file(${PROJECT_SOURCE_DIR}/doc/Doxyfile.in ${DOXYGEN_DOXYFILE_OUT} @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/doc/mango_header.html.in ${DOXYGEN_OUT}/header.html @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/doc/mango_footer.html.in ${DOXYGEN_OUT}/footer.html @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/doc/mango_stylesheet.css.in ${DOXYGEN_OUT}/extra_style.css @ONLY)

message("Doxygen build started.")

add_custom_target(doc_doxygen
    ALL
    COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_DOXYFILE_OUT}
    COMMAND ${CMAKE_COMMAND} -E echo "Done."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)
