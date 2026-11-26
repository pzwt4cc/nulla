import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var tabModel: null
    property int activeIndex: 0
    property color bgShell: "#111215"
    property color bgEditor: "#16171b"
    property color bgHover: "#20222a"
    property color borderSubtle: "#22242c"
    property color borderControl: "#2c2f38"
    property color borderHover: "#484e5e"
    property color fgMain: "#eceef2"
    property color fgMuted: "#8c92a0"

    signal tabActivated(int index)
    signal tabClosed(int index)
    signal newTabRequested()

    height: 32
    color: bgShell

    Behavior on color {
        ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
    }

    Flickable {
        id: tabFlick
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: addTabBtn.left
        anchors.rightMargin: 6
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentWidth: tabRow.implicitWidth
        contentHeight: height
        clip: true
        interactive: false
        boundsBehavior: Flickable.StopAtBounds

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            onWheel: function(wheel) {
                var d = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.angleDelta.x
                if (d !== 0) {
                    var maxX = Math.max(0, tabFlick.contentWidth - tabFlick.width)
                    tabFlick.contentX = Math.max(0, Math.min(maxX, tabFlick.contentX - (d / 120.0) * 90))
                }
                wheel.accepted = true
            }
        }

        Row {
            id: tabRow
            height: parent.height
            spacing: 4

            Repeater {
                model: root.tabModel

                delegate: Item {
                    id: tabItem
                    readonly property bool isActive: index === root.activeIndex
                    readonly property bool hovered: tabMa.containsMouse || closeBtn.hovered
                    readonly property bool isDirty: session ? session.modified : false

                    height: 24
                    width: Math.min(196, Math.max(96, labelText.implicitWidth + 46))
                    anchors.verticalCenter: parent.verticalCenter

                    onIsActiveChanged: {
                        if (isActive) {
                            if (x < tabFlick.contentX) {
                                tabFlick.contentX = x
                            } else if (x + width > tabFlick.contentX + tabFlick.width) {
                                tabFlick.contentX = Math.max(0, x + width - tabFlick.width)
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 7
                        color: tabItem.isActive
                               ? root.bgEditor
                               : (tabItem.hovered ? root.bgHover : "transparent")
                        border.width: 1
                        border.color: tabItem.isActive
                                      ? root.borderControl
                                      : (tabItem.hovered ? root.borderHover : "transparent")

                        Behavior on color {
                            ColorAnimation { duration: 105; easing.type: Easing.OutCubic }
                        }
                        Behavior on border.color {
                            ColorAnimation { duration: 105; easing.type: Easing.OutCubic }
                        }
                    }

                    MouseArea {
                        id: tabMa
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.MiddleButton) {
                                root.tabClosed(index)
                            } else {
                                root.tabActivated(index)
                            }
                        }
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.right: closeBtn.left
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        Rectangle {
                            visible: tabItem.isDirty
                            width: 5
                            height: 5
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.fgMain
                        }

                        Text {
                            id: labelText
                            width: Math.max(20, parent.width - (tabItem.isDirty ? 11 : 0))
                            elide: Text.ElideMiddle
                            text: session ? session.fileName : "Untitled"
                            color: tabItem.isActive || tabItem.hovered ? root.fgMain : root.fgMuted
                            font.pixelSize: 12
                            font.weight: tabItem.isActive ? Font.Medium : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter

                            Behavior on color {
                                ColorAnimation { duration: 95 }
                            }
                        }
                    }

                    M3Button {
                        id: closeBtn
                        anchors.right: parent.right
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: (tabItem.isActive || tabItem.hovered) ? 1.0 : 0.0
                        iconKind: "close"
                        iconOnly: true
                        ghost: true
                        dangerHover: true
                        buttonHeight: 18
                        iconSize: 11
                        cornerRadius: 5
                        darkMode: root.darkMode
                        onClicked: root.tabClosed(index)

                        Behavior on opacity {
                            NumberAnimation { duration: 85 }
                        }
                    }
                }
            }
        }
    }

    M3Button {
        id: addTabBtn
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        iconKind: "plus"
        iconOnly: true
        ghost: true
        buttonHeight: 24
        iconSize: 13
        cornerRadius: 6
        darkMode: root.darkMode
        tooltipText: "New Tab (Ctrl+T)"
        onClicked: root.newTabRequested()
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
