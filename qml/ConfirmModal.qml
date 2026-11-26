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
        NumberAnimation { duration: 95; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        color: darkMode ? "#9608090c" : "#66101216"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.cancelled()
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 48, 396)
        height: contentCol.implicitHeight + 40
        anchors.centerIn: parent
        radius: 10
        color: darkMode ? "#17191e" : "#ffffff"
        border.width: 1
        border.color: darkMode ? "#2a2d36" : "#d4d8e2"
        scale: root.open ? 1.0 : 0.985

        Behavior on scale {
            NumberAnimation { duration: 95; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: contentCol
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            Text {
                text: root.title
                color: darkMode ? "#eceef2" : "#14161a"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Text {
                text: root.message
                color: darkMode ? "#8f96a4" : "#5e6573"
                font.pixelSize: 13
                lineHeight: 1.35
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Item { width: 1; height: 8 }

            Row {
                anchors.right: parent.right
                spacing: 8

                M3Button {
                    visible: !root.errorOnly
                    text: "Cancel"
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 30
                    horizontalPadding: 12
                    onClicked: root.cancelled()
                }

                M3Button {
                    visible: !root.errorOnly
                    text: "Discard"
                    darkMode: root.darkMode
                    dangerHover: true
                    buttonHeight: 30
                    horizontalPadding: 12
                    onClicked: root.discarded()
                }

                M3Button {
                    text: root.errorOnly ? "OK" : "Save"
                    darkMode: root.darkMode
                    primaryFill: true
                    buttonHeight: 30
                    horizontalPadding: 14
                    onClicked: root.errorOnly ? root.cancelled() : root.saved()
                }
            }
        }
    }
}
