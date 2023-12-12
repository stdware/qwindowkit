#include "win10borderhandler_p.h"

namespace QWK {

    Win10BorderHandler::Win10BorderHandler(QWindow *window) : m_window(window), m_borderThickness(0) {
    }

    Win10BorderHandler::~Win10BorderHandler() = default;

}