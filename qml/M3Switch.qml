import QtQuick

Item {
    id: root

    property bool checked: false
    property bool darkMode: true
    property var theme: ({})

    signal toggled(bool newValue)

    implicitWidth: 44
    implicitHeight: 24

    readonly property color trackBorderOff: ma.containsMouse ? (theme.borderHover || (darkMode ? "#565d6e" : "#788092")) : (theme.borderControl || (darkMode ? "#383c47" : "#b5bac6"))
    readonly property color trackBorderOn: theme.primaryBg || (darkMode ? "#f0f2f6" : "#14161a")
    readonly property color trackFillOff: theme.bgGroup || (darkMode ? "#131418" : "#eceef2")
    readonly property color trackFillOn: theme.primaryBg || (darkMode ? "#f0f2f6" : "#14161a")
    readonly property color thumbOff: ma.containsMouse ? (theme.fgMain || (darkMode ? "#a8b0bf" : "#454b57")) : (theme.fgMuted || (darkMode ? "#7f8694" : "#6b7280"))
    readonly property color thumbOn: theme.fgOnPrimary || (darkMode ? "#111215" : "#ffffff")

    Rectangle {
        id: track
        anchors.fill: parent
        radius: 10
        color: root.checked ? root.trackFillOn : root.trackFillOff
        border.width: 1.25
        border.color: root.checked ? root.trackBorderOn : root.trackBorderOff

        Rectangle {
            id: thumb
            width: 16
            height: 16
            radius: 7
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? (parent.width - width - 4) : 4
            color: root.checked ? root.thumbOn : root.thumbOff
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
