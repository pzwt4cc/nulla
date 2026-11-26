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

    height: 44
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
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 28
        width: groupRow.implicitWidth + 2
        radius: 8
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl

        Behavior on border.color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Row {
            id: groupRow
            anchors.centerIn: parent
            spacing: 0

            M3Button {
                iconKind: "plus"
                iconOnly: true
                ghost: true
                buttonHeight: 26
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "New Tab (Ctrl+T)"
                onClicked: root.newTabRequested()
            }

            Rectangle {
                width: 1
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            M3Button {
                text: "Open"
                ghost: true
                buttonHeight: 26
                horizontalPadding: 10
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "Open File (Ctrl+O)"
                onClicked: root.openFileRequested()
            }

            Rectangle {
                width: 1
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            M3Button {
                readonly property bool isDirty: root.activeTab ? root.activeTab.modified : false
                text: "Save"
                ghost: !isDirty
                accent: isDirty
                showDot: isDirty
                buttonHeight: 26
                horizontalPadding: 10
                cornerRadius: 7
                darkMode: root.darkMode
                tooltipText: "Save File (Ctrl+S)"
                onClicked: root.saveFileRequested()
            }
        }
    }

    Row {
        id: centerTitle
        anchors.centerIn: parent
        spacing: 7
        readonly property bool dirty: root.activeTab ? root.activeTab.modified : false
        width: Math.min(24 + titleLabel.implicitWidth + (dirty ? 12 : 0),
                        Math.max(80, root.width - actionGroup.width - rightControls.width - 48))

        Image {
            source: "qrc:/assets/nulla.svg"
            width: 14
            height: 14
            sourceSize: Qt.size(28, 28)
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.darkMode ? 0.88 : 0.82
        }

        Text {
            id: titleLabel
            width: Math.min(implicitWidth, centerTitle.width - 24 - (centerTitle.dirty ? 12 : 0))
            elide: Text.ElideMiddle
            text: root.activeTab ? root.activeTab.fileName : "Untitled"
            color: centerTitle.dirty ? root.fgMain : root.fgMuted
            font.pixelSize: 13
            font.weight: centerTitle.dirty ? Font.DemiBold : Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            visible: centerTitle.dirty
            width: 6
            height: 6
            radius: 3
            anchors.verticalCenter: parent.verticalCenter
            color: root.fgMain
        }
    }

    Row {
        id: rightControls
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        M3Button {
            iconKind: "search"
            iconOnly: true
            ghost: true
            buttonHeight: 28
            darkMode: root.darkMode
            tooltipText: "Find (Ctrl+F)"
            onClicked: root.findRequested()
        }

        M3Button {
            iconKind: "gear"
            iconOnly: true
            ghost: true
            buttonHeight: 28
            darkMode: root.darkMode
            tooltipText: "Settings (Ctrl+,)"
            onClicked: root.settingsRequested()
        }

        M3Button {
            id: menuBtn
            iconKind: "dots"
            iconOnly: true
            ghost: !root.menuOpened
            activeChip: root.menuOpened
            buttonHeight: 28
            darkMode: root.darkMode
            tooltipText: "Menu"
            onClicked: root.menuToggleRequested(rightControls.x + menuBtn.x + menuBtn.width)
        }

        Rectangle {
            visible: root.showWindowControls
            width: 1
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            color: root.borderSubtle
        }

        M3Button {
            visible: root.showWindowControls
            iconKind: "minus"
            iconOnly: true
            ghost: true
            buttonHeight: 28
            darkMode: root.darkMode
            tooltipText: "Minimize"
            onClicked: root.minimizeRequested()
        }

        M3Button {
            visible: root.showWindowControls
            iconKind: root.isMaximized ? "restore" : "max"
            iconOnly: true
            ghost: true
            buttonHeight: 28
            darkMode: root.darkMode
            tooltipText: root.isMaximized ? "Restore" : "Maximize"
            onClicked: root.toggleMaximizeRequested()
        }

        M3Button {
            visible: root.showWindowControls
            iconKind: "close"
            iconOnly: true
            ghost: true
            buttonHeight: 28
            darkMode: root.darkMode
            dangerHover: true
            tooltipText: "Close"
            onClicked: root.closeWindowRequested()
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
