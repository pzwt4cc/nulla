import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var tabModel: null
    property int tabCount: 1
    property int activeIndex: 0
    property bool showWindowControls: false
    property bool isMaximized: false
    property bool menuOpened: false
    property int iconSize: 18
    property int railWidth: 44
    property var ui: ({})

    property color bgHeader: "#101012"
    property color bgEditor: "#18181b"
    property color bgSurfaceElev: "#202025"
    property color bgHover: "#27272d"
    property color borderSubtle: "#242429"
    property color borderControl: "#34343c"
    property color fgMain: "#f2f2f5"
    property color fgMuted: "#9d9da9"

    signal moveWindowRequested
    signal toggleMaximizeRequested
    signal minimizeRequested
    signal closeWindowRequested
    signal tabActivated(int index)
    signal tabClosed(int index)
    signal newTabRequested
    signal findRequested
    signal menuToggleRequested(real anchorX)

    readonly property int topIconSize: Math.max(15, Math.min(25, iconSize))
    readonly property int topBtnSize: Math.max(28, topIconSize + 11)
    readonly property int logoSize: Math.max(18, Math.min(26, iconSize + 1))

    height: Math.max(40, iconSize + 20)
    color: bgHeader
    clip: true

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: root.moveWindowRequested()
        onDoubleClicked: root.toggleMaximizeRequested()
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }

    Item {
        id: brandSlot
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.railWidth

        Image {
            source: "qrc:/assets/nulla.svg"
            width: root.logoSize
            height: root.logoSize
            sourceSize: Qt.size(52, 52)
            anchors.centerIn: parent
            opacity: root.darkMode ? 0.94 : 0.9
        }
    }

    Item {
        id: tabArea
        anchors.left: brandSlot.right
        anchors.leftMargin: 4
        anchors.right: rightControls.left
        anchors.rightMargin: 8
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        clip: true

        Flickable {
            id: tabFlick
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.min(contentWidth, Math.max(0, tabArea.width - addTabBtn.width - 8))
            contentWidth: tabRow.implicitWidth
            contentHeight: height
            clip: true
            interactive: false
            boundsBehavior: Flickable.StopAtBounds

            onWidthChanged: {
                const maxX = Math.max(0, contentWidth - width);
                if (contentX > maxX) {
                    contentX = maxX;
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                onWheel: function (wheel) {
                    const d = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.angleDelta.x;
                    if (d !== 0) {
                        const maxX = Math.max(0, tabFlick.contentWidth - tabFlick.width);
                        tabFlick.contentX = Math.max(0, Math.min(maxX, tabFlick.contentX - (d / 120.0) * 90));
                    }
                    wheel.accepted = true;
                }
            }

            Row {
                id: tabRow
                height: parent.height
                spacing: 3

                Repeater {
                    model: root.tabModel

                    delegate: Item {
                        id: tabItem
                        readonly property bool isActive: index === root.activeIndex
                        readonly property bool hovered: tabMa.containsMouse || closeBtn.hovered
                        readonly property bool isDirty: session ? session.modified : false

                        TextMetrics {
                            id: tabMetrics
                            font.pixelSize: 12
                            font.weight: tabItem.isActive ? Font.DemiBold : Font.Normal
                            text: (session && session.filePath !== "") ? session.fileName : (root.ui.untitled || "Untitled")
                        }

                        height: parent.height
                        width: Math.min(200, Math.max(108, Math.ceil(Math.max(tabMetrics.advanceWidth, tabMetrics.width)) + 52))

                        onIsActiveChanged: {
                            if (isActive) {
                                if (x < tabFlick.contentX) {
                                    tabFlick.contentX = x;
                                } else if (x + width > tabFlick.contentX + tabFlick.width) {
                                    tabFlick.contentX = Math.max(0, x + width - tabFlick.width);
                                }
                            }
                        }

                        Item {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: root.height - 7

                            Rectangle {
                                id: tabBg
                                anchors.fill: parent
                                radius: 7
                                color: tabItem.isActive ? root.bgEditor : (tabItem.hovered ? root.bgHover : "transparent")
                                border.width: tabItem.isActive ? 1 : 0
                                border.color: root.borderSubtle
                            }

                            Rectangle {
                                visible: tabItem.isActive
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 1
                                anchors.rightMargin: 1
                                height: 8
                                color: root.bgEditor
                            }

                            Rectangle {
                                visible: !tabItem.isActive && index !== root.activeIndex - 1
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 1
                                height: 14
                                color: root.borderSubtle
                            }

                            Rectangle {
                                visible: tabItem.isActive
                                anchors.top: parent.top
                                anchors.topMargin: 1
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 7
                                anchors.rightMargin: 7
                                height: 1
                                color: root.theme.sheenBright || (root.darkMode ? "#16ffffff" : "#90ffffff")
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
                                    width: Math.max(20, parent.width - (tabItem.isDirty ? 13 : 0))
                                    elide: Text.ElideMiddle
                                    text: (session && session.filePath !== "") ? session.fileName : (root.ui.untitled || "Untitled")
                                    color: tabItem.isActive || tabItem.hovered ? root.fgMain : root.fgMuted
                                    font.pixelSize: 12
                                    font.weight: tabItem.isActive ? Font.DemiBold : Font.Normal
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            M3Button {
                                id: closeBtn
                                anchors.right: parent.right
                                anchors.rightMargin: 5
                                anchors.verticalCenter: parent.verticalCenter
                                visible: tabItem.isActive || tabItem.hovered
                                iconKind: "close"
                                iconOnly: true
                                ghost: true
                                dangerHover: true
                                buttonHeight: 19
                                buttonWidth: 19
                                iconSize: 10
                                cornerRadius: 4
                                darkMode: root.darkMode
                                theme: root.theme
                                onClicked: root.tabClosed(index)
                            }
                        }
                    }
                }
            }
        }

        M3Button {
            id: addTabBtn
            anchors.left: tabFlick.right
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            iconKind: "plus"
            iconOnly: true
            ghost: true
            buttonHeight: root.topBtnSize - 2
            buttonWidth: root.topBtnSize - 2
            iconSize: Math.max(14, root.topIconSize - 1)
            cornerRadius: 6
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.newTabTip || "New Tab (Ctrl+T)"
            onClicked: root.newTabRequested()
        }
    }

    Row {
        id: rightControls
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        M3Button {
            iconKind: "search"
            iconOnly: true
            ghost: true
            buttonHeight: root.topBtnSize
            buttonWidth: root.topBtnSize
            iconSize: root.topIconSize
            cornerRadius: 6
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.findTip || "Find (Ctrl+F)"
            onClicked: root.findRequested()
        }

        M3Button {
            id: menuBtn
            iconKind: "dots"
            iconOnly: true
            ghost: !root.menuOpened
            activeChip: root.menuOpened
            buttonHeight: root.topBtnSize
            buttonWidth: root.topBtnSize
            iconSize: root.topIconSize
            cornerRadius: 6
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.menuTip || "Menu"
            onClicked: root.menuToggleRequested(rightControls.x + menuBtn.x + menuBtn.width)
        }

        Rectangle {
            visible: root.showWindowControls
            width: 1
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            color: root.borderSubtle
        }

        Row {
            visible: root.showWindowControls
            spacing: 2

            M3Button {
                iconKind: "minus"
                iconOnly: true
                ghost: true
                buttonHeight: root.topBtnSize
                buttonWidth: root.topBtnSize
                iconSize: Math.max(13, root.topIconSize - 2)
                cornerRadius: 6
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.minimizeTip || "Minimize"
                onClicked: root.minimizeRequested()
            }

            M3Button {
                iconKind: root.isMaximized ? "restore" : "max"
                iconOnly: true
                ghost: true
                buttonHeight: root.topBtnSize
                buttonWidth: root.topBtnSize
                iconSize: Math.max(13, root.topIconSize - 2)
                cornerRadius: 6
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.isMaximized ? (root.ui.restoreTip || "Restore") : (root.ui.maximizeTip || "Maximize")
                onClicked: root.toggleMaximizeRequested()
            }

            M3Button {
                iconKind: "close"
                iconOnly: true
                ghost: true
                dangerHover: true
                buttonHeight: root.topBtnSize
                buttonWidth: root.topBtnSize
                iconSize: Math.max(13, root.topIconSize - 2)
                cornerRadius: 6
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.closeTip || "Close"
                onClicked: root.closeWindowRequested()
            }
        }
    }
}
