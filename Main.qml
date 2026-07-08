import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vnc_mouse_touch_input_qtmcus_proxy

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("RFB Pointer Event VNC Proxy")

    header: RowLayout {
        id: headerRowLayout
        function socketStateColor(socketState, socketName) {
            console.log(socketName, "socket state:", socketState)
            switch (socketState) {
            case QAbstractSocket.UnconnectedState:
                return "red";
            case QAbstractSocket.ConnectingState:
                return "yellow";
            case QAbstractSocket.ConnectedState:
                return "green";
            case QAbstractSocket.ClosingState:
                return "pink";
            default:
                break;
            }
            return "white"
        }

        spacing: 5
        property int toolTipDelay: 750
        Rectangle {
            id: listenBlob
            Layout.leftMargin: 5
            Layout.topMargin: 5
            Layout.preferredWidth: radius*2
            Layout.preferredHeight: radius*2
            radius: 12
            color: vncProxyServer.listening ? "green" : "red"

            HoverHandler { id: lBlobHH }
            ToolTip.visible: lBlobHH.hovered
            ToolTip.delay: parent.toolTipDelay
            ToolTip.text: vncProxyServer.listening ? "Listening" : "Not Listening"
        }
        Rectangle {
            id: viewerConnectedBlob
            Layout.topMargin: 5
            Layout.preferredWidth: radius*2
            Layout.preferredHeight: radius*2
            radius: 12
            color: vncProxyServer.viewerConnected ? "green" : "red"
            HoverHandler { id: vcBlobHH }
            ToolTip.visible: vcBlobHH.hovered
            ToolTip.delay: parent.toolTipDelay
            ToolTip.text: vncProxyServer.viewerConnected ? "Viewer Connected" : "Viewer Disconnected"
        }
        Rectangle {
            id: deviceUartConnectedBlob
            Layout.topMargin: 5
            Layout.preferredWidth: radius*2
            Layout.preferredHeight: radius*2
            radius: 12
            color: vncProxyServer.sessionBroker ? headerRowLayout.socketStateColor(vncProxyServer.sessionBroker.deviceUartSocketState, "deviceUart") :  "red";
            HoverHandler { id: duBlobHH }
            ToolTip.visible: duBlobHH.hovered
            ToolTip.delay: parent.toolTipDelay
            ToolTip.text: "Device UART"
        }
        Rectangle {
            id: deviceVncConnectedBlob
            Layout.topMargin: 5
            Layout.preferredWidth: radius*2
            Layout.preferredHeight: radius*2
            radius: 12
            color: vncProxyServer.sessionBroker ? headerRowLayout.socketStateColor(vncProxyServer.sessionBroker.deviceVncSocketState, "deviceVnc") :  "red";
            HoverHandler { id: dvBlobHH }
            ToolTip.visible: dvBlobHH.hovered
            ToolTip.delay: parent.toolTipDelay
            ToolTip.text: "Device VNC"
        }

        Item {
            Layout.fillWidth: true
        }
    }

    Item {
        anchors.fill: parent
    TapHandler {
        onTapped: {
            vncProxyServer.printState();
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        RowLayout {
            Label {
                text: "UART Port"
            }

            SpinBox {
                id: uartPortTField
                from: 5000
                to: 65535
                value: 12345

                textFromValue: function(value, locale) {
                    return value;
                }
            }
        }

        Button {
            id: startListeningBut
            text: "Listen"
            enabled: !vncProxyServer.listening

            Connections {
                function onClicked() {
                    vncProxyServer.start();
                }
            }
        }
        Item { Layout.fillHeight: true }
    }
    }

    VncProxyServer {
        id: vncProxyServer
        proxyPort: 5901
        vncPort: 5900
        uartPort: uartPortTField.value

        function printState() {
            var viewerConn = vncProxyServer.viewerConnected;
            console.log("L:", vncProxyServer.listening, "V:", viewerConn);
            if (viewerConn) {
                var sessionBroker = vncProxyServer.sessionBroker;
                console.log("Uart:", sessionBroker.deviceUartSocketState, "Vnc:", sessionBroker.deviceVncSocketState);
            }
        }
    }
}
