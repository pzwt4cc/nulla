import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var activeTab: null
    property int zoomPercent: 100
    property color bgShell: "#111215"
    property color borderSubtle: "#22242c"
    property color fgMuted: "#8c92a0"
    property color fgGutter: "#4a4f5c"

    signal languageClicked()
    signal zoomResetRequested()

    height: 28
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

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "Ln " + (root.activeTab ? root.activeTab.cursorLine : 1) +
                  ", Col " + (root.activeTab ? root.activeTab.cursorColumn : 1)
            color: root.fgMuted
            font.pixelSize: 11
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
            text: lines + (lines === 1 ? " line" : " lines")
            color: root.fgGutter
            font.pixelSize: 11
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        M3Button {
            text: root.activeTab ? root.activeTab.statusLanguage : "Plain Text"
            ghost: true
            darkMode: root.darkMode
            buttonHeight: 22
            horizontalPadding: 8
            fontPixelSize: 11
            cornerRadius: 6
            tooltipText: "Change syntax language"
            onClicked: root.languageClicked()
        }

        M3Button {
            text: root.zoomPercent + "%"
            ghost: true
            darkMode: root.darkMode
            buttonHeight: 22
            horizontalPadding: 8
            fontPixelSize: 11
            cornerRadius: 6
            tooltipText: "Reset zoom (Ctrl+0)"
            onClicked: root.zoomResetRequested()
        }
    }
}
