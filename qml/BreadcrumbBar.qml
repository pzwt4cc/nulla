import QtQuick

Rectangle {
    id: root

    property var activeTab: null
    property var ui: ({})
    property color bgEditor: "#18181b"
    property color borderSubtle: "#242429"
    property color fgMain: "#f2f2f5"
    property color fgMuted: "#9d9da9"
    property color fgGutter: "#5c5c68"

    implicitHeight: visible ? 25 : 0
    height: implicitHeight
    color: bgEditor
    clip: true

    Behavior on color {
        ColorAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    function formattedBreadcrumb() {
        if (!activeTab || !activeTab.filePath || activeTab.filePath === "") {
            return root.ui.untitled || "Untitled";
        }
        const norm = activeTab.filePath.replace(/\\/g, "/");
        const parts = norm.split("/").filter(function (p) {
            return p.length > 0;
        });
        if (parts.length <= 3) {
            return parts.join("  ›  ");
        }
        return "…  ›  " + parts.slice(parts.length - 3).join("  ›  ");
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.formattedBreadcrumb()
            color: root.fgMuted
            font.pixelSize: 11
            elide: Text.ElideLeft
            width: Math.min(implicitWidth, Math.max(40, parent.width - 68))
        }

        Rectangle {
            width: 1
            height: 11
            anchors.verticalCenter: parent.verticalCenter
            color: root.borderSubtle
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "UTF-8"
            color: root.fgGutter
            font.pixelSize: 11
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
