import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var theme: ({
    })
    property var backend: null
    property var ui: ({
    })
    property int activeSettingsTab: 0
    readonly property var updater: backend ? backend.updater : null
    readonly property int updStatus: updater ? updater.status : 0
    readonly property bool dropdownOpen: langPopup.opened
    readonly property int fontDelta: card.width >= 760 ? 2 : (card.width >= 650 ? 1 : 0)
    readonly property color bgCard: theme.bgCard || (darkMode ? "#17191e" : "#ffffff")
    readonly property color bgGroup: theme.bgGroup || (darkMode ? "#131418" : "#f7f8fa")
    readonly property color borderGroup: theme.borderGroup || (darkMode ? "#262831" : "#dfe2ea")
    readonly property color borderDivider: theme.borderDivider || (darkMode ? "#20222a" : "#e7e9f0")
    readonly property color fgTitle: theme.fgTitle || (darkMode ? "#eceef2" : "#14161a")
    readonly property color fgLabel: theme.fgLabel || (darkMode ? "#d8dce4" : "#1f2228")
    readonly property color fgSub: theme.fgSub || (darkMode ? "#787f8e" : "#6a7180")

    signal closed()
    signal updateRestartRequested()

    function checkIconUri(col) {
        const hex = col.toString();
        const svg = "<svg xmlns=\"http:\x2f\x2fwww.w3.org\x2f2000\x2fsvg\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"" + hex + "\" stroke-width=\"2.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M5 12.5 9.5 17 19 7.5\"/></svg>";
        return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
    }

    function closeDropdown() {
        langPopup.close();
    }

    function formattedLastCheck() {
        if (!updater || updater.lastCheckEpoch <= 0)
            return "";

        return Qt.formatDateTime(new Date(updater.lastCheckEpoch * 1000), Qt.DefaultLocaleShortDate);
    }

    function currentUiLanguageName() {
        if (!backend || !backend.uiLanguagesModel)
            return "English";

        const m = backend.uiLanguagesModel;
        for (let i = 0; i < m.length; i++) {
            if (m[i].id === backend.uiLanguage)
                return m[i].name;

        }
        return backend.uiLanguage || "English";
    }

    onOpenChanged: {
        if (open) {
            cardFlick.contentY = 0;
            forceActiveFocus();
        } else if (langPopup.opened) {
            langPopup.close();
        }
    }
    onActiveSettingsTabChanged: {
        cardFlick.contentY = 0;
        if (activeSettingsTab !== 0 && langPopup.opened)
            langPopup.close();

    }
    visible: open
    focus: open
    anchors.fill: parent
    z: 150
    Keys.onPressed: function(event) {
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        color: theme.scrim || (darkMode ? "#9608090c" : "#66101216")
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onClicked: root.closed()
        onWheel: function(wheel) {
            wheel.accepted = true;
        }
    }

    Rectangle {
        id: card

        width: Math.max(0, Math.min(parent.width - 40, Math.min(860, Math.max(560, Math.round(parent.width * 0.4)))))
        height: Math.max(0, Math.min(parent.height - 40, Math.min(780, Math.max(520, Math.round(parent.height * 0.68)))))
        anchors.centerIn: parent
        radius: 12
        color: root.bgCard
        border.width: 1
        border.color: theme.borderControl || (darkMode ? "#2c303a" : "#cfd4df")
        clip: true

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onWheel: function(wheel) {
                wheel.accepted = true;
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: 1
            color: theme.sheenBright || (darkMode ? "#14ffffff" : "#90ffffff")
        }

        Item {
            id: headerBar

            anchors.top: parent.top
            anchors.topMargin: 14
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            height: 36

            Text {
                anchors.left: parent.left
                anchors.right: settingsTabsBox.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: root.ui.settingsTitle || "Settings"
                color: root.fgTitle
                font.pixelSize: 15 + root.fontDelta
                font.weight: Font.DemiBold
            }

            Rectangle {
                id: settingsTabsBox

                anchors.right: closeSettingsBtn.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                height: 36
                width: settingsTabsRow.implicitWidth + 6
                radius: 10
                color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                border.width: 1
                border.color: root.borderGroup

                TextMetrics {
                    id: tabTm0

                    font.pixelSize: 13 + root.fontDelta
                    font.weight: Font.DemiBold
                    text: root.ui.tabGeneral || "General"
                }

                TextMetrics {
                    id: tabTm1

                    font.pixelSize: 13 + root.fontDelta
                    font.weight: Font.DemiBold
                    text: root.ui.tabTheme || "Theme"
                }

                TextMetrics {
                    id: tabTm2

                    font.pixelSize: 13 + root.fontDelta
                    font.weight: Font.DemiBold
                    text: root.ui.tabUpdates || "Updates"
                }

                Row {
                    id: settingsTabsRow

                    anchors.centerIn: parent
                    spacing: 3

                    M3Button {
                        text: tabTm0.text
                        ghost: root.activeSettingsTab !== 0
                        activeChip: root.activeSettingsTab === 0
                        fontPixelSize: 13 + root.fontDelta
                        darkMode: root.darkMode
                        theme: root.theme
                        buttonHeight: 30
                        buttonWidth: Math.ceil(Math.max(tabTm0.advanceWidth, tabTm0.width)) + 30
                        horizontalPadding: 14
                        cornerRadius: 8
                        onClicked: root.activeSettingsTab = 0
                    }

                    M3Button {
                        text: tabTm1.text
                        ghost: root.activeSettingsTab !== 1
                        activeChip: root.activeSettingsTab === 1
                        fontPixelSize: 13 + root.fontDelta
                        darkMode: root.darkMode
                        theme: root.theme
                        buttonHeight: 30
                        buttonWidth: Math.ceil(Math.max(tabTm1.advanceWidth, tabTm1.width)) + 30
                        horizontalPadding: 14
                        cornerRadius: 8
                        onClicked: root.activeSettingsTab = 1
                    }

                    M3Button {
                        text: tabTm2.text
                        ghost: root.activeSettingsTab !== 2
                        activeChip: root.activeSettingsTab === 2
                        showDot: root.updStatus === 3 || root.updStatus === 5
                        fontPixelSize: 13 + root.fontDelta
                        darkMode: root.darkMode
                        theme: root.theme
                        buttonHeight: 30
                        buttonWidth: Math.ceil(Math.max(tabTm2.advanceWidth, tabTm2.width)) + 30 + (showDot ? 12 : 0)
                        horizontalPadding: 14
                        cornerRadius: 8
                        onClicked: root.activeSettingsTab = 2
                    }

                }

            }

            M3Button {
                id: closeSettingsBtn

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconKind: "close"
                iconOnly: true
                ghost: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: 32
                cornerRadius: 9
                onClicked: root.closed()
            }

        }

        Rectangle {
            id: topDivider

            anchors.top: headerBar.bottom
            anchors.topMargin: 12
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.right: parent.right
            anchors.rightMargin: 18
            height: 1
            color: root.borderDivider
        }

        Flickable {
            id: cardFlick

            anchors.top: topDivider.bottom
            anchors.topMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            anchors.left: parent.left
            anchors.right: parent.right
            contentWidth: width
            contentHeight: mainCol.implicitHeight + 6
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Column {
                id: mainCol

                x: 18
                y: 2
                width: cardFlick.width - 36
                spacing: 14

                Column {
                    visible: root.activeSettingsTab === 0
                    width: parent.width
                    spacing: 12

                    Text {
                        text: root.ui.secAppearance || "APPEARANCE"
                        color: root.fgSub
                        font.pixelSize: 10 + root.fontDelta
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.8
                    }

                    Rectangle {
                        width: parent.width
                        height: appearanceCol.implicitHeight
                        radius: 10
                        color: root.bgGroup
                        border.width: 1
                        border.color: root.borderGroup

                        Column {
                            id: appearanceCol

                            width: parent.width
                            spacing: 0

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblViewMode || "View mode"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 30
                                    width: layoutRow.implicitWidth + 4
                                    radius: 10
                                    color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                                    border.width: 1
                                    border.color: root.borderGroup

                                    Row {
                                        id: layoutRow

                                        anchors.centerIn: parent
                                        spacing: 2

                                        M3Button {
                                            text: root.ui.layoutMinimal || "Minimal"
                                            ghost: !(root.backend && root.backend.layoutMode === 0)
                                            activeChip: !!(root.backend && root.backend.layoutMode === 0)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 11
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.layoutMode = 0;

                                            }
                                        }

                                        M3Button {
                                            text: root.ui.layoutExpanded || "Expanded"
                                            ghost: !(root.backend && root.backend.layoutMode === 1)
                                            activeChip: !!(root.backend && root.backend.layoutMode === 1)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 11
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.layoutMode = 1;

                                            }
                                        }

                                    }

                                }

                            }

                            Rectangle {
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                visible: !!(root.backend && root.backend.layoutMode === 1)
                                width: parent.width
                                height: visible ? 48 : 0
                                clip: true

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblIconSize || "Icon size"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 30
                                    width: iconSizeRow.implicitWidth + 4
                                    radius: 10
                                    color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                                    border.width: 1
                                    border.color: root.borderGroup

                                    Row {
                                        id: iconSizeRow

                                        anchors.centerIn: parent
                                        spacing: 2

                                        M3Button {
                                            text: root.ui.iconSmall || "S"
                                            ghost: !(root.backend && root.backend.expandedIconSize <= 16)
                                            activeChip: !!(root.backend && root.backend.expandedIconSize <= 16)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 10
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.expandedIconSize = 15;

                                            }
                                        }

                                        M3Button {
                                            text: root.ui.iconMedium || "M"
                                            ghost: !(root.backend && root.backend.expandedIconSize > 16 && root.backend.expandedIconSize <= 19)
                                            activeChip: !!(root.backend && root.backend.expandedIconSize > 16 && root.backend.expandedIconSize <= 19)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 10
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.expandedIconSize = 18;

                                            }
                                        }

                                        M3Button {
                                            text: root.ui.iconLarge || "L"
                                            ghost: !(root.backend && root.backend.expandedIconSize > 19 && root.backend.expandedIconSize <= 22)
                                            activeChip: !!(root.backend && root.backend.expandedIconSize > 19 && root.backend.expandedIconSize <= 22)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 10
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.expandedIconSize = 21;

                                            }
                                        }

                                        M3Button {
                                            text: root.ui.iconXLarge || "XL"
                                            ghost: !(root.backend && root.backend.expandedIconSize > 22)
                                            activeChip: !!(root.backend && root.backend.expandedIconSize > 22)
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 10
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.expandedIconSize = 25;

                                            }
                                        }

                                    }

                                }

                            }

                            Rectangle {
                                visible: !!(root.backend && root.backend.layoutMode === 1)
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                visible: !!(root.backend && root.backend.layoutMode === 1)
                                width: parent.width
                                height: visible ? 48 : 0
                                clip: true

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblSidebarScale || "Sidebar scale"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 30
                                    width: sidebarScaleRow.implicitWidth + 4
                                    radius: 10
                                    color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                                    border.width: 1
                                    border.color: root.borderGroup

                                    Row {
                                        id: sidebarScaleRow

                                        anchors.centerIn: parent
                                        spacing: 0

                                        M3Button {
                                            iconKind: "minus"
                                            iconOnly: true
                                            ghost: true
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            buttonWidth: 28
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.sidebarScaleOut();

                                            }
                                        }

                                        Rectangle {
                                            width: 1
                                            height: 14
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: root.borderDivider
                                        }

                                        M3Button {
                                            text: (root.backend ? root.backend.sidebarScalePercent : 100) + "%"
                                            ghost: true
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 12
                                            cornerRadius: 9
                                            tooltipText: root.ui.resetSidebarScaleTip || "Reset sidebar scale"
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.sidebarScaleReset();

                                            }
                                        }

                                        Rectangle {
                                            width: 1
                                            height: 14
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: root.borderDivider
                                        }

                                        M3Button {
                                            iconKind: "plus"
                                            iconOnly: true
                                            ghost: true
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            buttonWidth: 28
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.sidebarScaleIn();

                                            }
                                        }

                                    }

                                }

                            }

                            Rectangle {
                                visible: !!(root.backend && root.backend.layoutMode === 1)
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblUiLanguage || "Language"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                Rectangle {
                                    id: langDropdownBtn

                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 176 + root.fontDelta * 12
                                    height: 30
                                    radius: 10
                                    color: langPopup.opened ? (theme.bgPressed || (darkMode ? "#262a35" : "#e2e6ef")) : (langBtnMa.pressed ? (theme.bgPressed || (darkMode ? "#242832" : "#dde1eb")) : (langBtnMa.containsMouse ? (theme.bgHover || (darkMode ? "#21252f" : "#e8ebf2")) : (theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3"))))
                                    border.width: 1
                                    border.color: (langPopup.opened || langBtnMa.containsMouse) ? (theme.borderHover || (darkMode ? "#4d5466" : "#9aa2b4")) : root.borderGroup

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 11
                                        anchors.right: langChevron.left
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.currentUiLanguageName()
                                        color: root.fgTitle
                                        font.pixelSize: 12 + root.fontDelta
                                        font.weight: Font.Medium
                                        elide: Text.ElideRight
                                    }

                                    Canvas {
                                        id: langChevron

                                        property color strokeColor: root.fgSub
                                        property bool up: langPopup.opened

                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 10
                                        height: 6
                                        onStrokeColorChanged: requestPaint()
                                        onUpChanged: requestPaint()
                                        onPaint: {
                                            const ctx = getContext("2d");
                                            ctx.reset();
                                            ctx.strokeStyle = strokeColor;
                                            ctx.lineWidth = 1.5;
                                            ctx.lineCap = "round";
                                            ctx.lineJoin = "round";
                                            ctx.beginPath();
                                            if (up) {
                                                ctx.moveTo(1.5, 4.5);
                                                ctx.lineTo(5, 1.5);
                                                ctx.lineTo(8.5, 4.5);
                                            } else {
                                                ctx.moveTo(1.5, 1.5);
                                                ctx.lineTo(5, 4.5);
                                                ctx.lineTo(8.5, 1.5);
                                            }
                                            ctx.stroke();
                                        }
                                    }

                                    MouseArea {
                                        id: langBtnMa

                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (langPopup.opened)
                                                langPopup.close();
                                            else
                                                langPopup.open();
                                        }
                                    }

                                    Popup {
                                        id: langPopup

                                        x: parent.width - width
                                        y: parent.height + 5
                                        width: 196 + root.fontDelta * 12
                                        padding: 5
                                        margins: 8
                                        modal: true
                                        focus: true
                                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                                        onOpened: {
                                            if (root.backend && root.backend.uiLanguagesModel) {
                                                const m = root.backend.uiLanguagesModel;
                                                for (let i = 0; i < m.length; i++) {
                                                    if (m[i].id === root.backend.uiLanguage) {
                                                        langDropList.positionViewAtIndex(i, ListView.Contain);
                                                        break;
                                                    }
                                                }
                                            }
                                        }

                                        Overlay.modal: Rectangle {
                                            color: "transparent"
                                        }

                                        background: Rectangle {
                                            radius: 10
                                            color: theme.bgSurfaceElev || (darkMode ? "#1c1f27" : "#ffffff")
                                            border.width: 1
                                            border.color: theme.borderControl || (darkMode ? "#313644" : "#c6ccd8")
                                            clip: true

                                            Rectangle {
                                                anchors.top: parent.top
                                                anchors.topMargin: 1
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.leftMargin: 10
                                                anchors.rightMargin: 10
                                                height: 1
                                                color: theme.sheenBright || (darkMode ? "#14ffffff" : "#80ffffff")
                                            }

                                        }

                                        contentItem: ListView {
                                            id: langDropList

                                            implicitWidth: langPopup.width - langPopup.padding * 2
                                            implicitHeight: Math.min(248, contentHeight, Math.max(88, root.height - 32))
                                            clip: true
                                            spacing: 2
                                            boundsBehavior: Flickable.StopAtBounds
                                            model: root.backend ? root.backend.uiLanguagesModel : []

                                            ScrollBar.vertical: ScrollBar {
                                                policy: ScrollBar.AsNeeded
                                                width: 5

                                                contentItem: Rectangle {
                                                    implicitWidth: 3
                                                    radius: 2
                                                    color: theme.scrollHover || (darkMode ? "#454b5a" : "#b8beca")
                                                }

                                            }

                                            delegate: Rectangle {
                                                readonly property bool isCurrent: root.backend && root.backend.uiLanguage === modelData.id

                                                width: langDropList.width - 4
                                                height: 30
                                                radius: 7
                                                color: isCurrent ? (theme.chipBg || (darkMode ? "#293245" : "#e3ebfa")) : (dropItemMa.pressed ? (theme.bgPressed || (darkMode ? "#282b34" : "#d8dce5")) : (dropItemMa.containsMouse ? (theme.bgHover || (darkMode ? "#252934" : "#eceff5")) : "transparent"))
                                                border.width: 1
                                                border.color: isCurrent ? (theme.chipBorder || (darkMode ? "#44577a" : "#b7c9ec")) : (dropItemMa.containsMouse ? (theme.borderControl || (darkMode ? "#343947" : "#cfd5e1")) : "transparent")

                                                Text {
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 10
                                                    anchors.right: activeDot.left
                                                    anchors.rightMargin: 8
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: modelData.name
                                                    color: parent.isCurrent ? (theme.chipFg || (darkMode ? "#f3f6fc" : "#153672")) : root.fgTitle
                                                    font.pixelSize: 12 + root.fontDelta
                                                    font.weight: parent.isCurrent ? Font.DemiBold : Font.Normal
                                                    elide: Text.ElideRight
                                                }

                                                Rectangle {
                                                    id: activeDot

                                                    visible: parent.isCurrent
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 10
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    width: 6
                                                    height: 6
                                                    radius: 3
                                                    color: theme.accent || (darkMode ? "#7aa2f7" : "#2563eb")
                                                }

                                                MouseArea {
                                                    id: dropItemMa

                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        if (root.backend)
                                                            root.backend.uiLanguage = modelData.id;

                                                        langPopup.close();
                                                    }
                                                }

                                            }

                                        }

                                    }

                                }

                            }

                            Rectangle {
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblZoom || "Zoom"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 30
                                    width: zoomRow.implicitWidth + 4
                                    radius: 10
                                    color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                                    border.width: 1
                                    border.color: root.borderGroup

                                    Row {
                                        id: zoomRow

                                        anchors.centerIn: parent
                                        spacing: 0

                                        M3Button {
                                            iconKind: "minus"
                                            iconOnly: true
                                            ghost: true
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            buttonWidth: 28
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.zoomOut();

                                            }
                                        }

                                        Rectangle {
                                            width: 1
                                            height: 14
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: root.borderDivider
                                        }

                                        M3Button {
                                            text: (root.backend ? root.backend.zoomPercent : 100) + "%"
                                            ghost: true
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            horizontalPadding: 12
                                            cornerRadius: 9
                                            tooltipText: root.ui.resetZoomTip || "Reset (Ctrl+0)"
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.zoomReset();

                                            }
                                        }

                                        Rectangle {
                                            width: 1
                                            height: 14
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: root.borderDivider
                                        }

                                        M3Button {
                                            iconKind: "plus"
                                            iconOnly: true
                                            ghost: true
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 26
                                            buttonWidth: 28
                                            cornerRadius: 9
                                            onClicked: {
                                                if (root.backend)
                                                    root.backend.zoomIn();

                                            }
                                        }

                                    }

                                }

                            }

                        }

                    }

                    Text {
                        text: root.ui.secEditor || "EDITOR"
                        color: root.fgSub
                        font.pixelSize: 10 + root.fontDelta
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.8
                    }

                    Rectangle {
                        width: parent.width
                        height: editorCol.implicitHeight
                        radius: 10
                        color: root.bgGroup
                        border.width: 1
                        border.color: root.borderGroup

                        Column {
                            id: editorCol

                            width: parent.width
                            spacing: 0

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblLineNumbers || "Line numbers"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                M3Switch {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    darkMode: root.darkMode
                                    theme: root.theme
                                    checked: root.backend ? root.backend.showLineNumbers : true
                                    onToggled: function(v) {
                                        if (root.backend)
                                            root.backend.showLineNumbers = v;

                                    }
                                }

                            }

                            Rectangle {
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblIndentGuides || "Indent guides"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                M3Switch {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    darkMode: root.darkMode
                                    theme: root.theme
                                    checked: root.backend ? root.backend.showIndentGuides : true
                                    onToggled: function(v) {
                                        if (root.backend)
                                            root.backend.showIndentGuides = v;

                                    }
                                }

                            }

                            Rectangle {
                                width: parent.width - 24
                                height: 1
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: root.borderDivider
                            }

                            Item {
                                width: parent.width
                                height: 48

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.ui.lblSyntax || "Syntax highlighting"
                                    color: root.fgLabel
                                    font.pixelSize: 13 + root.fontDelta
                                }

                                M3Switch {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    darkMode: root.darkMode
                                    theme: root.theme
                                    checked: root.backend ? root.backend.syntaxEnabled : true
                                    onToggled: function(v) {
                                        if (root.backend)
                                            root.backend.syntaxEnabled = v;

                                    }
                                }

                            }

                        }

                    }

                }

                Column {
                    visible: root.activeSettingsTab === 1
                    width: parent.width
                    spacing: 12

                    Text {
                        text: (root.ui.lblTheme || "THEME").toUpperCase()
                        color: root.fgSub
                        font.pixelSize: 10 + root.fontDelta
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.8
                    }

                    Grid {
                        id: themeGrid

                        width: parent.width
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 10

                        Repeater {
                            model: [{
                                "mode": 2,
                                "title": root.ui.themeDark || "Dark",
                                "desc": root.ui.themeDarkDesc || "Deep slate monochrome",
                                "badge": "",
                                "cShell": "#0d0e11",
                                "cGutter": "#121318",
                                "cEditor": "#16181e",
                                "cLine": "#1f232c",
                                "cFg": "#f0f2f6",
                                "cMuted": "#697082",
                                "cAccent": "#7aa2f7",
                                "cBorder": "#2a2f3c",
                                "cBadgeBg": "transparent",
                                "cBadgeBorder": "transparent"
                            }, {
                                "mode": 1,
                                "title": root.ui.themeLight || "Light",
                                "desc": root.ui.themeLightDesc || "Clean bright surface",
                                "badge": "",
                                "cShell": "#ebedf2",
                                "cGutter": "#f5f6f9",
                                "cEditor": "#ffffff",
                                "cLine": "#eff2f8",
                                "cFg": "#14161a",
                                "cMuted": "#9aa2b2",
                                "cAccent": "#2563eb",
                                "cBorder": "#cfd5e2",
                                "cBadgeBg": "transparent",
                                "cBadgeBorder": "transparent"
                            }, {
                                "mode": 3,
                                "title": root.ui.themeGraphite || "Graphite",
                                "desc": root.ui.themeGraphiteDesc || "Warm neutral zinc-graphite",
                                "badge": "",
                                "cShell": "#101012",
                                "cGutter": "#151518",
                                "cEditor": "#18181b",
                                "cLine": "#212126",
                                "cFg": "#f2f2f5",
                                "cMuted": "#6b6b76",
                                "cAccent": "#d4d4dc",
                                "cBorder": "#2f2f37",
                                "cBadgeBg": "transparent",
                                "cBadgeBorder": "transparent"
                            }, {
                                "mode": 4,
                                "title": root.ui.themeCarbonNo8 || "Carbon №8",
                                "desc": root.ui.themeCarbonNo8Desc || "Cool charcoal & electric cyan-teal",
                                "badge": "№8",
                                "cShell": "#0d1012",
                                "cGutter": "#111518",
                                "cEditor": "#14191c",
                                "cLine": "#162d28",
                                "cFg": "#edf4f2",
                                "cMuted": "#5a6b73",
                                "cAccent": "#00e5a3",
                                "cBorder": "#243538",
                                "cBadgeBg": "#0f2b26",
                                "cBadgeBorder": "#00b88a"
                            }]

                            delegate: Rectangle {
                                readonly property bool isCurrent: root.backend && root.backend.themeMode === modelData.mode

                                width: Math.floor((themeGrid.width - themeGrid.columnSpacing) / 2)
                                height: 128 + root.fontDelta * 6
                                radius: 12
                                color: isCurrent ? (theme.chipBg || (darkMode ? "#1e2430" : "#eef2fb")) : (themeCardMa.containsMouse ? (theme.bgHover || (darkMode ? "#1c1e24" : "#eceff5")) : root.bgGroup)
                                border.width: isCurrent ? 1.5 : 1
                                border.color: isCurrent ? (theme.accent || root.fgTitle) : (themeCardMa.containsMouse ? (theme.borderHover || root.borderGroup) : root.borderGroup)
                                clip: true

                                Rectangle {
                                    id: miniPreview

                                    anchors.top: parent.top
                                    anchors.topMargin: 9
                                    anchors.left: parent.left
                                    anchors.leftMargin: 9
                                    anchors.right: parent.right
                                    anchors.rightMargin: 9
                                    height: 70 + root.fontDelta * 3
                                    radius: 8
                                    color: modelData.cShell
                                    border.width: 1
                                    border.color: modelData.cBorder
                                    clip: true

                                    Item {
                                        id: miniTop

                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 14

                                        Row {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 7
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 4

                                            Rectangle {
                                                width: 4
                                                height: 4
                                                radius: 2
                                                color: modelData.cAccent
                                            }

                                            Rectangle {
                                                width: 4
                                                height: 4
                                                radius: 2
                                                color: modelData.cMuted
                                                opacity: 0.65
                                            }

                                            Rectangle {
                                                width: 4
                                                height: 4
                                                radius: 2
                                                color: modelData.cMuted
                                                opacity: 0.45
                                            }

                                        }

                                    }

                                    Rectangle {
                                        id: miniTopDiv

                                        anchors.top: miniTop.bottom
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 1
                                        color: modelData.cBorder
                                    }

                                    Row {
                                        anchors.top: miniTopDiv.bottom
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        spacing: 0

                                        Rectangle {
                                            width: 18
                                            height: parent.height
                                            color: modelData.cGutter

                                            Column {
                                                anchors.centerIn: parent
                                                spacing: 5

                                                Rectangle {
                                                    width: 7
                                                    height: 2
                                                    radius: 1
                                                    color: modelData.cMuted
                                                    opacity: 0.55
                                                }

                                                Rectangle {
                                                    width: 7
                                                    height: 2
                                                    radius: 1
                                                    color: modelData.cAccent
                                                }

                                                Rectangle {
                                                    width: 7
                                                    height: 2
                                                    radius: 1
                                                    color: modelData.cMuted
                                                    opacity: 0.55
                                                }

                                                Rectangle {
                                                    width: 7
                                                    height: 2
                                                    radius: 1
                                                    color: modelData.cMuted
                                                    opacity: 0.4
                                                }

                                            }

                                        }

                                        Rectangle {
                                            width: 1
                                            height: parent.height
                                            color: modelData.cBorder
                                        }

                                        Rectangle {
                                            width: parent.width - 19
                                            height: parent.height
                                            color: modelData.cEditor

                                            Column {
                                                anchors.fill: parent
                                                anchors.margins: 7
                                                spacing: 4

                                                Row {
                                                    width: parent.width
                                                    height: 6
                                                    spacing: 4

                                                    Rectangle {
                                                        width: parent.width * 0.28
                                                        height: 3
                                                        radius: 1.5
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: modelData.cAccent
                                                    }

                                                    Rectangle {
                                                        width: parent.width * 0.36
                                                        height: 3
                                                        radius: 1.5
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: modelData.cMuted
                                                        opacity: 0.75
                                                    }

                                                }

                                                Rectangle {
                                                    width: parent.width
                                                    height: 9
                                                    radius: 2
                                                    color: modelData.cLine

                                                    Row {
                                                        anchors.left: parent.left
                                                        anchors.leftMargin: 3
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        spacing: 4

                                                        Rectangle {
                                                            width: miniPreview.width * 0.34
                                                            height: 3
                                                            radius: 1.5
                                                            color: modelData.cFg
                                                        }

                                                        Rectangle {
                                                            width: miniPreview.width * 0.2
                                                            height: 3
                                                            radius: 1.5
                                                            color: modelData.cAccent
                                                        }

                                                    }

                                                }

                                                Row {
                                                    width: parent.width
                                                    height: 6
                                                    spacing: 4

                                                    Rectangle {
                                                        width: parent.width * 0.18
                                                        height: 3
                                                        radius: 1.5
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: modelData.cMuted
                                                        opacity: 0.65
                                                    }

                                                    Rectangle {
                                                        width: parent.width * 0.46
                                                        height: 3
                                                        radius: 1.5
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: modelData.cFg
                                                        opacity: 0.55
                                                    }

                                                }

                                                Row {
                                                    width: parent.width
                                                    height: 6
                                                    spacing: 4

                                                    Rectangle {
                                                        width: parent.width * 0.32
                                                        height: 3
                                                        radius: 1.5
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        color: modelData.cAccent
                                                        opacity: 0.8
                                                    }

                                                }

                                            }

                                        }

                                    }

                                }

                                Item {
                                    anchors.top: miniPreview.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.leftMargin: 11
                                    anchors.rightMargin: 10

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: cardCheck.left
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 2

                                        Row {
                                            width: parent.width
                                            spacing: 6

                                            Text {
                                                text: modelData.title
                                                color: isCurrent ? root.fgTitle : root.fgLabel
                                                font.pixelSize: 13 + root.fontDelta
                                                font.weight: isCurrent ? Font.DemiBold : Font.Medium
                                                anchors.verticalCenter: parent.verticalCenter
                                            }

                                            Rectangle {
                                                visible: modelData.badge !== ""
                                                width: badgeTxt.implicitWidth + 10
                                                height: badgeTxt.implicitHeight + 2
                                                radius: 4
                                                color: modelData.cBadgeBg
                                                border.width: 1
                                                border.color: modelData.cBadgeBorder
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    id: badgeTxt

                                                    anchors.centerIn: parent
                                                    text: modelData.badge
                                                    color: modelData.cAccent
                                                    font.pixelSize: 9 + root.fontDelta
                                                    font.weight: Font.Bold
                                                }

                                            }

                                        }

                                        Text {
                                            width: parent.width
                                            text: modelData.desc
                                            color: root.fgSub
                                            font.pixelSize: 11 + root.fontDelta
                                            elide: Text.ElideRight
                                        }

                                    }

                                    Rectangle {
                                        id: cardCheck

                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 18
                                        height: 18
                                        radius: 9
                                        color: isCurrent ? (theme.accent || root.fgTitle) : "transparent"
                                        border.width: isCurrent ? 0 : 1.5
                                        border.color: root.borderGroup

                                        Image {
                                            visible: isCurrent
                                            anchors.centerIn: parent
                                            width: 11
                                            height: 11
                                            sourceSize: Qt.size(22, 22)
                                            smooth: true
                                            source: visible ? root.checkIconUri(theme.fgOnPrimary || (darkMode ? "#111215" : "#ffffff")) : ""
                                        }

                                    }

                                }

                                MouseArea {
                                    id: themeCardMa

                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.backend)
                                            root.backend.themeMode = modelData.mode;

                                    }
                                }

                            }

                        }

                    }

                    Rectangle {
                        readonly property bool isCurrent: root.backend && root.backend.themeMode === 0

                        width: parent.width
                        height: 76 + root.fontDelta * 4
                        radius: 12
                        color: isCurrent ? (theme.chipBg || (darkMode ? "#1e2430" : "#eef2fb")) : (sysThemeMa.containsMouse ? (theme.bgHover || (darkMode ? "#1c1e24" : "#eceff5")) : root.bgGroup)
                        border.width: isCurrent ? 1.5 : 1
                        border.color: isCurrent ? (theme.accent || root.fgTitle) : (sysThemeMa.containsMouse ? (theme.borderHover || root.borderGroup) : root.borderGroup)
                        clip: true

                        Rectangle {
                            id: sysPreview

                            anchors.left: parent.left
                            anchors.leftMargin: 9
                            anchors.verticalCenter: parent.verticalCenter
                            width: 152 + root.fontDelta * 8
                            height: parent.height - 18
                            radius: 8
                            border.width: 1
                            border.color: root.borderGroup
                            clip: true

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    width: Math.floor(parent.width / 2)
                                    height: parent.height
                                    color: "#16181e"

                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 12
                                        color: "#0d0e11"
                                    }

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.topMargin: 18
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 6
                                        spacing: 5

                                        Rectangle {
                                            width: parent.width * 0.72
                                            height: 3
                                            radius: 1.5
                                            color: "#7aa2f7"
                                        }

                                        Rectangle {
                                            width: parent.width * 0.9
                                            height: 3
                                            radius: 1.5
                                            color: "#f0f2f6"
                                            opacity: 0.75
                                        }

                                        Rectangle {
                                            width: parent.width * 0.55
                                            height: 3
                                            radius: 1.5
                                            color: "#697082"
                                        }

                                    }

                                }

                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: root.borderGroup
                                }

                                Rectangle {
                                    width: parent.width - Math.floor(parent.width / 2) - 1
                                    height: parent.height
                                    color: "#ffffff"

                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: 12
                                        color: "#ebedf2"
                                    }

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.topMargin: 18
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 6
                                        spacing: 5

                                        Rectangle {
                                            width: parent.width * 0.72
                                            height: 3
                                            radius: 1.5
                                            color: "#2563eb"
                                        }

                                        Rectangle {
                                            width: parent.width * 0.9
                                            height: 3
                                            radius: 1.5
                                            color: "#14161a"
                                            opacity: 0.75
                                        }

                                        Rectangle {
                                            width: parent.width * 0.55
                                            height: 3
                                            radius: 1.5
                                            color: "#9aa2b2"
                                        }

                                    }

                                }

                            }

                        }

                        Column {
                            anchors.left: sysPreview.right
                            anchors.leftMargin: 14
                            anchors.right: sysCheck.left
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3

                            Text {
                                text: root.ui.themeSystem || "System"
                                color: parent.parent.isCurrent ? root.fgTitle : root.fgLabel
                                font.pixelSize: 13 + root.fontDelta
                                font.weight: parent.parent.isCurrent ? Font.DemiBold : Font.Medium
                            }

                            Text {
                                width: parent.width
                                text: root.ui.themeSystemDesc || "Follows OS & desktop palette"
                                color: root.fgSub
                                font.pixelSize: 11 + root.fontDelta
                                elide: Text.ElideRight
                            }

                        }

                        Rectangle {
                            id: sysCheck

                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18
                            radius: 9
                            color: parent.isCurrent ? (theme.accent || root.fgTitle) : "transparent"
                            border.width: parent.isCurrent ? 0 : 1.5
                            border.color: root.borderGroup

                            Image {
                                visible: parent.parent.isCurrent
                                anchors.centerIn: parent
                                width: 11
                                height: 11
                                sourceSize: Qt.size(22, 22)
                                smooth: true
                                source: visible ? root.checkIconUri(theme.fgOnPrimary || (darkMode ? "#111215" : "#ffffff")) : ""
                            }

                        }

                        MouseArea {
                            id: sysThemeMa

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.backend)
                                    root.backend.themeMode = 0;

                            }
                        }

                    }

                }

                Column {
                    id: updatesTabCol

                    visible: root.activeSettingsTab === 2
                    width: parent.width
                    spacing: 0

                    Column {
                        id: updatesTopCol

                        width: parent.width
                        spacing: 12

                        Text {
                            text: root.ui.secUpdates || "UPDATES"
                            color: root.fgSub
                            font.pixelSize: 10 + root.fontDelta
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.8
                        }

                        Rectangle {
                            width: parent.width
                            height: updatesCol.implicitHeight
                            radius: 10
                            color: root.bgGroup
                            border.width: 1
                            border.color: root.borderGroup
                            clip: true

                            Column {
                                id: updatesCol

                                width: parent.width
                                spacing: 0

                                Item {
                                    width: parent.width
                                    height: Math.max(58, updStatusCol.implicitHeight + 22)

                                    Column {
                                        id: updStatusCol

                                        anchors.left: parent.left
                                        anchors.leftMargin: 14
                                        anchors.right: updBtnRow.left
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 2

                                        Text {
                                            width: parent.width
                                            elide: Text.ElideRight
                                            color: root.fgLabel
                                            font.pixelSize: 13 + root.fontDelta
                                            font.weight: Font.Medium
                                            text: {
                                                if (!root.updater)
                                                    return "";

                                                if (root.updStatus === 1)
                                                    return root.ui.updStatusChecking || "Checking for updates...";

                                                if (root.updStatus === 2)
                                                    return root.ui.updStatusUpToDate || "You're up to date";

                                                if (root.updStatus === 3)
                                                    return (root.ui.updStatusAvailable || "Update available") + " · v" + root.updater.availableVersion;

                                                if (root.updStatus === 4)
                                                    return (root.ui.updStatusDownloading || "Downloading update...") + " " + root.updater.downloadProgress + "%";

                                                if (root.updStatus === 5)
                                                    return (root.ui.updStatusReady || "Ready to install") + " · v" + root.updater.availableVersion;

                                                if (root.updStatus === 6)
                                                    return root.ui.updStatusInstalling || "Installing update...";

                                                if (root.updStatus === 7)
                                                    return root.ui.updStatusError || "Update check failed";

                                                return (root.ui.updStatusIdle || "Current version") + " v" + root.updater.currentVersion;
                                            }
                                        }

                                        Text {
                                            width: parent.width
                                            elide: Text.ElideRight
                                            color: root.fgSub
                                            font.pixelSize: 11 + root.fontDelta
                                            text: {
                                                if (!root.updater)
                                                    return "";

                                                if (root.updStatus === 7 && root.updater.errorMessage !== "")
                                                    return root.updater.errorMessage;

                                                if ((root.updStatus === 3 || root.updStatus === 5) && (root.updater.systemInstalled || !root.updater.canSelfUpdate))
                                                    return root.ui.updSystemHint || "System package detected — update via package manager or GitHub release.";

                                                const checked = root.formattedLastCheck();
                                                if (checked !== "")
                                                    return (root.ui.updLastChecked || "Last checked:") + " " + checked;

                                                return (root.ui.updCurrentPrefix || "Installed:") + " v" + root.updater.currentVersion;
                                            }
                                        }

                                    }

                                    Row {
                                        id: updBtnRow

                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 6

                                        M3Button {
                                            visible: root.updStatus === 3 || root.updStatus === 5
                                            iconKind: "refresh"
                                            iconOnly: true
                                            ghost: true
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 28
                                            cornerRadius: 8
                                            tooltipText: root.ui.updBtnCheck || "Check for updates"
                                            onClicked: {
                                                if (root.updater)
                                                    root.updater.checkForUpdates(true);

                                            }
                                        }

                                        M3Button {
                                            visible: root.updStatus !== 3 && root.updStatus !== 4 && root.updStatus !== 5 && root.updStatus !== 6
                                            iconKind: "refresh"
                                            text: root.updStatus === 1 ? (root.ui.updStatusChecking || "Checking...") : (root.ui.updBtnCheck || "Check for updates")
                                            elevated: root.updStatus !== 1
                                            ghost: root.updStatus === 1
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 28
                                            horizontalPadding: 11
                                            cornerRadius: 8
                                            onClicked: {
                                                if (root.updater && root.updStatus !== 1)
                                                    root.updater.checkForUpdates(true);

                                            }
                                        }

                                        M3Button {
                                            visible: root.updStatus === 3
                                            text: (root.updater && root.updater.canSelfUpdate) ? (root.ui.updBtnUpdate || "Update") : (root.ui.updBtnRelease || "Release")
                                            primaryFill: true
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 28
                                            horizontalPadding: 12
                                            cornerRadius: 8
                                            onClicked: {
                                                if (root.updater)
                                                    root.updater.startDownload();

                                            }
                                        }

                                        M3Button {
                                            visible: root.updStatus === 5
                                            text: root.ui.updBtnRestart || "Restart & Update"
                                            primaryFill: true
                                            fontPixelSize: 12 + root.fontDelta
                                            darkMode: root.darkMode
                                            theme: root.theme
                                            buttonHeight: 28
                                            horizontalPadding: 12
                                            cornerRadius: 8
                                            onClicked: root.updateRestartRequested()
                                        }

                                    }

                                }

                                Item {
                                    visible: root.updStatus === 4
                                    width: parent.width
                                    height: visible ? 24 : 0
                                    clip: true

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.leftMargin: 14
                                        anchors.rightMargin: 14
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 5
                                        radius: 2.5
                                        color: root.borderDivider
                                        clip: true

                                        Rectangle {
                                            height: parent.height
                                            radius: 2.5
                                            color: root.fgTitle
                                            width: Math.max(6, parent.width * (root.updater ? Math.max(0, Math.min(100, root.updater.downloadProgress)) / 100 : 0))
                                        }

                                    }

                                }

                                Rectangle {
                                    width: parent.width - 24
                                    height: 1
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: root.borderDivider
                                }

                                Item {
                                    width: parent.width
                                    height: 48

                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 14
                                        anchors.right: autoUpdSwitch.left
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                        text: root.ui.lblAutoUpdate || "Check automatically"
                                        color: root.fgLabel
                                        font.pixelSize: 13 + root.fontDelta
                                    }

                                    M3Switch {
                                        id: autoUpdSwitch

                                        anchors.right: parent.right
                                        anchors.rightMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        darkMode: root.darkMode
                                        theme: root.theme
                                        checked: root.updater ? root.updater.autoCheck : true
                                        onToggled: function(v) {
                                            if (root.updater)
                                                root.updater.autoCheck = v;

                                        }
                                    }

                                }

                            }

                        }

                    }

                    Item {
                        width: parent.width
                        height: Math.max(18, cardFlick.height - updatesTopCol.implicitHeight - updatesAboutCol.implicitHeight - 10)
                    }

                    Column {
                        id: updatesAboutCol

                        width: parent.width
                        spacing: 12

                        Text {
                            text: root.ui.secAbout || "ABOUT"
                            color: root.fgSub
                            font.pixelSize: 10 + root.fontDelta
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.8
                        }

                        Rectangle {
                            width: parent.width
                            height: 60
                            radius: 10
                            color: root.bgGroup
                            border.width: 1
                            border.color: root.borderGroup
                            clip: true

                            NullaMark {
                                id: aboutMark

                                size: 22
                                color: root.fgTitle
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Column {
                                anchors.left: aboutMark.right
                                anchors.leftMargin: 12
                                anchors.right: repoBtn.left
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2

                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: "Nulla" + (root.updater ? (" v" + root.updater.currentVersion) : "")
                                    color: root.fgTitle
                                    font.pixelSize: 13 + root.fontDelta
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: (root.ui.updCreatedBy || "Created by") + " " + (root.updater ? root.updater.authorName : "pzwt4cc")
                                    color: root.fgSub
                                    font.pixelSize: 12 + root.fontDelta
                                }

                            }

                            M3Button {
                                id: repoBtn

                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "GitHub"
                                elevated: true
                                fontPixelSize: 12 + root.fontDelta
                                darkMode: root.darkMode
                                theme: root.theme
                                buttonHeight: 28
                                horizontalPadding: 12
                                cornerRadius: 8
                                tooltipText: root.updater ? root.updater.repoUrl : "https://github.com/pzwt4cc/nulla"
                                onClicked: {
                                    if (root.updater)
                                        root.updater.openRepoPage();

                                }
                            }

                        }

                    }

                }

            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 6
                rightPadding: 2

                contentItem: Rectangle {
                    implicitWidth: 3
                    radius: 2
                    color: theme.scrollIdle || (darkMode ? "#3a3f4c" : "#c1c6d0")
                }

            }

        }

    }

}
