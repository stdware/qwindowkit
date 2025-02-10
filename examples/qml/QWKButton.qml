import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: root
    width: height * 1.5
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    leftInset: 0
    topInset: 0
    rightInset: 0
    bottomInset: 0
    property alias source: image.source
    contentItem: Item {
        Image {
            id: image
            anchors.centerIn: parent
            mipmap: true
            width: 12
            height: 12
            fillMode: Image.PreserveAspectFit
        }
    }
    background: Rectangle {
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