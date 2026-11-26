import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var backend: null

    signal closed()

    property string langFilter: ""

    readonly property color bgCard: darkMode ? "#17191e" : "#ffffff"
    readonly property color bgGroup: darkMode ? "#131418" : "#f7f8fa"
    readonly property color borderGroup: darkMode ? "#262831" : "#dfe2ea"
    readonly property color borderDivider: darkMode ? "#20222a" : "#e7e9f0"
    readonly property color fgTitle: darkMode ? "#eceef2" : "#14161a"
    readonly property color fgSub: darkMode ? "#787f8e" : "#6a7180"

    onOpenChanged: {
        if (open) {
            langFilter = ""
            Qt.callLater(function() { langSearch.forceActiveFocus() })
        } else {
            langFilter = ""
        }
    }

    visible: opacity > 0.01
    opacity: open ? 1.0 : 0.0
    anchors.fill: parent
    z: 160

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
        width: Math.min(parent.width - 40, 380)
        height: Math.min(parent.height - 40, mainCol.implicitHeight + 36)
        anchors.centerIn: parent
        radius: 12
        color: root.bgCard
        border.width: 1
        border.color: darkMode ? "#2c303a" : "#cfd4df"
        scale: root.open ? 1.0 : 0.985
        clip: true

        Behavior on scale {
            NumberAnimation { duration: 95; easing.type: Easing.OutCubic }
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
            spacing: 12

            Item {
                width: parent.width
                height: 30

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Syntax Language"
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

            Rectangle {
                width: parent.width
                height: 36
                radius: 10
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
                    Keys.onEscapePressed: root.closed()

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
                height: 248
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
                    width: 6
                    contentItem: Rectangle {
                        implicitWidth: 4
                        radius: 2
                        color: darkMode ? "#373b46" : "#b8beca"
                    }
                }

                delegate: M3Button {
                    width: langList.width - 8
                    buttonHeight: 32
                    cornerRadius: 9
                    fontPixelSize: 13
                    text: modelData.name
                    ghost: !(root.backend && root.backend.languageId === modelData.id)
                    darkMode: root.darkMode
                    activeChip: root.backend && root.backend.languageId === modelData.id
                    onClicked: {
                        if (root.backend) {
                            root.backend.languageId = modelData.id
                        }
                        root.closed()
                    }
                }
            }
        }
    }
}
