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
    title: ((editorBackend.activeTab && editorBackend.activeTab.modified) ? "* " : "") + ((editorBackend.activeTab && editorBackend.activeTab.filePath !== "") ? editorBackend.activeTab.fileName : (win.ui.untitled || "Untitled")) + " — Nulla"

    property bool forceClose: false
    property string pendingAction: ""
    property int pendingTabIndex: -1
    property real menuAnchorX: 0

    readonly property bool dark: editorBackend.effectiveDark
    readonly property var theme: editorBackend.theme
    readonly property var ui: editorBackend.ui
    readonly property bool expandedMode: editorBackend.layoutMode === 1
    readonly property color bgShell: theme.bgShell || (dark ? "#0d0e11" : "#ebedf2")
    readonly property color bgHeader: theme.bgHeader || (dark ? "#0d0e11" : "#ebedf2")
    readonly property color bgEditor: theme.bgEditor || (dark ? "#16181e" : "#ffffff")
    readonly property color bgGutter: theme.bgGutter || (dark ? "#121318" : "#f5f6f9")
    readonly property color bgDrawer: theme.bgDrawer || (dark ? "#101217" : "#f2f4f8")
    readonly property color bgSurfaceElev: theme.bgSurfaceElev || (dark ? "#1c1f27" : "#ffffff")
    readonly property color bgHover: theme.bgHover || (dark ? "#252934" : "#e2e5ed")
    readonly property color bgLineHighlight: theme.bgLineHighlight || (dark ? "#1f232c" : "#eff2f8")
    readonly property color borderSubtle: theme.borderSubtle || (dark ? "#222630" : "#dadde6")
    readonly property color borderControl: theme.borderControl || (dark ? "#313644" : "#c6ccd8")
    readonly property color borderHover: theme.borderHover || (dark ? "#52596d" : "#8b93a5")
    readonly property color borderFocus: theme.borderFocus || (dark ? "#6e768b" : "#687183")
    readonly property color fgMain: theme.fgMain || (dark ? "#f0f2f6" : "#14161a")
    readonly property color fgMuted: theme.fgMuted || (dark ? "#9ca3b4" : "#565e6d")
    readonly property color fgGutter: theme.fgGutter || (dark ? "#565d6e" : "#8c93a3")
    readonly property color selBg: theme.selBg || (dark ? "#2e3d59" : "#cde0ff")

    readonly property var activePane: mainEditorPane
    readonly property bool modalOpen: confirmModal.open || settingsModal.open || languageModal.open || menuPopup.opened || (activePane ? activePane.contextMenuOpen : false)

    EditorBackend {
        id: editorBackend
        onActiveTabChanged: {
            if (findBar.open && !findBar.gotoMode) {
                updateMatchStats();
            }
            if (!win.modalOpen && !findBar.open) {
                focusActiveEditor();
                Qt.callLater(function () {
                    if (!win.modalOpen && !findBar.open) {
                        focusActiveEditor();
                    }
                });
            }
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
            editorBackend.setInitialFiles(initialFilesArg);
        }
        focusActiveEditor();
    }

    function focusActiveEditor() {
        mainEditorPane.focusEditor();
    }

    function openFindBar(replaceMode, gotoMode) {
        findBar.gotoMode = !!gotoMode;
        findBar.replaceMode = !gotoMode && !!replaceMode;
        findBar.open = true;
        const sel = (!gotoMode && editorBackend.activeTab) ? editorBackend.activeTab.selectedText() : "";
        if (!gotoMode && sel !== "" && sel.indexOf("\n") === -1 && sel.indexOf("\u2029") === -1) {
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
        if (!editorBackend.activeTab || findBar.query === "") {
            findBar.matchCurrent = 0;
            findBar.matchTotal = 0;
            return;
        }
        const st = editorBackend.activeTab.matchStats(findBar.query);
        findBar.matchCurrent = st.current;
        findBar.matchTotal = st.total;
    }

    function findNext(backward) {
        if (!editorBackend.activeTab || findBar.query === "") {
            updateMatchStats();
            return;
        }
        if (editorBackend.activeTab.findNext(findBar.query, backward)) {
            mainEditorPane.ensureCursorVisible();
        }
        updateMatchStats();
    }

    function replaceOne(replacement) {
        if (!editorBackend.activeTab || findBar.query === "") {
            return;
        }
        if (editorBackend.activeTab.replaceMatch(findBar.query, replacement)) {
            mainEditorPane.ensureCursorVisible();
        }
        updateMatchStats();
    }

    function replaceAll(replacement) {
        if (!editorBackend.activeTab || findBar.query === "") {
            return;
        }
        editorBackend.activeTab.replaceAllMatches(findBar.query, replacement);
        updateMatchStats();
    }

    function localizedTabName(index) {
        const name = editorBackend.tabFileName(index);
        return name === "Untitled" ? (win.ui.untitled || "Untitled") : name;
    }

    function requestCloseTab(index) {
        if (editorBackend.tabCount === 1 && editorBackend.isActivePristineUntitled()) {
            win.close();
            return;
        }
        if (editorBackend.isTabModified(index)) {
            win.pendingAction = "closeTab";
            win.pendingTabIndex = index;
            confirmModal.errorOnly = false;
            confirmModal.title = win.ui.confirmUnsavedTitle || "Unsaved changes";
            confirmModal.message = (win.ui.confirmClosePrefix || 'Save changes to "') + localizedTabName(index) + (win.ui.confirmCloseSuffix || '" before closing?');
            confirmModal.open = true;
            return;
        }
        editorBackend.closeTabImmediate(index);
    }

    function requestQuit() {
        const dirtyIdx = editorBackend.firstModifiedTabIndex();
        if (dirtyIdx >= 0) {
            editorBackend.activateTab(dirtyIdx);
            win.pendingAction = "quitApp";
            win.pendingTabIndex = dirtyIdx;
            confirmModal.errorOnly = false;
            confirmModal.title = win.ui.confirmUnsavedTitle || "Unsaved changes";
            confirmModal.message = (win.ui.confirmClosePrefix || 'Save changes to "') + localizedTabName(dirtyIdx) + (win.ui.confirmExitSuffix || '" before exiting?');
            confirmModal.open = true;
            return;
        }
        win.forceClose = true;
        win.close();
    }

    function requestUpdateRestart() {
        const dirtyIdx = editorBackend.firstModifiedTabIndex();
        if (dirtyIdx >= 0) {
            editorBackend.activateTab(dirtyIdx);
            win.pendingAction = "applyUpdate";
            win.pendingTabIndex = dirtyIdx;
            confirmModal.errorOnly = false;
            confirmModal.title = win.ui.confirmUnsavedTitle || "Unsaved changes";
            confirmModal.message = (win.ui.confirmClosePrefix || 'Save changes to "') + localizedTabName(dirtyIdx) + (win.ui.confirmExitSuffix || '" before exiting?');
            confirmModal.open = true;
            return;
        }
        if (editorBackend.updater && editorBackend.updater.installAndRestart()) {
            win.forceClose = true;
            win.close();
            Qt.quit();
        }
    }

    function continuePendingAction() {
        const act = win.pendingAction;
        const idx = win.pendingTabIndex;
        win.pendingAction = "";
        win.pendingTabIndex = -1;
        if (act === "closeTab") {
            editorBackend.closeTabImmediate(idx);
            focusActiveEditor();
        } else if (act === "quitApp") {
            requestQuit();
        } else if (act === "applyUpdate") {
            requestUpdateRestart();
        }
    }

    onClosing: function (close) {
        if (!win.forceClose && editorBackend.firstModifiedTabIndex() >= 0) {
            close.accepted = false;
            requestQuit();
        }
    }

    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+T"
        onActivated: editorBackend.newTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+N"
        onActivated: editorBackend.newTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+O"
        onActivated: editorBackend.openFileDialog()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+O"
        onActivated: editorBackend.openFolderDialog()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+S"
        onActivated: editorBackend.saveFile()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+S"
        onActivated: editorBackend.saveFileAsDialog()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+W"
        onActivated: requestCloseTab(editorBackend.activeTabIndex)
    }
    Shortcut {
        enabled: !win.modalOpen && win.expandedMode
        sequence: "Ctrl+B"
        onActivated: editorBackend.sidebarOpen = !editorBackend.sidebarOpen
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Q"
        onActivated: requestQuit()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Tab"
        onActivated: editorBackend.nextTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+Tab"
        onActivated: editorBackend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Backtab"
        onActivated: editorBackend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+PgDown"
        onActivated: editorBackend.nextTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+PgUp"
        onActivated: editorBackend.prevTab()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+1"
        onActivated: editorBackend.activateTab(0)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+2"
        onActivated: editorBackend.activateTab(1)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+3"
        onActivated: editorBackend.activateTab(2)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+4"
        onActivated: editorBackend.activateTab(3)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+5"
        onActivated: editorBackend.activateTab(4)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+6"
        onActivated: editorBackend.activateTab(5)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+7"
        onActivated: editorBackend.activateTab(6)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+8"
        onActivated: editorBackend.activateTab(7)
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+9"
        onActivated: editorBackend.activateTab(8)
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
        onActivated: {
            if (confirmModal.open || languageModal.open) {
                return;
            }
            if (menuPopup.opened) {
                menuPopup.close();
            }
            if (activePane && activePane.contextMenuOpen) {
                activePane.closeContextMenu();
            }
            settingsModal.open = !settingsModal.open;
            if (!settingsModal.open) {
                focusActiveEditor();
            }
        }
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl++"
        onActivated: editorBackend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+="
        onActivated: editorBackend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+Shift+="
        onActivated: editorBackend.zoomIn()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+-"
        onActivated: editorBackend.zoomOut()
    }
    Shortcut {
        enabled: !win.modalOpen
        sequence: "Ctrl+0"
        onActivated: editorBackend.zoomReset()
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
            } else if (settingsModal.open && settingsModal.dropdownOpen) {
                settingsModal.closeDropdown();
                return;
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

        Item {
            anchors.fill: parent
            anchors.margins: shell.edgeToEdge ? 0 : 1
            clip: true

            Item {
                id: headerHost
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: win.expandedMode ? expandedHeader.implicitHeight : (topbar.implicitHeight + (tabBar.visible ? tabBar.implicitHeight : 0))
                clip: true

                TopBar {
                    id: topbar
                    visible: !win.expandedMode
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    darkMode: win.dark
                    theme: win.theme
                    activeTab: editorBackend.activeTab
                    showWindowControls: editorBackend.showWindowControls
                    isMaximized: win.visibility === Window.Maximized
                    menuOpened: menuPopup.opened
                    ui: win.ui
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
                    onNewTabRequested: editorBackend.newTab()
                    onOpenFileRequested: editorBackend.openFileDialog()
                    onSaveFileRequested: editorBackend.saveFile()
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
                    visible: !win.expandedMode && editorBackend.tabCount > 1
                    anchors.top: topbar.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    darkMode: win.dark
                    theme: win.theme
                    tabModel: editorBackend.tabModel
                    tabCount: editorBackend.tabCount
                    activeIndex: editorBackend.activeTabIndex
                    ui: win.ui
                    bgShell: win.bgShell
                    bgEditor: win.bgEditor
                    bgHover: win.bgHover
                    borderSubtle: win.borderSubtle
                    borderControl: win.borderControl
                    borderHover: win.borderHover
                    fgMain: win.fgMain
                    fgMuted: win.fgMuted

                    onTabActivated: function (index) {
                        editorBackend.activateTab(index);
                    }
                    onTabClosed: function (index) {
                        requestCloseTab(index);
                    }
                    onNewTabRequested: editorBackend.newTab()
                }

                ExpandedHeader {
                    id: expandedHeader
                    visible: win.expandedMode
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    darkMode: win.dark
                    theme: win.theme
                    tabModel: editorBackend.tabModel
                    tabCount: editorBackend.tabCount
                    activeIndex: editorBackend.activeTabIndex
                    showWindowControls: editorBackend.showWindowControls
                    isMaximized: win.visibility === Window.Maximized
                    menuOpened: menuPopup.opened
                    iconSize: editorBackend.expandedIconSize
                    railWidth: leftRail.width
                    ui: win.ui
                    bgHeader: win.bgHeader
                    bgEditor: win.bgEditor
                    bgSurfaceElev: win.bgSurfaceElev
                    bgHover: win.bgHover
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
                    onTabActivated: function (index) {
                        editorBackend.activateTab(index);
                    }
                    onTabClosed: function (index) {
                        requestCloseTab(index);
                    }
                    onNewTabRequested: editorBackend.newTab()
                    onFindRequested: openFindBar(false, false)
                    onMenuToggleRequested: function (anchorX) {
                        win.menuAnchorX = anchorX;
                        if (menuPopup.opened) {
                            menuPopup.close();
                        } else {
                            menuPopup.open();
                        }
                    }
                }
            }

            Item {
                id: workspaceHost
                x: 0
                y: headerHost.height
                width: parent.width
                height: Math.max(0, parent.height - headerHost.height - footerHost.height)
                clip: true

                LeftRail {
                    id: leftRail
                    visible: win.expandedMode
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    darkMode: win.dark
                    theme: win.theme
                    activeTab: editorBackend.activeTab
                    sidebarOpen: editorBackend.sidebarOpen
                    folderOpen: editorBackend.workspaceRoot !== ""
                    settingsOpen: settingsModal.open
                    iconSize: editorBackend.expandedIconSize
                    ui: win.ui
                    bgShell: win.bgShell
                    borderSubtle: win.borderSubtle
                    fgMain: win.fgMain

                    onToggleSidebarRequested: editorBackend.sidebarOpen = !editorBackend.sidebarOpen
                    onNewTabRequested: editorBackend.newTab()
                    onOpenFileRequested: editorBackend.openFileDialog()
                    onOpenFolderRequested: editorBackend.openFolderDialog()
                    onSaveFileRequested: editorBackend.saveFile()
                    onSettingsRequested: settingsModal.open = true
                }

                OpenFilesDrawer {
                    id: openFilesDrawer
                    open: win.expandedMode && editorBackend.sidebarOpen
                    preferredWidth: editorBackend.sidebarWidth
                    maxWidth: Math.max(0, workspaceHost.width - leftRail.width - 180)
                    scalePercent: editorBackend.sidebarScalePercent
                    visible: win.expandedMode && width > 1
                    anchors.left: leftRail.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    darkMode: win.dark
                    theme: win.theme
                    tabModel: editorBackend.tabModel
                    activeIndex: editorBackend.activeTabIndex
                    activeFilePath: editorBackend.activeTab ? editorBackend.activeTab.filePath : ""
                    workspaceRoot: editorBackend.workspaceRoot
                    workspaceName: editorBackend.workspaceName
                    folderEntries: editorBackend.folderEntries
                    ui: win.ui
                    bgDrawer: win.bgDrawer
                    bgHover: win.bgHover
                    bgSurfaceElev: win.bgSurfaceElev
                    borderSubtle: win.borderSubtle
                    borderControl: win.borderControl
                    fgMain: win.fgMain
                    fgMuted: win.fgMuted
                    fgSub: win.fgGutter

                    onTabActivated: function (index) {
                        editorBackend.activateTab(index);
                    }
                    onTabClosed: function (index) {
                        requestCloseTab(index);
                    }
                    onResizeRequested: function (newWidth) {
                        editorBackend.sidebarWidth = newWidth;
                    }
                    onOpenFolderRequested: editorBackend.openFolderDialog()
                    onCloseFolderRequested: editorBackend.closeFolder()
                    onRefreshFolderRequested: editorBackend.refreshFolder()
                    onFolderToggled: function (dirPath) {
                        editorBackend.toggleFolderExpanded(dirPath);
                    }
                    onFileOpenRequested: function (filePath) {
                        editorBackend.loadFile(filePath, false);
                    }
                }

                Item {
                    id: mainColumn
                    x: win.expandedMode ? (openFilesDrawer.x + openFilesDrawer.width) : 0
                    y: 0
                    width: Math.max(0, parent.width - x)
                    height: parent.height
                    clip: true

                    FindBar {
                        id: findBar
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        darkMode: win.dark
                        theme: win.theme
                        totalLines: editorBackend.activeTab ? editorBackend.activeTab.lineCount : 1
                        ui: win.ui
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

                    BreadcrumbBar {
                        id: breadcrumbBar
                        visible: win.expandedMode
                        anchors.top: findBar.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        activeTab: editorBackend.activeTab
                        ui: win.ui
                        bgEditor: win.bgEditor
                        borderSubtle: win.borderSubtle
                        fgMain: win.fgMain
                        fgMuted: win.fgMuted
                        fgGutter: win.fgGutter
                    }

                    Item {
                        id: editorHost
                        readonly property real topY: win.expandedMode ? (breadcrumbBar.y + breadcrumbBar.height) : (findBar.y + findBar.height)
                        x: 0
                        y: topY
                        width: parent.width
                        height: Math.max(0, parent.height - topY)
                        clip: true

                        EditorPane {
                            id: mainEditorPane
                            anchors.fill: parent
                            session: editorBackend.activeTab
                            darkMode: win.dark
                            theme: win.theme
                            showLineNumbers: editorBackend.showLineNumbers
                            showIndentGuides: editorBackend.showIndentGuides
                            zoomPercent: editorBackend.zoomPercent
                            monoFontFamily: editorBackend.monoFontFamily
                            ui: win.ui
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

                            onZoomInRequested: editorBackend.zoomIn()
                            onZoomOutRequested: editorBackend.zoomOut()
                            onOpenFindRequested: openFindBar(false, false)
                            onOpenReplaceRequested: openFindBar(true, false)
                            onOpenGotoRequested: openFindBar(false, true)
                            onNewTabRequested: editorBackend.newTab()
                            onOpenFileRequested: editorBackend.openFileDialog()
                            onSaveFileRequested: editorBackend.saveFile()
                            onSaveAsRequested: editorBackend.saveFileAsDialog()
                            onCloseTabRequested: requestCloseTab(editorBackend.activeTabIndex)
                            onQuitRequested: requestQuit()
                            onToggleSettingsRequested: settingsModal.open = !settingsModal.open
                        }
                    }
                }
            }

            Item {
                id: footerHost
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: win.expandedMode ? expandedStatusBar.implicitHeight : statusBar.implicitHeight
                clip: true

                StatusBar {
                    id: statusBar
                    visible: !win.expandedMode
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    darkMode: win.dark
                    theme: win.theme
                    activeTab: editorBackend.activeTab
                    zoomPercent: editorBackend.zoomPercent
                    selectedChars: editorBackend.activeTab ? editorBackend.activeTab.selectedChars : 0
                    ui: win.ui
                    bgShell: win.bgShell
                    bgSurfaceElev: win.bgSurfaceElev
                    borderSubtle: win.borderSubtle
                    borderControl: win.borderControl
                    fgMain: win.fgMain
                    fgMuted: win.fgMuted
                    fgGutter: win.fgGutter

                    onGotoLineClicked: openFindBar(false, true)
                    onLanguageClicked: languageModal.open = true
                    onZoomInRequested: editorBackend.zoomIn()
                    onZoomOutRequested: editorBackend.zoomOut()
                    onZoomResetRequested: editorBackend.zoomReset()
                }

                ExpandedStatusBar {
                    id: expandedStatusBar
                    visible: win.expandedMode
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    darkMode: win.dark
                    theme: win.theme
                    activeTab: editorBackend.activeTab
                    zoomPercent: editorBackend.zoomPercent
                    selectedChars: editorBackend.activeTab ? editorBackend.activeTab.selectedChars : 0
                    ui: win.ui
                    bgShell: win.bgShell
                    borderSubtle: win.borderSubtle
                    fgMain: win.fgMain
                    fgMuted: win.fgMuted
                    fgGutter: win.fgGutter

                    onGotoLineClicked: openFindBar(false, true)
                    onLanguageClicked: languageModal.open = true
                    onZoomInRequested: editorBackend.zoomIn()
                    onZoomOutRequested: editorBackend.zoomOut()
                    onZoomResetRequested: editorBackend.zoomReset()
                }
            }

            UpdateToast {
                id: updateToast
                anchors.right: parent.right
                anchors.bottom: footerHost.top
                anchors.rightMargin: 14
                anchors.bottomMargin: 14
                z: 80
                darkMode: win.dark
                theme: win.theme
                ui: win.ui
                updater: editorBackend.updater
                suppressed: settingsModal.open || confirmModal.open || languageModal.open
                onRestartRequested: win.requestUpdateRestart()
            }
        }

        Popup {
            id: menuPopup
            x: win.menuAnchorX > 0 ? Math.max(8, Math.min(win.width - width - 8, win.menuAnchorX - width)) : Math.max(8, win.width - width - 10)
            y: Math.max(8, Math.min(win.height - height - 8, (win.expandedMode ? expandedHeader.implicitHeight : topbar.implicitHeight) - 2))
            width: Math.min(248, Math.max(120, win.width - 16))
            height: Math.min(menuCol.implicitHeight + padding * 2, Math.max(80, win.height - 16))
            padding: 6
            modal: true
            focus: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            Overlay.modal: Rectangle {
                color: "transparent"
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
                    color: win.theme.sheen || (win.dark ? "#12ffffff" : "#80ffffff")
                }
            }

            contentItem: Flickable {
                id: menuFlick
                clip: true
                contentWidth: width
                contentHeight: menuCol.implicitHeight
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 4
                    contentItem: Rectangle {
                        implicitWidth: 3
                        radius: 2
                        color: win.theme.scrollIdle || (win.dark ? "#3a3a42" : "#c1c6d0")
                    }
                }

                Column {
                    id: menuCol
                    width: menuFlick.width
                    spacing: 2

                    Repeater {
                        model: [
                            {
                                label: win.ui.menuNewTab || "New Tab",
                                shortcut: "Ctrl+T",
                                act: "new",
                                sep: false
                            },
                            {
                                label: win.ui.menuOpen || "Open...",
                                shortcut: "Ctrl+O",
                                act: "open",
                                sep: false
                            },
                            {
                                label: win.ui.menuOpenFolder || "Open Folder...",
                                shortcut: "Ctrl+Shift+O",
                                act: "openfolder",
                                sep: false
                            },
                            {
                                label: win.ui.menuSave || "Save",
                                shortcut: "Ctrl+S",
                                act: "save",
                                sep: false
                            },
                            {
                                label: win.ui.menuSaveAs || "Save As...",
                                shortcut: "Ctrl+Shift+S",
                                act: "saveas",
                                sep: false
                            },
                            {
                                label: win.ui.menuCloseTab || "Close Tab",
                                shortcut: "Ctrl+W",
                                act: "closetab",
                                sep: true
                            },
                            {
                                label: win.ui.menuFind || "Find...",
                                shortcut: "Ctrl+F",
                                act: "find",
                                sep: false
                            },
                            {
                                label: win.ui.menuReplace || "Replace...",
                                shortcut: "Ctrl+H",
                                act: "replace",
                                sep: false
                            },
                            {
                                label: win.ui.menuGoto || "Go to Line...",
                                shortcut: "Ctrl+G",
                                act: "goto",
                                sep: true
                            },
                            {
                                label: win.ui.menuZoomIn || "Zoom In",
                                shortcut: "Ctrl++",
                                act: "zoomin",
                                sep: false
                            },
                            {
                                label: win.ui.menuZoomOut || "Zoom Out",
                                shortcut: "Ctrl+-",
                                act: "zoomout",
                                sep: false
                            },
                            {
                                label: win.ui.menuZoomReset || "Reset Zoom",
                                shortcut: "Ctrl+0",
                                act: "zoomreset",
                                sep: true
                            },
                            {
                                label: win.ui.menuSettings || "Settings",
                                shortcut: "Ctrl+,",
                                act: "settings",
                                sep: false
                            },
                            {
                                label: win.ui.menuQuit || "Quit",
                                shortcut: "Ctrl+Q",
                                act: "quit",
                                sep: false
                            }
                        ]

                        delegate: Column {
                            width: menuCol.width
                            spacing: 2

                            Rectangle {
                                width: parent.width
                                height: 29
                                radius: 5
                                color: rowMa.pressed ? (win.theme.bgPressed || (win.dark ? "#282b34" : "#d8dce5")) : (rowMa.containsMouse ? win.bgHover : "transparent")
                                border.width: 1
                                border.color: rowMa.containsMouse ? win.borderControl : "transparent"

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
                                        const a = modelData.act;
                                        if (a === "new")
                                            editorBackend.newTab();
                                        else if (a === "open")
                                            editorBackend.openFileDialog();
                                        else if (a === "openfolder")
                                            editorBackend.openFolderDialog();
                                        else if (a === "save")
                                            editorBackend.saveFile();
                                        else if (a === "saveas")
                                            editorBackend.saveFileAsDialog();
                                        else if (a === "closetab")
                                            requestCloseTab(editorBackend.activeTabIndex);
                                        else if (a === "find")
                                            openFindBar(false, false);
                                        else if (a === "replace")
                                            openFindBar(true, false);
                                        else if (a === "goto")
                                            openFindBar(false, true);
                                        else if (a === "zoomin")
                                            editorBackend.zoomIn();
                                        else if (a === "zoomout")
                                            editorBackend.zoomOut();
                                        else if (a === "zoomreset")
                                            editorBackend.zoomReset();
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
        }

        SettingsModal {
            id: settingsModal
            darkMode: win.dark
            theme: win.theme
            backend: editorBackend
            ui: win.ui
            onClosed: {
                open = false;
                focusActiveEditor();
            }
            onUpdateRestartRequested: win.requestUpdateRestart()
        }

        LanguageModal {
            id: languageModal
            darkMode: win.dark
            theme: win.theme
            backend: editorBackend
            ui: win.ui
            onClosed: {
                open = false;
                focusActiveEditor();
            }
        }

        ConfirmModal {
            id: confirmModal
            darkMode: win.dark
            theme: win.theme
            ui: win.ui
            onCancelled: {
                open = false;
                win.pendingAction = "";
                win.pendingTabIndex = -1;
                focusActiveEditor();
            }
            onDiscarded: {
                open = false;
                editorBackend.discardTabChanges(win.pendingTabIndex);
                win.continuePendingAction();
            }
            onSaved: {
                open = false;
                if (editorBackend.saveFile(win.pendingTabIndex)) {
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
                    for (let i = 0; i < drop.urls.length; i++) {
                        const u = drop.urls[i].toString();
                        if (u.indexOf("file:") === 0 || u.indexOf("/") === 0) {
                            editorBackend.loadFile(u, true);
                        }
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
