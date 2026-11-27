import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var theme: ({})
    property var backend: null
    property var ui: ({})

    signal closed

    property string langFilter: ""

    readonly property color bgCard: theme.bgCard || (darkMode ? "#17191e" : "#ffffff")
    readonly property color bgGroup: theme.bgGroup || (darkMode ? "#131418" : "#f7f8fa")
    readonly property color borderGroup: theme.borderGroup || (darkMode ? "#262831" : "#dfe2ea")
    readonly property color borderDivider: theme.borderDivider || (darkMode ? "#20222a" : "#e7e9f0")
    readonly property color fgTitle: theme.fgTitle || (darkMode ? "#eceef2" : "#14161a")
    readonly property color fgSub: theme.fgSub || (darkMode ? "#787f8e" : "#6a7180")

    function selectFirstMatch() {
        const m = langList.model;
        if (m && m.length > 0 && root.backend) {
            root.backend.languageId = m[0].id;
            root.closed();
        }
    }

    onOpenChanged: {
        if (open) {
            langFilter = "";
            Qt.callLater(function () {
                langSearch.forceActiveFocus();
            });
        } else {
            langFilter = "";
        }
    }

    visible: open
    focus: open
    anchors.fill: parent
    z: 160

    Keys.onPressed: function (event) {
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        color: theme.scrim || (darkMode ? "#9608090c" : "#66101216")
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onClicked: root.closed()
        onWheel: function (wheel) {
            wheel.accepted = true;
        }
    }

    Rectangle {
        id: card
        width: Math.max(0, Math.min(parent.width - 40, 380))
        height: Math.max(0, Math.min(parent.height - 40, mainCol.implicitHeight + 36))
        anchors.centerIn: parent
        radius: 12
        color: root.bgCard
        border.width: 1
        border.color: theme.borderControl || (darkMode ? "#2c303a" : "#cfd4df")
        clip: true

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onWheel: function (wheel) {
                wheel.accepted = true;
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: 1
            color: theme.sheenBright || (darkMode ? "#14ffffff" : "#90ffffff")
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
                    text: root.ui.langModalTitle || "Syntax Language"
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
                    theme: root.theme
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
                border.color: langSearch.activeFocus ? (theme.borderFocus || (darkMode ? "#6e7687" : "#687082")) : root.borderGroup

                TextInput {
                    id: langSearch
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    color: root.fgTitle
                    selectionColor: theme.selBg || (darkMode ? "#2a3347" : "#dbe4f5")
                    font.pixelSize: 13
                    clip: true
                    text: root.langFilter
                    onTextChanged: root.langFilter = text
                    Keys.onEscapePressed: root.closed()
                    Keys.onReturnPressed: root.selectFirstMatch()
                    Keys.onEnterPressed: root.selectFirstMatch()

                    Text {
                        visible: langSearch.text === ""
                        text: root.ui.langFilterPlaceholder || "Filter languages..."
                        color: root.fgSub
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            ListView {
                id: langList
                width: parent.width
                height: Math.max(60, Math.min(248, root.height - 180))
                clip: true
                spacing: 4
                model: {
                    if (!root.backend)
                        return [];
                    const raw = root.backend.languagesModel;
                    const q = root.langFilter.trim().toLowerCase();
                    const out = [];
                    for (let i = 0; i < raw.length; i++) {
                        const disp = raw[i].id === "auto" ? (root.ui.langAuto || raw[i].name) : raw[i].name;
                        if (q === "" || disp.toLowerCase().indexOf(q) !== -1 || raw[i].name.toLowerCase().indexOf(q) !== -1) {
                            out.push({
                                id: raw[i].id,
                                name: disp
                            });
                        }
                    }
                    return out;
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 6
                    contentItem: Rectangle {
                        implicitWidth: 4
                        radius: 2
                        color: theme.scrollIdle || (darkMode ? "#373b46" : "#b8beca")
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
                    theme: root.theme
                    activeChip: root.backend && root.backend.languageId === modelData.id
                    onClicked: {
                        if (root.backend) {
                            root.backend.languageId = modelData.id;
                        }
                        root.closed();
                    }
                }
            }
        }
    }
}
