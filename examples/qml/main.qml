import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

FramelessWindow {
    property FramelessWindow childWindow: FramelessWindow {
        showWhenReady: false
    }

    Drawer {
        id: drawer
        width: 0.66 * parent.width
        height: parent.height
        edge: Qt.RightEdge
        onAboutToShow: titleBar.enabled = false
        onAboutToHide: titleBar.enabled = true

        Label {
            text: "Content goes here!"
            anchors.centerIn: parent
        }
    }

    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }
        spacing: 10

        Button {
            text: qsTr("Open Child Window")
            onClicked: childWindow.visible = true
        }

        Button {
            text: qsTr("Open Drawer")
            onClicked: drawer.visible = true
        }
    }
}
