import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Framework.Qt6

ApplicationWindow {
    visible: true
    width: 520
    height: 320
    title: "Framework Runtime"

    RuntimeBridge {
        id: runtimeBridge
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16

        Label {
            text: "Runtime / Example Module"
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "State: " + runtimeBridge.state
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Button {
                text: "Start"
                enabled: runtimeBridge.state !== "Running"
                onClicked: runtimeBridge.start()
            }

            Button {
                text: "Stop"
                enabled: runtimeBridge.state === "Running"
                onClicked: runtimeBridge.stop()
            }
        }
    }
}