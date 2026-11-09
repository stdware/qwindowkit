#[[
# Sets <out> to the file of an imported executable or library target.
#
# qwk_get_imported_location(<target> <out>)
#
# IMPORTED_LOCATION is tried first, then IMPORTED_LOCATION_<config> for RELEASE, RELWITHDEBINFO,
# MINSIZEREL and DEBUG in this order. A target without any of them is reported as an error.
#]]
function(qwk_get_imported_location _target _out)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "qwk_get_imported_location: '${_target}' is not a target")
    endif()

    foreach(_property IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_RELWITHDEBINFO
        IMPORTED_LOCATION_MINSIZEREL IMPORTED_LOCATION_DEBUG)
        get_target_property(_location ${_target} ${_property})

        if(_location)
            set(${_out} "${_location}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "qwk_get_imported_location: '${_target}' has no imported location")
endfunction()
