import QtQuick
import QtQuick.Window

Item {
    id: root

    property real size: 16
    property color color: "#f0f2f6"

    readonly property real dpr: (Window.window && Window.window.devicePixelRatio > 0) ? Window.window.devicePixelRatio : Math.max(1, Screen.devicePixelRatio)

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    function svgUri(col) {
        const hex = col.toString();
        const svg = '<svg xmlns="http:\\x2f\\x2fwww.w3.org\\x2f2000\\x2fsvg" viewBox="0 0 24 24" fill="none">' + '<path d="M16.5 2.5H7.7A5.2 5.2 0 0 0 2.5 7.7V16.5M21.5 7.5V16.3A5.2 5.2 0 0 1 16.3 21.5H7.5" stroke="' + hex + '" stroke-width="1.85" stroke-linecap="round"/>' + '<path d="M7.5 16.5V7.5L10.2 10.2M13.8 13.8L16.5 16.5V7.5" stroke="' + hex + '" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>' + '<path d="M4.5 19.5L19.5 4.5" stroke="' + hex + '" stroke-width="2" stroke-linecap="round"/>' + '</svg>';
        return "data:image/svg+xml;utf8," + encodeURIComponent(svg);
    }

    Image {
        anchors.fill: parent
        source: root.svgUri(root.color)
        sourceSize: Qt.size(Math.max(1, Math.round(root.width * root.dpr)), Math.max(1, Math.round(root.height * root.dpr)))
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: false
    }
}
