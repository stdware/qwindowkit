import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1
import QWindowKit 1.0

Window {
    id: window
    width: 800
    height: 600
    color: darkStyle.windowBackgroundColor
    title: qsTr("Hello, world!")
    Component.onCompleted: {
        windowAgent.setup(window)
        window.visible = true
    }

    QtObject {
        id: lightStyle
    }

    QtObject {
        id: darkStyle
        readonly property color windowBackgroundColor: "#1E1E1E"
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

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: contextMenu.open()
    }

    Rectangle {
        id: titleBar
        anchors {
            top: parent.top
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
            Component.onCompleted: windowAgent.setSystemButton(WindowAgent.WindowIcon, iconButton)
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

    Menu {
        id: contextMenu

        Menu {
            id: themeMenu
            title: qsTr("Theme")

            MenuItemGroup {
                id: themeMenuGroup
                items: themeMenu.items
            }

            MenuItem {
                text: qsTr("Light")
                checkable: true
                onTriggered: windowAgent.setWindowAttribute("dark-mode", false)
            }

            MenuItem {
                text: qsTr("Dark")
                checkable: true
                onTriggered: windowAgent.setWindowAttribute("dark-mode", true)
            }
        }

        Menu {
            id: specialEffectMenu
            title: qsTr("Special effect")

            MenuItemGroup {
                id: specialEffectMenuGroup
                items: specialEffectMenu.items
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("DWM blur")
                checkable: true
                onTriggered: {
                    window.color = checked ? "transparent" : darkStyle.windowBackgroundColor
                    windowAgent.setWindowAttribute("dwm-blur", checked)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Acrylic material")
                checkable: true
                onTriggered: {
                    window.color = checked ? "transparent" : darkStyle.windowBackgroundColor
                    windowAgent.setWindowAttribute("acrylic-material", checked)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Mica")
                checkable: true
                onTriggered: {
                    window.color = checked ? "transparent" : darkStyle.windowBackgroundColor
                    windowAgent.setWindowAttribute("mica", checked)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Mica Alt")
                checkable: true
                onTriggered: {
                    window.color = checked ? "transparent" : darkStyle.windowBackgroundColor
                    windowAgent.setWindowAttribute("mica-alt", checked)
                }
            }
        }
    }
}