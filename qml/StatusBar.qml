import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var activeTab: null
    property int zoomPercent: 100
    property int selectedChars: 0
    property var ui: ({})
    property color bgShell: "#0d0e11"
    property color bgSurfaceElev: "#1c1f27"
    property color borderSubtle: "#222630"
    property color borderControl: "#313644"
    property color fgMain: "#f0f2f6"
    property color fgMuted: "#9ca3b4"
    property color fgGutter: "#565d6e"

    signal gotoLineClicked
    signal languageClicked
    signal zoomInRequested
    signal zoomOutRequested
    signal zoomResetRequested

    height: 36
    color: bgShell

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.top: parent.top
        color: root.borderSubtle
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 26
        width: leftInfoRow.implicitWidth + 4
        radius: 6
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            height: 1
            color: root.darkMode ? "#12ffffff" : "#80ffffff"
        }

        Row {
            id: leftInfoRow
            anchors.centerIn: parent
            spacing: 0

            M3Button {
                anchors.verticalCenter: parent.verticalCenter
                text: (root.ui.statusLn || "Ln") + " " + (root.activeTab ? root.activeTab.cursorLine : 1) + ", " + (root.ui.statusCol || "Col") + " " + (root.activeTab ? root.activeTab.cursorColumn : 1) + (root.selectedChars > 0 ? (" · " + root.selectedChars + " " + (root.ui.statusSel || "sel")) : "")
                ghost: true
                darkMode: root.darkMode
                buttonHeight: 22
                horizontalPadding: 9
                fontPixelSize: 12
                cornerRadius: 4
                tooltipText: root.ui.statusGotoTip || "Go to Line (Ctrl+G)"
                onClicked: root.gotoLineClicked()
            }

            Rectangle {
                width: 1
                height: 12
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                readonly property int lines: root.activeTab ? root.activeTab.lineCount : 1
                text: lines + " " + (lines === 1 ? (root.ui.statusLineOne || "line") : (root.ui.statusLineMany || "lines"))
                color: root.fgMuted
                font.pixelSize: 12
                leftPadding: 9
                rightPadding: 9
            }

            Rectangle {
                visible: root.activeTab ? root.activeTab.modified : false
                width: 1
                height: 12
                anchors.verticalCenter: parent.verticalCenter
                color: root.borderSubtle
            }

            Row {
                visible: root.activeTab ? root.activeTab.modified : false
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                leftPadding: 9
                rightPadding: 9

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.fgMain
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.statusUnsaved || "Unsaved"
                    color: root.fgMain
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            text: root.activeTab ? (root.activeTab.statusLanguage === "Plain" ? (root.ui.langPlain || "Plain") : root.activeTab.statusLanguage) : (root.ui.langPlain || "Plain")
            iconKind: "chevron"
            iconOnRight: true
            iconSize: 12
            elevated: true
            darkMode: root.darkMode
            buttonHeight: 26
            horizontalPadding: 11
            fontPixelSize: 12
            cornerRadius: 6
            tooltipText: root.ui.statusLangTip || "Change syntax language"
            onClicked: root.languageClicked()
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            height: 26
            width: zoomPillRow.implicitWidth + 4
            radius: 6
            color: root.bgSurfaceElev
            border.width: 1
            border.color: root.borderControl

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 5
                anchors.rightMargin: 5
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
                    buttonHeight: 22
                    buttonWidth: 24
                    iconSize: 12
                    cornerRadius: 4
                    tooltipText: root.ui.statusZoomOutTip || "Zoom Out (Ctrl+-)"
                    onClicked: root.zoomOutRequested()
                }

                Rectangle {
                    width: 1
                    height: 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    text: root.zoomPercent + "%"
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 22
                    horizontalPadding: 9
                    fontPixelSize: 12
                    cornerRadius: 4
                    tooltipText: root.ui.statusZoomResetTip || "Reset Zoom (Ctrl+0)"
                    onClicked: root.zoomResetRequested()
                }

                Rectangle {
                    width: 1
                    height: 12
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.borderSubtle
                }

                M3Button {
                    iconKind: "plus"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    buttonHeight: 22
                    buttonWidth: 24
                    iconSize: 12
                    cornerRadius: 4
                    tooltipText: root.ui.statusZoomInTip || "Zoom In (Ctrl++)"
                    onClicked: root.zoomInRequested()
                }
            }
        }
    }
}
