import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property var ui: ({})
    property var updater: null
    property bool suppressed: false

    signal restartRequested

    readonly property int statusVal: updater ? updater.status : 0
    readonly property color bgCard: theme.bgSurfaceElev || (darkMode ? "#1c1f27" : "#ffffff")
    readonly property color borderCard: theme.borderControl || (darkMode ? "#313644" : "#c6ccd8")
    readonly property color borderDivider: theme.borderDivider || (darkMode ? "#262934" : "#e3e6ee")
    readonly property color fgTitle: theme.fgTitle || (darkMode ? "#eceef2" : "#14161a")
    readonly property color fgSub: theme.fgSub || (darkMode ? "#8a91a0" : "#5e6573")

    visible: !!(updater && updater.bannerVisible && !suppressed)
    width: Math.min(344, Math.max(220, (parent ? parent.width : 400) - 28))
    height: toastCol.implicitHeight + 24
    radius: 11
    color: bgCard
    border.width: 1
    border.color: borderCard
    clip: true

    MouseArea {
        anchors.fill: parent
        onWheel: function (wheel) {
            wheel.accepted = true;
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 11
        anchors.rightMargin: 11
        height: 1
        color: theme.sheenBright || (darkMode ? "#14ffffff" : "#90ffffff")
    }

    Column {
        id: toastCol
        x: 14
        y: 12
        width: parent.width - 28
        spacing: 8

        Item {
            width: parent.width
            height: 24

            NullaMark {
                id: mark
                size: 15
                color: root.fgTitle
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                anchors.left: mark.right
                anchors.leftMargin: 8
                anchors.right: dismissIconBtn.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                color: root.fgTitle
                font.pixelSize: 13
                font.weight: Font.DemiBold
                text: {
                    if (!root.updater) {
                        return "";
                    }
                    if (root.statusVal === 4) {
                        return (root.ui.updToastDownloading || "Downloading") + " v" + root.updater.availableVersion + " (" + root.updater.downloadProgress + "%)";
                    }
                    if (root.statusVal === 5) {
                        return (root.ui.updToastReady || "Update ready") + " · v" + root.updater.availableVersion;
                    }
                    if (root.statusVal === 7) {
                        return root.ui.updToastError || "Update failed";
                    }
                    return (root.ui.updToastAvail || "Update available") + " · v" + root.updater.availableVersion;
                }
            }

            M3Button {
                id: dismissIconBtn
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconKind: "close"
                iconOnly: true
                ghost: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: 24
                iconSize: 13
                cornerRadius: 7
                tooltipText: root.ui.updBtnLater || "Later"
                onClicked: if (root.updater) {
                    root.updater.dismissBanner();
                }
            }
        }

        Text {
            visible: root.statusVal !== 4
            width: parent.width
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: root.fgSub
            font.pixelSize: 12
            lineHeight: 1.3
            text: {
                if (!root.updater) {
                    return "";
                }
                if (root.statusVal === 7) {
                    return root.updater.errorMessage;
                }
                if (root.statusVal === 5) {
                    return root.ui.updToastReadyDesc || "Restart Nulla to finish installing.";
                }
                if (root.updater.systemInstalled || !root.updater.canSelfUpdate) {
                    return root.ui.updToastSystemDesc || "System package detected. Open release page to update.";
                }
                return (root.ui.updToastCurrent || "Installed:") + " v" + root.updater.currentVersion;
            }
        }

        Rectangle {
            visible: root.statusVal === 4
            width: parent.width
            height: 5
            radius: 2.5
            color: root.borderDivider
            clip: true

            Rectangle {
                height: parent.height
                radius: 2.5
                color: root.fgTitle
                width: Math.max(6, parent.width * (root.updater ? Math.max(0, Math.min(100, root.updater.downloadProgress)) / 100.0 : 0))
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 6

            M3Button {
                visible: root.statusVal === 3
                text: root.ui.updBtnSkip || "Skip"
                ghost: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: 26
                horizontalPadding: 9
                cornerRadius: 8
                fontPixelSize: 11
                onClicked: if (root.updater) {
                    root.updater.skipVersion();
                }
            }

            M3Button {
                text: root.ui.updBtnLater || "Later"
                ghost: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: 26
                horizontalPadding: 9
                cornerRadius: 8
                fontPixelSize: 11
                onClicked: if (root.updater) {
                    root.updater.dismissBanner();
                }
            }

            M3Button {
                visible: root.statusVal === 3 || root.statusVal === 5 || root.statusVal === 7
                primaryFill: true
                darkMode: root.darkMode
                theme: root.theme
                buttonHeight: 26
                horizontalPadding: 11
                cornerRadius: 8
                fontPixelSize: 11
                text: {
                    if (!root.updater) {
                        return "";
                    }
                    if (root.statusVal === 5) {
                        return root.ui.updBtnRestart || "Restart & Update";
                    }
                    if (root.statusVal === 7) {
                        return root.ui.updBtnRetry || "Retry";
                    }
                    return root.updater.canSelfUpdate ? (root.ui.updBtnUpdate || "Update") : (root.ui.updBtnRelease || "Release");
                }
                onClicked: {
                    if (!root.updater) {
                        return;
                    }
                    if (root.statusVal === 5) {
                        root.restartRequested();
                    } else {
                        root.updater.startDownload();
                    }
                }
            }
        }
    }
}
