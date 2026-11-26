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
    property bool findOpen: false
    property string findQuery: ""
    property int matchCurrent: 0
    property int matchTotal: 0

    readonly property bool dark: backend.effectiveDark
    readonly property color bgShell: dark ? "#131417" : "#eceef2"
    readonly property color bgHeader: dark ? "#16181c" : "#f2f4f7"
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

    function openFindBar() {
        findOpen = true
        if (editor.selectedText !== "" && editor.selectedText.indexOf("\n") === -1) {
            findQuery = editor.selectedText
            findInput.text = findQuery
        }
        updateMatchStats()
        findInput.forceActiveFocus()
        findInput.selectAll()
    }

    function closeFindBar() {
        findOpen = false
        editor.forceActiveFocus()
    }

    function updateMatchStats() {
        if (findQuery === "") {
            matchCurrent = 0
            matchTotal = 0
            return
        }
        var st = backend.matchStats(findQuery, editor.selectionStart)
        matchCurrent = st.current
        matchTotal = st.total
    }

    function findNext(backward) {
        if (findQuery === "") {
            updateMatchStats()
            return
        }
        var fromPos = backward ? Math.max(0, editor.selectionStart) : editor.selectionEnd
        var res = backend.findText(findQuery, fromPos, backward)
        if (res.found) {
            editor.select(res.start, res.end)
        }
        updateMatchStats()
    }

    function indentSelection(unindent) {
        var text = editor.text
        var selStart = editor.selectionStart
        var selEnd = editor.selectionEnd
        if (selStart === selEnd && !unindent) {
            editor.insert(editor.cursorPosition, "    ")
            return
        }
        var lineStart = text.lastIndexOf("\n", Math.max(0, selStart - 1)) + 1
        var lineEnd = text.indexOf("\n", selEnd)
        if (lineEnd === -1) {
            lineEnd = text.length
        }
        var block = text.slice(lineStart, lineEnd)
        var lines = block.split("\n")
        for (var i = 0; i < lines.length; i++) {
            if (unindent) {
                if (lines[i].startsWith("    ")) {
                    lines[i] = lines[i].slice(4)
                } else if (lines[i].startsWith("\t")) {
                    lines[i] = lines[i].slice(1)
                } else {
                    lines[i] = lines[i].replace(/^ {1,3}/, "")
                }
            } else {
                lines[i] = "    " + lines[i]
            }
        }
        var updated = lines.join("\n")
        editor.remove(lineStart, lineEnd)
        editor.insert(lineStart, updated)
        editor.select(lineStart, lineStart + updated.length)
    }

    function duplicateCurrentLine() {
        var text = editor.text
        var pos = editor.cursorPosition
        var lineStart = pos === 0 ? 0 : text.lastIndexOf("\n", pos - 1) + 1
        var lineEnd = text.indexOf("\n", pos)
        if (lineEnd === -1) {
            lineEnd = text.length
        }
        var line = text.slice(lineStart, lineEnd)
        editor.insert(lineEnd, "\n" + line)
        editor.cursorPosition = pos + line.length + 1
    }

    function cutCurrentLine() {
        var text = editor.text
        if (text.length === 0) {
            return
        }
        var pos = editor.cursorPosition
        var lineStart = pos === 0 ? 0 : text.lastIndexOf("\n", pos - 1) + 1
        var lineEnd = text.indexOf("\n", pos)
        if (lineEnd === -1) {
            lineEnd = text.length
        }
        if (lineEnd < text.length) {
            editor.select(lineStart, lineEnd + 1)
        } else if (lineStart > 0) {
            editor.select(lineStart - 1, lineEnd)
        } else {
            editor.select(0, text.length)
        }
        editor.cut()
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
    Shortcut { sequence: "Ctrl+F"; onActivated: openFindBar() }
    Shortcut { sequence: "F3"; onActivated: findNext(false) }
    Shortcut { sequence: "Shift+F3"; onActivated: findNext(true) }
    Shortcut { sequence: "Ctrl+,"; onActivated: settingsModal.open = !settingsModal.open }
    Shortcut { sequence: "Ctrl++"; onActivated: backend.zoomIn() }
    Shortcut { sequence: "Ctrl+="; onActivated: backend.zoomIn() }
    Shortcut { sequence: "Ctrl+Shift+="; onActivated: backend.zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: backend.zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: backend.zoomReset() }
    Shortcut { sequence: "Ctrl+W"; onActivated: win.close() }
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
            } else if (win.findOpen) {
                closeFindBar()
                return
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

            Rectangle {
                id: topbar
                width: parent.width
                height: 44
                color: win.bgHeader

                Behavior on color {
                    ColorAnimation { duration: 110; easing.type: Easing.OutCubic }
                }

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

                Rectangle {
                    id: actionGroup
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    height: 30
                    width: groupRow.implicitWidth
                    radius: 8
                    color: "transparent"
                    border.width: 1
                    border.color: win.borderSubtle

                    Row {
                        id: groupRow
                        anchors.fill: parent
                        spacing: 0

                        M3Button {
                            iconKind: "plus"
                            iconOnly: true
                            ghost: true
                            buttonHeight: 30
                            cornerRadius: 7
                            darkMode: win.dark
                            tooltipText: "New File (Ctrl+N)"
                            onClicked: requestGuardedAction("new", "")
                        }

                        Rectangle {
                            width: 1
                            height: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: win.borderSubtle
                        }

                        M3Button {
                            text: "Open"
                            ghost: true
                            buttonHeight: 30
                            horizontalPadding: 11
                            cornerRadius: 7
                            darkMode: win.dark
                            tooltipText: "Open File (Ctrl+O)"
                            onClicked: requestGuardedAction("open", "")
                        }

                        Rectangle {
                            width: 1
                            height: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: win.borderSubtle
                        }

                        M3Button {
                            text: backend.modified ? "• Save" : "Save"
                            ghost: !backend.modified
                            accent: backend.modified
                            buttonHeight: 30
                            horizontalPadding: 11
                            cornerRadius: 7
                            darkMode: win.dark
                            tooltipText: "Save File (Ctrl+S)"
                            onClicked: backend.saveFile()
                        }
                    }
                }

                Row {
                    id: centerTitle
                    anchors.centerIn: parent
                    spacing: 7
                    width: Math.min(24 + titleLabel.implicitWidth, Math.max(80, topbar.width - actionGroup.width - rightControls.width - 48))

                    Image {
                        source: "qrc:/assets/nulla.svg"
                        width: 15
                        height: 15
                        sourceSize: Qt.size(32, 32)
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: win.dark ? 0.85 : 0.78
                    }

                    Text {
                        id: titleLabel
                        width: Math.min(implicitWidth, centerTitle.width - 24)
                        elide: Text.ElideMiddle
                        text: (backend.modified ? "• " : "") + backend.fileName
                        color: backend.modified ? win.fgMain : win.fgMuted
                        font.pixelSize: 13
                        font.weight: backend.modified ? Font.DemiBold : Font.Medium
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Row {
                    id: rightControls
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    M3Button {
                        iconKind: "search"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
                        darkMode: win.dark
                        tooltipText: "Find (Ctrl+F)"
                        onClicked: openFindBar()
                    }

                    M3Button {
                        iconKind: "gear"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
                        darkMode: win.dark
                        tooltipText: "Settings (Ctrl+,)"
                        onClicked: settingsModal.open = true
                    }

                    M3Button {
                        id: menuBtn
                        iconKind: "dots"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
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

                    Rectangle {
                        visible: backend.showWindowControls
                        width: 1
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        color: win.borderSubtle
                    }

                    M3Button {
                        visible: backend.showWindowControls
                        iconKind: "minus"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
                        darkMode: win.dark
                        tooltipText: "Minimize"
                        onClicked: win.showMinimized()
                    }

                    M3Button {
                        visible: backend.showWindowControls
                        iconKind: "max"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
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
                        visible: backend.showWindowControls
                        iconKind: "close"
                        iconOnly: true
                        ghost: true
                        buttonHeight: 28
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

                Column {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        id: findBar
                        visible: win.findOpen
                        width: parent.width
                        height: visible ? 38 : 0
                        color: win.bgGutter

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: Math.min(260, Math.max(120, findBar.width - 190))
                                height: 26
                                radius: 7
                                color: win.bgEditor
                                border.width: 1
                                border.color: findInput.activeFocus ? win.borderFocus : win.borderSubtle

                                TextInput {
                                    id: findInput
                                    anchors.fill: parent
                                    anchors.leftMargin: 9
                                    anchors.rightMargin: 9
                                    verticalAlignment: TextInput.AlignVCenter
                                    color: win.fgMain
                                    selectionColor: win.selBg
                                    font.pixelSize: 12
                                    clip: true
                                    onTextChanged: {
                                        win.findQuery = text
                                        win.updateMatchStats()
                                    }
                                    Keys.onPressed: function(event) {
                                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                            findNext((event.modifiers & Qt.ShiftModifier) !== 0)
                                            event.accepted = true
                                        } else if (event.key === Qt.Key_Escape) {
                                            closeFindBar()
                                            event.accepted = true
                                        }
                                    }

                                    Text {
                                        visible: findInput.text === ""
                                        text: "Find..."
                                        color: win.fgGutter
                                        font.pixelSize: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 64
                                text: win.findQuery === ""
                                      ? ""
                                      : (win.matchTotal > 0 ? (win.matchCurrent + " / " + win.matchTotal) : "0 found")
                                color: win.fgMuted
                                font.pixelSize: 12
                            }

                            M3Button {
                                anchors.verticalCenter: parent.verticalCenter
                                iconKind: "up"
                                iconOnly: true
                                buttonHeight: 24
                                cornerRadius: 7
                                darkMode: win.dark
                                tooltipText: "Previous (Shift+Enter)"
                                onClicked: findNext(true)
                            }

                            M3Button {
                                anchors.verticalCenter: parent.verticalCenter
                                iconKind: "chevron"
                                iconOnly: true
                                buttonHeight: 24
                                cornerRadius: 7
                                darkMode: win.dark
                                tooltipText: "Next (Enter)"
                                onClicked: findNext(false)
                            }

                            Item {
                                width: Math.max(1, findBar.width - 396)
                                height: 1
                            }

                            M3Button {
                                anchors.verticalCenter: parent.verticalCenter
                                iconKind: "close"
                                iconOnly: true
                                ghost: true
                                buttonHeight: 24
                                cornerRadius: 7
                                darkMode: win.dark
                                tooltipText: "Close (Esc)"
                                onClicked: closeFindBar()
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            anchors.bottom: parent.bottom
                            color: win.borderSubtle
                        }
                    }

                    Item {
                        width: parent.width
                        height: parent.height - findBar.height

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

                            Item {
                                width: parent.width - gutterContainer.width
                                height: parent.height

                                Flickable {
                                    id: flick
                                    anchors.fill: parent
                                    interactive: false
                                    contentWidth: Math.max(width, editor.contentWidth + 20)
                                    contentHeight: Math.max(height, editor.contentHeight + 16)
                                    clip: true
                                    boundsBehavior: Flickable.StopAtBounds

                                    property real targetY: 0
                                    property real targetX: 0

                                    function clampY(v) {
                                        return Math.max(0, Math.min(Math.max(0, contentHeight - height), v))
                                    }

                                    function clampX(v) {
                                        return Math.max(0, Math.min(Math.max(0, contentWidth - width), v))
                                    }

                                    function stopScrollAnims() {
                                        scrollYAnim.stop()
                                        scrollXAnim.stop()
                                        targetY = contentY
                                        targetX = contentX
                                    }

                                    function smoothScrollBy(dx, dy, animDuration) {
                                        if (dy !== 0) {
                                            var baseY = scrollYAnim.running ? targetY : contentY
                                            var nextY = clampY(baseY + dy)
                                            targetY = nextY
                                            if (animDuration > 0) {
                                                scrollYAnim.duration = animDuration
                                                scrollYAnim.to = nextY
                                                scrollYAnim.restart()
                                            } else {
                                                scrollYAnim.stop()
                                                contentY = nextY
                                            }
                                        }
                                        if (dx !== 0) {
                                            var baseX = scrollXAnim.running ? targetX : contentX
                                            var nextX = clampX(baseX + dx)
                                            targetX = nextX
                                            if (animDuration > 0) {
                                                scrollXAnim.duration = animDuration
                                                scrollXAnim.to = nextX
                                                scrollXAnim.restart()
                                            } else {
                                                scrollXAnim.stop()
                                                contentX = nextX
                                            }
                                        }
                                    }

                                    onContentHeightChanged: {
                                        var maxY = Math.max(0, contentHeight - height)
                                        if (contentY > maxY) {
                                            scrollYAnim.stop()
                                            contentY = maxY
                                            targetY = maxY
                                        }
                                    }

                                    onContentWidthChanged: {
                                        var maxX = Math.max(0, contentWidth - width)
                                        if (contentX > maxX) {
                                            scrollXAnim.stop()
                                            contentX = maxX
                                            targetX = maxX
                                        }
                                    }

                                    NumberAnimation {
                                        id: scrollYAnim
                                        target: flick
                                        property: "contentY"
                                        duration: 240
                                        easing.type: Easing.OutCubic
                                    }

                                    NumberAnimation {
                                        id: scrollXAnim
                                        target: flick
                                        property: "contentX"
                                        duration: 200
                                        easing.type: Easing.OutCubic
                                    }

                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                        width: 8
                                        onPressedChanged: if (pressed) flick.stopScrollAnims()
                                        contentItem: Rectangle {
                                            implicitWidth: 5
                                            radius: 3
                                            color: parent.pressed
                                                   ? (win.dark ? "#7d8594" : "#6c7382")
                                                   : (parent.hovered ? (win.dark ? "#5a606e" : "#8d94a3") : (win.dark ? "#373b46" : "#c1c6d0"))
                                            Behavior on color { ColorAnimation { duration: 100 } }
                                        }
                                    }

                                    ScrollBar.horizontal: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                        height: 8
                                        onPressedChanged: if (pressed) flick.stopScrollAnims()
                                        contentItem: Rectangle {
                                            implicitHeight: 5
                                            radius: 3
                                            color: parent.pressed
                                                   ? (win.dark ? "#7d8594" : "#6c7382")
                                                   : (parent.hovered ? (win.dark ? "#5a606e" : "#8d94a3") : (win.dark ? "#373b46" : "#c1c6d0"))
                                            Behavior on color { ColorAnimation { duration: 100 } }
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

                                        property bool suppressCursorScroll: false
                                        property int lastZoomPercent: backend.zoomPercent

                                        Timer {
                                            id: zoomSuppressTimer
                                            interval: 60
                                            onTriggered: editor.suppressCursorScroll = false
                                        }

                                        Connections {
                                            target: backend
                                            function onSettingsChanged() {
                                                if (editor.lastZoomPercent !== backend.zoomPercent) {
                                                    editor.lastZoomPercent = backend.zoomPercent
                                                    editor.suppressCursorScroll = true
                                                    zoomSuppressTimer.restart()
                                                }
                                            }
                                        }

                                        onCursorPositionChanged: backend.updateCursorPosition(cursorPosition)

                                        onCursorRectangleChanged: {
                                            if (suppressCursorScroll) {
                                                return
                                            }
                                            var r = cursorRectangle
                                            var cy = r.y + y
                                            var cx = r.x + x
                                            if (cy - flick.contentY < 8) {
                                                flick.stopScrollAnims()
                                                flick.contentY = Math.max(0, cy - 8)
                                                flick.targetY = flick.contentY
                                            } else if (cy + r.height - flick.contentY > flick.height - 8) {
                                                flick.stopScrollAnims()
                                                flick.contentY = Math.max(0, cy + r.height - flick.height + 8)
                                                flick.targetY = flick.contentY
                                            }
                                            if (cx - flick.contentX < 10) {
                                                flick.stopScrollAnims()
                                                flick.contentX = Math.max(0, cx - 10)
                                                flick.targetX = flick.contentX
                                            } else if (cx + r.width - flick.contentX > flick.width - 10) {
                                                flick.stopScrollAnims()
                                                flick.contentX = Math.max(0, cx + r.width - flick.width + 10)
                                                flick.targetX = flick.contentX
                                            }
                                        }

                                        Keys.onPressed: function(event) {
                                            if (event.key === Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier) === 0) {
                                                indentSelection(false)
                                                event.accepted = true
                                                return
                                            }
                                            if (event.key === Qt.Key_Backtab ||
                                                (event.key === Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier) !== 0)) {
                                                indentSelection(true)
                                                event.accepted = true
                                                return
                                            }
                                            if ((event.modifiers & Qt.ControlModifier) !== 0 &&
                                                (event.modifiers & (Qt.AltModifier | Qt.MetaModifier)) === 0) {
                                                var k = event.key
                                                var shift = (event.modifiers & Qt.ShiftModifier) !== 0
                                                if (k === Qt.Key_Z && shift) {
                                                    editor.redo()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === Qt.Key_D || k === 0x0412 || k === 0x0432) {
                                                    duplicateCurrentLine()
                                                    event.accepted = true
                                                    return
                                                }
                                                if ((k === Qt.Key_X || k === 0x0427 || k === 0x0447) && !shift) {
                                                    if (editor.selectionStart === editor.selectionEnd) {
                                                        cutCurrentLine()
                                                    } else {
                                                        editor.cut()
                                                    }
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x042f || k === 0x044f) {
                                                    if (shift) editor.redo()
                                                    else editor.undo()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x041d || k === 0x043d) {
                                                    editor.redo()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0421 || k === 0x0441) {
                                                    editor.copy()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x041c || k === 0x043c) {
                                                    editor.paste()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0424 || k === 0x0444) {
                                                    editor.selectAll()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0410 || k === 0x0430) {
                                                    openFindBar()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x042b || k === 0x044b) {
                                                    if (shift) backend.saveFileAsDialog()
                                                    else backend.saveFile()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0429 || k === 0x0449) {
                                                    requestGuardedAction("open", "")
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0422 || k === 0x0442) {
                                                    requestGuardedAction("new", "")
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0419 || k === 0x0439 || k === 0x0426 || k === 0x0446) {
                                                    win.close()
                                                    event.accepted = true
                                                    return
                                                }
                                                if (k === 0x0411 || k === 0x0431) {
                                                    settingsModal.open = !settingsModal.open
                                                    event.accepted = true
                                                    return
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.NoButton

                            property real zoomAccum: 0

                            onWheel: function(wheel) {
                                if ((wheel.modifiers & Qt.ControlModifier) !== 0) {
                                    var zDelta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y
                                    if (Math.abs(zDelta) >= 120) {
                                        editor.suppressCursorScroll = true
                                        zoomSuppressTimer.restart()
                                        if (zDelta > 0) backend.zoomIn()
                                        else backend.zoomOut()
                                        zoomAccum = 0
                                    } else if (zDelta !== 0) {
                                        zoomAccum += zDelta
                                        if (Math.abs(zoomAccum) >= 50) {
                                            editor.suppressCursorScroll = true
                                            zoomSuppressTimer.restart()
                                            if (zoomAccum > 0) backend.zoomIn()
                                            else backend.zoomOut()
                                            zoomAccum = 0
                                        }
                                    }
                                    wheel.accepted = true
                                    return
                                }

                                var shiftHeld = (wheel.modifiers & Qt.ShiftModifier) !== 0
                                var dx = 0
                                var dy = 0
                                var animMs = 240

                                if (wheel.pixelDelta.y !== 0 && Math.abs(wheel.angleDelta.y) < 120) {
                                    dy = -wheel.pixelDelta.y * 1.45
                                    animMs = 90
                                } else if (wheel.angleDelta.y !== 0) {
                                    var stepPx = Math.max(150, editor.font.pixelSize * 10.5)
                                    dy = -(wheel.angleDelta.y / 120.0) * stepPx
                                }

                                if (wheel.pixelDelta.x !== 0 && Math.abs(wheel.angleDelta.x) < 120) {
                                    dx = -wheel.pixelDelta.x * 1.45
                                    animMs = 90
                                } else if (wheel.angleDelta.x !== 0) {
                                    var stepX = Math.max(120, editor.font.pixelSize * 8)
                                    dx = -(wheel.angleDelta.x / 120.0) * stepX
                                }

                                if (shiftHeld && dx === 0 && dy !== 0) {
                                    dx = dy
                                    dy = 0
                                }

                                if (dx !== 0 || dy !== 0) {
                                    flick.smoothScrollBy(dx, dy, animMs)
                                }
                                wheel.accepted = true
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
                    anchors.leftMargin: 8
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
                    anchors.rightMargin: 8
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
            x: rightControls.x + menuBtn.x + menuBtn.width - width + 4
            y: topbar.height - 2
            width: 214
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
                        { label: "Find...", shortcut: "Ctrl+F", act: "find", sep: true },
                        { label: "Zoom In", shortcut: "Ctrl++", act: "zoomin", sep: false },
                        { label: "Zoom Out", shortcut: "Ctrl+-", act: "zoomout", sep: false },
                        { label: "Reset Zoom", shortcut: "Ctrl+0", act: "zoomreset", sep: true },
                        { label: "Settings", shortcut: "Ctrl+,", act: "settings", sep: false },
                        { label: "Quit", shortcut: "Ctrl+Q", act: "quit", sep: false }
                    ]

                    delegate: Column {
                        width: 202
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

                            Behavior on color { ColorAnimation { duration: 90 } }
                            Behavior on scale { NumberAnimation { duration: 65 } }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: win.fgMain
                                font.pixelSize: 13
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
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
                                    else if (a === "find") openFindBar()
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
