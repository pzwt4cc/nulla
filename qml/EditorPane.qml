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
    property bool showIndentGuides: true
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

    property alias editorItem: editorView
    property alias flickItem: flick
    readonly property int selectedLength: pane.session ? pane.session.selectedChars : 0
    readonly property bool hasSelection: editorView.hasSelection
    readonly property bool contextMenuOpen: contextMenu.opened
    readonly property bool hoverOnActiveLine: Math.abs(editorView.hoverRect.y - editorView.cursorRect.y) < 2
    property bool typingBurst: false
    property bool suppressCursorScroll: false
    property int lastZoomPercent: zoomPercent
    property bool restoringScroll: false

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
        editorView.forceActiveFocus();
    }

    function closeContextMenu() {
        if (contextMenu.opened) {
            contextMenu.close();
        }
    }

    function suppressZoomScroll() {
        suppressCursorScroll = true;
        zoomSuppressTimer.restart();
    }

    function ensureCursorVisible() {
        if (suppressCursorScroll || flick.height <= 24 || flick.width <= 24) {
            return;
        }
        const r = editorView.cursorRect;
        const rightLimit = flick.width - editorView.rightInset - 10;
        const bottomLimit = flick.height - editorView.bottomInset - 8;
        if (r.y < 8) {
            flick.stopScrollAnims();
            flick.contentY = flick.clampY(flick.contentY + r.y - 8);
            flick.targetY = flick.contentY;
        } else if (r.y + r.height > bottomLimit) {
            flick.stopScrollAnims();
            flick.contentY = flick.clampY(flick.contentY + (r.y + r.height) - bottomLimit);
            flick.targetY = flick.contentY;
        }
        if (r.x < 10) {
            flick.stopScrollAnims();
            flick.contentX = flick.clampX(flick.contentX + r.x - 10);
            flick.targetX = flick.contentX;
        } else if (r.x + r.width > rightLimit) {
            flick.stopScrollAnims();
            flick.contentX = flick.clampX(flick.contentX + (r.x + r.width) - rightLimit);
            flick.targetX = flick.contentX;
        }
    }

    function jumpToLine(lineNumber) {
        if (!pane.session) {
            return;
        }
        pane.session.gotoLine(lineNumber);
        editorView.forceActiveFocus();
        Qt.callLater(ensureCursorVisible);
    }

    function syncSessionScroll() {
        restoringScroll = true;
        editorView.session = pane.session;
        flick.stopScrollAnims();
        if (pane.session) {
            flick.contentX = flick.clampX(pane.session.scrollX);
            flick.contentY = flick.clampY(pane.session.scrollY);
            flick.targetX = flick.contentX;
            flick.targetY = flick.contentY;
        } else {
            flick.contentX = 0;
            flick.contentY = 0;
            flick.targetX = 0;
            flick.targetY = 0;
        }
        restoringScroll = false;
    }

    Component.onCompleted: syncSessionScroll()

    onSessionChanged: syncSessionScroll()

    onZoomPercentChanged: {
        if (lastZoomPercent !== zoomPercent) {
            lastZoomPercent = zoomPercent;
            suppressZoomScroll();
        }
    }

    Connections {
        target: pane.session
        function onDocumentLoaded() {
            flick.stopScrollAnims();
            flick.contentY = 0;
            flick.contentX = 0;
            flick.targetY = 0;
            flick.targetX = 0;
        }
    }

    Timer {
        id: zoomSuppressTimer
        interval: 140
        onTriggered: pane.suppressCursorScroll = false
    }

    Timer {
        id: typingBurstTimer
        interval: 420
        onTriggered: pane.typingBurst = false
    }

    Rectangle {
        anchors.fill: parent
        color: pane.bgEditor
    }

    Item {
        id: viewportHost
        anchors.fill: parent
        clip: true

        Item {
            id: bandsClip
            x: Math.min(parent.width, editorView.gutterWidth)
            y: 0
            width: Math.max(0, parent.width - x - editorView.rightInset)
            height: Math.max(0, parent.height - editorView.bottomInset)
            clip: true

            Rectangle {
                id: hoverLineBand
                x: 0
                y: editorView.hoverRect.y
                width: bandsClip.width
                height: Math.max(editorView.lineHeight, editorView.hoverRect.height)
                color: pane.theme.hoverLineBand || (pane.darkMode ? "#0effffff" : "#07000000")
                visible: editorView.hoverInside && !contextMenu.visible && !pane.hoverOnActiveLine

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
                visible: editorView.activeFocus
                x: 0
                y: editorView.cursorRect.y
                width: bandsClip.width
                height: Math.max(editorView.lineHeight, editorView.cursorRect.height)
                color: pane.hasSelection ? (pane.theme.bgLineHighlightSel || (pane.darkMode ? "#1f2738" : "#eaf0fb")) : pane.bgLineHighlight

                Rectangle {
                    width: 2
                    height: parent.height
                    anchors.left: parent.left
                    color: pane.hasSelection ? (pane.theme.accent || (pane.darkMode ? "#7aa2f7" : "#2563eb")) : pane.fgMain
                    opacity: pane.hasSelection ? 0.95 : (editorView.hoverInside ? 0.85 : 0.45)
                }

                Rectangle {
                    visible: pane.hasSelection || editorView.hoverInside
                    width: parent.width
                    height: 1
                    anchors.top: parent.top
                    color: pane.borderFocus
                    opacity: pane.hasSelection ? 0.28 : 0.14
                }

                Rectangle {
                    visible: pane.hasSelection || editorView.hoverInside
                    width: parent.width
                    height: 1
                    anchors.bottom: parent.bottom
                    color: pane.borderFocus
                    opacity: pane.hasSelection ? 0.28 : 0.14
                }
            }

            Rectangle {
                id: caretHalo
                visible: editorView.activeFocus
                x: editorView.cursorRect.x - 2
                y: editorView.cursorRect.y
                width: 6
                height: Math.max(editorView.lineHeight, editorView.cursorRect.height)
                radius: 3
                color: pane.hasSelection ? (pane.theme.accent || (pane.darkMode ? "#7aa2f7" : "#2563eb")) : pane.fgMain
                opacity: pane.hasSelection ? 0.24 : (pane.typingBurst ? 0.22 : (editorView.hoverInside ? 0.1 : 0.0))
            }

            Rectangle {
                id: ghostCaret
                x: editorView.hoverRect.x
                y: editorView.hoverRect.y
                width: 1.5
                height: Math.max(editorView.lineHeight, editorView.hoverRect.height)
                radius: 1
                color: pane.fgMain
                opacity: 0.34
                visible: editorView.hoverInside && !contextMenu.visible && !pane.hasSelection && (Math.abs(editorView.hoverRect.x - editorView.cursorRect.x) > 3 || !pane.hoverOnActiveLine)
            }
        }

        NullaEditorView {
            id: editorView
            anchors.fill: parent
            darkMode: pane.darkMode
            showLineNumbers: pane.showLineNumbers
            showIndentGuides: pane.showIndentGuides
            font.family: pane.monoFontFamily
            font.pixelSize: Math.round(14 * pane.zoomPercent / 100)
            contentX: flick.contentX
            contentY: flick.contentY
            rightInset: (vScroll.visible && vScroll.size < 1.0) ? 8 : 0
            bottomInset: (hScroll.visible && hScroll.size < 1.0) ? 8 : 0
            textColor: pane.fgMain
            gutterBackground: pane.bgGutter
            gutterBorder: pane.borderSubtle
            gutterTextColor: pane.fgGutter
            gutterActiveTextColor: pane.fgMain
            gutterActiveLineColor: editorView.activeFocus ? pane.bgLineHighlight : "transparent"
            selectionColor: pane.selBg
            caretColor: pane.fgMain
            caretSelectionColor: pane.theme.accentStrong || (pane.darkMode ? "#8fb4ff" : "#1d4ed8")
            guideColor: pane.theme.indentGuide || (pane.darkMode ? "#262b36" : "#e5e8ef")
            activeGuideColor: pane.theme.indentGuideActive || (pane.darkMode ? "#4e586e" : "#aeb6c7")

            onEdited: {
                pane.typingBurst = true;
                typingBurstTimer.restart();
            }

            onCursorMoved: {
                pane.typingBurst = true;
                typingBurstTimer.restart();
                pane.ensureCursorVisible();
            }

            onContextMenuRequested: function (mx, my) {
                contextMenu.canPaste = editorView.clipboardHasText();
                contextMenu.openAt(mx, my, pane.width, pane.height);
            }

            onScrollByRequested: function (dx, dy) {
                flick.smoothScrollBy(dx, dy, 0);
            }

            onActionRequested: function (act) {
                if (act === "find") {
                    pane.openFindRequested();
                } else if (act === "replace") {
                    pane.openReplaceRequested();
                } else if (act === "goto") {
                    pane.openGotoRequested();
                } else if (act === "save") {
                    pane.saveFileRequested();
                } else if (act === "saveAs") {
                    pane.saveAsRequested();
                } else if (act === "open") {
                    pane.openFileRequested();
                } else if (act === "newTab") {
                    pane.newTabRequested();
                } else if (act === "closeTab") {
                    pane.closeTabRequested();
                } else if (act === "quit") {
                    pane.quitRequested();
                } else if (act === "settings") {
                    pane.toggleSettingsRequested();
                }
            }
        }

        Flickable {
            id: flick
            x: Math.min(parent.width, editorView.gutterWidth)
            y: 0
            width: Math.max(0, parent.width - x)
            height: parent.height
            interactive: false
            contentWidth: Math.max(width, editorView.docContentWidth)
            contentHeight: Math.max(height, editorView.docContentHeight)
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
                    const baseY = scrollYAnim.running ? targetY : contentY;
                    const nextY = clampY(baseY + dy);
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
                    const baseX = scrollXAnim.running ? targetX : contentX;
                    const nextX = clampX(baseX + dx);
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
                if (!pane.restoringScroll && pane.session && editorView.session === pane.session) {
                    pane.session.scrollY = contentY;
                }
            }

            onContentXChanged: {
                if (!pane.restoringScroll && pane.session && editorView.session === pane.session) {
                    pane.session.scrollX = contentX;
                }
            }

            onContentHeightChanged: {
                if (pane.restoringScroll) {
                    return;
                }
                const maxY = Math.max(0, contentHeight - height);
                if (contentY > maxY) {
                    scrollYAnim.stop();
                    contentY = maxY;
                    targetY = maxY;
                }
            }

            onContentWidthChanged: {
                if (pane.restoringScroll) {
                    return;
                }
                const maxX = Math.max(0, contentWidth - width);
                if (contentX > maxX) {
                    scrollXAnim.stop();
                    contentX = maxX;
                    targetX = maxX;
                }
            }

            onHeightChanged: {
                const maxY = Math.max(0, contentHeight - height);
                if (contentY > maxY) {
                    scrollYAnim.stop();
                    contentY = maxY;
                    targetY = maxY;
                }
            }

            onWidthChanged: {
                const maxX = Math.max(0, contentWidth - width);
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
                    color: vScroll.pressed ? (pane.theme.scrollPress || (pane.darkMode ? "#7d8594" : "#6c7382")) : (vScroll.hovered ? (pane.theme.scrollHover || (pane.darkMode ? "#5a606e" : "#8d94a3")) : (pane.theme.scrollIdle || (pane.darkMode ? "#373b46" : "#c1c6d0")))
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
                    color: hScroll.pressed ? (pane.theme.scrollPress || (pane.darkMode ? "#7d8594" : "#6c7382")) : (hScroll.hovered ? (pane.theme.scrollHover || (pane.darkMode ? "#5a606e" : "#8d94a3")) : (pane.theme.scrollIdle || (pane.darkMode ? "#373b46" : "#c1c6d0")))
                }
            }
        }
    }

    EditorContextMenu {
        id: contextMenu
        darkMode: pane.darkMode
        theme: pane.theme
        ui: pane.ui
        canUndo: pane.session ? pane.session.canUndo : false
        canRedo: pane.session ? pane.session.canRedo : false
        canPaste: false
        hasSelection: pane.hasSelection
        hasText: pane.session ? pane.session.hasText : false
        hasSingleLineSelection: pane.session ? pane.session.hasSingleLineSelection : false
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
            editorView.performAction(act);
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
                const zDelta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y;
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

            const shiftHeld = (wheel.modifiers & Qt.ShiftModifier) !== 0;
            let dx = 0;
            let dy = 0;
            let animMs = 240;

            if (wheel.pixelDelta.y !== 0 && Math.abs(wheel.angleDelta.y) < 120) {
                dy = -wheel.pixelDelta.y * 1.45;
                animMs = 90;
            } else if (wheel.angleDelta.y !== 0) {
                const stepPx = Math.max(150, editorView.font.pixelSize * 10.5);
                dy = -(wheel.angleDelta.y / 120.0) * stepPx;
            }

            if (wheel.pixelDelta.x !== 0 && Math.abs(wheel.angleDelta.x) < 120) {
                dx = -wheel.pixelDelta.x * 1.45;
                animMs = 90;
            } else if (wheel.angleDelta.x !== 0) {
                const stepX = Math.max(120, editorView.font.pixelSize * 8);
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
