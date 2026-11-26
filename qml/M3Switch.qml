import QtQuick

Item {
    id: root

    property bool checked: false
    property bool darkMode: true

    signal toggled(bool newValue)

    implicitWidth: 40
    implicitHeight: 22

    readonly property color trackBorderOff: darkMode ? "#383c47" : "#b5bac6"
    readonly property color trackBorderOn: darkMode ? "#e6e8ec" : "#14161a"
    readonly property color trackFillOff: darkMode ? "#131418" : "#eceef2"
    readonly property color trackFillOn: darkMode ? "#e6e8ec" : "#14161a"
    readonly property color thumbOff: darkMode ? "#7f8694" : "#6b7280"
    readonly property color thumbOn: darkMode ? "#111215" : "#ffffff"

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 7
        color: root.checked ? root.trackFillOn : root.trackFillOff
        border.width: 1.25
        border.color: root.checked ? root.trackBorderOn : root.trackBorderOff
        scale: ma.pressed ? 0.96 : 1.0

        Behavior on color {
            ColorAnimation { duration: 95; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 95; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 60; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: thumb
            width: 14
            height: 14
            radius: 4
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? (parent.width - width - 4) : 4
            color: root.checked ? root.thumbOn : root.thumbOff

            Behavior on x {
                NumberAnimation { duration: 105; easing.type: Easing.OutCubic }
            }
            Behavior on color {
                ColorAnimation { duration: 95; easing.type: Easing.OutCubic }
            }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
    }
}
