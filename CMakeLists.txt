cmake_minimum_required(VERSION 3.16)

project(digiview_mavlink_headers LANGUAGES NONE)

set(DIGIVIEW_MAVLINK_WIRE_PROTOCOL "2.0" CACHE STRING "MAVLink wire protocol version used for generated C headers")

set(DIGIVIEW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MAVLINK_SUBMODULE_SOURCE_DIR "${DIGIVIEW_REPO_ROOT}/mavlink")
set(MAVLINK_DEFINITIONS_SOURCE_DIR "${MAVLINK_SUBMODULE_SOURCE_DIR}/message_definitions/v1.0")
set(MAVLINK_ALL_XML_SOURCE "${MAVLINK_DEFINITIONS_SOURCE_DIR}/all.xml")
set(MAVLINK_REQUIREMENTS_FILE "${MAVLINK_SUBMODULE_SOURCE_DIR}/pymavlink/requirements.txt")
set(SV_DIALECT_SOURCE "${DIGIVIEW_REPO_ROOT}/sv_mavlink_dialect.xml")

function(require_mavlink_content path_value)
    if(NOT EXISTS "${path_value}")
        message(FATAL_ERROR
            "Required MAVLink submodule content is missing:\n"
            "  Missing path: ${path_value}\n"
            "  Expected MAVLink source root: ${MAVLINK_SUBMODULE_SOURCE_DIR}\n"
            "Run:\n"
            "  git submodule update --init --recursive"
        )
    endif()
endfunction()

require_mavlink_content("${MAVLINK_ALL_XML_SOURCE}")
require_mavlink_content("${MAVLINK_REQUIREMENTS_FILE}")

if(NOT EXISTS "${SV_DIALECT_SOURCE}")
    message(FATAL_ERROR "Required DigiView dialect file is missing: ${SV_DIALECT_SOURCE}")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m pip --version
    RESULT_VARIABLE PIP_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)
if(NOT PIP_RESULT EQUAL 0)
    message(FATAL_ERROR "Python pip not found, pip is required to generate MAVLink headers")
endif()

