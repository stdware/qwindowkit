# Run with -P and -DNAVTREE=<generated navtree.js>.
#
# Doxygen 1.17.0 builds the labels of the treeview with
#
#     ahref.textContent = escapeHtml(name);
#
# textContent assigns a text node, which parses no markup. Escaping beforehand therefore corrupts
# the label instead of protecting it: operator== is shown as operator&#x3D;&#x3D;, and every name
# that contains one of < > " ' / ` = is shown with entities. Doxygen 1.10.0 used innerHTML and is
# not affected.
#
# Removing the call is the whole fix and loses no safety, because textContent does not interpret
# its value. Taken from stdcorelib.
#
# A version that does not contain the call does not match, and the file is then left unchanged.

if(NOT EXISTS "${NAVTREE}")
    return()
endif()

file(READ "${NAVTREE}" _content)
string(REPLACE "textContent = escapeHtml(" "textContent = (" _fixed "${_content}")

if(NOT _fixed STREQUAL _content)
    file(WRITE "${NAVTREE}" "${_fixed}")
    message(STATUS "Unescaped the treeview labels in ${NAVTREE}")
endif()
