import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var activeTab: null
    property bool showWindowControls: false
    property bool isMaximized: false
    property bool menuOpened: false

    property color bgHeader: "#111215"
    property color bgSurfaceElev: "#181a20"
    property color borderSubtle: "#22242c"
    property color borderControl: "#2c2f38"
    property color fgMain: "#eceef2"
    property color fgMuted: "#8c92a0"

    signal moveWindowRequested()
    signal toggleMaximizeRequested()
    signal minimizeRequested()
    signal closeWindowRequested()
    signal newTabRequested()
    signal openFileRequested()
    signal saveFileRequested()
    signal findRequested()
    signal settingsRequested()
    signal menuToggleRequested(real anchorX)

    height: 50
    color: bgHeader

    Behavior on color {
        ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: root.moveWindowRequested()
        onDoubleClicked: root.toggleMaximizeRequested()
    }

    Rectangle {
        id: actionGroup
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        height: 34
        width: groupRow.implicitWidth + 4
        radius: 10
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl

        Behavior on color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 9
            anchors.rightMargin: 9
            height: 1
            color: root.darkMode ? "#12ffffff" : "#80ffffff"
        }

        Row {
            id: groupRow
            anchors.centerIn: parent
            spacing: 0

            M3Button {
                iconKind: "plus"
                iconOnly: true
                ghost: true
                buttonHeight: 30
                buttonWidth: 32
                iconSize: 15
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "New Tab (Ctrl+T)"
                onClicked: root.newTabRequested()
            }

            Rectangle {
                width: 1
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            M3Button {
                text: "Open"
                ghost: true
                buttonHeight: 30
                horizontalPadding: 13
                fontPixelSize: 13
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "Open File (Ctrl+O)"
                onClicked: root.openFileRequested()
            }

            Rectangle {
                width: 1
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            M3Button {
                readonly property bool isDirty: root.activeTab ? root.activeTab.modified : false
                text: "Save"
                ghost: !isDirty
                accent: isDirty
                showDot: isDirty
                buttonHeight: 30
                horizontalPadding: 13
                fontPixelSize: 13
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "Save File (Ctrl+S)"
                onClicked: root.saveFileRequested()
            }
        }
    }

    Rectangle {
        id: centerBadge
        readonly property bool dirty: root.activeTab ? root.activeTab.modified : false
        readonly property real fixedContentWidth: 15 + 8 + (dirty ? 14 : 0)
        readonly property real availableSpan: Math.max(64, root.width - actionGroup.width - rightControls.width - 44)
        readonly property real naturalWidth: 24 + fixedContentWidth + titleLabel.implicitWidth

        y: (parent.height - height) * 0.5
        x: Math.max(actionGroup.x + actionGroup.width + 10,
                    Math.min(rightControls.x - width - 10, (root.width - width) * 0.5))
        height: 32
        width: Math.min(naturalWidth, availableSpan)
        radius: 9
        clip: true
        color: root.bgSurfaceElev
        border.width: 1
        border.color: dirty ? root.borderControl : root.borderSubtle

        Behavior on color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 9
            anchors.rightMargin: 9
            height: 1
            color: root.darkMode ? "#10ffffff" : "#80ffffff"
        }

        Row {
            id: titleRow
            anchors.centerIn: parent
            spacing: 8

            Image {
                source: "qrc:/assets/nulla.svg"
                width: 15
                height: 15
                sourceSize: Qt.size(30, 30)
                anchors.verticalCenter: parent.verticalCenter
                opacity: root.darkMode ? 0.90 : 0.85
            }

            Text {
                id: titleLabel
                width: Math.max(12, Math.min(implicitWidth, centerBadge.width - 24 - centerBadge.fixedContentWidth))
                elide: Text.ElideMiddle
                text: root.activeTab ? root.activeTab.fileName : "Untitled"
                color: centerBadge.dirty ? root.fgMain : root.fgMuted
                font.pixelSize: 13
                font.weight: centerBadge.dirty ? Font.DemiBold : Font.Medium
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                visible: centerBadge.dirty
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.fgMain
            }
        }
    }

    Row {
        id: rightControls
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            id: utilityGroup
            height: 34
            width: utilRow.implicitWidth + 4
            radius: 9
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }
            Behavior on border.color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 9
                anchors.rightMargin: 9
                height: 1
                color: root.darkMode ? "#12ffffff" : "#80ffffff"
            }

            Row {
                id: utilRow
                anchors.centerIn: parent
                spacing: 0

                M3Button {
                    iconKind: "search"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 15
                    cornerRadius: 7
                    darkMode: root.darkMode
                    tooltipText: "Find (Ctrl+F)"
                    onClicked: root.findRequested()
                }

                Rectangle {
                    width: 1
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    iconKind: "gear"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 15
                    cornerRadius: 7
                    darkMode: root.darkMode
                    tooltipText: "Settings (Ctrl+,)"
                    onClicked: root.settingsRequested()
                }

                Rectangle {
                    width: 1
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    id: menuBtn
                    iconKind: "dots"
                    iconOnly: true
                    ghost: !root.menuOpened
                    activeChip: root.menuOpened
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 15
                    cornerRadius: 7
                    darkMode: root.darkMode
                    tooltipText: "Menu"
                    onClicked: root.menuToggleRequested(rightControls.x + utilityGroup.x + utilRow.x + menuBtn.x + menuBtn.width)
                }
            }
        }

        Rectangle {
            visible: root.showWindowControls
            height: 34
            width: winRow.implicitWidth + 4
            radius: 9
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }
            Behavior on border.color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 9
                anchors.rightMargin: 9
                height: 1
                color: root.darkMode ? "#12ffffff" : "#80ffffff"
            }

            Row {
                id: winRow
                anchors.centerIn: parent
                spacing: 0

                M3Button {
                    iconKind: "minus"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 14
                    cornerRadius: 7
                    darkMode: root.darkMode
                    tooltipText: "Minimize"
                    onClicked: root.minimizeRequested()
                }

                Rectangle {
                    width: 1
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    iconKind: root.isMaximized ? "restore" : "max"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 14
                    cornerRadius: 7
                    darkMode: root.darkMode
                    tooltipText: root.isMaximized ? "Restore" : "Maximize"
                    onClicked: root.toggleMaximizeRequested()
                }

                Rectangle {
                    width: 1
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    iconKind: "close"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 30
                    buttonWidth: 32
                    iconSize: 14
                    cornerRadius: 7
                    darkMode: root.darkMode
                    dangerHover: true
                    tooltipText: "Close"
                    onClicked: root.closeWindowRequested()
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
