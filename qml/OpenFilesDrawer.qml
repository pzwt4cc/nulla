import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property bool open: true
    property int preferredWidth: 180
    property int scalePercent: 100
    property bool resizing: false
    property bool darkMode: true
    property var theme: ({})
    property var tabModel: null
    property int activeIndex: 0
    property string activeFilePath: ""
    property string workspaceRoot: ""
    property string workspaceName: ""
    property var folderEntries: []
    property var ui: ({})

    property color bgDrawer: "#131316"
    property color bgHover: "#27272d"
    property color bgSurfaceElev: "#202025"
    property color borderSubtle: "#242429"
    property color borderControl: "#34343c"
    property color borderFocus: "#727280"
    property color fgMain: "#f2f2f5"
    property color fgMuted: "#9d9da9"
    property color fgSub: "#787885"

    signal tabActivated(int index)
    signal tabClosed(int index)
    signal resizeRequested(int newWidth)
    signal openFolderRequested
    signal closeFolderRequested
    signal refreshFolderRequested
    signal folderToggled(string dirPath)
    signal fileOpenRequested(string filePath)

    readonly property real uiScale: Math.max(0.8, Math.min(1.5, scalePercent / 100.0))
    readonly property int rowHeight: Math.round(28 * uiScale)
    readonly property int headerHeight: Math.round(30 * uiScale)
    readonly property int headerFontSize: Math.round(11 * uiScale)
    readonly property int itemFontSize: Math.round(12 * uiScale)
    readonly property int itemIconSize: Math.round(14 * uiScale)
    readonly property int chevronSize: Math.round(11 * uiScale)
    readonly property int closeBtnSize: Math.round(18 * uiScale)
    readonly property int indentStep: Math.round(13 * uiScale)
    readonly property bool hasFolder: workspaceRoot !== ""

    function svgUri(kind, col) {
        const hex = col.toString();
        let body = "";
        if (kind === "file") {
            body = '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/>';
        } else if (kind === "folder") {
            body = '<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>';
        } else if (kind === "folderopen") {
            body = '<path d="m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"/>';
        } else if (kind === "chevronRight") {
            body = '<path d="m9 6 6 6-6 6"/>';
        } else if (kind === "chevronDown") {
            body = '<path d="m6 9 6 6 6-6"/>';
        }
        const svg = '<svg xmlns="http:\x2f\x2fwww.w3.org\x2f2000\x2fsvg" viewBox="0 0 24 24" fill="none" stroke="' + hex + '" stroke-width="1.85" stroke-linecap="round" stroke-linejoin="round">' + body + '</svg>';
        return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
    }

    width: open ? preferredWidth : 0
    visible: width > 1
    clip: true
    color: bgDrawer

    Item {
        width: Math.max(140, root.preferredWidth)
        height: parent.height

        Item {
            id: openFilesHeader
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: root.headerHeight

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.ui.openFiles || "Open Files"
                color: root.fgMuted
                font.pixelSize: root.headerFontSize
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
        }

        ListView {
            id: fileList
            anchors.top: openFilesHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 6
            anchors.rightMargin: 8
            height: root.hasFolder ? Math.min(contentHeight + 4, Math.min(root.rowHeight * 6, Math.max(0, parent.height - openFilesHeader.height - root.headerHeight - root.rowHeight * 3))) : Math.max(0, parent.height - openFilesHeader.height - emptyFolderBar.height - 6)
            clip: true
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds
            model: root.tabModel

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 4
                contentItem: Rectangle {
                    implicitWidth: 3
                    radius: 2
                    color: root.theme.scrollIdle || (root.darkMode ? "#3a3a42" : "#c1c6d0")
                }
            }

            delegate: Item {
                id: fileRow
                width: fileList.width
                height: root.rowHeight

                readonly property bool isActive: index === root.activeIndex
                readonly property bool hovered: rowMa.containsMouse || rowCloseBtn.hovered
                readonly property bool isDirty: session ? session.modified : false

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: fileRow.isActive ? root.bgSurfaceElev : (rowMa.pressed ? root.borderSubtle : (fileRow.hovered ? root.bgHover : "transparent"))
                    border.width: 1
                    border.color: fileRow.isActive ? root.borderControl : "transparent"

                    Rectangle {
                        visible: fileRow.isActive
                        width: 2
                        height: Math.max(10, Math.round(parent.height * 0.5))
                        radius: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.fgMain
                    }
                }

                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: function (mouse) {
                        if (mouse.button === Qt.MiddleButton) {
                            root.tabClosed(index);
                        } else {
                            root.tabActivated(index);
                        }
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: rowCloseBtn.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Math.max(5, Math.round(7 * root.uiScale))

                    Image {
                        width: root.itemIconSize
                        height: root.itemIconSize
                        sourceSize: Qt.size(root.itemIconSize * 2, root.itemIconSize * 2)
                        smooth: true
                        anchors.verticalCenter: parent.verticalCenter
                        source: root.svgUri("file", fileRow.isActive || fileRow.hovered ? root.fgMain : root.fgMuted)
                    }

                    Text {
                        width: Math.max(16, parent.width - root.itemIconSize - (fileRow.isDirty ? 19 : 7))
                        elide: Text.ElideMiddle
                        text: (session && session.filePath !== "") ? session.fileName : (root.ui.untitled || "Untitled")
                        color: fileRow.isActive || fileRow.hovered ? root.fgMain : root.fgMuted
                        font.pixelSize: root.itemFontSize
                        font.weight: fileRow.isActive ? Font.DemiBold : Font.Normal
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        visible: fileRow.isDirty
                        width: 6
                        height: 6
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.fgMain
                    }
                }

                M3Button {
                    id: rowCloseBtn
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    visible: fileRow.isActive || fileRow.hovered
                    iconKind: "close"
                    iconOnly: true
                    ghost: true
                    dangerHover: true
                    buttonHeight: root.closeBtnSize
                    buttonWidth: root.closeBtnSize
                    iconSize: Math.max(9, Math.round(10 * root.uiScale))
                    cornerRadius: 4
                    darkMode: root.darkMode
                    theme: root.theme
                    onClicked: root.tabClosed(index)
                }
            }
        }

        Item {
            id: emptyFolderBar
            visible: !root.hasFolder
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: visible ? Math.round(40 * root.uiScale) : 0

            Rectangle {
                width: parent.width - 16
                height: 1
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                color: root.borderSubtle
            }

            M3Button {
                anchors.centerIn: parent
                width: parent.width - 16
                text: root.ui.openFolderBtn || "Open Folder"
                iconKind: "folder"
                ghost: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: Math.round(28 * root.uiScale)
                iconSize: root.itemIconSize
                fontPixelSize: root.itemFontSize
                cornerRadius: 6
                onClicked: root.openFolderRequested()
            }
        }

        Item {
            id: folderSection
            visible: root.hasFolder
            anchors.top: fileList.bottom
            anchors.topMargin: 6
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Rectangle {
                width: parent.width - 16
                height: 1
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                color: root.borderSubtle
            }

            Item {
                id: folderHeader
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.left: parent.left
                anchors.right: parent.right
                height: root.headerHeight

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: folderActions.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.workspaceName
                    color: root.fgMuted
                    font.pixelSize: root.headerFontSize
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Row {
                    id: folderActions
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    M3Button {
                        iconKind: "refresh"
                        iconOnly: true
                        ghost: true
                        buttonHeight: root.closeBtnSize + 2
                        buttonWidth: root.closeBtnSize + 2
                        iconSize: Math.max(10, Math.round(12 * root.uiScale))
                        cornerRadius: 4
                        darkMode: root.darkMode
                        theme: root.theme
                        tooltipText: root.ui.refreshFolderTip || "Refresh Folder"
                        onClicked: root.refreshFolderRequested()
                    }

                    M3Button {
                        iconKind: "close"
                        iconOnly: true
                        ghost: true
                        dangerHover: true
                        buttonHeight: root.closeBtnSize + 2
                        buttonWidth: root.closeBtnSize + 2
                        iconSize: Math.max(10, Math.round(11 * root.uiScale))
                        cornerRadius: 4
                        darkMode: root.darkMode
                        theme: root.theme
                        tooltipText: root.ui.closeFolderTip || "Close Folder"
                        onClicked: root.closeFolderRequested()
                    }
                }
            }

            ListView {
                id: treeList
                anchors.top: folderHeader.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 6
                anchors.rightMargin: 8
                anchors.bottomMargin: 8
                clip: true
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds
                model: root.folderEntries

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 4
                    contentItem: Rectangle {
                        implicitWidth: 3
                        radius: 2
                        color: root.theme.scrollIdle || (root.darkMode ? "#3a3a42" : "#c1c6d0")
                    }
                }

                delegate: Item {
                    id: treeRow
                    width: treeList.width
                    height: root.rowHeight

                    readonly property bool isDir: !!modelData.isDir
                    readonly property bool isExpanded: !!modelData.expanded
                    readonly property int depth: modelData.depth || 0
                    readonly property bool isActiveFile: !isDir && root.activeFilePath !== "" && modelData.path === root.activeFilePath
                    readonly property bool hovered: treeMa.containsMouse

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: treeRow.isActiveFile ? root.bgSurfaceElev : (treeMa.pressed ? root.borderSubtle : (treeRow.hovered ? root.bgHover : "transparent"))
                        border.width: 1
                        border.color: treeRow.isActiveFile ? root.borderControl : "transparent"

                        Rectangle {
                            visible: treeRow.isActiveFile
                            width: 2
                            height: Math.max(10, Math.round(parent.height * 0.5))
                            radius: 1
                            anchors.left: parent.left
                            anchors.leftMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.fgMain
                        }
                    }

                    MouseArea {
                        id: treeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (treeRow.isDir) {
                                root.folderToggled(modelData.path);
                            } else {
                                root.fileOpenRequested(modelData.path);
                            }
                        }
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 8 + treeRow.depth * root.indentStep
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Math.max(4, Math.round(5 * root.uiScale))

                        Item {
                            width: root.chevronSize
                            height: root.chevronSize
                            anchors.verticalCenter: parent.verticalCenter

                            Image {
                                visible: treeRow.isDir
                                anchors.fill: parent
                                sourceSize: Qt.size(root.chevronSize * 2, root.chevronSize * 2)
                                smooth: true
                                source: visible ? root.svgUri(treeRow.isExpanded ? "chevronDown" : "chevronRight", treeRow.hovered ? root.fgMain : root.fgSub) : ""
                            }
                        }

                        Image {
                            width: root.itemIconSize
                            height: root.itemIconSize
                            sourceSize: Qt.size(root.itemIconSize * 2, root.itemIconSize * 2)
                            smooth: true
                            anchors.verticalCenter: parent.verticalCenter
                            source: root.svgUri(treeRow.isDir ? (treeRow.isExpanded ? "folderopen" : "folder") : "file", (treeRow.isActiveFile || treeRow.hovered) ? root.fgMain : (treeRow.isDir ? (root.theme.fgLabel || root.fgMain) : root.fgMuted))
                        }

                        Text {
                            width: Math.max(16, parent.width - root.chevronSize - root.itemIconSize - parent.spacing * 2)
                            elide: Text.ElideRight
                            text: modelData.name || ""
                            color: (treeRow.isActiveFile || treeRow.hovered) ? root.fgMain : root.fgMuted
                            font.pixelSize: root.itemFontSize
                            font.weight: treeRow.isActiveFile ? Font.DemiBold : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        width: (resizeMa.containsMouse || root.resizing) ? 2 : 1
        height: parent.height
        anchors.right: parent.right
        color: (resizeMa.containsMouse || root.resizing) ? root.borderFocus : root.borderSubtle
    }

    MouseArea {
        id: resizeMa
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 6
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        property real pressScreenX: 0
        property int startWidth: 180

        onPressed: function (mouse) {
            root.resizing = true;
            pressScreenX = mapToGlobal(mouse.x, 0).x;
            startWidth = root.preferredWidth;
        }
        onPositionChanged: function (mouse) {
            if (pressed) {
                const gx = mapToGlobal(mouse.x, 0).x;
                const nextW = Math.max(140, Math.min(480, Math.round(startWidth + (gx - pressScreenX))));
                root.resizeRequested(nextW);
            }
        }
        onReleased: root.resizing = false
        onCanceled: root.resizing = false
    }
}
