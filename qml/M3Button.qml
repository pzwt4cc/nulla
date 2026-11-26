import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string text: ""
    property string iconKind: ""
    property bool darkMode: true
    property bool accent: false
    property bool activeChip: false
    property bool iconOnly: false
    property bool dangerHover: false
    property real buttonHeight: 30
    property real horizontalPadding: 11
    property string tooltipText: ""
    property int fontPixelSize: 13
    property int cornerRadius: 8

    signal clicked()

    readonly property bool hovered: ma.containsMouse
    readonly property bool pressed: ma.pressed

    readonly property color fgNormal: darkMode ? "#e6e8ec" : "#16181d"
    readonly property color fgMuted: darkMode ? "#b0b6c2" : "#4b5260"
    readonly property color borderNormal: darkMode ? "#323640" : "#c5cad4"
    readonly property color borderHover: darkMode ? "#6d7484" : "#6c7382"
    readonly property color borderAccent: darkMode ? "#e6e8ec" : "#16181d"
    readonly property color fillHover: darkMode ? "#1cffffff" : "#12000000"
    readonly property color fillPress: darkMode ? "#2cffffff" : "#1e000000"
    readonly property color fillAccent: darkMode ? "#14ffffff" : "#0e000000"
    readonly property color fillDanger: "#30e5534b"
    readonly property color borderDanger: "#e5534b"

    implicitHeight: buttonHeight
    implicitWidth: iconOnly ? buttonHeight : (contentRow.implicitWidth + horizontalPadding * 2)

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: root.cornerRadius
        color: {
            if (pressed) {
                return dangerHover ? fillDanger : fillPress
            }
            if (hovered) {
                return dangerHover ? fillDanger : fillHover
            }
            return (accent || activeChip) ? fillAccent : "transparent"
        }
        border.width: (accent || activeChip) ? 1.5 : 1
        border.color: {
            if (dangerHover && hovered) {
                return borderDanger
            }
            if (accent || activeChip) {
                return borderAccent
            }
            return hovered ? borderHover : borderNormal
        }

        scale: pressed ? 0.97 : 1.0
        transformOrigin: Item.Center

        Behavior on color {
            ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on scale {
            NumberAnimation { duration: 65; easing.type: Easing.OutCubic }
        }

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 6

            Canvas {
                id: iconCanvas
                visible: root.iconKind !== ""
                width: 14
                height: 14
                anchors.verticalCenter: parent.verticalCenter

                property color strokeColor: (root.dangerHover && root.hovered)
                                            ? root.borderDanger
                                            : ((root.accent || root.activeChip || root.hovered) ? root.fgNormal : root.fgMuted)

                onStrokeColorChanged: requestPaint()
                onVisibleChanged: if (visible) requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.strokeStyle = strokeColor
                    ctx.fillStyle = strokeColor
                    ctx.lineWidth = 1.5
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"

                    if (root.iconKind === "plus") {
                        ctx.beginPath()
                        ctx.moveTo(7, 2.5)
                        ctx.lineTo(7, 11.5)
                        ctx.moveTo(2.5, 7)
                        ctx.lineTo(11.5, 7)
                        ctx.stroke()
                    } else if (root.iconKind === "minus") {
                        ctx.beginPath()
                        ctx.moveTo(3, 7)
                        ctx.lineTo(11, 7)
                        ctx.stroke()
                    } else if (root.iconKind === "close") {
                        ctx.beginPath()
                        ctx.moveTo(3.5, 3.5)
                        ctx.lineTo(10.5, 10.5)
                        ctx.moveTo(10.5, 3.5)
                        ctx.lineTo(3.5, 10.5)
                        ctx.stroke()
                    } else if (root.iconKind === "max") {
                        ctx.strokeRect(3.2, 3.2, 7.6, 7.6)
                    } else if (root.iconKind === "dots") {
                        ctx.beginPath()
                        ctx.arc(3, 7, 1.15, 0, Math.PI * 2)
                        ctx.arc(7, 7, 1.15, 0, Math.PI * 2)
                        ctx.arc(11, 7, 1.15, 0, Math.PI * 2)
                        ctx.fill()
                    } else if (root.iconKind === "gear") {
                        ctx.beginPath()
                        ctx.arc(7, 7, 2.2, 0, Math.PI * 2)
                        ctx.stroke()
                        for (var i = 0; i < 6; i++) {
                            var a = i * Math.PI / 3
                            ctx.beginPath()
                            ctx.moveTo(7 + Math.cos(a) * 3.6, 7 + Math.sin(a) * 3.6)
                            ctx.lineTo(7 + Math.cos(a) * 5.2, 7 + Math.sin(a) * 5.2)
                            ctx.stroke()
                        }
                    } else if (root.iconKind === "chevron") {
                        ctx.beginPath()
                        ctx.moveTo(3.5, 5.5)
                        ctx.lineTo(7, 9)
                        ctx.lineTo(10.5, 5.5)
                        ctx.stroke()
                    }
                }
            }

            Text {
                visible: root.text !== ""
                text: root.text
                color: (root.accent || root.activeChip || root.hovered) ? root.fgNormal : root.fgMuted
                font.pixelSize: root.fontPixelSize
                font.weight: (root.accent || root.activeChip) ? Font.DemiBold : Font.Medium
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color {
                    ColorAnimation { duration: 90; easing.type: Easing.OutCubic }
                }
            }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip {
        visible: root.tooltipText !== "" && ma.containsMouse
        delay: 500
        text: root.tooltipText
        contentItem: Text {
            text: root.tooltipText
            color: root.darkMode ? "#e6e8ec" : "#16181d"
            font.pixelSize: 12
        }
        background: Rectangle {
            radius: 7
            color: root.darkMode ? "#20232a" : "#ffffff"
            border.width: 1
            border.color: root.darkMode ? "#353945" : "#d0d5de"
        }
    }
}
