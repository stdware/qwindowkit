import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

FramelessWindow {
    property FramelessWindow childWindow: FramelessWindow {
        showWhenReady: false
    }

    // Average actual frame intervals, as in Qt's CanvasView.qml fpsItem.
    // FrameAnimation measures animation updates and keeps the animation loop running.
    FrameAnimation {
        id: fps
        property int ticks: 0
        property real frameTimes: 0
        property real frameRate: 0
        readonly property int value: Math.round(frameRate)
        property int maxVal: 0
        running: true
        onValueChanged: fps.maxVal = Math.max(fps.value, fps.maxVal)
        onTriggered: {
            ++ticks
            frameTimes += frameTime
            if (frameTimes > 1.0) {
                frameRate = ticks / frameTimes
                ticks = 0
                frameTimes = 0
            }
        }
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