set(DIGIVIEW_STAGED_MAVLINK_ROOT "${CMAKE_CURRENT_BINARY_DIR}/mavlink")
set(DIGIVIEW_XML_BUILD_DIR "${DIGIVIEW_STAGED_MAVLINK_ROOT}/message_definitions/v1.0")
set(DIGIVIEW_STAGED_ALL_XML "${DIGIVIEW_XML_BUILD_DIR}/all.xml")
set(DIGIVIEW_XML_PATCH_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/patch_all_xml.cmake")
set(DIGIVIEW_XML_STAGE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/.xml_staged.stamp")
set(DIGIVIEW_MAVGEN_WORKING_DIR "${DIGIVIEW_STAGED_MAVLINK_ROOT}")
set(DIGIVIEW_MAVGEN_INPUT_XML_REL "message_definitions/v1.0/all.xml")

set(DIGIVIEW_GENERATED_INCLUDE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(DIGIVIEW_GENERATED_MAVLINK_DIR "${DIGIVIEW_GENERATED_INCLUDE_ROOT}/mavlink/v${DIGIVIEW_MAVLINK_WIRE_PROTOCOL}")
set(DIGIVIEW_GENERATED_HEADER "${DIGIVIEW_GENERATED_MAVLINK_DIR}/all/mavlink.h")
set(DIGIVIEW_PYTHON_DEPS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/pip-dependencies")
if(WIN32)
    set(DIGIVIEW_PYTHONPATH_SEPARATOR ";")
else()
    set(DIGIVIEW_PYTHONPATH_SEPARATOR ":")
endif()
set(DIGIVIEW_MAVGEN_PYTHONPATH "${DIGIVIEW_PYTHON_DEPS_DIR}${DIGIVIEW_PYTHONPATH_SEPARATOR}${MAVLINK_SUBMODULE_SOURCE_DIR}")

file(GLOB MAVLINK_XML_FILES CONFIGURE_DEPENDS "${MAVLINK_DEFINITIONS_SOURCE_DIR}/*.xml")

file(GENERATE OUTPUT "${DIGIVIEW_XML_PATCH_SCRIPT}" CONTENT [=[
if(NOT DEFINED ALL_XML_PATH)
    message(FATAL_ERROR "ALL_XML_PATH was not provided")
endif()

if(NOT EXISTS "${ALL_XML_PATH}")
    message(FATAL_ERROR "Cannot patch missing all.xml: ${ALL_XML_PATH}")
endif()

file(READ "${ALL_XML_PATH}" ALL_XML_CONTENT)

string(REGEX MATCH "<include>[ \t\r\n]*sv_mavlink_dialect\\.xml[ \t\r\n]*</include>" HAS_SV_INCLUDE "${ALL_XML_CONTENT}")
if(NOT HAS_SV_INCLUDE)
    set(SV_INCLUDE_LINE "  <include>sv_mavlink_dialect.xml</include>")
    set(INSERTION_DONE FALSE)

    # Prefer insertion after the last existing include.
    string(FIND "${ALL_XML_CONTENT}" "</include>" LAST_INCLUDE_CLOSE_INDEX REVERSE)
    if(NOT LAST_INCLUDE_CLOSE_INDEX EQUAL -1)
        string(LENGTH "</include>" INCLUDE_CLOSE_LENGTH)
        math(EXPR INSERT_INDEX "${LAST_INCLUDE_CLOSE_INDEX} + ${INCLUDE_CLOSE_LENGTH}")
        set(INSERTION_DONE TRUE)
    else()
        # If there are no includes, insert after <version> when present.
        string(FIND "${ALL_XML_CONTENT}" "</version>" VERSION_CLOSE_INDEX)
        if(NOT VERSION_CLOSE_INDEX EQUAL -1)
            string(LENGTH "</version>" VERSION_CLOSE_LENGTH)
            math(EXPR INSERT_INDEX "${VERSION_CLOSE_INDEX} + ${VERSION_CLOSE_LENGTH}")
            set(INSERTION_DONE TRUE)
        else()
            # Final fallback: insert immediately after opening <mavlink> tag.
            string(REGEX MATCH "<mavlink[^>]*>" MAVLINK_OPEN_TAG "${ALL_XML_CONTENT}")
            if(MAVLINK_OPEN_TAG)
                string(FIND "${ALL_XML_CONTENT}" "${MAVLINK_OPEN_TAG}" MAVLINK_OPEN_TAG_INDEX)
                string(LENGTH "${MAVLINK_OPEN_TAG}" MAVLINK_OPEN_TAG_LENGTH)
                math(EXPR INSERT_INDEX "${MAVLINK_OPEN_TAG_INDEX} + ${MAVLINK_OPEN_TAG_LENGTH}")
                set(INSERTION_DONE TRUE)
            endif()
        endif()
    endif()

    if(NOT INSERTION_DONE)
        message(FATAL_ERROR "Unable to patch ${ALL_XML_PATH}: no valid insertion anchor found")
    endif()

    string(LENGTH "${ALL_XML_CONTENT}" ALL_XML_LENGTH)
    math(EXPR SUFFIX_LENGTH "${ALL_XML_LENGTH} - ${INSERT_INDEX}")

    if(INSERT_INDEX GREATER 0)
        string(SUBSTRING "${ALL_XML_CONTENT}" 0 ${INSERT_INDEX} ALL_XML_PREFIX)
    else()
        set(ALL_XML_PREFIX "")
    endif()

    if(SUFFIX_LENGTH GREATER 0)
        string(SUBSTRING "${ALL_XML_CONTENT}" ${INSERT_INDEX} ${SUFFIX_LENGTH} ALL_XML_SUFFIX)
    else()
        set(ALL_XML_SUFFIX "")
    endif()

    set(PATCHED_ALL_XML_CONTENT "${ALL_XML_PREFIX}\n${SV_INCLUDE_LINE}${ALL_XML_SUFFIX}")

    file(WRITE "${ALL_XML_PATH}" "${PATCHED_ALL_XML_CONTENT}")
endif()
]=])

add_custom_command(
    OUTPUT "${DIGIVIEW_XML_STAGE_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIGIVIEW_STAGED_MAVLINK_ROOT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIGIVIEW_XML_BUILD_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${MAVLINK_DEFINITIONS_SOURCE_DIR}" "${DIGIVIEW_XML_BUILD_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SV_DIALECT_SOURCE}" "${DIGIVIEW_XML_BUILD_DIR}/sv_mavlink_dialect.xml"
    COMMAND "${CMAKE_COMMAND}" "-DALL_XML_PATH=${DIGIVIEW_STAGED_ALL_XML}" -P "${DIGIVIEW_XML_PATCH_SCRIPT}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${DIGIVIEW_XML_STAGE_STAMP}"
    DEPENDS
        "${SV_DIALECT_SOURCE}"
        "${MAVLINK_ALL_XML_SOURCE}"
        "${DIGIVIEW_XML_PATCH_SCRIPT}"
        ${MAVLINK_XML_FILES}
    COMMENT "Staging and patching build-local MAVLink XML files"
    VERBATIM
)

add_custom_command(
    OUTPUT "${DIGIVIEW_GENERATED_HEADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIGIVIEW_PYTHON_DEPS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIGIVIEW_GENERATED_MAVLINK_DIR}"
    COMMAND "${Python3_EXECUTABLE}" -m pip install --upgrade -r "${MAVLINK_REQUIREMENTS_FILE}" -t "${DIGIVIEW_PYTHON_DEPS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${DIGIVIEW_MAVGEN_PYTHONPATH}" "${Python3_EXECUTABLE}" -m pymavlink.tools.mavgen --lang=C --wire-protocol=${DIGIVIEW_MAVLINK_WIRE_PROTOCOL} --output "${DIGIVIEW_GENERATED_MAVLINK_DIR}" "${DIGIVIEW_MAVGEN_INPUT_XML_REL}"
    WORKING_DIRECTORY "${DIGIVIEW_MAVGEN_WORKING_DIR}"
    DEPENDS
        "${DIGIVIEW_XML_STAGE_STAMP}"
        "${MAVLINK_REQUIREMENTS_FILE}"
    COMMENT "Generating DigiView MAVLink C headers"
    VERBATIM
)

add_custom_target(digiview_mavlink_generate_headers ALL DEPENDS "${DIGIVIEW_GENERATED_HEADER}")

add_library(digiview_mavlink_headers INTERFACE)
add_library(DigiView::MAVLinkHeaders ALIAS digiview_mavlink_headers)

add_dependencies(digiview_mavlink_headers digiview_mavlink_generate_headers)

target_include_directories(digiview_mavlink_headers
    INTERFACE
        "${DIGIVIEW_GENERATED_INCLUDE_ROOT}"
)

message(STATUS "Using MAVLink source submodule at: ${MAVLINK_SUBMODULE_SOURCE_DIR}")
message(STATUS "Using build-local MAVLink staging root: ${DIGIVIEW_STAGED_MAVLINK_ROOT}")
message(STATUS "mavgen working directory: ${DIGIVIEW_MAVGEN_WORKING_DIR}")
message(STATUS "mavgen input XML (relative): ${DIGIVIEW_MAVGEN_INPUT_XML_REL}")
message(STATUS "DigiView MAVLink headers will be generated in: ${DIGIVIEW_GENERATED_INCLUDE_ROOT}")
