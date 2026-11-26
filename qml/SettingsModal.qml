import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property bool darkMode: true
    property var theme: ({})
    property var backend: null
    property var ui: ({})

    signal closed

    readonly property bool dropdownOpen: langPopup.opened

    function closeDropdown() {
        langPopup.close();
    }

    readonly property color bgCard: theme.bgCard || (darkMode ? "#17191e" : "#ffffff")
    readonly property color bgGroup: theme.bgGroup || (darkMode ? "#131418" : "#f7f8fa")
    readonly property color borderGroup: theme.borderGroup || (darkMode ? "#262831" : "#dfe2ea")
    readonly property color borderDivider: theme.borderDivider || (darkMode ? "#20222a" : "#e7e9f0")
    readonly property color fgTitle: theme.fgTitle || (darkMode ? "#eceef2" : "#14161a")
    readonly property color fgLabel: theme.fgLabel || (darkMode ? "#d8dce4" : "#1f2228")
    readonly property color fgSub: theme.fgSub || (darkMode ? "#787f8e" : "#6a7180")

    function currentUiLanguageName() {
        if (!backend || !backend.uiLanguagesModel) {
            return "English";
        }
        var m = backend.uiLanguagesModel;
        for (var i = 0; i < m.length; i++) {
            if (m[i].id === backend.uiLanguage) {
                return m[i].name;
            }
        }
        return backend.uiLanguage || "English";
    }

    onOpenChanged: {
        if (!open && langPopup.opened) {
            langPopup.close();
        }
    }

    visible: opacity > 0.01
    opacity: open ? 1.0 : 0.0
    anchors.fill: parent
    z: 150

    Behavior on opacity {
        NumberAnimation {
            duration: 95
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        color: theme.scrim || (darkMode ? "#9608090c" : "#66101216")
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.closed()
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 40, 444)
        height: Math.min(parent.height - 40, mainCol.implicitHeight + 36)
        anchors.centerIn: parent
        radius: 12
        color: root.bgCard
        border.width: 1
        border.color: theme.borderControl || (darkMode ? "#2c303a" : "#cfd4df")
        scale: root.open ? 1.0 : 0.985
        clip: true

        Behavior on scale {
            NumberAnimation {
                duration: 95
                easing.type: Easing.OutCubic
            }
        }

        MouseArea {
            anchors.fill: parent
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

        Column {
            id: mainCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 18
            spacing: 14

            Item {
                width: parent.width
                height: 30

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.settingsTitle || "Settings"
                    color: root.fgTitle
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                M3Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconKind: "close"
                    iconOnly: true
                    ghost: true
                    darkMode: root.darkMode
                    theme: root.theme
                    buttonHeight: 28
                    cornerRadius: 9
                    onClicked: root.closed()
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.borderDivider
            }

            Column {
                width: parent.width
                spacing: 12

                Text {
                    text: root.ui.secAppearance || "APPEARANCE"
                    color: root.fgSub
                    font.pixelSize: 10
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
                                text: root.ui.lblTheme || "Theme"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                height: 30
                                width: themeRow.implicitWidth + 4
                                radius: 10
                                color: theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3")
                                border.width: 1
                                border.color: root.borderGroup

                                Row {
                                    id: themeRow
                                    anchors.centerIn: parent
                                    spacing: 2

                                    M3Button {
                                        text: root.ui.themeSystem || "System"
                                        ghost: !(root.backend && root.backend.themeMode === 0)
                                        activeChip: root.backend && root.backend.themeMode === 0
                                        darkMode: root.darkMode
                    theme: root.theme
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 0
                                    }
                                    M3Button {
                                        text: root.ui.themeDark || "Dark"
                                        ghost: !(root.backend && root.backend.themeMode === 2)
                                        activeChip: root.backend && root.backend.themeMode === 2
                                        darkMode: root.darkMode
                    theme: root.theme
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 2
                                    }
                                    M3Button {
                                        text: root.ui.themeLight || "Light"
                                        ghost: !(root.backend && root.backend.themeMode === 1)
                                        activeChip: root.backend && root.backend.themeMode === 1
                                        darkMode: root.darkMode
                    theme: root.theme
                                        buttonHeight: 26
                                        horizontalPadding: 11
                                        cornerRadius: 9
                                        onClicked: if (root.backend)
                                            root.backend.themeMode = 1
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
                                text: root.ui.lblUiLanguage || "Language"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            Rectangle {
                                id: langDropdownBtn
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                width: 176
                                height: 30
                                radius: 10
                                color: langPopup.opened ? (theme.bgPressed || (darkMode ? "#262a35" : "#e2e6ef")) : (langBtnMa.pressed ? (theme.bgPressed || (darkMode ? "#242832" : "#dde1eb")) : (langBtnMa.containsMouse ? (theme.bgHover || (darkMode ? "#21252f" : "#e8ebf2")) : (theme.bgInput || (darkMode ? "#1a1c22" : "#eceef3"))))
                                border.width: 1
                                border.color: (langPopup.opened || langBtnMa.containsMouse) ? (theme.borderHover || (darkMode ? "#4d5466" : "#9aa2b4")) : root.borderGroup
                                scale: langBtnMa.pressed ? 0.985 : 1.0

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 90
                                    }
                                }
                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: 90
                                    }
                                }
                                Behavior on scale {
                                    NumberAnimation {
                                        duration: 60
                                    }
                                }

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 11
                                    anchors.right: langChevron.left
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.currentUiLanguageName()
                                    color: root.fgTitle
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                }

                                Canvas {
                                    id: langChevron
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 10
                                    height: 6
                                    property color strokeColor: root.fgSub
                                    property bool up: langPopup.opened

                                    onStrokeColorChanged: requestPaint()
                                    onUpChanged: requestPaint()
                                    onPaint: {
                                        var ctx = getContext("2d");
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
                                        if (langPopup.opened) {
                                            langPopup.close();
                                        } else {
                                            langPopup.open();
                                        }
                                    }
                                }

                                Popup {
                                    id: langPopup
                                    x: parent.width - width
                                    y: parent.height + 5
                                    width: 196
                                    padding: 5
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
                                        NumberAnimation {
                                            property: "scale"
                                            from: 0.97
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
                                            duration: 65
                                            easing.type: Easing.OutCubic
                                        }
                                    }

                                    onOpened: {
                                        if (root.backend && root.backend.uiLanguagesModel) {
                                            var m = root.backend.uiLanguagesModel;
                                            for (var i = 0; i < m.length; i++) {
                                                if (m[i].id === root.backend.uiLanguage) {
                                                    langDropList.positionViewAtIndex(i, ListView.Contain);
                                                    break;
                                                }
                                            }
                                        }
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
                                        implicitHeight: Math.min(248, contentHeight)
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
                                            width: langDropList.width - 4
                                            height: 30
                                            radius: 7
                                            readonly property bool isCurrent: root.backend && root.backend.uiLanguage === modelData.id
                                            color: isCurrent ? (theme.chipBg || (darkMode ? "#293245" : "#e3ebfa")) : (dropItemMa.pressed ? (theme.bgPressed || (darkMode ? "#282b34" : "#d8dce5")) : (dropItemMa.containsMouse ? (theme.bgHover || (darkMode ? "#252934" : "#eceff5")) : "transparent"))
                                            border.width: 1
                                            border.color: isCurrent ? (theme.chipBorder || (darkMode ? "#44577a" : "#b7c9ec")) : (dropItemMa.containsMouse ? (theme.borderControl || (darkMode ? "#343947" : "#cfd5e1")) : "transparent")
                                            scale: dropItemMa.pressed ? 0.985 : 1.0

                                            Behavior on color {
                                                ColorAnimation {
                                                    duration: 85
                                                }
                                            }
                                            Behavior on border.color {
                                                ColorAnimation {
                                                    duration: 85
                                                }
                                            }
                                            Behavior on scale {
                                                NumberAnimation {
                                                    duration: 55
                                                }
                                            }

                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 10
                                                anchors.right: activeDot.left
                                                anchors.rightMargin: 8
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: modelData.name
                                                color: parent.isCurrent ? (theme.chipFg || (darkMode ? "#f3f6fc" : "#153672")) : root.fgTitle
                                                font.pixelSize: 12
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
                                                    if (root.backend) {
                                                        root.backend.uiLanguage = modelData.id;
                                                    }
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
                                font.pixelSize: 13
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
                                        onClicked: if (root.backend)
                                            root.backend.zoomOut()
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
                                        darkMode: root.darkMode
                    theme: root.theme
                                        buttonHeight: 26
                                        horizontalPadding: 12
                                        cornerRadius: 9
                                        tooltipText: root.ui.resetZoomTip || "Reset (Ctrl+0)"
                                        onClicked: if (root.backend)
                                            root.backend.zoomReset()
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
                                        onClicked: if (root.backend)
                                            root.backend.zoomIn()
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    text: root.ui.secEditor || "EDITOR"
                    color: root.fgSub
                    font.pixelSize: 10
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
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                    theme: root.theme
                                checked: root.backend ? root.backend.showLineNumbers : true
                                onToggled: function (v) {
                                    if (root.backend) {
                                        root.backend.showLineNumbers = v;
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
                                text: root.ui.lblSyntax || "Syntax highlighting"
                                color: root.fgLabel
                                font.pixelSize: 13
                            }

                            M3Switch {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                darkMode: root.darkMode
                    theme: root.theme
                                checked: root.backend ? root.backend.syntaxEnabled : true
                                onToggled: function (v) {
                                    if (root.backend) {
                                        root.backend.syntaxEnabled = v;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
