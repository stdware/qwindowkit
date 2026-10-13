import QtQuick 2.15
import QtQuick.Window 2.15
import QWindowKit 1.0

Window {
    id: window
    property bool initialized: false
    property int minimizeRole: WindowAgent.Minimize
    Item { id: titleBar }
    Item { id: minimizeButton }
    WindowAgent { id: agent }

    Component.onCompleted: {
        if (!agent.setup(window))
            return
        agent.setTitleBar(titleBar)
        agent.setSystemButton(WindowAgent.Minimize, minimizeButton)
        agent.setHitTestVisible(minimizeButton, true)
        agent.setWindowAttribute("dark-mode", true)
        agent.centralize()
        window.initialized = agent.titleBar() === titleBar
                && agent.systemButton(WindowAgent.Minimize) === minimizeButton
                && agent.isHitTestVisible(minimizeButton)
    }
}
