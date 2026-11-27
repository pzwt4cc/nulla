import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var activeTab: null
    property int zoomPercent: 100
    property int selectedChars: 0
    property var ui: ({})
    property color bgShell: "#101012"
    property color borderSubtle: "#242429"
    property color fgMain: "#f2f2f5"
    property color fgMuted: "#9d9da9"
    property color fgGutter: "#5c5c68"

    signal gotoLineClicked
    signal languageClicked
    signal zoomInRequested
    signal zoomOutRequested
    signal zoomResetRequested

    implicitHeight: 26
    height: implicitHeight
    clip: true
    color: "transparent"

    Rectangle {
        width: parent.width
        height: 1
        anchors.top: parent.top
        color: root.borderSubtle
    }

    Item {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: rightRow.left
        anchors.rightMargin: 12
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        clip: true

        Row {
            id: leftRow
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            text: (root.ui.statusLn || "Ln") + " " + (root.activeTab ? root.activeTab.cursorLine : 1) + ", " + (root.ui.statusCol || "Col") + " " + (root.activeTab ? root.activeTab.cursorColumn : 1) + (root.selectedChars > 0 ? (" · " + root.selectedChars + " " + (root.ui.statusSel || "sel")) : "")
            ghost: true
            darkMode: root.darkMode
            theme: root.theme
            buttonHeight: 20
            horizontalPadding: 6
            fontPixelSize: 11
            cornerRadius: 4
            tooltipText: root.ui.statusGotoTip || "Go to Line (Ctrl+G)"
            onClicked: root.gotoLineClicked()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "·"
            color: root.fgGutter
            font.pixelSize: 11
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            readonly property int lines: root.activeTab ? root.activeTab.lineCount : 1
            text: lines + " " + (lines === 1 ? (root.ui.statusLineOne || "line") : (root.ui.statusLineMany || "lines"))
            color: root.fgMuted
            font.pixelSize: 11
        }

        Row {
            visible: root.activeTab ? root.activeTab.modified : false
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            leftPadding: 4

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "·"
                color: root.fgGutter
                font.pixelSize: 11
            }

            Rectangle {
                width: 5
                height: 5
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.fgMain
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.ui.statusUnsaved || "Unsaved"
                color: root.fgMain
                font.pixelSize: 11
                font.weight: Font.Medium
            }
        }
        }
    }

    Row {
        id: rightRow
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            text: root.activeTab ? (root.activeTab.statusLanguage === "Plain" ? (root.ui.langPlain || "Plain") : root.activeTab.statusLanguage) : (root.ui.langPlain || "Plain")
            iconKind: "chevron"
            iconOnRight: true
            iconSize: 11
            ghost: true
            darkMode: root.darkMode
            theme: root.theme
            buttonHeight: 20
            horizontalPadding: 7
            fontPixelSize: 11
            cornerRadius: 4
            tooltipText: root.ui.statusLangTip || "Change syntax language"
            onClicked: root.languageClicked()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "·"
            color: root.fgGutter
            font.pixelSize: 11
        }

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            iconKind: "minus"
            iconOnly: true
            ghost: true
            darkMode: root.darkMode
            theme: root.theme
            buttonHeight: 20
            buttonWidth: 20
            iconSize: 11
            cornerRadius: 4
            tooltipText: root.ui.statusZoomOutTip || "Zoom Out (Ctrl+-)"
            onClicked: root.zoomOutRequested()
        }

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            text: root.zoomPercent + "%"
            ghost: true
            darkMode: root.darkMode
            theme: root.theme
            buttonHeight: 20
            horizontalPadding: 6
            fontPixelSize: 11
            cornerRadius: 4
            tooltipText: root.ui.statusZoomResetTip || "Reset Zoom (Ctrl+0)"
            onClicked: root.zoomResetRequested()
        }

        M3Button {
            anchors.verticalCenter: parent.verticalCenter
            iconKind: "plus"
            iconOnly: true
            ghost: true
            darkMode: root.darkMode
            theme: root.theme
            buttonHeight: 20
            buttonWidth: 20
            iconSize: 11
            cornerRadius: 4
            tooltipText: root.ui.statusZoomInTip || "Zoom In (Ctrl++)"
            onClicked: root.zoomInRequested()
        }
    }
}
