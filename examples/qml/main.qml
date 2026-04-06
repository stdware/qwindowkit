import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QWK.Demo 1.0

FramelessWindow {
    property FramelessWindow childWindow: FramelessWindow {
        showWhenReady: false
    }

    FPSCounter {
        property int maxVal: 0

        id: fps
        onValueChanged: fps.maxVal = Math.max(fps.value, fps.maxVal)
    }

    Text {
        anchors {
            bottom: buttonsRow.top
            bottomMargin: 10
            horizontalCenter: parent.horizontalCenter
        }
        font {
            pixelSize: 25
            bold: true
        }
        color: "green"
        text: qsTr("FPS: ") + fps.value + qsTr(", Max: ") + fps.maxVal
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
        id: buttonsRow
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
