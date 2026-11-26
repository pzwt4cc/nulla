import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var activeTab: null
    property int zoomPercent: 100
    property color bgShell: "#111215"
    property color bgSurfaceElev: "#181a20"
    property color borderSubtle: "#22242c"
    property color borderControl: "#2c2f38"
    property color fgMain: "#eceef2"
    property color fgMuted: "#8c92a0"
    property color fgGutter: "#4a4f5c"

    signal gotoLineClicked()
    signal languageClicked()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()

    height: 38
    color: bgShell

    Behavior on color {
        ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.top: parent.top
        color: root.borderSubtle
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        height: 28
        width: leftInfoRow.implicitWidth + 4
        radius: 8
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl

        Behavior on color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 7
            anchors.rightMargin: 7
            height: 1
            color: root.darkMode ? "#12ffffff" : "#80ffffff"
        }

        Row {
            id: leftInfoRow
            anchors.centerIn: parent
            spacing: 0

            M3Button {
                anchors.verticalCenter: parent.verticalCenter
                text: "Ln " + (root.activeTab ? root.activeTab.cursorLine : 1) +
                      ", Col " + (root.activeTab ? root.activeTab.cursorColumn : 1)
                ghost: true
                darkMode: root.darkMode
                buttonHeight: 24
                horizontalPadding: 10
                fontPixelSize: 12
                cornerRadius: 6
                tooltipText: "Go to Line (Ctrl+G)"
                onClicked: root.gotoLineClicked()
            }

            Rectangle {
                width: 1
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                readonly property int lines: root.activeTab ? root.activeTab.lineCount : 1
                text: lines + (lines === 1 ? " line" : " lines")
                color: root.fgMuted
                font.pixelSize: 12
                leftPadding: 10
                rightPadding: 10
            }

            Rectangle {
                visible: root.activeTab ? root.activeTab.modified : false
                width: 1
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            Row {
                visible: root.activeTab ? root.activeTab.modified : false
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                leftPadding: 10
                rightPadding: 10

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.fgMain
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Unsaved"
                    color: root.fgMain
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            text: root.activeTab ? root.activeTab.statusLanguage : "Plain Text"
            iconKind: "chevron"
            iconOnRight: true
            iconSize: 12
            elevated: true
            darkMode: root.darkMode
            buttonHeight: 28
            horizontalPadding: 12
            fontPixelSize: 12
            cornerRadius: 8
            tooltipText: "Change syntax language"
            onClicked: root.languageClicked()
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            height: 28
            width: zoomPillRow.implicitWidth + 4
            radius: 8
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }
            Behavior on border.color {
                ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                height: 1
                color: root.darkMode ? "#12ffffff" : "#80ffffff"
            }

            Row {
                id: zoomPillRow
                anchors.centerIn: parent
                spacing: 0

                M3Button {
                    iconKind: "minus"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 24
                    buttonWidth: 26
                    iconSize: 12
                    cornerRadius: 6
                    tooltipText: "Zoom Out (Ctrl+-)"
                    onClicked: root.zoomOutRequested()
                }

                Rectangle {
                    width: 1
                    height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    text: root.zoomPercent + "%"
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 24
                    horizontalPadding: 10
                    fontPixelSize: 12
                    cornerRadius: 6
                    tooltipText: "Reset Zoom (Ctrl+0)"
                    onClicked: root.zoomResetRequested()
                }

                Rectangle {
                    width: 1
                    height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    iconKind: "plus"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 24
                    buttonWidth: 26
                    iconSize: 12
                    cornerRadius: 6
                    tooltipText: "Zoom In (Ctrl++)"
                    onClicked: root.zoomInRequested()
                }
            }
        }
    }
}
