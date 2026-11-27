import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string text: ""
    property string iconKind: ""
    property bool iconOnRight: false
    property bool darkMode: true
    property var theme: ({})
    property bool accent: false
    property bool activeChip: false
    property bool primaryFill: false
    property bool elevated: false
    property bool ghost: false
    property bool iconOnly: false
    property bool dangerHover: false
    property bool showDot: false
    property real buttonHeight: 28
    property real buttonWidth: 0
    property real horizontalPadding: 10
    property string tooltipText: ""
    property int fontPixelSize: 12
    property int cornerRadius: 5
    property int iconSize: 14

    signal clicked

    readonly property bool hovered: ma.containsMouse
    readonly property bool pressed: ma.pressed

    readonly property color fgNormal: theme.fgMain || (darkMode ? "#f0f2f6" : "#14161a")
    readonly property color fgMuted: theme.fgMuted || (darkMode ? "#9ca3b2" : "#585e6c")
    readonly property color fgOnPrimary: theme.fgOnPrimary || (darkMode ? "#111215" : "#ffffff")
    readonly property color borderNormal: theme.btnBorderNormal || (darkMode ? "#2f333d" : "#cdd2dc")
    readonly property color borderHover: theme.btnBorderHover || (darkMode ? "#61697c" : "#70788a")
    readonly property color borderAccent: theme.btnBorderAccent || (darkMode ? "#d2d7e2" : "#252932")
    readonly property color fillHover: theme.btnFillHover || (darkMode ? "#1cffffff" : "#10000000")
    readonly property color fillPress: theme.btnFillPress || (darkMode ? "#2cffffff" : "#1b000000")
    readonly property color fillAccent: theme.btnFillAccent || (darkMode ? "#14ffffff" : "#0b000000")
    readonly property color fillChip: theme.btnFillChip || (darkMode ? "#262a34" : "#e3e6ee")
    readonly property color fillDanger: theme.btnFillDanger || (darkMode ? "#34e5534b" : "#22e04840")
    readonly property color borderDanger: theme.danger || "#e0554d"
    readonly property color primaryBg: theme.primaryBg || fgNormal
    readonly property color primaryHover: theme.primaryHover || (darkMode ? "#dfe2e8" : "#22252d")
    readonly property color primaryPress: theme.primaryPress || (darkMode ? "#c4c8d1" : "#2e323c")

    readonly property color iconColor: {
        if (primaryFill) {
            return fgOnPrimary;
        }
        if (dangerHover && (hovered || pressed)) {
            return borderDanger;
        }
        if (accent || activeChip || hovered || pressed) {
            return fgNormal;
        }
        return fgMuted;
    }

    function svgBody(kind) {
        if (kind === "plus") {
            return '<path d="M12 5v14M5 12h14"/>';
        }
        if (kind === "minus") {
            return '<path d="M5 12h14"/>';
        }
        if (kind === "close") {
            return '<path d="M17 7L7 17M7 7l10 10"/>';
        }
        if (kind === "max") {
            return '<rect x="5.5" y="5.5" width="13" height="13" rx="2"/>';
        }
        if (kind === "restore") {
            return '<rect x="5" y="8" width="11" height="11" rx="1.8"/><path d="M8 8V6.8A1.8 1.8 0 0 1 9.8 5h7.4A1.8 1.8 0 0 1 19 6.8v7.4a1.8 1.8 0 0 1-1.8 1.8H16"/>';
        }
        if (kind === "dots") {
            return '<circle cx="5.5" cy="12" r="1.4" fill="currentColor" stroke="none"/><circle cx="12" cy="12" r="1.4" fill="currentColor" stroke="none"/><circle cx="18.5" cy="12" r="1.4" fill="currentColor" stroke="none"/>';
        }
        if (kind === "gear") {
            return '<circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9c.26.6.85 1 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/>';
        }
        if (kind === "chevron") {
            return '<path d="m6 9 6 6 6-6"/>';
        }
        if (kind === "up") {
            return '<path d="m18 15-6-6-6 6"/>';
        }
        if (kind === "back") {
            return '<path d="M19 12H5M11 6l-6 6 6 6"/>';
        }
        if (kind === "search") {
            return '<circle cx="11" cy="11" r="6.5"/><path d="m16 16 4.5 4.5"/>';
        }
        if (kind === "folder") {
            return '<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>';
        }
        if (kind === "save") {
            return '<path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><path d="M17 21v-8H7v8M7 3v5h8"/>';
        }
        if (kind === "sidebar") {
            return '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M9 4v16"/>';
        }
        if (kind === "newfile") {
            return '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/><path d="M12 18v-6M9 15h6"/>';
        }
        if (kind === "openfile") {
            return '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/><path d="M12 18v-6M9.5 14.5 12 12l2.5 2.5"/>';
        }
        if (kind === "file") {
            return '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/>';
        }
        if (kind === "refresh") {
            return '<path d="M21 12a9 9 0 1 1-2.64-6.36"/><path d="M21 3v6h-6"/>';
        }
        return "";
    }

    function iconDataUri(kind, col) {
        if (kind === "") {
            return "";
        }
        const hex = col.toString();
        const body = svgBody(kind).split("currentColor").join(hex);
        const svg = '<svg xmlns="http:\x2f\x2fwww.w3.org\x2f2000\x2fsvg" viewBox="0 0 24 24" fill="none" stroke="' + hex + '" stroke-width="1.85" stroke-linecap="round" stroke-linejoin="round">' + body + '</svg>';
        return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
    }

    implicitHeight: buttonHeight
    implicitWidth: buttonWidth > 0 ? buttonWidth : (iconOnly ? buttonHeight : (contentRow.implicitWidth + horizontalPadding * 2))

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: root.cornerRadius
        color: {
            if (primaryFill) {
                if (pressed)
                    return root.primaryPress;
                if (hovered)
                    return root.primaryHover;
                return root.primaryBg;
            }
            if (pressed) {
                return dangerHover ? fillDanger : fillPress;
            }
            if (hovered) {
                return dangerHover ? fillDanger : fillHover;
            }
            if (activeChip) {
                return fillChip;
            }
            if (accent) {
                return fillAccent;
            }
            if (elevated) {
                return root.theme.bgSurfaceElev || (darkMode ? "#1c1f27" : "#ffffff");
            }
            return "transparent";
        }
        border.width: primaryFill ? 0 : (accent ? 1.25 : 1)
        border.color: {
            if (dangerHover && (hovered || pressed)) {
                return borderDanger;
            }
            if (accent || activeChip) {
                return borderAccent;
            }
            if (ghost && !hovered && !pressed) {
                return "transparent";
            }
            return (hovered || pressed) ? borderHover : borderNormal;
        }


        Rectangle {
            visible: !root.pressed && (root.primaryFill || root.elevated || root.activeChip || root.accent || root.hovered)
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Math.max(4, root.cornerRadius - 1)
            anchors.rightMargin: Math.max(4, root.cornerRadius - 1)
            height: 1
            color: root.primaryFill ? (root.theme.sheenPrimary || (root.darkMode ? "#26ffffff" : "#30ffffff")) : (root.theme.sheen || (root.darkMode ? "#12ffffff" : "#80ffffff"))
        }

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                visible: root.showDot
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.primaryFill ? root.fgOnPrimary : root.fgNormal
            }

            Image {
                visible: root.iconKind !== "" && !root.iconOnRight
                width: root.iconSize
                height: root.iconSize
                sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
                smooth: true
                anchors.verticalCenter: parent.verticalCenter
                source: visible ? root.iconDataUri(root.iconKind, root.iconColor) : ""
            }

            Text {
                visible: root.text !== ""
                text: root.text
                color: {
                    if (root.primaryFill)
                        return root.fgOnPrimary;
                    if (root.dangerHover && (root.hovered || root.pressed))
                        return root.borderDanger;
                    if (root.accent || root.activeChip || root.hovered || root.pressed)
                        return root.fgNormal;
                    return root.fgMuted;
                }
                font.pixelSize: root.fontPixelSize
                font.weight: (root.primaryFill || root.accent || root.activeChip) ? Font.DemiBold : Font.Medium
                anchors.verticalCenter: parent.verticalCenter
            }

            Image {
                visible: root.iconKind !== "" && root.iconOnRight
                width: root.iconSize
                height: root.iconSize
                sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
                smooth: true
                anchors.verticalCenter: parent.verticalCenter
                source: visible ? root.iconDataUri(root.iconKind, root.iconColor) : ""
            }
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip {
        visible: root.tooltipText !== "" && ma.containsMouse
        delay: 520
        text: root.tooltipText
        contentItem: Text {
            text: root.tooltipText
            color: root.theme.fgTitle || (root.darkMode ? "#eceef2" : "#14161a")
            font.pixelSize: 11
        }
        background: Rectangle {
            radius: 7
            color: root.theme.bgSurfaceElev || (root.darkMode ? "#1c1e24" : "#ffffff")
            border.width: 1
            border.color: root.theme.borderControl || (root.darkMode ? "#323642" : "#cdd2dc")
        }
    }
}
