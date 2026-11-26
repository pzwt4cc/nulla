import QtQuick
import QtQuick.Controls
import Nulla 1.0

ApplicationWindow {
    id: win

    visible: true
    width: 900
    height: 620
    minimumWidth: 520
    minimumHeight: 360
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    title: ((backend.activeTab && backend.activeTab.modified) ? "* " : "") + (backend.activeTab ? backend.activeTab.fileName : "Untitled") + " — Nulla"

    property bool forceClose: false
    property string pendingAction: ""
    property int pendingTabIndex: -1
    property real menuAnchorX: 0

    readonly property bool dark: backend.effectiveDark
    readonly property color bgShell: dark ? "#0d0e11" : "#ebedf2"
    readonly property color bgHeader: dark ? "#0d0e11" : "#ebedf2"
    readonly property color bgEditor: dark ? "#16181e" : "#ffffff"
    readonly property color bgGutter: dark ? "#121318" : "#f5f6f9"
    readonly property color bgSurfaceElev: dark ? "#1c1f27" : "#ffffff"
    readonly property color bgHover: dark ? "#252934" : "#e2e5ed"
    readonly property color bgLineHighlight: dark ? "#1f232c" : "#eff2f8"
    readonly property color borderSubtle: dark ? "#222630" : "#dadde6"
    readonly property color borderControl: dark ? "#313644" : "#c6ccd8"
    readonly property color borderHover: dark ? "#52596d" : "#8b93a5"
    readonly property color borderFocus: dark ? "#6e768b" : "#687183"
    readonly property color fgMain: dark ? "#f0f2f6" : "#14161a"
    readonly property color fgMuted: dark ? "#9ca3b4" : "#565e6d"
    readonly property color fgGutter: dark ? "#565d6e" : "#8c93a3"
    readonly property color selBg: dark ? "#2e3d59" : "#cde0ff"

    readonly property var activePane: tabRepeater.itemAt(backend.activeTabIndex)
    readonly property bool modalOpen: confirmModal.open || settingsModal.open || languageModal.open

    EditorBackend {
        id: backend
        onActiveTabChanged: {
            if (findBar.open && !findBar.gotoMode) {
                updateMatchStats();
            }
            Qt.callLater(focusActiveEditor);
        }
        onErrorOccurred: function (title, message) {
            confirmModal.errorOnly = true;
            confirmModal.title = title;
            confirmModal.message = message;
            confirmModal.open = true;
        }
    }

    Component.onCompleted: {
        if (initialFilesArg && initialFilesArg.length > 0) {
            backend.setInitialFiles(initialFilesArg);
        }
        focusActiveEditor();
    }

    function focusActiveEditor() {
        if (activePane) {
            activePane.focusEditor();
        }
    }

    function openFindBar(replaceMode, gotoMode) {
        findBar.gotoMode = !!gotoMode;
        findBar.replaceMode = !gotoMode && !!replaceMode;
        findBar.open = true;
        var sel = (!gotoMode && activePane) ? activePane.editorItem.selectedText : "";
        if (!gotoMode && sel !== "" && sel.indexOf("\n") === -1) {
            findBar.query = sel;
            findBar.focusInput(sel, false);
        } else {
            findBar.focusInput("", !!gotoMode);
        }
        if (!gotoMode) {
            updateMatchStats();
        }
    }

    function closeFindBar() {
        findBar.open = false;
        findBar.gotoMode = false;
        focusActiveEditor();
    }

    function updateMatchStats() {
        if (!backend.activeTab || !activePane || findBar.query === "") {
            findBar.matchCurrent = 0;
            findBar.matchTotal = 0;
            return;
        }
        var st = backend.activeTab.matchStats(findBar.query, activePane.editorItem.selectionStart);
        findBar.matchCurrent = st.current;
        findBar.matchTotal = st.total;
    }

    function findNext(backward) {
        if (!backend.activeTab || !activePane || findBar.query === "") {
            updateMatchStats();
            return;
        }
        var ed = activePane.editorItem;
        var fromPos = backward ? Math.max(0, ed.selectionStart) : ed.selectionEnd;
        var res = backend.activeTab.findText(findBar.query, fromPos, backward);
        if (res.found) {
            ed.select(res.start, res.end);
        }
        updateMatchStats();
    }

    function replaceOne(replacement) {
        if (!backend.activeTab || !activePane || findBar.query === "") {
            return;
        }
        var ed = activePane.editorItem;
        var res = backend.activeTab.replaceMatch(findBar.query, replacement, ed.selectionStart, ed.selectionEnd);
        if (res.found) {
            ed.select(res.start, res.end);
        }
        updateMatchStats();
    }

    function replaceAll(replacement) {
        if (!backend.activeTab || !activePane || findBar.query === "") {
            return;
        }
        backend.activeTab.replaceAllMatches(findBar.query, replacement);
        updateMatchStats();
    }

    function requestCloseTab(index) {
        if (backend.tabCount === 1 && backend.isActivePristineUntitled()) {
            win.close();
            return;
        }
        if (backend.isTabModified(index)) {
            win.pendingAction = "closeTab";
            win.pendingTabIndex = index;
            confirmModal.errorOnly = false;
            confirmModal.title = "Unsaved changes";
            confirmModal.message = 'Save changes to "' + backend.tabFileName(index) + '" before closing?';
            confirmModal.open = true;
            return;
        }
        backend.closeTabImmediate(index);
    }

    function requestQuit() {
        var dirtyIdx = backend.firstModifiedTabIndex();
        if (dirtyIdx >= 0) {
            backend.activateTab(dirtyIdx);
            win.pendingAction = "quitApp";
            win.pendingTabIndex = dirtyIdx;
            confirmModal.errorOnly = false;
            confirmModal.title = "Unsaved changes";
            confirmModal.message = 'Save changes to "' + backend.tabFileName(dirtyIdx) + '" before exiting?';
            confirmModal.open = true;
            return;
        }
        win.forceClose = true;
        win.close();
    }

    function continuePendingAction() {
        var act = win.pendingAction;
        var idx = win.pendingTabIndex;
        win.pendingAction = "";
        win.pendingTabIndex = -1;
        if (act === "closeTab") {
            backend.closeTabImmediate(idx);
            focusActiveEditor();
        } else if (act === "quitApp") {
            requestQuit();
        }
    }

    onClosing: function (close) {
        if (!win.forceClose && backend.firstModifiedTabIndex() >= 0) {
            close.accepted = false;
            requestQuit();
        }
    }

    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+T"
        onActivated: backend.newTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+N"
        onActivated: backend.newTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+O"
        onActivated: backend.openFileDialog()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+S"
        onActivated: backend.saveFile()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+S"
        onActivated: backend.saveFileAsDialog()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+W"
        onActivated: requestCloseTab(backend.activeTabIndex)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Q"
        onActivated: requestQuit()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Tab"
        onActivated: backend.nextTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+Tab"
        onActivated: backend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Backtab"
        onActivated: backend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+PgDown"
        onActivated: backend.nextTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+PgUp"
        onActivated: backend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+1"
        onActivated: backend.activateTab(0)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+2"
        onActivated: backend.activateTab(1)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+3"
        onActivated: backend.activateTab(2)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+4"
        onActivated: backend.activateTab(3)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+5"
        onActivated: backend.activateTab(4)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+6"
        onActivated: backend.activateTab(5)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+7"
        onActivated: backend.activateTab(6)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+8"
        onActivated: backend.activateTab(7)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+9"
        onActivated: backend.activateTab(8)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+F"
        onActivated: openFindBar(false, false)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+H"
        onActivated: openFindBar(true, false)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+G"
        onActivated: openFindBar(false, true)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "F3"
        onActivated: findNext(false)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Shift+F3"
        onActivated: findNext(true)
    }
    Shortcut {
        sequence: "Ctrl+,"
        onActivated: settingsModal.open = !settingsModal.open
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl++"
        onActivated: backend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+="
        onActivated: backend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+="
        onActivated: backend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+-"
        onActivated: backend.zoomOut()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+0"
        onActivated: backend.zoomReset()
    }
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (win.visibility === Window.FullScreen) {
                win.showNormal();
            } else {
                win.showFullScreen();
            }
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (confirmModal.open) {
                confirmModal.open = false;
                win.pendingAction = "";
                win.pendingTabIndex = -1;
            } else if (languageModal.open) {
                languageModal.open = false;
            } else if (settingsModal.open) {
                settingsModal.open = false;
            } else if (menuPopup.opened) {
                menuPopup.close();
            } else if (activePane && activePane.contextMenuOpen) {
                activePane.closeContextMenu();
            } else if (findBar.open) {
                closeFindBar();
                return;
            }
            focusActiveEditor();
        }
    }

    Rectangle {
        id: shell
        anchors.fill: parent
        readonly property bool edgeToEdge: win.visibility === Window.Maximized || win.visibility === Window.FullScreen
        radius: edgeToEdge ? 0 : 7
        color: win.bgShell
        border.width: edgeToEdge ? 0 : 1
        border.color: win.borderSubtle
        clip: true

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

        Column {
            anchors.fill: parent
            spacing: 0

            TopBar {
                id: topbar
                width: parent.width
                darkMode: win.dark
                activeTab: backend.activeTab
                showWindowControls: backend.showWindowControls
                isMaximized: win.visibility === Window.Maximized
                menuOpened: menuPopup.opened
                bgHeader: win.bgHeader
                bgSurfaceElev: win.bgSurfaceElev
                borderSubtle: win.borderSubtle
                borderControl: win.borderControl
                fgMain: win.fgMain
                fgMuted: win.fgMuted

                onMoveWindowRequested: win.startSystemMove()
                onToggleMaximizeRequested: {
                    if (win.visibility === Window.Maximized) {
                        win.showNormal();
                    } else {
                        win.showMaximized();
                    }
                }
                onMinimizeRequested: win.showMinimized()
                onCloseWindowRequested: win.requestQuit()
                onNewTabRequested: backend.newTab()
                onOpenFileRequested: backend.openFileDialog()
                onSaveFileRequested: backend.saveFile()
                onFindRequested: openFindBar(false, false)
                onSettingsRequested: settingsModal.open = true
                onMenuToggleRequested: function (anchorX) {
                    win.menuAnchorX = anchorX;
                    if (menuPopup.opened) {
                        menuPopup.close();
                    } else {
                        menuPopup.open();
                    }
                }
            }

            TabStrip {
                id: tabBar
                width: parent.width
                darkMode: win.dark
                tabModel: backend.tabModel
                tabCount: backend.tabCount
                activeIndex: backend.activeTabIndex
                bgShell: win.bgShell
                bgEditor: win.bgEditor
                bgHover: win.bgHover
                borderSubtle: win.borderSubtle
                borderControl: win.borderControl
                borderHover: win.borderHover
                fgMain: win.fgMain
                fgMuted: win.fgMuted

                onTabActivated: function (index) {
                    backend.activateTab(index);
                }
                onTabClosed: function (index) {
                    requestCloseTab(index);
                }
                onNewTabRequested: backend.newTab()
            }

            FindBar {
                id: findBar
                width: parent.width
                darkMode: win.dark
                totalLines: backend.activeTab ? backend.activeTab.lineCount : 1
                bgGutter: win.bgGutter
                bgEditor: win.bgEditor
                bgSurfaceElev: win.bgSurfaceElev
                borderSubtle: win.borderSubtle
                borderControl: win.borderControl
                borderFocus: win.borderFocus
                fgMain: win.fgMain
                fgMuted: win.fgMuted
                fgGutter: win.fgGutter
                selBg: win.selBg

                onQueryEdited: updateMatchStats()
                onFindNextRequested: function (backward) {
                    findNext(backward);
                }
                onReplaceOneRequested: function (rep) {
                    replaceOne(rep);
                }
                onReplaceAllRequested: function (rep) {
                    replaceAll(rep);
                }
                onGotoLineRequested: function (ln) {
                    if (activePane) {
                        activePane.jumpToLine(ln);
                    }
                }
                onClosed: closeFindBar()
            }

            Item {
                id: editorHost
                width: parent.width
                height: Math.max(80, parent.height - topbar.height - tabBar.height - findBar.height - statusBar.height)
                clip: true

                Repeater {
                    id: tabRepeater
                    model: backend.tabModel

                    delegate: EditorPane {
                        anchors.fill: parent
                        visible: index === backend.activeTabIndex
                        session: model.session
                        darkMode: win.dark
                        showLineNumbers: backend.showLineNumbers
                        zoomPercent: backend.zoomPercent
                        monoFontFamily: backend.monoFontFamily
                        bgEditor: win.bgEditor
                        bgGutter: win.bgGutter
                        bgSurfaceElev: win.bgSurfaceElev
                        bgHover: win.bgHover
                        bgLineHighlight: win.bgLineHighlight
                        borderSubtle: win.borderSubtle
                        borderControl: win.borderControl
                        borderFocus: win.borderFocus
                        fgMain: win.fgMain
                        fgMuted: win.fgMuted
                        fgGutter: win.fgGutter
                        selBg: win.selBg

                        onZoomInRequested: backend.zoomIn()
                        onZoomOutRequested: backend.zoomOut()
                        onOpenFindRequested: openFindBar(false, false)
                        onOpenReplaceRequested: openFindBar(true, false)
                        onOpenGotoRequested: openFindBar(false, true)
                        onNewTabRequested: backend.newTab()
                        onOpenFileRequested: backend.openFileDialog()
                        onSaveFileRequested: backend.saveFile()
                        onSaveAsRequested: backend.saveFileAsDialog()
                        onCloseTabRequested: requestCloseTab(index)
                        onQuitRequested: requestQuit()
                        onToggleSettingsRequested: settingsModal.open = !settingsModal.open
                    }
                }
            }

            StatusBar {
                id: statusBar
                width: parent.width
                darkMode: win.dark
                activeTab: backend.activeTab
                zoomPercent: backend.zoomPercent
                selectedChars: activePane ? activePane.selectedLength : 0
                bgShell: win.bgShell
                bgSurfaceElev: win.bgSurfaceElev
                borderSubtle: win.borderSubtle
                borderControl: win.borderControl
                fgMain: win.fgMain
                fgMuted: win.fgMuted
                fgGutter: win.fgGutter

                onGotoLineClicked: openFindBar(false, true)
                onLanguageClicked: languageModal.open = true
                onZoomInRequested: backend.zoomIn()
                onZoomOutRequested: backend.zoomOut()
                onZoomResetRequested: backend.zoomReset()
            }
        }

        Popup {
            id: menuPopup
            x: win.menuAnchorX > 0 ? Math.max(8, win.menuAnchorX - width) : (win.width - width - 10)
            y: topbar.height - 2
            width: 224
            padding: 6
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            Overlay.modal: Rectangle {
                color: "transparent"
            }

            enter: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: 85
                    easing.type: Easing.OutCubic
                }
            }
            exit: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 70
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 7
                color: win.bgSurfaceElev
                border.width: 1
                border.color: win.borderControl
                clip: true

                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 1
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    height: 1
                    color: win.dark ? "#12ffffff" : "#80ffffff"
                }
            }

            contentItem: Column {
                spacing: 2

                Repeater {
                    model: [
                        {
                            label: "New Tab",
                            shortcut: "Ctrl+T",
                            act: "new",
                            sep: false
                        },
                        {
                            label: "Open...",
                            shortcut: "Ctrl+O",
                            act: "open",
                            sep: false
                        },
                        {
                            label: "Save",
                            shortcut: "Ctrl+S",
                            act: "save",
                            sep: false
                        },
                        {
                            label: "Save As...",
                            shortcut: "Ctrl+Shift+S",
                            act: "saveas",
                            sep: false
                        },
                        {
                            label: "Close Tab",
                            shortcut: "Ctrl+W",
                            act: "closetab",
                            sep: true
                        },
                        {
                            label: "Find...",
                            shortcut: "Ctrl+F",
                            act: "find",
                            sep: false
                        },
                        {
                            label: "Replace...",
                            shortcut: "Ctrl+H",
                            act: "replace",
                            sep: false
                        },
                        {
                            label: "Go to Line...",
                            shortcut: "Ctrl+G",
                            act: "goto",
                            sep: true
                        },
                        {
                            label: "Zoom In",
                            shortcut: "Ctrl++",
                            act: "zoomin",
                            sep: false
                        },
                        {
                            label: "Zoom Out",
                            shortcut: "Ctrl+-",
                            act: "zoomout",
                            sep: false
                        },
                        {
                            label: "Reset Zoom",
                            shortcut: "Ctrl+0",
                            act: "zoomreset",
                            sep: true
                        },
                        {
                            label: "Settings",
                            shortcut: "Ctrl+,",
                            act: "settings",
                            sep: false
                        },
                        {
                            label: "Quit",
                            shortcut: "Ctrl+Q",
                            act: "quit",
                            sep: false
                        }
                    ]

                    delegate: Column {
                        width: 212
                        spacing: 2

                        Rectangle {
                            width: parent.width
                            height: 29
                            radius: 5
                            color: rowMa.pressed ? (win.dark ? "#282b34" : "#d8dce5") : (rowMa.containsMouse ? win.bgHover : "transparent")
                            border.width: 1
                            border.color: rowMa.containsMouse ? win.borderControl : "transparent"
                            scale: rowMa.pressed ? 0.985 : 1.0

                            Behavior on color {
                                ColorAnimation {
                                    duration: 100
                                }
                            }
                            Behavior on border.color {
                                ColorAnimation {
                                    duration: 100
                                }
                            }
                            Behavior on scale {
                                NumberAnimation {
                                    duration: 60
                                }
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: win.fgMain
                                font.pixelSize: 12
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
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
                                    menuPopup.close();
                                    var a = modelData.act;
                                    if (a === "new")
                                        backend.newTab();
                                    else if (a === "open")
                                        backend.openFileDialog();
                                    else if (a === "save")
                                        backend.saveFile();
                                    else if (a === "saveas")
                                        backend.saveFileAsDialog();
                                    else if (a === "closetab")
                                        requestCloseTab(backend.activeTabIndex);
                                    else if (a === "find")
                                        openFindBar(false, false);
                                    else if (a === "replace")
                                        openFindBar(true, false);
                                    else if (a === "goto")
                                        openFindBar(false, true);
                                    else if (a === "zoomin")
                                        backend.zoomIn();
                                    else if (a === "zoomout")
                                        backend.zoomOut();
                                    else if (a === "zoomreset")
                                        backend.zoomReset();
                                    else if (a === "settings")
                                        settingsModal.open = true;
                                    else if (a === "quit")
                                        requestQuit();
                                }
                            }
                        }

                        Rectangle {
                            visible: modelData.sep
                            width: parent.width - 12
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
                open = false;
                focusActiveEditor();
            }
        }

        LanguageModal {
            id: languageModal
            darkMode: win.dark
            backend: backend
            onClosed: {
                open = false;
                focusActiveEditor();
            }
        }

        ConfirmModal {
            id: confirmModal
            darkMode: win.dark
            onCancelled: {
                open = false;
                win.pendingAction = "";
                win.pendingTabIndex = -1;
                focusActiveEditor();
            }
            onDiscarded: {
                open = false;
                backend.discardTabChanges(win.pendingTabIndex);
                win.continuePendingAction();
            }
            onSaved: {
                open = false;
                if (backend.saveFile(win.pendingTabIndex)) {
                    win.continuePendingAction();
                } else {
                    win.pendingAction = "";
                    win.pendingTabIndex = -1;
                }
            }
        }

        DropArea {
            anchors.fill: parent
            onDropped: function (drop) {
                if (drop.hasUrls && drop.urls.length > 0) {
                    for (var i = 0; i < drop.urls.length; i++) {
                        backend.loadFile(drop.urls[i].toString(), true);
                    }
                }
            }
        }
    }

    MouseArea {
        visible: !shell.edgeToEdge
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.BottomEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.left: parent.left
        anchors.top: parent.top
        width: 10
        height: 10
        cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.right: parent.right
        anchors.top: parent.top
        width: 10
        height: 10
        cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 10
        height: 10
        cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }
    MouseArea {
        visible: !shell.edgeToEdge
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 10
        height: 10
        cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
}
