import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var backend: null

    signal closed()

    property bool langPickerOpen: false
    property string langFilter: ""

    readonly property color bgCard: darkMode ? "#17191e" : "#ffffff"
    readonly property color bgGroup: darkMode ? "#131418" : "#f7f8fa"
    readonly property color borderGroup: darkMode ? "#262831" : "#dfe2ea"
    readonly property color borderDivider: darkMode ? "#20222a" : "#e7e9f0"
    readonly property color fgTitle: darkMode ? "#eceef2" : "#14161a"
    readonly property color fgLabel: darkMode ? "#d8dce4" : "#1f2228"
    readonly property color fgSub: darkMode ? "#787f8e" : "#6a7180"

    onOpenChanged: {
        if (!open) {
            langPickerOpen = false
            langFilter = ""
        }
    }

    visible: opacity > 0.01
    opacity: open ? 1.0 : 0.0
    anchors.fill: parent
    z: 150

    Behavior on opacity {
        NumberAnimation { duration: 95; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        color: darkMode ? "#9608090c" : "#66101216"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closed()
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 40, 428)
        height: Math.min(parent.height - 40, mainCol.implicitHeight + 36)
        anchors.centerIn: parent
        radius: 10
        color: root.bgCard
        border.width: 1
        border.color: darkMode ? "#2a2d36" : "#d4d8e2"
        scale: root.open ? 1.0 : 0.985
        clip: true

        Behavior on scale {
            NumberAnimation { duration: 95; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: mainCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 18
            spacing: 14

            Item {
                width: parent.width
                height: 30

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    M3Button {
                        visible: root.langPickerOpen
                        iconKind: "back"
                        iconOnly: true
                        ghost: true
                        darkMode: root.darkMode
                        buttonHeight: 26
                        onClicked: {
                            root.langPickerOpen = false
                            root.langFilter = ""
                        }
                    }

                    Text {
                        text: root.langPickerOpen ? "Syntax Language" : "Settings"
                        color: root.fgTitle
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                M3Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconKind: "close"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 26
                    onClicked: root.closed()
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.borderDivider
            }

            Column {
                visible: !root.langPickerOpen
                width: parent.width
                spacing: 12

                Text {
                    text: "APPEARANCE"
                    color: root.fgSub
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.8
                }

                Rectangle {
                    width: parent.width
                    height: appearanceCol.implicitHeight
                    radius: 8
                    color: root.bgGroup
                    border.width: 1
                    border.color: root.borderGroup

                    Column {
                        id: appearanceCol
                        width: parent.width
                        spacing: 0

                        Item {
                            width: parent.width
                            height: 46

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Theme"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 28
                                width: themeRow.implicitWidth + 4
                                radius: 7
                                color: darkMode ? "#1a1c22" : "#eceef3"
                                border.width: 1
                                border.color: root.borderGroup

                                Row {
                                    id: themeRow
                                    anchors.centerIn: parent
                                    spacing: 2

                                    M3Button {
                                        text: "System"
                                        ghost: !(root.backend && root.backend.themeMode === 0)
                                        activeChip: root.backend && root.backend.themeMode === 0
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        horizontalPadding: 10
                                        cornerRadius: 6
                                        onClicked: if (root.backend) root.backend.themeMode = 0
                                    }
                                    M3Button {
                                        text: "Dark"
                                        ghost: !(root.backend && root.backend.themeMode === 2)
                                        activeChip: root.backend && root.backend.themeMode === 2
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        horizontalPadding: 10
                                        cornerRadius: 6
                                        onClicked: if (root.backend) root.backend.themeMode = 2
                                    }
                                    M3Button {
                                        text: "Light"
                                        ghost: !(root.backend && root.backend.themeMode === 1)
                                        activeChip: root.backend && root.backend.themeMode === 1
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        horizontalPadding: 10
                                        cornerRadius: 6
                                        onClicked: if (root.backend) root.backend.themeMode = 1
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width - 24
                            height: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: root.borderDivider
                        }

                        Item {
                            width: parent.width
                            height: 46

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Zoom"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 28
                                width: zoomRow.implicitWidth + 4
                                radius: 7
                                color: darkMode ? "#1a1c22" : "#eceef3"
                                border.width: 1
                                border.color: root.borderGroup

                                Row {
                                    id: zoomRow
                                    anchors.centerIn: parent
                                    spacing: 0

                                    M3Button {
                                        iconKind: "minus"
                                        iconOnly: true
                                        ghost: true
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        cornerRadius: 6
                                        onClicked: if (root.backend) root.backend.zoomOut()
                                    }

                                    Rectangle {
                                        width: 1
                                        height: 14
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: root.borderDivider
                                    }

                                    M3Button {
                                        text: (root.backend ? root.backend.zoomPercent : 100) + "%"
                                        ghost: true
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        horizontalPadding: 11
                                        cornerRadius: 6
                                        tooltipText: "Reset (Ctrl+0)"
                                        onClicked: if (root.backend) root.backend.zoomReset()
                                    }

                                    Rectangle {
                                        width: 1
                                        height: 14
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: root.borderDivider
                                    }

                                    M3Button {
                                        iconKind: "plus"
                                        iconOnly: true
                                        ghost: true
                                        darkMode: root.darkMode
                                        buttonHeight: 24
                                        cornerRadius: 6
                                        onClicked: if (root.backend) root.backend.zoomIn()
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    text: "EDITOR"
                    color: root.fgSub
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.8
                }

                Rectangle {
                    width: parent.width
                    height: editorCol.implicitHeight
                    radius: 8
                    color: root.bgGroup
                    border.width: 1
                    border.color: root.borderGroup

                    Column {
                        id: editorCol
                        width: parent.width
                        spacing: 0

                        Item {
                            width: parent.width
                            height: 46

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Line numbers"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                                checked: root.backend ? root.backend.showLineNumbers : true
                                onToggled: function(v) {
                                    if (root.backend) {
                                        root.backend.showLineNumbers = v
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width - 24
                            height: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: root.borderDivider
                        }

                        Item {
                            width: parent.width
                            height: 46

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Syntax highlighting"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                                checked: root.backend ? root.backend.syntaxEnabled : true
                                onToggled: function(v) {
                                    if (root.backend) {
                                        root.backend.syntaxEnabled = v
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width - 24
                            height: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: root.borderDivider
                        }

                        Item {
                            width: parent.width
                            height: 46
                            opacity: (root.backend && root.backend.syntaxEnabled) ? 1.0 : 0.4
                            enabled: root.backend ? root.backend.syntaxEnabled : true

                            Behavior on opacity {
                                NumberAnimation { duration: 120 }
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Language"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Button {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                                buttonHeight: 28
                                horizontalPadding: 11
                                fontPixelSize: 12
                                iconKind: "chevron"
                                text: {
                                    if (!root.backend) return "Auto"
                                    var list = root.backend.languagesModel
                                    for (var i = 0; i < list.length; i++) {
                                        if (list[i].id === root.backend.languageId) {
                                            return list[i].name
                                        }
                                    }
                                    return "Auto"
                                }
                                onClicked: {
                                    root.langFilter = ""
                                    root.langPickerOpen = true
                                    langSearch.forceActiveFocus()
                                }
                            }
                        }
                    }
                }
            }

            Column {
                visible: root.langPickerOpen
                width: parent.width
                spacing: 10

                Rectangle {
                    width: parent.width
                    height: 34
                    radius: 8
                    color: root.bgGroup
                    border.width: 1
                    border.color: langSearch.activeFocus
                                  ? (darkMode ? "#6e7687" : "#687082")
                                  : root.borderGroup

                    TextInput {
                        id: langSearch
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: root.fgTitle
                        selectionColor: darkMode ? "#2a3347" : "#dbe4f5"
                        font.pixelSize: 13
                        clip: true
                        text: root.langFilter
                        onTextChanged: root.langFilter = text

                        Text {
                            visible: langSearch.text === ""
                            text: "Filter languages..."
                            color: root.fgSub
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                ListView {
                    id: langList
                    width: parent.width
                    height: 224
                    clip: true
                    spacing: 3
                    model: {
                        if (!root.backend) return []
                        var raw = root.backend.languagesModel
                        var q = root.langFilter.trim().toLowerCase()
                        if (q === "") return raw
                        var out = []
                        for (var i = 0; i < raw.length; i++) {
                            if (raw[i].name.toLowerCase().indexOf(q) !== -1) {
                                out.push(raw[i])
                            }
                        }
                        return out
                    }

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        width: 6
                        contentItem: Rectangle {
                            implicitWidth: 4
                            radius: 2
                            color: darkMode ? "#373b46" : "#b8beca"
                        }
                    }

                    delegate: M3Button {
                        width: langList.width - 8
                        buttonHeight: 30
                        text: modelData.name
                        ghost: !(root.backend && root.backend.languageId === modelData.id)
                        darkMode: root.darkMode
                        activeChip: root.backend && root.backend.languageId === modelData.id
                        onClicked: {
                            if (root.backend) {
                                root.backend.languageId = modelData.id
                            }
                            root.langPickerOpen = false
                            root.langFilter = ""
                        }
                    }
                }
            }
        }
    }
}
