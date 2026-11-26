import QtQuick

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property bool errorOnly: false
    property string title: ""
    property string message: ""

    signal cancelled()
    signal discarded()
    signal saved()

    visible: opacity > 0.01
    opacity: open ? 1.0 : 0.0
    anchors.fill: parent
    z: 200

    Behavior on opacity {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        color: darkMode ? "#8808090b" : "#66101216"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.cancelled()
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 48, 390)
        height: contentCol.implicitHeight + 44
        anchors.centerIn: parent
        radius: 20
        color: darkMode ? "#191b20" : "#f7f8fa"
        border.width: 1
        border.color: darkMode ? "#323640" : "#cdd2dc"
        scale: root.open ? 1.0 : 0.94

        Behavior on scale {
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: contentCol
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            Text {
                text: root.title
                color: darkMode ? "#eef0f4" : "#16181d"
                font.pixelSize: 16
                font.weight: Font.DemiBold
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Text {
                text: root.message
                color: darkMode ? "#9ca3b0" : "#555c68"
                font.pixelSize: 13
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Item { width: 1; height: 6 }

            Row {
                anchors.right: parent.right
                spacing: 8

                M3Button {
                    visible: !root.errorOnly
                    text: "Cancel"
                    darkMode: root.darkMode
                    buttonHeight: 32
                    onClicked: root.cancelled()
                }

                M3Button {
                    visible: !root.errorOnly
                    text: "Discard"
                    darkMode: root.darkMode
                    dangerHover: true
                    buttonHeight: 32
                    onClicked: root.discarded()
                }

                M3Button {
                    text: root.errorOnly ? "OK" : "Save"
                    darkMode: root.darkMode
                    accent: true
                    buttonHeight: 32
                    onClicked: root.errorOnly ? root.cancelled() : root.saved()
                }
            }
        }
    }
}
