import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var tabModel: null
    property int tabCount: 1
    property int activeIndex: 0
    property var ui: ({})
    property color bgShell: "#0d0e11"
    property color bgEditor: "#16181e"
    property color bgHover: "#252934"
    property color borderSubtle: "#222630"
    property color borderControl: "#313644"
    property color borderHover: "#52596d"
    property color fgMain: "#f0f2f6"
    property color fgMuted: "#9ca3b4"

    signal tabActivated(int index)
    signal tabClosed(int index)
    signal newTabRequested

    visible: tabCount > 1
    height: visible ? 36 : 0
    clip: true
    color: bgShell

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    Flickable {
        id: tabFlick
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: addTabBtn.left
        anchors.rightMargin: 8
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentWidth: tabRow.implicitWidth
        contentHeight: height
        clip: true
        interactive: false
        boundsBehavior: Flickable.StopAtBounds

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            onWheel: function (wheel) {
                var d = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.angleDelta.x;
                if (d !== 0) {
                    var maxX = Math.max(0, tabFlick.contentWidth - tabFlick.width);
                    tabFlick.contentX = Math.max(0, Math.min(maxX, tabFlick.contentX - (d / 120.0) * 90));
                }
                wheel.accepted = true;
            }
        }

        Row {
            id: tabRow
            height: parent.height
            spacing: 5

            Repeater {
                model: root.tabModel

                delegate: Item {
                    id: tabItem
                    readonly property bool isActive: index === root.activeIndex
                    readonly property bool hovered: tabMa.containsMouse || closeBtn.hovered
                    readonly property bool isDirty: session ? session.modified : false

                    height: 28
                    width: Math.min(220, Math.max(116, labelText.implicitWidth + 56))
                    anchors.verticalCenter: parent.verticalCenter

                    onIsActiveChanged: {
                        if (isActive) {
                            if (x < tabFlick.contentX) {
                                tabFlick.contentX = x;
                            } else if (x + width > tabFlick.contentX + tabFlick.width) {
                                tabFlick.contentX = Math.max(0, x + width - tabFlick.width);
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: tabItem.isActive ? root.bgEditor : (tabMa.pressed ? root.borderSubtle : (tabItem.hovered ? root.bgHover : "transparent"))
                        border.width: 1
                        border.color: tabItem.isActive ? root.borderControl : (tabItem.hovered ? root.borderControl : "transparent")
                        scale: tabMa.pressed ? 0.975 : 1.0
                        transformOrigin: Item.Center

                        Behavior on color {
                            ColorAnimation {
                                duration: 95
                                easing.type: Easing.OutCubic
                            }
                        }
                        Behavior on border.color {
                            ColorAnimation {
                                duration: 95
                                easing.type: Easing.OutCubic
                            }
                        }
                        Behavior on scale {
                            NumberAnimation {
                                duration: 75
                                easing.type: Easing.OutCubic
                            }
                        }

                        Rectangle {
                            visible: tabItem.isActive
                            anchors.top: parent.top
                            anchors.topMargin: 1
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 5
                            anchors.rightMargin: 5
                            height: 1
                            color: root.theme.sheenBright || (root.darkMode ? "#12ffffff" : "#90ffffff")
                        }

                        Rectangle {
                            visible: tabItem.isActive
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 16
                            height: 2
                            radius: 1
                            color: root.fgMain
                            opacity: 0.78
                        }
                    }

                    MouseArea {
                        id: tabMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function (mouse) {
                            if (mouse.button === Qt.MiddleButton) {
                                root.tabClosed(index);
                            } else {
                                root.tabActivated(index);
                            }
                        }
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 11
                        anchors.right: closeBtn.left
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 7

                        Rectangle {
                            visible: tabItem.isDirty
                            width: 6
                            height: 6
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.fgMain
                        }

                        Text {
                            id: labelText
                            width: Math.max(20, parent.width - (tabItem.isDirty ? 13 : 0))
                            elide: Text.ElideMiddle
                            text: (session && session.filePath !== "") ? session.fileName : (root.ui.untitled || "Untitled")
                            color: tabItem.isActive || tabItem.hovered ? root.fgMain : root.fgMuted
                            font.pixelSize: 12
                            font.weight: tabItem.isActive ? Font.DemiBold : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation {
                                    duration: 95
                                }
                            }
                        }
                    }

                    M3Button {
                        id: closeBtn
                        anchors.right: parent.right
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: (tabItem.isActive || tabItem.hovered) ? 1.0 : 0.0
                        iconKind: "close"
                        iconOnly: true
                        ghost: true
                        dangerHover: true
                        buttonHeight: 20
                        buttonWidth: 20
                        iconSize: 11
                        cornerRadius: 4
                        darkMode: root.darkMode
            theme: root.theme
                        onClicked: root.tabClosed(index)

                        Behavior on opacity {
                            NumberAnimation {
                                duration: 85
                            }
                        }
                    }
                }
            }
        }
    }

    M3Button {
        id: addTabBtn
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        iconKind: "plus"
        iconOnly: true
        ghost: true
        buttonHeight: 28
        buttonWidth: 28
        iconSize: 15
        cornerRadius: 6
        darkMode: root.darkMode
            theme: root.theme
        tooltipText: root.ui.newTabTip || "New Tab (Ctrl+T)"
        onClicked: root.newTabRequested()
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
