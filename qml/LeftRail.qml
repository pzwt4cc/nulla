import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var activeTab: null
    property bool sidebarOpen: true
    property bool folderOpen: false
    property bool settingsOpen: false
    property int iconSize: 18
    property var ui: ({})

    property color bgShell: "#101012"
    property color borderSubtle: "#242429"
    property color fgMain: "#f2f2f5"

    signal toggleSidebarRequested
    signal newTabRequested
    signal openFileRequested
    signal openFolderRequested
    signal saveFileRequested
    signal settingsRequested

    readonly property int btnSize: Math.max(30, iconSize + 14)

    width: Math.max(42, iconSize + 26)
    color: bgShell

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        width: 1
        height: parent.height
        anchors.right: parent.right
        color: root.borderSubtle
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Math.max(5, Math.round(root.iconSize * 0.3))

        M3Button {
            iconKind: "sidebar"
            iconOnly: true
            ghost: !root.sidebarOpen
            activeChip: root.sidebarOpen
            buttonHeight: root.btnSize
            buttonWidth: root.btnSize
            iconSize: root.iconSize
            cornerRadius: 7
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.toggleSidebarTip || "Open Files & Explorer (Ctrl+B)"
            onClicked: root.toggleSidebarRequested()
        }

        M3Button {
            iconKind: "newfile"
            iconOnly: true
            ghost: true
            buttonHeight: root.btnSize
            buttonWidth: root.btnSize
            iconSize: root.iconSize
            cornerRadius: 7
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.newTabTip || "New Tab (Ctrl+T)"
            onClicked: root.newTabRequested()
        }

        M3Button {
            iconKind: "openfile"
            iconOnly: true
            ghost: true
            buttonHeight: root.btnSize
            buttonWidth: root.btnSize
            iconSize: root.iconSize
            cornerRadius: 7
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.openFileTip || "Open File (Ctrl+O)"
            onClicked: root.openFileRequested()
        }

        M3Button {
            iconKind: "folder"
            iconOnly: true
            ghost: !root.folderOpen
            activeChip: root.folderOpen
            buttonHeight: root.btnSize
            buttonWidth: root.btnSize
            iconSize: root.iconSize
            cornerRadius: 7
            darkMode: root.darkMode
            theme: root.theme
            tooltipText: root.ui.openFolderTip || "Open Folder (Ctrl+Shift+O)"
            onClicked: root.openFolderRequested()
        }

        Item {
            width: root.btnSize
            height: root.btnSize
            readonly property bool isDirty: root.activeTab ? root.activeTab.modified : false

            M3Button {
                anchors.fill: parent
                iconKind: "save"
                iconOnly: true
                ghost: !parent.isDirty
                primaryFill: parent.isDirty
                buttonHeight: root.btnSize
                buttonWidth: root.btnSize
                iconSize: root.iconSize
                cornerRadius: 7
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.saveFileTip || "Save File (Ctrl+S)"
                onClicked: root.saveFileRequested()
            }

            Rectangle {
                visible: parent.isDirty
                width: 6
                height: 6
                radius: 3
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: -1
                anchors.rightMargin: -1
                color: root.fgMain
                border.width: 1
                border.color: root.bgShell
            }
        }
    }

    M3Button {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        iconKind: "gear"
        iconOnly: true
        ghost: !root.settingsOpen
        activeChip: root.settingsOpen
        buttonHeight: root.btnSize
        buttonWidth: root.btnSize
        iconSize: root.iconSize
        cornerRadius: 7
        darkMode: root.darkMode
        theme: root.theme
        tooltipText: root.ui.settingsTip || "Settings (Ctrl+,)"
        onClicked: root.settingsRequested()
    }
}
