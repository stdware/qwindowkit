#[[
# Writes a file at configure time, unless the file already has the given content.
#
# qwk_write_if_different(<file> <content>)
#
# The modification time of an unchanged file is preserved, so that a reconfigure does not cause
# the translation units that include it to be compiled again. configure_file() and
# file(CONFIGURE) preserve it as well, but write a temporary file before the comparison, which
# took 6.6 s for 2000 files on Windows against 0.3 s for this function.
#]]
function(qwk_write_if_different _file _content)
    if(EXISTS "${_file}")
        file(READ "${_file}" _old)

        if(_old STREQUAL _content)
            return()
        endif()
    endif()

    file(WRITE "${_file}" "${_content}")
endfunction()
