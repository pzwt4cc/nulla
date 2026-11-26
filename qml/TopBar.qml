import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var activeTab: null
    property bool showWindowControls: false
    property bool isMaximized: false
    property bool menuOpened: false
    property var ui: ({})

    property color bgHeader: "#0d0e11"
    property color bgSurfaceElev: "#1c1f27"
    property color borderSubtle: "#222630"
    property color borderControl: "#313644"
    property color fgMain: "#f0f2f6"
    property color fgMuted: "#9ca3b4"

    signal moveWindowRequested
    signal toggleMaximizeRequested
    signal minimizeRequested
    signal closeWindowRequested
    signal newTabRequested
    signal openFileRequested
    signal saveFileRequested
    signal findRequested
    signal settingsRequested
    signal menuToggleRequested(real anchorX)

    height: 48
    clip: true
    color: bgHeader

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
        id: actionGroup
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 32
        width: groupRow.implicitWidth + 4
        radius: 6
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            height: 1
            color: root.theme.sheen || (root.darkMode ? "#12ffffff" : "#80ffffff")
        }

        Row {
            id: groupRow
            anchors.centerIn: parent
            spacing: 0

            M3Button {
                iconKind: "plus"
                iconOnly: true
                ghost: true
                buttonHeight: 28
                buttonWidth: 30
                iconSize: 15
                cornerRadius: 4
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.newTabTip || "New Tab (Ctrl+T)"
                onClicked: root.newTabRequested()
            }

            Rectangle {
                width: 1
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            M3Button {
                iconKind: "folder"
                text: root.ui.open || "Open"
                ghost: true
                buttonHeight: 28
                horizontalPadding: 11
                iconSize: 14
                fontPixelSize: 12
                cornerRadius: 4
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.openFileTip || "Open File (Ctrl+O)"
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
                iconKind: "save"
                text: root.ui.save || "Save"
                ghost: !isDirty
                primaryFill: isDirty
                buttonHeight: 28
                horizontalPadding: 11
                iconSize: 14
                fontPixelSize: 12
                cornerRadius: 4
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.saveFileTip || "Save File (Ctrl+S)"
                onClicked: root.saveFileRequested()
            }
        }
    }

    Item {
        id: centerBadge
        readonly property bool dirty: root.activeTab ? root.activeTab.modified : false
        readonly property real fixedContentWidth: 16 + 8 + (dirty ? 14 : 0)
        readonly property string displayTitle: (root.activeTab && root.activeTab.filePath !== "") ? root.activeTab.fileName : (root.ui.untitled || "Untitled")

        anchors.left: actionGroup.right
        anchors.leftMargin: 10
        anchors.right: rightControls.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 28
        visible: width > 12
        clip: true

        TextMetrics {
            id: titleMetrics
            font.pixelSize: 13
            font.weight: Font.DemiBold
            text: centerBadge.displayTitle
        }

        Row {
            id: titleRow
            readonly property real maxRowW: Math.max(0, centerBadge.width - 8)
            readonly property real idealW: centerBadge.fixedContentWidth + Math.ceil(titleMetrics.width)
            readonly property real rowW: Math.min(idealW, maxRowW)
            width: rowW
            height: parent.height
            x: Math.max(0, Math.min(centerBadge.width - rowW, (root.width - rowW) * 0.5 - centerBadge.x))
            spacing: 8

            Image {
                source: "qrc:/assets/nulla.svg"
                width: 16
                height: 16
                sourceSize: Qt.size(32, 32)
                anchors.verticalCenter: parent.verticalCenter
                opacity: root.darkMode ? 0.92 : 0.88
            }

            Text {
                id: titleLabel
                width: Math.max(0, Math.min(Math.ceil(titleMetrics.width), titleRow.maxRowW - centerBadge.fixedContentWidth))
                elide: Text.ElideMiddle
                text: centerBadge.displayTitle
                color: root.fgMain
                font.pixelSize: 13
                font.weight: Font.DemiBold
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
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            id: utilityGroup
            height: 32
            width: utilRow.implicitWidth + 4
            radius: 6
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                height: 1
                color: root.theme.sheen || (root.darkMode ? "#12ffffff" : "#80ffffff")
            }

            Row {
                id: utilRow
                anchors.centerIn: parent
                spacing: 0

                M3Button {
                    iconKind: "search"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 15
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.ui.findTip || "Find (Ctrl+F)"
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
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 15
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.ui.settingsTip || "Settings (Ctrl+,)"
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
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 15
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.ui.menuTip || "Menu"
                    onClicked: root.menuToggleRequested(rightControls.x + utilityGroup.x + utilRow.x + menuBtn.x + menuBtn.width)
                }
            }
        }

        Rectangle {
            visible: root.showWindowControls
            height: 32
            width: winRow.implicitWidth + 4
            radius: 6
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.rightMargin: 5
                height: 1
                color: root.theme.sheen || (root.darkMode ? "#12ffffff" : "#80ffffff")
            }

            Row {
                id: winRow
                anchors.centerIn: parent
                spacing: 0

                M3Button {
                    iconKind: "minus"
                    iconOnly: true
                    ghost: true
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 14
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.ui.minimizeTip || "Minimize"
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
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 14
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.isMaximized ? (root.ui.restoreTip || "Restore") : (root.ui.maximizeTip || "Maximize")
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
                    buttonHeight: 28
                    buttonWidth: 30
                    iconSize: 14
                    cornerRadius: 4
                    darkMode: root.darkMode
                theme: root.theme
                    dangerHover: true
                    tooltipText: root.ui.closeTip || "Close"
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
