# Run with -P and -DDIR=<directory>. Downloads the files of doxygen-awesome-css that the
# documentation uses into DIR. A file that is already present with the expected checksum is not
# downloaded again, and one with another checksum is replaced.
#
# doxygen-awesome-css, https://github.com/jothepro/doxygen-awesome-css, MIT. v2.2.1 is the version
# that qmsetup carried and that header.html is written for.
#
# The download runs when the documentation is generated rather than at configure time, so that a
# configure with the documentation enabled requires no network access.

if(NOT DIR)
    message(FATAL_ERROR "FetchTheme.cmake: DIR is required")
endif()

set(_tag "v2.2.1")
set(_url "https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/${_tag}")

foreach(_item
    "doxygen-awesome.css;9b5549928906e9974cc12dcdde9265e016dc2388ec72d5aa3209f4870914a0c8"
    "doxygen-awesome-sidebar-only-darkmode-toggle.css;b1907142f53b2350507fa5e42ce72f2213c680b2e3482458d3def031c364b150"
    "doxygen-awesome-darkmode-toggle.js;931ed337831ecaa5b26105cbbdd1f3077f210ec44d5081e6647f4d98e18bddd3"
    "LICENSE;e3da754c3f657cc78594fa2e8a3283665f78c743df2485fa9e498a8973051191"
)
    list(GET _item 0 _name)
    list(GET _item 1 _hash)
    set(_file "${DIR}/${_name}")

    if(EXISTS "${_file}")
        file(SHA256 "${_file}" _actual)

        if(_actual STREQUAL _hash)
            continue()
        endif()
    endif()

    # Downloaded under a temporary name, because a checksum mismatch stops the script before the
    # STATUS can be examined, and the file would otherwise remain under its final name.
    message(STATUS "Fetching ${_name} from doxygen-awesome-css ${_tag}")
    file(DOWNLOAD "${_url}/${_name}" "${_file}.download"
        EXPECTED_HASH SHA256=${_hash}
        TLS_VERIFY ON
        STATUS _status
    )
    list(GET _status 0 _code)

    if(NOT _code EQUAL 0)
        list(GET _status 1 _reason)
        file(REMOVE "${_file}.download")
        message(FATAL_ERROR "Cannot fetch ${_name} for the documentation theme: ${_reason}")
    endif()

    file(RENAME "${_file}.download" "${_file}")
endforeach()
