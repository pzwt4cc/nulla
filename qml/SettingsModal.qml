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
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        color: darkMode ? "#8808090b" : "#66101216"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closed()
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 40, 440)
        height: Math.min(parent.height - 40, mainCol.implicitHeight + 44)
        anchors.centerIn: parent
        radius: 22
        color: darkMode ? "#191b20" : "#f7f8fa"
        border.width: 1
        border.color: darkMode ? "#323640" : "#cdd2dc"
        scale: root.open ? 1.0 : 0.94
        clip: true

        Behavior on scale {
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: mainCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 22
            spacing: 16

            Item {
                width: parent.width
                height: 32

                Text {
                    text: root.langPickerOpen ? "Language" : "Settings"
                    color: darkMode ? "#eef0f4" : "#16181d"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                }

                M3Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconKind: "close"
                    iconOnly: true
                    darkMode: root.darkMode
                    buttonHeight: 30
                    onClicked: {
                        if (root.langPickerOpen) {
                            root.langPickerOpen = false
                            root.langFilter = ""
                        } else {
                            root.closed()
                        }
                    }
                }
            }

            Column {
                visible: !root.langPickerOpen
                width: parent.width
                spacing: 14

                Item {
                    width: parent.width
                    height: 32

                    Text {
                        text: "Line numbers"
                        color: darkMode ? "#dfe2e8" : "#1f2228"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    M3Switch {
                        anchors.right: parent.right
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

                Item {
                    width: parent.width
                    height: 34

                    Text {
                        text: "Theme"
                        color: darkMode ? "#dfe2e8" : "#1f2228"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Row {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        M3Button {
                            text: "System"
                            darkMode: root.darkMode
                            buttonHeight: 28
                            horizontalPadding: 11
                            fontPixelSize: 12
                            activeChip: root.backend && root.backend.themeMode === 0
                            onClicked: if (root.backend) root.backend.themeMode = 0
                        }
                        M3Button {
                            text: "Dark"
                            darkMode: root.darkMode
                            buttonHeight: 28
                            horizontalPadding: 11
                            fontPixelSize: 12
                            activeChip: root.backend && root.backend.themeMode === 2
                            onClicked: if (root.backend) root.backend.themeMode = 2
                        }
                        M3Button {
                            text: "Light"
                            darkMode: root.darkMode
                            buttonHeight: 28
                            horizontalPadding: 11
                            fontPixelSize: 12
                            activeChip: root.backend && root.backend.themeMode === 1
                            onClicked: if (root.backend) root.backend.themeMode = 1
                        }
                    }
                }

                Item {
                    width: parent.width
                    height: 32

                    Text {
                        text: "Syntax highlighting"
                        color: darkMode ? "#dfe2e8" : "#1f2228"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    M3Switch {
                        anchors.right: parent.right
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

                Item {
                    width: parent.width
                    height: 34
                    opacity: (root.backend && root.backend.syntaxEnabled) ? 1.0 : 0.45
                    enabled: root.backend ? root.backend.syntaxEnabled : true

                    Behavior on opacity {
                        NumberAnimation { duration: 140 }
                    }

                    Text {
                        text: "Language"
                        color: darkMode ? "#dfe2e8" : "#1f2228"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    M3Button {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        darkMode: root.darkMode
                        buttonHeight: 30
                        horizontalPadding: 12
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

                Item {
                    width: parent.width
                    height: 34

                    Text {
                        text: "Zoom"
                        color: darkMode ? "#dfe2e8" : "#1f2228"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Row {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        M3Button {
                            iconKind: "minus"
                            iconOnly: true
                            darkMode: root.darkMode
                            buttonHeight: 28
                            onClicked: if (root.backend) root.backend.zoomOut()
                        }
                        M3Button {
                            text: (root.backend ? root.backend.zoomPercent : 100) + "%"
                            darkMode: root.darkMode
                            buttonHeight: 28
                            horizontalPadding: 12
                            fontPixelSize: 12
                            onClicked: if (root.backend) root.backend.zoomReset()
                        }
                        M3Button {
                            iconKind: "plus"
                            iconOnly: true
                            darkMode: root.darkMode
                            buttonHeight: 28
                            onClicked: if (root.backend) root.backend.zoomIn()
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
                    radius: 17
                    color: darkMode ? "#131519" : "#ffffff"
                    border.width: 1
                    border.color: langSearch.activeFocus
                                  ? (darkMode ? "#8b92a0" : "#4b5260")
                                  : (darkMode ? "#2f333d" : "#cdd2dc")

                    TextInput {
                        id: langSearch
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        verticalAlignment: TextInput.AlignVCenter
                        color: darkMode ? "#e6e8ec" : "#16181d"
                        selectionColor: darkMode ? "#323743" : "#d6dae2"
                        font.pixelSize: 13
                        clip: true
                        text: root.langFilter
                        onTextChanged: root.langFilter = text

                        Text {
                            visible: langSearch.text === ""
                            text: "Search language..."
                            color: darkMode ? "#646a76" : "#8c929e"
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                ListView {
                    id: langList
                    width: parent.width
                    height: 220
                    clip: true
                    spacing: 4
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
                        contentItem: Rectangle {
                            implicitWidth: 4
                            radius: 2
                            color: darkMode ? "#454a57" : "#a8afbd"
                        }
                    }

                    delegate: M3Button {
                        width: langList.width - 6
                        buttonHeight: 32
                        text: modelData.name
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
