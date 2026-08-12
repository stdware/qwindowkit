# Builds the qmake consumer project against a staged install tree.
#
# cmake -D QMAKE_EXECUTABLE=... -D MAKE_PROGRAM=... -D INSTALL_PREFIX=... -D PROJECT_FILE=...
#       -D WORK_DIR=... -D BUILD_CONFIG=debug|release -D QWK_MODULES=... -P qmake/RunBuild.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../testing/ConsumerBuildCommon.cmake")

qwk_require_variables(QMAKE_EXECUTABLE MAKE_PROGRAM INSTALL_PREFIX PROJECT_FILE WORK_DIR
    BUILD_CONFIG QWK_MODULES
)

# From scratch every run. A makefile left behind by an earlier one could carry paths into an
# install tree that has since changed, and build without ever consulting the files under test.
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# qmake takes a list as a whitespace separated value.
string(REPLACE ";" " " _modules "${QWK_MODULES}")

qwk_run_step("qmake" "${QMAKE_EXECUTABLE}" "${PROJECT_FILE}"
    "QWK_PREFIX=${INSTALL_PREFIX}"
    "QWK_MODULES=${_modules}"
    "CONFIG+=${BUILD_CONFIG}"
)

qwk_run_step("make" "${MAKE_PROGRAM}")
