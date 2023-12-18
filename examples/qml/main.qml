import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QWindowKit 1.0

Window {
    id: window
    width: 800
    height: 600
    color: "#1E1E1E"
    title: qsTr("Hello, world!")
    Component.onCompleted: {
        windowAgent.setup(window)
        window.visible = true
    }

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: timeLabel.text = Qt.formatTime(new Date(), "hh:mm:ss")
    }

    WindowAgent {
        id: windowAgent
    }

    Rectangle {
        id: titleBar
        anchors {
            top: parent.top
            topMargin: 1
            left: parent.left
            right: parent.right
        }
        height: 32
        color: window.active ? "#3C3C3C" : "#505050"
        Component.onCompleted: windowAgent.setTitleBar(titleBar)

        Image {
            id: iconButton
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 10
            }
            width: 18
            height: 18
            mipmap: true
            source: "qrc:///app/example.png"
        }

        Text {
            anchors {
                verticalCenter: parent.verticalCenter
                left: iconButton.right
                leftMargin: 10
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: window.title
            font.pixelSize: 14
            color: "#ECECEC"
        }

        Row {
            anchors {
                top: parent.top
                right: parent.right
            }
            height: parent.height

            QWKButton {
                id: minButton
                height: parent.height
                source: "qrc:///window-bar/minimize.svg"
                onClicked: window.showMinimized()
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Minimize, minButton)
            }

            QWKButton {
                id: maxButton
                height: parent.height
                source: window.visibility === Window.Maximized ? "qrc:///window-bar/restore.svg" : "qrc:///window-bar/maximize.svg"
                onClicked: {
                    if (window.visibility === Window.Maximized) {
                        window.showNormal()
                    } else {
                        window.showMaximized()
                    }
                }
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Maximize, maxButton)
            }

            QWKButton {
                id: closeButton
                height: parent.height
                source: "qrc:///window-bar/close.svg"
                background: Rectangle {
                    color: {
                        if (!closeButton.enabled) {
                            return "gray";
                        }
                        if (closeButton.pressed) {
                            return "#e81123";
                        }
                        if (closeButton.hovered) {
                            return "#e81123";
                        }
                        return "transparent";
                    }
                }
                onClicked: window.close()
                Component.onCompleted: windowAgent.setSystemButton(WindowAgent.Close, closeButton)
            }
        }
    }

    Label {
        id: timeLabel
        anchors.centerIn: parent
        font {
            pointSize: 75
            bold: true
        }
        color: "#FEFEFE"
    }
}