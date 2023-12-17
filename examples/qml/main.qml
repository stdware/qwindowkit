import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QWindowKit 1.0

Window {
    id: window
    width: 800
    height: 600
    color: "#f0f0f0"
    title: qsTr("Hello, world!")
    Component.onCompleted: {
        windowAgent.setup(window)
        window.visible = true
    }

    WindowAgent {
        id: windowAgent
    }

    Rectangle {
        id: titleBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 32
        color: "white"
        Component.onCompleted: windowAgent.setTitleBar(titleBar)

        Text {
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: window.title
            font.pixelSize: 14
            color: window.active ? "black" : "gray"
        }
    }
}