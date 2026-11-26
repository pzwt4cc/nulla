import QtQuick

Rectangle {
    id: root

    property bool darkMode: true
    property var theme: ({})
    property bool open: false
    property bool replaceMode: false
    property bool gotoMode: false
    property string query: ""
    property string replaceQuery: ""
    property int matchCurrent: 0
    property int matchTotal: 0
    property int totalLines: 1
    property var ui: ({})

    property color bgGutter: "#131418"
    property color bgEditor: "#16171b"
    property color bgSurfaceElev: "#181a20"
    property color borderSubtle: "#22242c"
    property color borderControl: "#2c2f38"
    property color borderFocus: "#687082"
    property color fgMain: "#eceef2"
    property color fgMuted: "#8c92a0"
    property color fgGutter: "#4a4f5c"
    property color selBg: "#2a3347"

    signal queryEdited(string text)
    signal findNextRequested(bool backward)
    signal replaceOneRequested(string replacement)
    signal replaceAllRequested(string replacement)
    signal gotoLineRequested(int lineNumber)
    signal closed

    function focusInput(initialText, isGoto) {
        if (isGoto) {
            findInput.text = "";
        } else if (initialText !== undefined && initialText !== "") {
            findInput.text = initialText;
        } else {
            findInput.text = root.query;
        }
        findInput.forceActiveFocus();
        findInput.selectAll();
    }

    visible: open
    height: visible ? (replaceMode && !gotoMode ? 72 : 38) : 0
    color: bgGutter
    clip: true

    Column {
        anchors.fill: parent
        spacing: 0

        Item {
            width: parent.width
            height: 38

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: closeFindBtn.left
                anchors.rightMargin: 8
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                spacing: 6

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(264, Math.max(120, root.width - 240))
                    height: 26
                    radius: 6
                    color: root.bgEditor
                    border.width: 1
                    border.color: findInput.activeFocus ? root.borderFocus : root.borderControl

                    Behavior on border.color {
                        ColorAnimation {
                            duration: 100
                        }
                    }

                    TextInput {
                        id: findInput
                        anchors.fill: parent
                        anchors.leftMargin: 9
                        anchors.rightMargin: 9
                        verticalAlignment: TextInput.AlignVCenter
                        color: root.fgMain
                        selectionColor: root.selBg
                        font.pixelSize: 12
                        clip: true
                        onTextChanged: {
                            if (!root.gotoMode) {
                                root.query = text;
                                root.queryEdited(text);
                            }
                        }
                        Keys.onPressed: function (event) {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (root.gotoMode) {
                                    var ln = parseInt(text.trim(), 10);
                                    if (!isNaN(ln) && ln >= 1) {
                                        root.gotoLineRequested(ln);
                                    }
                                    root.closed();
                                } else {
                                    root.findNextRequested((event.modifiers & Qt.ShiftModifier) !== 0);
                                }
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Escape) {
                                root.closed();
                                event.accepted = true;
                            }
                        }

                        Text {
                            visible: findInput.text === ""
                            text: root.gotoMode ? ((root.ui.gotoPlaceholderPrefix || "Go to line (1–") + root.totalLines + (root.ui.gotoPlaceholderSuffix || ")...")) : (root.ui.findPlaceholder || "Find in file...")
                            color: root.fgGutter
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Text {
                    visible: !root.gotoMode
                    anchors.verticalCenter: parent.verticalCenter
                    width: 82
                    elide: Text.ElideRight
                    text: root.query === "" ? "" : (root.matchTotal > 0 ? (root.matchCurrent + " / " + root.matchTotal) : (root.ui.zeroFound || "0 found"))
                    color: root.fgMuted
                    font.pixelSize: 12
                }

                Rectangle {
                    visible: !root.gotoMode
                    anchors.verticalCenter: parent.verticalCenter
                    height: 26
                    width: findNavRow.implicitWidth + 2
                    radius: 6
                    color: root.bgSurfaceElev
                    border.width: 1
                    border.color: root.borderControl

                    Row {
                        id: findNavRow
                        anchors.centerIn: parent
                        spacing: 0

                        M3Button {
                            iconKind: "up"
                            iconOnly: true
                            ghost: true
                            buttonHeight: 24
                            cornerRadius: 5
                            darkMode: root.darkMode
                theme: root.theme
                            tooltipText: root.ui.prevMatchTip || "Previous (Shift+Enter)"
                            onClicked: root.findNextRequested(true)
                        }

                        Rectangle {
                            width: 1
                            height: 14
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.borderSubtle
                        }

                        M3Button {
                            iconKind: "chevron"
                            iconOnly: true
                            ghost: true
                            buttonHeight: 24
                            cornerRadius: 5
                            darkMode: root.darkMode
                theme: root.theme
                            tooltipText: root.ui.nextMatchTip || "Next (Enter)"
                            onClicked: root.findNextRequested(false)
                        }
                    }
                }

                M3Button {
                    visible: !root.gotoMode
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.replaceBtn || "Replace"
                    ghost: !root.replaceMode
                    activeChip: root.replaceMode
                    buttonHeight: 26
                    horizontalPadding: 9
                    cornerRadius: 5
                    darkMode: root.darkMode
                theme: root.theme
                    tooltipText: root.ui.toggleReplaceTip || "Toggle Replace (Ctrl+H)"
                    onClicked: root.replaceMode = !root.replaceMode
                }
            }

            M3Button {
                id: closeFindBtn
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                iconKind: "close"
                iconOnly: true
                ghost: true
                buttonHeight: 26
                cornerRadius: 5
                darkMode: root.darkMode
                theme: root.theme
                tooltipText: root.ui.closeEscTip || "Close (Esc)"
                onClicked: root.closed()
            }
        }

        Item {
            visible: root.replaceMode && !root.gotoMode
            width: parent.width
            height: visible ? 34 : 0

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 6

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(264, Math.max(120, root.width - 240))
                    height: 26
                    radius: 6
                    color: root.bgEditor
                    border.width: 1
                    border.color: replaceInput.activeFocus ? root.borderFocus : root.borderControl

                    TextInput {
                        id: replaceInput
                        anchors.fill: parent
                        anchors.leftMargin: 9
                        anchors.rightMargin: 9
                        verticalAlignment: TextInput.AlignVCenter
                        color: root.fgMain
                        selectionColor: root.selBg
                        font.pixelSize: 12
                        clip: true
                        onTextChanged: root.replaceQuery = text
                        Keys.onPressed: function (event) {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                root.replaceOneRequested(text);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Escape) {
                                root.closed();
                                event.accepted = true;
                            }
                        }

                        Text {
                            visible: replaceInput.text === ""
                            text: root.ui.replacePlaceholder || "Replace with..."
                            color: root.fgGutter
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.replaceBtn || "Replace"
                    ghost: true
                    buttonHeight: 26
                    horizontalPadding: 10
                    cornerRadius: 5
                    darkMode: root.darkMode
                theme: root.theme
                    onClicked: root.replaceOneRequested(replaceInput.text)
                }

                M3Button {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.ui.replaceAllBtn || "All"
                    ghost: true
                    buttonHeight: 26
                    horizontalPadding: 10
                    cornerRadius: 5
                    darkMode: root.darkMode
                theme: root.theme
                    onClicked: root.replaceAllRequested(replaceInput.text)
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        anchors.bottom: parent.bottom
        color: root.borderSubtle
    }
}
