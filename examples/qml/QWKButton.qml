import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: root
    width: height * 1.5
    property alias source: image.source
    contentItem: Item {
        anchors.fill: parent

        Image {
            id: image
            anchors.centerIn: parent
            mipmap: true
            width: 10
            height: 10
        }
    }
    background: Rectangle {
        anchors.fill: parent
        color: {
            if (!root.enabled) {
                return "gray";
            }
            if (root.pressed) {
                return Qt.rgba(0, 0, 0, 0.15);
            }
            if (root.hovered) {
                return Qt.rgba(0, 0, 0, 0.15);
            }
            return "transparent";
        }
    }
}