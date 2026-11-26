import QtQuick

Item {
    id: root

    property bool checked: false
    property bool darkMode: true

    signal toggled(bool newValue)

    implicitWidth: 44
    implicitHeight: 24

    readonly property color trackBorderOff: darkMode ? (ma.containsMouse ? "#565d6e" : "#383c47") : (ma.containsMouse ? "#788092" : "#b5bac6")
    readonly property color trackBorderOn: darkMode ? "#f0f2f6" : "#14161a"
    readonly property color trackFillOff: darkMode ? "#131418" : "#eceef2"
    readonly property color trackFillOn: darkMode ? "#f0f2f6" : "#14161a"
    readonly property color thumbOff: darkMode ? (ma.containsMouse ? "#a8b0bf" : "#7f8694") : (ma.containsMouse ? "#454b57" : "#6b7280")
    readonly property color thumbOn: darkMode ? "#111215" : "#ffffff"

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 10
        color: root.checked ? root.trackFillOn : root.trackFillOff
        border.width: 1.25
        border.color: root.checked ? root.trackBorderOn : root.trackBorderOff
        scale: ma.pressed ? 0.95 : 1.0

        Behavior on color {
            ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 75; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: thumb
            width: 16
            height: 16
            radius: 7
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? (parent.width - width - 4) : 4
            color: root.checked ? root.thumbOn : root.thumbOff

            Behavior on x {
                NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
            }
            Behavior on color {
                ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
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
