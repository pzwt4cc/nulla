import QtQuick
import QtQuick.Controls

Popup {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var ui: ({})
    property bool canUndo: false
    property bool canRedo: false
    property bool canPaste: false
    property bool hasSelection: false
    property bool hasText: false
    property bool hasSingleLineSelection: false
    property bool canComment: true

    property color bgSurfaceElev: "#1c1f27"
    property color bgHover: "#252934"
    property color borderSubtle: "#222630"
    property color borderControl: "#313644"
    property color fgMain: "#f0f2f6"
    property color fgMuted: "#9ca3b4"
    property color fgGutter: "#565d6e"

    signal actionTriggered(string act)

    function openAt(px, py, maxW, maxH) {
        const menuW = Math.min(234, Math.max(120, maxW - 12));
        const menuH = Math.min(contentCol.implicitHeight + padding * 2, Math.max(80, maxH - 12));
        width = menuW;
        height = menuH;
        x = Math.max(6, Math.min(px, maxW - menuW - 6));
        y = Math.max(6, Math.min(py, maxH - menuH - 6));
        open();
    }

    width: 234
    padding: 6
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    Overlay.modal: Rectangle {
        color: "transparent"
    }

    background: Rectangle {
        radius: 7
        color: root.bgSurfaceElev
        border.width: 1
        border.color: root.borderControl
        clip: true

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            height: 1
            color: root.theme.sheenBright || (root.darkMode ? "#14ffffff" : "#80ffffff")
        }
    }

    contentItem: Flickable {
        id: ctxFlick
        clip: true
        contentWidth: width
        contentHeight: contentCol.implicitHeight
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: contentCol
            width: ctxFlick.width
            spacing: 2

        Repeater {
            model: [
                { label: root.ui.ctxUndo || "Undo", shortcut: "Ctrl+Z", act: "undo", enabled: root.canUndo, sep: false },
                { label: root.ui.ctxRedo || "Redo", shortcut: "Ctrl+Shift+Z", act: "redo", enabled: root.canRedo, sep: true },
                { label: root.hasSelection ? (root.ui.ctxCut || "Cut") : (root.ui.ctxCutLine || "Cut Line"), shortcut: "Ctrl+X", act: "cut", enabled: root.hasSelection || root.hasText, sep: false },
                { label: root.hasSelection ? (root.ui.ctxCopy || "Copy") : (root.ui.ctxCopyLine || "Copy Line"), shortcut: "Ctrl+C", act: "copy", enabled: root.hasSelection || root.hasText, sep: false },
                { label: root.ui.ctxPaste || "Paste", shortcut: "Ctrl+V", act: "paste", enabled: root.canPaste, sep: false },
                { label: root.ui.ctxDelete || "Delete", shortcut: "Del", act: "delete", enabled: root.hasSelection, sep: true },
                { label: root.ui.ctxDuplicateLine || "Duplicate Line", shortcut: "Ctrl+D", act: "duplicate", enabled: true, sep: false },
                { label: root.ui.ctxComment || "Toggle Comment", shortcut: "Ctrl+\x2f", act: "comment", enabled: root.canComment, sep: true },
                { label: root.ui.ctxSelectAll || "Select All", shortcut: "Ctrl+A", act: "selectall", enabled: root.hasText, sep: false },
                { label: root.hasSingleLineSelection ? (root.ui.ctxFindSelection || "Find Selection") : (root.ui.ctxFind || "Find..."), shortcut: "Ctrl+F", act: "find", enabled: true, sep: false }
            ]

            delegate: Column {
                width: contentCol.width
                spacing: 2

                Rectangle {
                    width: parent.width
                    height: 28
                    radius: 5
                    color: !modelData.enabled ? "transparent" : (itemMa.pressed ? (root.theme.bgPressed || (root.darkMode ? "#282b34" : "#d8dce5")) : (itemMa.containsMouse ? root.bgHover : "transparent"))
                    border.width: 1
                    border.color: (modelData.enabled && itemMa.containsMouse) ? root.borderControl : "transparent"

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.label
                        color: modelData.enabled ? root.fgMain : root.fgGutter
                        font.pixelSize: 12
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.shortcut
                        color: modelData.enabled ? root.fgMuted : root.fgGutter
                        opacity: modelData.enabled ? 0.78 : 0.55
                        font.pixelSize: 11
                    }

                    MouseArea {
                        id: itemMa
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: modelData.enabled
                        cursorShape: modelData.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            root.close();
                            root.actionTriggered(modelData.act);
                        }
                    }
                }

                Rectangle {
                    visible: modelData.sep
                    width: parent.width - 12
                    height: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: root.borderSubtle
                }
            }
        }
        }
    }
}
