import QtQuick
import QtQuick.Controls
import Nulla 1.0

Item {
    id: pane
    clip: true

    property var session: null
    property bool darkMode: true
    property var theme: ({})
    property bool showLineNumbers: true
    property int zoomPercent: 100
    property string monoFontFamily: "monospace"
    property var ui: ({})

    property color bgEditor: "#16171b"
    property color bgGutter: "#131418"
    property color bgSurfaceElev: "#1c1f27"
    property color bgHover: "#252934"
    property color bgLineHighlight: "#1c1e24"
    property color borderSubtle: "#22242c"
    property color borderControl: "#313644"
    property color borderFocus: "#6e768b"
    property color fgMain: "#eceef2"
    property color fgMuted: "#9ca3b4"
    property color fgGutter: "#4a4f5c"
    property color selBg: "#2e3d59"

    property alias editorItem: editor
    property alias flickItem: flick
    readonly property int selectedLength: Math.abs(editor.selectionEnd - editor.selectionStart)
    readonly property bool hasSelection: editor.selectionStart !== editor.selectionEnd
    readonly property bool contextMenuOpen: contextMenu.opened

    property bool hoverInsideTextArea: false
    property rect hoverLineRect: Qt.rect(0, 0, 1, 18)
    readonly property bool hoverOnActiveLine: Math.abs(hoverLineRect.y - editor.liveCursorRect.y) < 2
    property bool typingBurst: false

    signal zoomInRequested
    signal zoomOutRequested
    signal openFindRequested
    signal openReplaceRequested
    signal openGotoRequested
    signal newTabRequested
    signal openFileRequested
    signal saveFileRequested
    signal saveAsRequested
    signal closeTabRequested
    signal quitRequested
    signal toggleSettingsRequested

    function focusEditor() {
        editor.forceActiveFocus();
    }

    function closeContextMenu() {
        if (contextMenu.opened) {
            contextMenu.close();
        }
    }

    function updateHoverGuide(vx, vy) {
        var rightLimit = (vScroll.visible && vScroll.size < 1.0) ? (editorViewport.width - 8) : editorViewport.width;
        var bottomLimit = (hScroll.visible && hScroll.size < 1.0) ? (editorViewport.height - 8) : editorViewport.height;
        if (vx < 0 || vy < 0 || vx >= rightLimit || vy >= bottomLimit) {
            hoverInsideTextArea = false;
            return;
        }
        hoverInsideTextArea = true;
        var docX = vx + flick.contentX - editor.x;
        var docY = vy + flick.contentY - editor.y;
        var pos = editor.positionAt(docX, docY);
        var r = editor.positionToRectangle(pos);
        if (r.height <= 0) {
            r = Qt.rect(r.x, r.y, Math.max(1, r.width), editorMetrics.height);
        }
        hoverLineRect = r;
    }

    function lineBoundsAt(pos) {
        var text = editor.text;
        var start = pos === 0 ? 0 : text.lastIndexOf("\n", pos - 1) + 1;
        var end = text.indexOf("\n", pos);
        if (end === -1) {
            end = text.length;
        }
        return {
            start: start,
            end: end
        };
    }

    function indentSelection(unindent) {
        var text = editor.text;
        var selStart = editor.selectionStart;
        var selEnd = editor.selectionEnd;
        if (selStart === selEnd && !unindent) {
            editor.insert(editor.cursorPosition, "    ");
            return;
        }
        var lineStart = selStart === 0 ? 0 : text.lastIndexOf("\n", selStart - 1) + 1;
        var lineEnd = text.indexOf("\n", selEnd);
        if (lineEnd === -1) {
            lineEnd = text.length;
        }
        var block = text.slice(lineStart, lineEnd);
        var lines = block.split("\n");
        for (var i = 0; i < lines.length; i++) {
            if (unindent) {
                if (lines[i].startsWith("    ")) {
                    lines[i] = lines[i].slice(4);
                } else if (lines[i].startsWith("\t")) {
                    lines[i] = lines[i].slice(1);
                } else {
                    lines[i] = lines[i].replace(/^ {1,3}/, "");
                }
            } else {
                lines[i] = "    " + lines[i];
            }
        }
        var updated = lines.join("\n");
        editor.remove(lineStart, lineEnd);
        editor.insert(lineStart, updated);
        editor.select(lineStart, lineStart + updated.length);
    }

    function duplicateCurrentLine() {
        var text = editor.text;
        var selStart = editor.selectionStart;
        var selEnd = editor.selectionEnd;
        var first = lineBoundsAt(selStart);
        var last = lineBoundsAt(selEnd);
        var block = text.slice(first.start, last.end);
        editor.insert(last.end, "\n" + block);
        if (selStart === selEnd) {
            editor.cursorPosition = selStart + block.length + 1;
        } else {
            editor.select(last.end + 1, last.end + 1 + block.length);
        }
    }

    function cutCurrentLine() {
        var text = editor.text;
        if (text.length === 0) {
            return;
        }
        var b = lineBoundsAt(editor.cursorPosition);
        if (b.end < text.length) {
            editor.select(b.start, b.end + 1);
        } else if (b.start > 0) {
            editor.select(b.start - 1, b.end);
        } else {
            editor.select(0, text.length);
        }
        editor.cut();
    }

    function copyCurrentLine() {
        var text = editor.text;
        if (text.length === 0) {
            return;
        }
        var pos = editor.cursorPosition;
        var b = lineBoundsAt(pos);
        if (b.end < text.length) {
            editor.select(b.start, b.end + 1);
        } else {
            editor.select(b.start, b.end);
        }
        editor.copy();
        editor.cursorPosition = pos;
    }

    function deleteCurrentLine() {
        var text = editor.text;
        if (text.length === 0) {
            return;
        }
        var b = lineBoundsAt(editor.cursorPosition);
        if (b.end < text.length) {
            editor.remove(b.start, b.end + 1);
            editor.cursorPosition = b.start;
        } else if (b.start > 0) {
            editor.remove(b.start - 1, b.end);
            editor.cursorPosition = b.start - 1;
        } else {
            editor.remove(0, text.length);
        }
    }

    function selectCurrentLine() {
        var text = editor.text;
        var b = lineBoundsAt(editor.cursorPosition);
        var endPos = b.end < text.length ? (b.end + 1) : b.end;
        editor.select(b.start, endPos);
    }

    function insertLine(above) {
        var b = lineBoundsAt(editor.cursorPosition);
        if (above) {
            editor.insert(b.start, "\n");
            editor.cursorPosition = b.start;
        } else {
            editor.insert(b.end, "\n");
            editor.cursorPosition = b.end + 1;
        }
    }

    function moveCurrentLine(down) {
        var text = editor.text;
        var pos = editor.cursorPosition;
        var cur = lineBoundsAt(pos);
        var col = pos - cur.start;
        var line = text.slice(cur.start, cur.end);
        if (!down) {
            if (cur.start === 0) {
                return;
            }
            var prev = lineBoundsAt(cur.start - 1);
            var prevLine = text.slice(prev.start, prev.end);
            editor.remove(prev.start, cur.end);
            editor.insert(prev.start, line + "\n" + prevLine);
            editor.cursorPosition = prev.start + Math.min(col, line.length);
        } else {
            if (cur.end >= text.length) {
                return;
            }
            var next = lineBoundsAt(cur.end + 1);
            var nextLine = text.slice(next.start, next.end);
            editor.remove(cur.start, next.end);
            editor.insert(cur.start, nextLine + "\n" + line);
            editor.cursorPosition = cur.start + nextLine.length + 1 + Math.min(col, line.length);
        }
    }

    function toggleLineComment() {
        var prefix = pane.session ? pane.session.commentPrefix : "#";
        if (!prefix || prefix === "") {
            return;
        }
        var text = editor.text;
        var selStart = editor.selectionStart;
        var selEnd = editor.selectionEnd;
        var first = lineBoundsAt(selStart);
        var last = lineBoundsAt(selEnd);
        var block = text.slice(first.start, last.end);
        var lines = block.split("\n");
        var allCommented = true;
        for (var i = 0; i < lines.length; i++) {
            var trimmed = lines[i].trim();
            if (trimmed !== "" && !trimmed.startsWith(prefix)) {
                allCommented = false;
                break;
            }
        }
        for (var j = 0; j < lines.length; j++) {
            if (allCommented) {
                var idx = lines[j].indexOf(prefix);
                if (idx !== -1) {
                    var cutLen = prefix.length;
                    if (lines[j].charAt(idx + cutLen) === " ") {
                        cutLen += 1;
                    }
                    lines[j] = lines[j].slice(0, idx) + lines[j].slice(idx + cutLen);
                }
            } else if (lines[j].trim() !== "" || lines.length === 1) {
                var wsMatch = lines[j].match(/^\s*/);
                var indent = wsMatch ? wsMatch[0] : "";
                lines[j] = indent + prefix + " " + lines[j].slice(indent.length);
            }
        }
        var updated = lines.join("\n");
        editor.remove(first.start, last.end);
        editor.insert(first.start, updated);
        if (selStart === selEnd) {
            editor.cursorPosition = first.start + updated.length;
        } else {
            editor.select(first.start, first.start + updated.length);
        }
    }

    function jumpToLine(lineNumber) {
        if (!pane.session) {
            return;
        }
        var pos = pane.session.lineStartPosition(lineNumber);
        editor.forceActiveFocus();
        editor.cursorPosition = pos;
    }

    function suppressZoomScroll() {
        editor.suppressCursorScroll = true;
        zoomSuppressTimer.restart();
    }

    Component.onCompleted: {
        if (pane.session) {
            pane.session.attachDocument(editor.textDocument);
        }
    }

    onZoomPercentChanged: suppressZoomScroll()

    onSessionChanged: {
        if (pane.session) {
            pane.session.attachDocument(editor.textDocument);
        }
    }

    Connections {
        target: pane.session
        function onDocumentLoaded() {
            flick.stopScrollAnims();
            editor.cursorPosition = 0;
            flick.contentY = 0;
            flick.contentX = 0;
            flick.targetY = 0;
            flick.targetX = 0;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: pane.bgEditor

        Behavior on color {
            ColorAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }
    }

    Item {
        anchors.fill: parent
        clip: true

        Rectangle {
            id: gutterContainer
            visible: pane.showLineNumbers
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: visible ? gutter.implicitWidth : 0
            color: pane.bgGutter

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }

            LineNumberGutter {
                id: gutter
                anchors.fill: parent
                document: editor.textDocument
                contentY: flick.contentY
                currentLine: pane.session ? pane.session.cursorLine : 1
                font: editor.font
                textColor: pane.fgGutter
                activeTextColor: pane.fgMain
                activeLineColor: editor.activeFocus ? pane.bgLineHighlight : "transparent"
                topPadding: 8
            }

            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: pane.borderSubtle
            }
        }

        Item {
            id: editorViewport
            anchors.left: gutterContainer.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            clip: true

            Item {
                anchors.fill: parent
                anchors.rightMargin: (vScroll.visible && vScroll.size < 1.0) ? 8 : 0
                anchors.bottomMargin: (hScroll.visible && hScroll.size < 1.0) ? 8 : 0

                HoverHandler {
                    id: editorHover
                    cursorShape: Qt.IBeamCursor
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onPointChanged: pane.updateHoverGuide(point.position.x, point.position.y)
                    onHoveredChanged: {
                        if (!hovered) {
                            pane.hoverInsideTextArea = false;
                        } else {
                            pane.updateHoverGuide(point.position.x, point.position.y);
                        }
                    }
                }
            }

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
                    return Math.max(0, Math.min(Math.max(0, contentHeight - height), v));
                }

                function clampX(v) {
                    return Math.max(0, Math.min(Math.max(0, contentWidth - width), v));
                }

                function stopScrollAnims() {
                    scrollYAnim.stop();
                    scrollXAnim.stop();
                    targetY = contentY;
                    targetX = contentX;
                }

                function smoothScrollBy(dx, dy, animDuration) {
                    if (dy !== 0) {
                        var baseY = scrollYAnim.running ? targetY : contentY;
                        var nextY = clampY(baseY + dy);
                        targetY = nextY;
                        if (animDuration > 0) {
                            scrollYAnim.duration = animDuration;
                            scrollYAnim.to = nextY;
                            scrollYAnim.restart();
                        } else {
                            scrollYAnim.stop();
                            contentY = nextY;
                        }
                    }
                    if (dx !== 0) {
                        var baseX = scrollXAnim.running ? targetX : contentX;
                        var nextX = clampX(baseX + dx);
                        targetX = nextX;
                        if (animDuration > 0) {
                            scrollXAnim.duration = animDuration;
                            scrollXAnim.to = nextX;
                            scrollXAnim.restart();
                        } else {
                            scrollXAnim.stop();
                            contentX = nextX;
                        }
                    }
                }

                onContentYChanged: {
                    if (editorHover.hovered) {
                        pane.updateHoverGuide(editorHover.point.position.x, editorHover.point.position.y);
                    }
                }

                onContentXChanged: {
                    if (editorHover.hovered) {
                        pane.updateHoverGuide(editorHover.point.position.x, editorHover.point.position.y);
                    }
                }

                onContentHeightChanged: {
                    var maxY = Math.max(0, contentHeight - height);
                    if (contentY > maxY) {
                        scrollYAnim.stop();
                        contentY = maxY;
                        targetY = maxY;
                    }
                }

                onContentWidthChanged: {
                    var maxX = Math.max(0, contentWidth - width);
                    if (contentX > maxX) {
                        scrollXAnim.stop();
                        contentX = maxX;
                        targetX = maxX;
                    }
                }

                onHeightChanged: {
                    var maxY = Math.max(0, contentHeight - height);
                    if (contentY > maxY) {
                        scrollYAnim.stop();
                        contentY = maxY;
                        targetY = maxY;
                    }
                }

                onWidthChanged: {
                    var maxX = Math.max(0, contentWidth - width);
                    if (contentX > maxX) {
                        scrollXAnim.stop();
                        contentX = maxX;
                        targetX = maxX;
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
                    id: vScroll
                    policy: ScrollBar.AsNeeded
                    width: 8
                    onPressedChanged: if (pressed)
                        flick.stopScrollAnims()
                    contentItem: Rectangle {
                        implicitWidth: 5
                        radius: 3
                        color: parent.pressed ? (pane.theme.scrollPress || (pane.darkMode ? "#7d8594" : "#6c7382")) : (parent.hovered ? (pane.theme.scrollHover || (pane.darkMode ? "#5a606e" : "#8d94a3")) : (pane.theme.scrollIdle || (pane.darkMode ? "#373b46" : "#c1c6d0")))
                        Behavior on color {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    id: hScroll
                    policy: ScrollBar.AsNeeded
                    height: 8
                    onPressedChanged: if (pressed)
                        flick.stopScrollAnims()
                    contentItem: Rectangle {
                        implicitHeight: 5
                        radius: 3
                        color: parent.pressed ? (pane.theme.scrollPress || (pane.darkMode ? "#7d8594" : "#6c7382")) : (parent.hovered ? (pane.theme.scrollHover || (pane.darkMode ? "#5a606e" : "#8d94a3")) : (pane.theme.scrollIdle || (pane.darkMode ? "#373b46" : "#c1c6d0")))
                        Behavior on color {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    }
                }

                FontMetrics {
                    id: editorMetrics
                    font: editor.font
                }

                Rectangle {
                    id: hoverLineBand
                    x: 0
                    y: editor.y + pane.hoverLineRect.y
                    width: Math.max(flick.width, flick.contentWidth)
                    height: Math.max(editorMetrics.height, pane.hoverLineRect.height)
                    color: pane.theme.hoverLineBand || (pane.darkMode ? "#0effffff" : "#07000000")
                    opacity: (pane.hoverInsideTextArea && !contextMenu.visible && !pane.hoverOnActiveLine) ? 1.0 : 0.0
                    visible: opacity > 0.01

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 75
                            easing.type: Easing.OutCubic
                        }
                    }

                    Rectangle {
                        width: 2
                        height: parent.height
                        anchors.left: parent.left
                        color: pane.borderFocus
                        opacity: 0.6
                    }
                }

                Rectangle {
                    id: currentLineBand
                    visible: editor.activeFocus
                    x: 0
                    y: editor.y + editor.liveCursorRect.y
                    width: Math.max(flick.width, flick.contentWidth)
                    height: Math.max(editorMetrics.height, editor.liveCursorRect.height)
                    color: pane.hasSelection ? (pane.theme.bgLineHighlightSel || (pane.darkMode ? "#1f2738" : "#eaf0fb")) : pane.bgLineHighlight

                    Behavior on color {
                        ColorAnimation {
                            duration: 90
                        }
                    }

                    Rectangle {
                        width: 2
                        height: parent.height
                        anchors.left: parent.left
                        color: pane.hasSelection ? (pane.theme.accent || (pane.darkMode ? "#7aa2f7" : "#2563eb")) : pane.fgMain
                        opacity: pane.hasSelection ? 0.95 : (pane.hoverInsideTextArea ? 0.85 : 0.45)

                        Behavior on opacity {
                            NumberAnimation {
                                duration: 90
                            }
                        }
                        Behavior on color {
                            ColorAnimation {
                                duration: 90
                            }
                        }
                    }

                    Rectangle {
                        visible: pane.hasSelection || pane.hoverInsideTextArea
                        width: parent.width
                        height: 1
                        anchors.top: parent.top
                        color: pane.borderFocus
                        opacity: pane.hasSelection ? 0.28 : 0.14
                    }

                    Rectangle {
                        visible: pane.hasSelection || pane.hoverInsideTextArea
                        width: parent.width
                        height: 1
                        anchors.bottom: parent.bottom
                        color: pane.borderFocus
                        opacity: pane.hasSelection ? 0.28 : 0.14
                    }
                }

                Rectangle {
                    id: caretHalo
                    visible: editor.activeFocus
                    x: editor.x + editor.liveCursorRect.x - 2
                    y: editor.y + editor.liveCursorRect.y
                    width: 6
                    height: Math.max(editorMetrics.height, editor.liveCursorRect.height)
                    radius: 3
                    color: pane.hasSelection ? (pane.theme.accent || (pane.darkMode ? "#7aa2f7" : "#2563eb")) : pane.fgMain
                    opacity: pane.hasSelection ? 0.24 : (pane.typingBurst ? 0.22 : (pane.hoverInsideTextArea ? 0.1 : 0.0))

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 130
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                Rectangle {
                    id: ghostCaret
                    x: editor.x + pane.hoverLineRect.x
                    y: editor.y + pane.hoverLineRect.y
                    width: 1.5
                    height: Math.max(editorMetrics.height, pane.hoverLineRect.height)
                    radius: 1
                    color: pane.fgMain
                    opacity: (pane.hoverInsideTextArea && !contextMenu.visible && !pane.hasSelection && (Math.abs(pane.hoverLineRect.x - editor.liveCursorRect.x) > 3 || !pane.hoverOnActiveLine)) ? 0.34 : 0.0
                    visible: opacity > 0.01

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 75
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                MouseArea {
                    x: 0
                    y: 0
                    width: Math.max(flick.width, flick.contentWidth)
                    height: Math.max(flick.height, flick.contentHeight)
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.IBeamCursor
                    property int dragAnchor: 0
                    onPressed: function (mouse) {
                        editor.forceActiveFocus();
                        dragAnchor = editor.positionAt(mouse.x - editor.x, mouse.y - editor.y);
                        editor.cursorPosition = dragAnchor;
                    }
                    onPositionChanged: function (mouse) {
                        if ((mouse.buttons & Qt.LeftButton) !== 0) {
                            var pos = editor.positionAt(mouse.x - editor.x, mouse.y - editor.y);
                            editor.select(dragAnchor, pos);
                        }
                    }
                }

                TextEdit {
                    id: editor
                    x: 10
                    y: 8
                    width: Math.max(flick.width - 10, contentWidth + 10)
                    height: Math.max(flick.height - 8, contentHeight + 8)
                    textFormat: TextEdit.PlainText
                    selectByMouse: true
                    persistentSelection: true
                    wrapMode: TextEdit.NoWrap
                    color: pane.fgMain
                    selectionColor: pane.selBg
                    selectedTextColor: pane.fgMain
                    cursorVisible: activeFocus
                    font.family: pane.monoFontFamily
                    font.pixelSize: Math.round(14 * pane.zoomPercent / 100)

                    cursorDelegate: Component {
                        Item {
                            width: pane.hasSelection ? 2.5 : 2

                            Rectangle {
                                anchors.fill: parent
                                radius: 1
                                color: pane.hasSelection ? (pane.theme.accentStrong || (pane.darkMode ? "#8fb4ff" : "#1d4ed8")) : pane.fgMain
                            }
                        }
                    }

                    property bool suppressCursorScroll: false
                    property int lastZoomPercent: pane.zoomPercent
                    property rect liveCursorRect: Qt.rect(0, 0, 1, 18)

                    function syncCursorBand() {
                        var r = editor.positionToRectangle(editor.cursorPosition);
                        if (r.height <= 0) {
                            r = Qt.rect(r.x, r.y, Math.max(1, r.width), editorMetrics.height);
                        }
                        liveCursorRect = r;
                    }

                    Component.onCompleted: Qt.callLater(syncCursorBand)

                    onFontChanged: {
                        if (lastZoomPercent !== pane.zoomPercent) {
                            lastZoomPercent = pane.zoomPercent;
                            suppressCursorScroll = true;
                            zoomSuppressTimer.restart();
                        }
                        Qt.callLater(syncCursorBand);
                    }

                    onContentHeightChanged: {
                        syncCursorBand();
                        gutter.refresh();
                    }

                    onTextChanged: {
                        pane.typingBurst = true;
                        typingBurstTimer.restart();
                        Qt.callLater(syncCursorBand);
                        if (editorHover.hovered) {
                            Qt.callLater(function () {
                                pane.updateHoverGuide(editorHover.point.position.x, editorHover.point.position.y);
                            });
                        }
                    }

                    Timer {
                        id: zoomSuppressTimer
                        interval: 140
                        onTriggered: editor.suppressCursorScroll = false
                    }

                    Timer {
                        id: typingBurstTimer
                        interval: 420
                        onTriggered: pane.typingBurst = false
                    }

                    onCursorPositionChanged: {
                        syncCursorBand();
                        pane.typingBurst = true;
                        typingBurstTimer.restart();
                        if (pane.session) {
                            pane.session.updateCursorPosition(cursorPosition);
                        }
                    }

                    onCursorRectangleChanged: {
                        syncCursorBand();
                        if (suppressCursorScroll || flick.height <= 24 || flick.width <= 24) {
                            return;
                        }
                        var r = cursorRectangle;
                        var cy = r.y + y;
                        var cx = r.x + x;
                        if (cy - flick.contentY < 8) {
                            flick.stopScrollAnims();
                            flick.contentY = Math.max(0, cy - 8);
                            flick.targetY = flick.contentY;
                        } else if (cy + r.height - flick.contentY > flick.height - 8) {
                            flick.stopScrollAnims();
                            flick.contentY = Math.max(0, cy + r.height - flick.height + 8);
                            flick.targetY = flick.contentY;
                        }
                        if (cx - flick.contentX < 10) {
                            flick.stopScrollAnims();
                            flick.contentX = Math.max(0, cx - 10);
                            flick.targetX = flick.contentX;
                        } else if (cx + r.width - flick.contentX > flick.width - 10) {
                            flick.stopScrollAnims();
                            flick.contentX = Math.max(0, cx + r.width - flick.width + 10);
                            flick.targetX = flick.contentX;
                        }
                    }

                    Keys.onPressed: function (event) {
                        var k = event.key;
                        var ctrl = (event.modifiers & Qt.ControlModifier) !== 0;
                        var shift = (event.modifiers & Qt.ShiftModifier) !== 0;
                        var alt = (event.modifiers & Qt.AltModifier) !== 0;
                        var meta = (event.modifiers & Qt.MetaModifier) !== 0;

                        if (k === Qt.Key_Menu && !ctrl && !alt && !meta) {
                            editor.forceActiveFocus();
                            var cr = editor.liveCursorRect;
                            var mx = gutterContainer.width + editor.x + cr.x - flick.contentX;
                            var my = editor.y + cr.y + cr.height - flick.contentY;
                            contextMenu.openAt(mx, my, pane.width, pane.height);
                            event.accepted = true;
                            return;
                        }
                        if (k === Qt.Key_Tab && !ctrl && !alt && !shift) {
                            indentSelection(false);
                            event.accepted = true;
                            return;
                        }
                        if ((k === Qt.Key_Backtab || (k === Qt.Key_Tab && shift)) && !ctrl && !alt) {
                            indentSelection(true);
                            event.accepted = true;
                            return;
                        }
                        if (alt && !ctrl && !meta) {
                            if (k === Qt.Key_Up) {
                                moveCurrentLine(false);
                                event.accepted = true;
                                return;
                            }
                            if (k === Qt.Key_Down) {
                                moveCurrentLine(true);
                                event.accepted = true;
                                return;
                            }
                        }
                        if (ctrl && !alt && !meta) {
                            if (k === Qt.Key_Return || k === Qt.Key_Enter) {
                                insertLine(shift);
                                event.accepted = true;
                                return;
                            }
                            if ((k === Qt.Key_Z && shift) || k === Qt.Key_Y || k === 0x041d || k === 0x043d) {
                                editor.redo();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x042f || k === 0x044f) {
                                if (shift)
                                    editor.redo();
                                else
                                    editor.undo();
                                event.accepted = true;
                                return;
                            }
                            if ((k === Qt.Key_X || k === 0x0427 || k === 0x0447) && !shift) {
                                if (editor.selectionStart === editor.selectionEnd) {
                                    cutCurrentLine();
                                } else {
                                    editor.cut();
                                }
                                event.accepted = true;
                                return;
                            }
                            if ((k === Qt.Key_C || k === 0x0421 || k === 0x0441) && !shift) {
                                if (editor.selectionStart === editor.selectionEnd) {
                                    copyCurrentLine();
                                } else {
                                    editor.copy();
                                }
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x041c || k === 0x043c) {
                                editor.paste();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0424 || k === 0x0444) {
                                editor.selectAll();
                                event.accepted = true;
                                return;
                            }
                            if (k === Qt.Key_D || k === 0x0412 || k === 0x0432) {
                                duplicateCurrentLine();
                                event.accepted = true;
                                return;
                            }
                            if (shift && (k === Qt.Key_K || k === 0x041b || k === 0x043b)) {
                                deleteCurrentLine();
                                event.accepted = true;
                                return;
                            }
                            if ((k === Qt.Key_L || k === 0x0414 || k === 0x0434) && !shift) {
                                selectCurrentLine();
                                event.accepted = true;
                                return;
                            }
                            if (k === Qt.Key_Slash || k === Qt.Key_Question) {
                                toggleLineComment();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0410 || k === 0x0430) {
                                pane.openFindRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0420 || k === 0x0440) {
                                pane.openReplaceRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x041f || k === 0x043f) {
                                pane.openGotoRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x042b || k === 0x044b) {
                                if (shift)
                                    pane.saveAsRequested();
                                else
                                    pane.saveFileRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0429 || k === 0x0449) {
                                pane.openFileRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0415 || k === 0x0435 || k === 0x0422 || k === 0x0442) {
                                pane.newTabRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0426 || k === 0x0446) {
                                pane.closeTabRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0419 || k === 0x0439) {
                                pane.quitRequested();
                                event.accepted = true;
                                return;
                            }
                            if (k === 0x0411 || k === 0x0431) {
                                pane.toggleSettingsRequested();
                                event.accepted = true;
                                return;
                            }
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onPressed: function (mouse) {
                    editor.forceActiveFocus();
                    var clickPos = editor.positionAt(mouse.x + flick.contentX - editor.x, mouse.y + flick.contentY - editor.y);
                    if (!pane.hasSelection || clickPos < editor.selectionStart || clickPos > editor.selectionEnd) {
                        editor.cursorPosition = clickPos;
                    }
                    contextMenu.openAt(mouse.x + gutterContainer.width, mouse.y, pane.width, pane.height);
                    mouse.accepted = true;
                }
            }

            MouseArea {
                anchors.fill: parent
                anchors.rightMargin: (vScroll.visible && vScroll.size < 1.0) ? 8 : 0
                anchors.bottomMargin: (hScroll.visible && hScroll.size < 1.0) ? 8 : 0
                acceptedButtons: Qt.NoButton
                hoverEnabled: true
                cursorShape: Qt.IBeamCursor
            }
        }
    }

    EditorContextMenu {
        id: contextMenu
        darkMode: pane.darkMode
        theme: pane.theme
        ui: pane.ui
        canUndo: editor.canUndo
        canRedo: editor.canRedo
        canPaste: editor.canPaste
        hasSelection: pane.hasSelection
        hasText: editor.text.length > 0
        hasSingleLineSelection: pane.hasSelection && editor.selectedText.indexOf("\n") === -1
        canComment: pane.session ? (pane.session.commentPrefix !== "") : true
        bgSurfaceElev: pane.bgSurfaceElev
        bgHover: pane.bgHover
        borderSubtle: pane.borderSubtle
        borderControl: pane.borderControl
        fgMain: pane.fgMain
        fgMuted: pane.fgMuted
        fgGutter: pane.fgGutter

        onClosed: pane.focusEditor()
        onActionTriggered: function (act) {
            if (act === "undo") {
                editor.undo();
            } else if (act === "redo") {
                editor.redo();
            } else if (act === "cut") {
                if (pane.hasSelection)
                    editor.cut();
                else
                    pane.cutCurrentLine();
            } else if (act === "copy") {
                if (pane.hasSelection)
                    editor.copy();
                else
                    pane.copyCurrentLine();
            } else if (act === "paste") {
                editor.paste();
            } else if (act === "delete") {
                if (pane.hasSelection) {
                    editor.remove(editor.selectionStart, editor.selectionEnd);
                }
            } else if (act === "duplicate") {
                pane.duplicateCurrentLine();
            } else if (act === "comment") {
                pane.toggleLineComment();
            } else if (act === "selectall") {
                editor.selectAll();
            } else if (act === "find") {
                pane.openFindRequested();
            }
        }
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

        property real zoomAccum: 0

        onWheel: function (wheel) {
            if (contextMenu.visible) {
                contextMenu.close();
            }
            if ((wheel.modifiers & Qt.ControlModifier) !== 0) {
                var zDelta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
                if (Math.abs(zDelta) >= 120) {
                    pane.suppressZoomScroll();
                    if (zDelta > 0)
                        pane.zoomInRequested();
                    else
                        pane.zoomOutRequested();
                    zoomAccum = 0;
                } else if (zDelta !== 0) {
                    zoomAccum += zDelta;
                    if (Math.abs(zoomAccum) >= 50) {
                        pane.suppressZoomScroll();
                        if (zoomAccum > 0)
                            pane.zoomInRequested();
                        else
                            pane.zoomOutRequested();
                        zoomAccum = 0;
                    }
                }
                wheel.accepted = true;
                return;
            }

            var shiftHeld = (wheel.modifiers & Qt.ShiftModifier) !== 0;
            var dx = 0;
            var dy = 0;
            var animMs = 240;

            if (wheel.pixelDelta.y !== 0 && Math.abs(wheel.angleDelta.y) < 120) {
                dy = -wheel.pixelDelta.y * 1.45;
                animMs = 90;
            } else if (wheel.angleDelta.y !== 0) {
                var stepPx = Math.max(150, editor.font.pixelSize * 10.5);
                dy = -(wheel.angleDelta.y / 120.0) * stepPx;
            }

            if (wheel.pixelDelta.x !== 0 && Math.abs(wheel.angleDelta.x) < 120) {
                dx = -wheel.pixelDelta.x * 1.45;
                animMs = 90;
            } else if (wheel.angleDelta.x !== 0) {
                var stepX = Math.max(120, editor.font.pixelSize * 8);
                dx = -(wheel.angleDelta.x / 120.0) * stepX;
            }

            if (shiftHeld && dx === 0 && dy !== 0) {
                dx = dy;
                dy = 0;
            }

            if (dx !== 0 || dy !== 0) {
                flick.smoothScrollBy(dx, dy, animMs);
            }
            wheel.accepted = true;
        }
    }
}
