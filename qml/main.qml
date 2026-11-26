import QtQuick
import QtQuick.Controls
import Nulla 1.0

ApplicationWindow {
    id: win

    visible: true
    width: 880
    height: 610
    minimumWidth: 520
    minimumHeight: 360
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    title: (backend.modified ? "* " : "") + backend.fileName + " — Nulla"

    property bool forceClose: false
    property string pendingAction: ""
    property string pendingDropPath: ""

    readonly property bool dark: backend.effectiveDark
    readonly property color bgShell: dark ? "#131417" : "#eceef2"
    readonly property color bgEditor: dark ? "#181a1f" : "#f6f7f9"
    readonly property color bgGutter: dark ? "#16181c" : "#f0f2f5"
    readonly property color bgHover: dark ? "#23262e" : "#e2e5ec"
    readonly property color bgLineHighlight: dark ? "#1f2229" : "#eceff4"
    readonly property color borderSubtle: dark ? "#282b33" : "#d3d7e0"
    readonly property color borderFocus: dark ? "#5c6370" : "#7c8494"
    readonly property color fgMain: dark ? "#e6e8ec" : "#16181d"
    readonly property color fgMuted: dark ? "#8e95a3" : "#5e6573"
    readonly property color fgGutter: dark ? "#565c68" : "#9aa1ae"
    readonly property color selBg: dark ? "#323744" : "#d4d9e3"

    EditorBackend {
        id: backend
        onErrorOccurred: function(title, message) {
            confirmModal.errorOnly = true
            confirmModal.title = title
            confirmModal.message = message
            confirmModal.open = true
        }
    }

    Component.onCompleted: {
        backend.attachDocument(editor.textDocument)
        if (initialFileArg !== "") {
            backend.setInitialFile(initialFileArg)
        }
        editor.forceActiveFocus()
    }

    function runPendingAction() {
        var act = win.pendingAction
        var dropTarget = win.pendingDropPath
        win.pendingAction = ""
        win.pendingDropPath = ""
        if (act === "new") {
            backend.newFile()
            editor.forceActiveFocus()
        } else if (act === "open") {
            backend.openFileDialog()
            editor.forceActiveFocus()
        } else if (act === "drop" && dropTarget !== "") {
            backend.loadFile(dropTarget, true)
            editor.forceActiveFocus()
        } else if (act === "close") {
            win.forceClose = true
            win.close()
        }
    }

    function requestGuardedAction(actionName, dropPath) {
        if (backend.modified) {
            win.pendingAction = actionName
            win.pendingDropPath = dropPath || ""
            confirmModal.errorOnly = false
            confirmModal.title = "Unsaved changes"
            confirmModal.message = 'Save changes to "' + backend.fileName + '" before continuing?'
            confirmModal.open = true
            return
        }
        win.pendingAction = actionName
        win.pendingDropPath = dropPath || ""
        runPendingAction()
    }

    onClosing: function(close) {
        if (!win.forceClose && backend.modified) {
            close.accepted = false
            requestGuardedAction("close", "")
        }
    }

    Shortcut { sequence: "Ctrl+N"; onActivated: requestGuardedAction("new", "") }
    Shortcut { sequence: "Ctrl+O"; onActivated: requestGuardedAction("open", "") }
    Shortcut { sequence: "Ctrl+S"; onActivated: backend.saveFile() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: backend.saveFileAsDialog() }
    Shortcut { sequence: "Ctrl+,"; onActivated: settingsModal.open = !settingsModal.open }
    Shortcut { sequence: "Ctrl++"; onActivated: backend.zoomIn() }
    Shortcut { sequence: "Ctrl+="; onActivated: backend.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: backend.zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: backend.zoomReset() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: win.close() }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (confirmModal.open) {
                confirmModal.open = false
                win.pendingAction = ""
                win.pendingDropPath = ""
            } else if (settingsModal.open) {
                if (settingsModal.langPickerOpen) {
                    settingsModal.langPickerOpen = false
                } else {
                    settingsModal.open = false
                }
            } else if (menuPopup.opened) {
                menuPopup.close()
            }
            editor.forceActiveFocus()
        }
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: win.visibility === Window.Maximized ? 0 : 10
        color: win.bgShell
        border.width: win.visibility === Window.Maximized ? 0 : 1
        border.color: win.borderSubtle
        clip: true

        Behavior on color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }
        Behavior on border.color {
            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
        }

        Column {
            anchors.fill: parent
            spacing: 0

            Item {
                id: topbar
                width: parent.width
                height: 42

                MouseArea {
                    anchors.fill: parent
                    onPressed: win.startSystemMove()
                    onDoubleClicked: {
                        if (win.visibility === Window.Maximized) {
                            win.showNormal()
                        } else {
                            win.showMaximized()
                        }
                    }
                }

                Row {
                    id: leftControls
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    M3Button {
                        text: "Open"
                        darkMode: win.dark
                        tooltipText: "Open File (Ctrl+O)"
                        onClicked: requestGuardedAction("open", "")
                    }

                    M3Button {
                        iconKind: "plus"
                        iconOnly: true
                        darkMode: win.dark
                        tooltipText: "New File (Ctrl+N)"
                        onClicked: requestGuardedAction("new", "")
                    }

                    M3Button {
                        text: backend.modified ? "• Save" : "Save"
                        darkMode: win.dark
                        accent: backend.modified
                        tooltipText: "Save File (Ctrl+S)"
                        onClicked: backend.saveFile()
                    }
                }

                Rectangle {
                    id: titlePill
                    height: 28
                    width: Math.min(titleLabel.implicitWidth + 24, Math.max(96, topbar.width - leftControls.width - rightControls.width - 56))
                    radius: 8
                    anchors.centerIn: parent
                    color: "transparent"
                    border.width: 1
                    border.color: win.borderSubtle

                    Behavior on border.color {
                        ColorAnimation { duration: 100; easing.type: Easing.OutCubic }
                    }

                    Text {
                        id: titleLabel
                        anchors.centerIn: parent
                        width: Math.min(implicitWidth, parent.width - 20)
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignHCenter
                        text: (backend.modified ? "• " : "") + backend.fileName
                        color: backend.modified ? win.fgMain : win.fgMuted
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }
                }

                Row {
                    id: rightControls
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    M3Button {
                        iconKind: "gear"
                        iconOnly: true
                        darkMode: win.dark
                        tooltipText: "Settings (Ctrl+,)"
                        onClicked: settingsModal.open = true
                    }

                    M3Button {
                        id: menuBtn
                        iconKind: "dots"
                        iconOnly: true
                        darkMode: win.dark
                        tooltipText: "Menu"
                        onClicked: {
                            if (menuPopup.opened) {
                                menuPopup.close()
                            } else {
                                menuPopup.open()
                            }
                        }
                    }

                    Item { width: 4; height: 1 }

                    M3Button {
                        iconKind: "minus"
                        iconOnly: true
                        buttonHeight: 26
                        darkMode: win.dark
                        tooltipText: "Minimize"
                        onClicked: win.showMinimized()
                    }

                    M3Button {
                        iconKind: "max"
                        iconOnly: true
                        buttonHeight: 26
                        darkMode: win.dark
                        tooltipText: win.visibility === Window.Maximized ? "Restore" : "Maximize"
                        onClicked: {
                            if (win.visibility === Window.Maximized) {
                                win.showNormal()
                            } else {
                                win.showMaximized()
                            }
                        }
                    }

                    M3Button {
                        iconKind: "close"
                        iconOnly: true
                        buttonHeight: 26
                        darkMode: win.dark
                        dangerHover: true
                        tooltipText: "Close"
                        onClicked: win.close()
                    }
                }
            }

            Rectangle {
                id: editorCard
                width: parent.width - 6
                height: parent.height - topbar.height - statusBar.height
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 8
                color: win.bgEditor
                border.width: 1
                border.color: editor.activeFocus ? win.borderFocus : win.borderSubtle
                clip: true

                Behavior on color {
                    ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
                }
                Behavior on border.color {
                    ColorAnimation { duration: 100; easing.type: Easing.OutCubic }
                }

                Row {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        id: gutterContainer
                        visible: backend.showLineNumbers
                        width: visible ? gutter.implicitWidth : 0
                        height: parent.height
                        color: win.bgGutter

                        Behavior on color {
                            ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
                        }

                        LineNumberGutter {
                            id: gutter
                            anchors.fill: parent
                            document: editor.textDocument
                            contentY: flick.contentY
                            currentLine: backend.cursorLine
                            font: editor.font
                            textColor: win.fgGutter
                            activeTextColor: win.fgMain
                            topPadding: 8
                        }

                        Rectangle {
                            width: 1
                            height: parent.height
                            anchors.right: parent.right
                            color: win.borderSubtle
                        }
                    }

                    Flickable {
                        id: flick
                        width: parent.width - gutterContainer.width
                        height: parent.height
                        contentWidth: Math.max(width, editor.contentWidth + 20)
                        contentHeight: Math.max(height, editor.contentHeight + 16)
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            width: 8
                            contentItem: Rectangle {
                                implicitWidth: 5
                                radius: 3
                                color: parent.pressed
                                       ? (win.dark ? "#7d8594" : "#6c7382")
                                       : (parent.hovered ? (win.dark ? "#5a606e" : "#8d94a3") : (win.dark ? "#373b46" : "#c1c6d0"))
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                        }

                        ScrollBar.horizontal: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            height: 8
                            contentItem: Rectangle {
                                implicitHeight: 5
                                radius: 3
                                color: parent.pressed
                                       ? (win.dark ? "#7d8594" : "#6c7382")
                                       : (parent.hovered ? (win.dark ? "#5a606e" : "#8d94a3") : (win.dark ? "#373b46" : "#c1c6d0"))
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }
                        }

                        WheelHandler {
                            acceptedModifiers: Qt.ControlModifier
                            onWheel: function(event) {
                                if (event.angleDelta.y > 0) {
                                    backend.zoomIn()
                                } else if (event.angleDelta.y < 0) {
                                    backend.zoomOut()
                                }
                            }
                        }

                        Rectangle {
                            id: currentLineBand
                            visible: editor.activeFocus
                            x: 0
                            y: editor.y + editor.cursorRectangle.y
                            width: Math.max(flick.width, flick.contentWidth)
                            height: Math.max(18, editor.cursorRectangle.height)
                            color: win.bgLineHighlight
                        }

                        MouseArea {
                            x: 0
                            y: 0
                            width: Math.max(flick.width, flick.contentWidth)
                            height: Math.max(flick.height, flick.contentHeight)
                            cursorShape: Qt.IBeamCursor
                            onPressed: function(mouse) {
                                editor.forceActiveFocus()
                                editor.cursorPosition = editor.positionAt(mouse.x - editor.x, mouse.y - editor.y)
                            }
                        }

                        TextEdit {
                            id: editor
                            x: 10
                            y: 8
                            textFormat: TextEdit.PlainText
                            selectByMouse: true
                            persistentSelection: true
                            wrapMode: TextEdit.NoWrap
                            color: win.fgMain
                            selectionColor: win.selBg
                            selectedTextColor: win.fgMain
                            cursorVisible: activeFocus
                            font.family: "monospace"
                            font.pixelSize: Math.round(14 * backend.zoomPercent / 100)

                            onCursorPositionChanged: backend.updateCursorPosition(cursorPosition)

                            onCursorRectangleChanged: {
                                var r = cursorRectangle
                                var cy = r.y + y
                                var cx = r.x + x
                                if (cy - flick.contentY < 8) {
                                    flick.contentY = Math.max(0, cy - 8)
                                } else if (cy + r.height - flick.contentY > flick.height - 8) {
                                    flick.contentY = Math.max(0, cy + r.height - flick.height + 8)
                                }
                                if (cx - flick.contentX < 10) {
                                    flick.contentX = Math.max(0, cx - 10)
                                } else if (cx + r.width - flick.contentX > flick.width - 10) {
                                    flick.contentX = Math.max(0, cx + r.width - flick.width + 10)
                                }
                            }

                            Keys.onPressed: function(event) {
                                if (event.key === Qt.Key_Tab && event.modifiers === Qt.NoModifier) {
                                    insert(cursorPosition, "    ")
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: statusBar
                width: parent.width
                height: 30

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Rectangle {
                        height: 22
                        width: posLabel.implicitWidth + 14
                        radius: 7
                        color: "transparent"
                        border.width: 1
                        border.color: win.borderSubtle

                        Text {
                            id: posLabel
                            anchors.centerIn: parent
                            text: "Ln " + backend.cursorLine + ", Col " + backend.cursorColumn
                            color: win.fgMuted
                            font.pixelSize: 12
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: backend.lineCount + (backend.lineCount === 1 ? " line" : " lines")
                        color: win.fgGutter
                        font.pixelSize: 12
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    M3Button {
                        text: backend.statusLanguage
                        darkMode: win.dark
                        buttonHeight: 22
                        horizontalPadding: 11
                        fontPixelSize: 12
                        cornerRadius: 7
                        tooltipText: "Change syntax language"
                        onClicked: {
                            settingsModal.open = true
                            settingsModal.langPickerOpen = true
                        }
                    }

                    M3Button {
                        text: backend.zoomPercent + "%"
                        darkMode: win.dark
                        buttonHeight: 22
                        horizontalPadding: 11
                        fontPixelSize: 12
                        cornerRadius: 7
                        tooltipText: "Reset zoom (Ctrl+0)"
                        onClicked: backend.zoomReset()
                    }
                }
            }
        }

        Popup {
            id: menuPopup
            x: rightControls.x + menuBtn.x + menuBtn.width - width + 12
            y: topbar.height - 4
            width: 210
            padding: 6
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            Overlay.modal: Rectangle { color: "transparent" }

            enter: Transition {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 85; easing.type: Easing.OutCubic }
            }
            exit: Transition {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70; easing.type: Easing.OutCubic }
            }

            background: Rectangle {
                radius: 10
                color: win.dark ? "#1b1d23" : "#f8f9fb"
                border.width: 1
                border.color: win.borderSubtle
            }

            contentItem: Column {
                spacing: 2

                Repeater {
                    model: [
                        { label: "New", shortcut: "Ctrl+N", act: "new", sep: false },
                        { label: "Open...", shortcut: "Ctrl+O", act: "open", sep: false },
                        { label: "Save", shortcut: "Ctrl+S", act: "save", sep: false },
                        { label: "Save As...", shortcut: "Ctrl+Shift+S", act: "saveas", sep: true },
                        { label: "Zoom In", shortcut: "Ctrl++", act: "zoomin", sep: false },
                        { label: "Zoom Out", shortcut: "Ctrl+-", act: "zoomout", sep: false },
                        { label: "Reset Zoom", shortcut: "Ctrl+0", act: "zoomreset", sep: true },
                        { label: "Settings", shortcut: "Ctrl+,", act: "settings", sep: false },
                        { label: "Quit", shortcut: "Ctrl+Q", act: "quit", sep: false }
                    ]

                    delegate: Column {
                        width: 198
                        spacing: 2

                        Rectangle {
                            id: menuRow
                            width: parent.width
                            height: 30
                            radius: 7
                            color: rowMa.pressed
                                   ? (win.dark ? "#2c303a" : "#d8dce5")
                                   : (rowMa.containsMouse ? win.bgHover : "transparent")
                            scale: rowMa.pressed ? 0.985 : 1.0

                            Behavior on color { ColorAnimation { duration: 110 } }
                            Behavior on scale { NumberAnimation { duration: 90 } }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: win.fgMain
                                font.pixelSize: 13
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.shortcut
                                color: win.fgGutter
                                font.pixelSize: 11
                            }

                            MouseArea {
                                id: rowMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    menuPopup.close()
                                    var a = modelData.act
                                    if (a === "new") requestGuardedAction("new", "")
                                    else if (a === "open") requestGuardedAction("open", "")
                                    else if (a === "save") backend.saveFile()
                                    else if (a === "saveas") backend.saveFileAsDialog()
                                    else if (a === "zoomin") backend.zoomIn()
                                    else if (a === "zoomout") backend.zoomOut()
                                    else if (a === "zoomreset") backend.zoomReset()
                                    else if (a === "settings") settingsModal.open = true
                                    else if (a === "quit") win.close()
                                }
                            }
                        }

                        Rectangle {
                            visible: modelData.sep
                            width: parent.width - 16
                            height: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: win.borderSubtle
                        }
                    }
                }
            }
        }

        SettingsModal {
            id: settingsModal
            darkMode: win.dark
            backend: backend
            onClosed: {
                open = false
                editor.forceActiveFocus()
            }
        }

        ConfirmModal {
            id: confirmModal
            darkMode: win.dark
            onCancelled: {
                open = false
                win.pendingAction = ""
                win.pendingDropPath = ""
                editor.forceActiveFocus()
            }
            onDiscarded: {
                open = false
                backend.discardChanges()
                win.runPendingAction()
            }
            onSaved: {
                open = false
                if (backend.saveFile()) {
                    win.runPendingAction()
                } else {
                    win.pendingAction = ""
                    win.pendingDropPath = ""
                }
            }
        }

        DropArea {
            anchors.fill: parent
            onDropped: function(drop) {
                if (drop.hasUrls && drop.urls.length > 0) {
                    requestGuardedAction("drop", drop.urls[0].toString())
                }
            }
        }
    }

    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.topMargin: 10; anchors.bottomMargin: 10
        width: 5; cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.topMargin: 10; anchors.bottomMargin: 10
        width: 5; cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 10; anchors.rightMargin: 10
        height: 5; cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 10; anchors.rightMargin: 10
        height: 5; cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.BottomEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.left: parent.left; anchors.top: parent.top
        width: 10; height: 10; cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.right: parent.right; anchors.top: parent.top
        width: 10; height: 10; cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.left: parent.left; anchors.bottom: parent.bottom
        width: 10; height: 10; cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }
    MouseArea {
        visible: win.visibility !== Window.Maximized
        anchors.right: parent.right; anchors.bottom: parent.bottom
        width: 10; height: 10; cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
}
