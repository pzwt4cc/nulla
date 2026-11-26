import QtQuick

Item {
    id: root

    property bool checked: false
    property bool darkMode: true

    signal toggled(bool newValue)

    implicitWidth: 48
    implicitHeight: 28

    readonly property color trackBorderOff: darkMode ? "#454a57" : "#9aa1ae"
    readonly property color trackBorderOn: darkMode ? "#e6e8ec" : "#16181d"
    readonly property color trackFillOff: darkMode ? "#0affffff" : "#08000000"
    readonly property color trackFillOn: darkMode ? "#1cffffff" : "#14000000"
    readonly property color thumbOff: darkMode ? "#8b92a0" : "#5e6573"
    readonly property color thumbOn: darkMode ? "#f3f4f6" : "#16181d"

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? root.trackFillOn : root.trackFillOff
        border.width: 1.5
        border.color: root.checked ? root.trackBorderOn : root.trackBorderOff
        scale: ma.pressed ? 0.92 : (ma.containsMouse ? 1.03 : 1.0)

        Behavior on color {
            ColorAnimation { duration: 160; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 160; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: thumb
            width: root.checked ? 18 : 13
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? (parent.width - width - 5) : 7
            color: root.checked ? root.thumbOn : root.thumbOff

            Behavior on x {
                NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
            }
            Behavior on width {
                NumberAnimation { duration: 170; easing.type: Easing.OutCubic }
            }
            Behavior on color {
                ColorAnimation { duration: 160; easing.type: Easing.OutCubic }
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
