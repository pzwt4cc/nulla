import QtQuick

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var backend: null
    property var ui: ({})

    signal closed

    readonly property color bgCard: darkMode ? "#17191e" : "#ffffff"
    readonly property color bgGroup: darkMode ? "#131418" : "#f7f8fa"
    readonly property color borderGroup: darkMode ? "#262831" : "#dfe2ea"
    readonly property color borderDivider: darkMode ? "#20222a" : "#e7e9f0"
    readonly property color fgTitle: darkMode ? "#eceef2" : "#14161a"
    readonly property color fgLabel: darkMode ? "#d8dce4" : "#1f2228"
    readonly property color fgSub: darkMode ? "#787f8e" : "#6a7180"

    visible: opacity > 0.01
    opacity: open ? 1.0 : 0.0
    anchors.fill: parent
    z: 150

    Behavior on opacity {
        NumberAnimation {
            duration: 95
            easing.type: Easing.OutCubic
        }
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
        width: Math.min(parent.width - 40, 444)
        height: Math.min(parent.height - 40, mainCol.implicitHeight + 36)
        anchors.centerIn: parent
        radius: 12
        color: root.bgCard
        border.width: 1
        border.color: darkMode ? "#2c303a" : "#cfd4df"
        scale: root.open ? 1.0 : 0.985
        clip: true

        Behavior on scale {
            NumberAnimation {
                duration: 95
                easing.type: Easing.OutCubic
            }
        }

        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: 1
            color: darkMode ? "#14ffffff" : "#90ffffff"
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

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.settingsTitle || "Settings"
                    color: root.fgTitle
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                M3Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconKind: "close"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 28
                    cornerRadius: 9
                    onClicked: root.closed()
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.borderDivider
            }

            Column {
                width: parent.width
                spacing: 12

                Text {
                    text: root.ui.secAppearance || "APPEARANCE"
                    color: root.fgSub
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.8
                }

                Rectangle {
                    width: parent.width
                    height: appearanceCol.implicitHeight
                    radius: 10
                    color: root.bgGroup
                    border.width: 1
                    border.color: root.borderGroup

                    Column {
                        id: appearanceCol
                        width: parent.width
                        spacing: 0

                        Item {
                            width: parent.width
                            height: 48

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.ui.lblTheme || "Theme"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 30
                                width: themeRow.implicitWidth + 4
                                radius: 10
                                color: darkMode ? "#1a1c22" : "#eceef3"
                                border.width: 1
                                border.color: root.borderGroup

                                Row {
                                    id: themeRow
                                    anchors.centerIn: parent
                                    spacing: 2

                                    M3Button {
                                        text: root.ui.themeSystem || "System"
                                        ghost: !(root.backend && root.backend.themeMode === 0)
                                        activeChip: root.backend && root.backend.themeMode === 0
                                        darkMode: root.darkMode
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 0
                                    }
                                    M3Button {
                                        text: root.ui.themeDark || "Dark"
                                        ghost: !(root.backend && root.backend.themeMode === 2)
                                        activeChip: root.backend && root.backend.themeMode === 2
                                        darkMode: root.darkMode
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 2
                                    }
                                    M3Button {
                                        text: root.ui.themeLight || "Light"
                                        ghost: !(root.backend && root.backend.themeMode === 1)
                                        activeChip: root.backend && root.backend.themeMode === 1
                                        darkMode: root.darkMode
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 1
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
                            height: 48

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.ui.lblUiLanguage || "Language"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 30
                                width: uiLangRow.implicitWidth + 4
                                radius: 10
                                color: darkMode ? "#1a1c22" : "#eceef3"
                                border.width: 1
                                border.color: root.borderGroup

                                Row {
                                    id: uiLangRow
                                    anchors.centerIn: parent
                                    spacing: 2

                                    Repeater {
                                        model: root.backend ? root.backend.uiLanguagesModel : []

                                        delegate: M3Button {
                                            text: modelData.name
                                            ghost: !(root.backend && root.backend.uiLanguage === modelData.id)
                                            activeChip: root.backend && root.backend.uiLanguage === modelData.id
                                            darkMode: root.darkMode
                                            buttonHeight: 26
                                            horizontalPadding: 11
                                            cornerRadius: 9
                                            onClicked: if (root.backend)
                                                root.backend.uiLanguage = modelData.id
                                        }
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
                            height: 48

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.ui.lblZoom || "Zoom"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 30
                                width: zoomRow.implicitWidth + 4
                                radius: 10
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
                                        buttonHeight: 26
                                        buttonWidth: 28
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.zoomOut()
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
                                        buttonHeight: 26
                                        horizontalPadding: 12
                                        cornerRadius: 9
                                        tooltipText: root.ui.resetZoomTip || "Reset (Ctrl+0)"
                                        onClicked: if (root.backend)
                                            root.backend.zoomReset()
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
                                        buttonHeight: 26
                                        buttonWidth: 28
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.zoomIn()
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    text: root.ui.secEditor || "EDITOR"
                    color: root.fgSub
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.8
                }

                Rectangle {
                    width: parent.width
                    height: editorCol.implicitHeight
                    radius: 10
                    color: root.bgGroup
                    border.width: 1
                    border.color: root.borderGroup

                    Column {
                        id: editorCol
                        width: parent.width
                        spacing: 0

                        Item {
                            width: parent.width
                            height: 48

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.ui.lblLineNumbers || "Line numbers"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                                checked: root.backend ? root.backend.showLineNumbers : true
                                onToggled: function (v) {
                                    if (root.backend) {
                                        root.backend.showLineNumbers = v;
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
                            height: 48

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.ui.lblSyntax || "Syntax highlighting"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                                checked: root.backend ? root.backend.syntaxEnabled : true
                                onToggled: function (v) {
                                    if (root.backend) {
                                        root.backend.syntaxEnabled = v;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
