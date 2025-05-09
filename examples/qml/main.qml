import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

FramelessWindow {
    property FramelessWindow childWindow: FramelessWindow {
        showWhenReady: false
    }

    Button {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 20
        }
        text: qsTr("Open Child Window")
        onClicked: childWindow.visible = true
    }
}