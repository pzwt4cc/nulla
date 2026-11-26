import QtQuick

Item {
    id: root

    property bool checked: false
    property bool darkMode: true

    signal toggled(bool newValue)

    implicitWidth: 44
    implicitHeight: 24

    readonly property color trackBorderOff: darkMode ? "#454a57" : "#9aa1ae"
    readonly property color trackBorderOn: darkMode ? "#e6e8ec" : "#16181d"
    readonly property color trackFillOff: darkMode ? "#0affffff" : "#08000000"
    readonly property color trackFillOn: darkMode ? "#1cffffff" : "#14000000"
    readonly property color thumbOff: darkMode ? "#8b92a0" : "#5e6573"
    readonly property color thumbOn: darkMode ? "#f3f4f6" : "#16181d"

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 8
        color: root.checked ? root.trackFillOn : root.trackFillOff
        border.width: 1.5
        border.color: root.checked ? root.trackBorderOn : root.trackBorderOff
        scale: ma.pressed ? 0.97 : 1.0

        Behavior on color {
            ColorAnimation { duration: 100; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 100; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: thumb
            width: 15
            height: 15
            radius: 5
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? (parent.width - width - 5) : 5
            color: root.checked ? root.thumbOn : root.thumbOff

            Behavior on x {
                NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
            }
            Behavior on color {
                ColorAnimation { duration: 100; easing.type: Easing.OutCubic }
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
