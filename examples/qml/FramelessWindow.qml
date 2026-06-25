import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1
import QWindowKit 1.0

Window {
    property bool showWhenReady: true
    property alias titleBar: titleBar
    readonly property bool isMacOS: Qt.platform.os === "osx"

    id: window
    width: 800
    height: 600
    color: darkStyle.windowBackgroundColor
    title: qsTr("QWindowKit QtQuick Demo")
    Component.onCompleted: {
        windowAgent.setup(window)
        windowAgent.setWindowAttribute("dark-mode", true)
        if (window.showWhenReady) {
            window.visible = true
        }
    }

    QtObject {
        id: lightStyle
    }

    QtObject {
        id: darkStyle
        readonly property color windowBackgroundColor: "#1E1E1E"
    }

    function applyMacBlurEffect(effect) {
        window.color = effect === "none" ? darkStyle.windowBackgroundColor : "transparent"
        windowAgent.setWindowAttribute("glass-effect", "none")
        windowAgent.setWindowAttribute("blur-effect", effect)
    }

    function applyMacGlassEffect(effect, tintColor, radius) {
        window.color = effect === "none" ? darkStyle.windowBackgroundColor : "transparent"
        windowAgent.setWindowAttribute("blur-effect", "none")
        windowAgent.setWindowAttribute("glass-corner-radius", radius === undefined ? 0 : radius)
        windowAgent.setWindowAttribute("glass-tint-color", tintColor === undefined ? "none" : tintColor)
        windowAgent.setWindowAttribute("glass-effect", effect)
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

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: contextMenu.open()
    }

    Rectangle {
        id: titleBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 32
        //color: window.active ? "#3C3C3C" : "#505050"
        color: "transparent"
        Component.onCompleted: windowAgent.setTitleBar(titleBar)

        Image {
            id: iconButton
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: window.isMacOS ? 0 : 10
            }
            visible: !window.isMacOS
            width: window.isMacOS ? 0 : 18
            height: width
            mipmap: true
            source: "qrc:///app/example.png"
            fillMode: Image.PreserveAspectFit
            Component.onCompleted: {
                if (!window.isMacOS) {
                    windowAgent.setSystemButton(WindowAgent.WindowIcon, iconButton)
                }
            }
        }

        Text {
            anchors {
                verticalCenter: parent.verticalCenter
                left: iconButton.right
                leftMargin: window.isMacOS ? 0 : 10
                right: captionButtonRow.left
                rightMargin: 10
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: window.title
            font.pixelSize: 14
            color: "#ECECEC"
        }

        Row {
            id: captionButtonRow
            visible: !window.isMacOS
            width: visible ? implicitWidth : 0
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
                Component.onCompleted: {
                    if (!window.isMacOS) {
                        windowAgent.setSystemButton(WindowAgent.Minimize, minButton)
                    }
                }
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
                Component.onCompleted: {
                    if (!window.isMacOS) {
                        windowAgent.setSystemButton(WindowAgent.Maximize, maxButton)
                    }
                }
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
                Component.onCompleted: {
                    if (!window.isMacOS) {
                        windowAgent.setSystemButton(WindowAgent.Close, closeButton)
                    }
                }
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
        Component.onCompleted: {
            if ($curveRenderingAvailable) {
                console.log("Curve rendering for text is available.")
                timeLabel.renderType = Text.CurveRendering
            }
        }
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
                checked: true
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
                text: qsTr("None")
                checkable: true
                checked: true
                onTriggered: {
                    window.color = darkStyle.windowBackgroundColor
                    windowAgent.setWindowAttribute("dwm-blur", false)
                    windowAgent.setWindowAttribute("acrylic-material", false)
                    windowAgent.setWindowAttribute("mica", false)
                    windowAgent.setWindowAttribute("mica-alt", false)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("DWM blur")
                checkable: true
                onTriggered: {
                    window.color = "transparent"
                    windowAgent.setWindowAttribute("acrylic-material", false)
                    windowAgent.setWindowAttribute("mica", false)
                    windowAgent.setWindowAttribute("mica-alt", false)
                    windowAgent.setWindowAttribute("dwm-blur", true)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Acrylic material")
                checkable: true
                onTriggered: {
                    window.color = "transparent"
                    windowAgent.setWindowAttribute("dwm-blur", false)
                    windowAgent.setWindowAttribute("mica", false)
                    windowAgent.setWindowAttribute("mica-alt", false)
                    windowAgent.setWindowAttribute("acrylic-material", true)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Mica")
                checkable: true
                onTriggered: {
                    window.color = "transparent"
                    windowAgent.setWindowAttribute("dwm-blur", false)
                    windowAgent.setWindowAttribute("acrylic-material", false)
                    windowAgent.setWindowAttribute("mica-alt", false)
                    windowAgent.setWindowAttribute("mica", true)
                }
            }

            MenuItem {
                enabled: Qt.platform.os === "windows"
                text: qsTr("Mica Alt")
                checkable: true
                onTriggered: {
                    window.color = "transparent"
                    windowAgent.setWindowAttribute("dwm-blur", false)
                    windowAgent.setWindowAttribute("acrylic-material", false)
                    windowAgent.setWindowAttribute("mica", false)
                    windowAgent.setWindowAttribute("mica-alt", true)
                }
            }

            MenuSeparator {
                visible: Qt.platform.os === "osx"
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Glass: regular")
                checkable: true
                onTriggered: window.applyMacGlassEffect("regular")
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Glass: clear")
                checkable: true
                onTriggered: window.applyMacGlassEffect("clear")
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Glass: regular, rounded")
                checkable: true
                onTriggered: window.applyMacGlassEffect("regular", undefined, 24)
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Glass: regular, dark tint")
                checkable: true
                onTriggered: window.applyMacGlassEffect("regular", Qt.rgba(0, 0, 0, 0.18))
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Glass: regular, light tint")
                checkable: true
                onTriggered: window.applyMacGlassEffect("regular", Qt.rgba(1, 1, 1, 0.18))
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Dark blur")
                checkable: true
                onTriggered: window.applyMacBlurEffect("dark")
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("Light blur")
                checkable: true
                onTriggered: window.applyMacBlurEffect("light")
            }

            MenuItem {
                enabled: Qt.platform.os === "osx"
                text: qsTr("No effect")
                checkable: true
                onTriggered: window.applyMacGlassEffect("none")
            }
        }
    }
}
