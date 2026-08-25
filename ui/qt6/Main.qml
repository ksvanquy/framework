import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Framework.Qt6

ApplicationWindow {
    visible: true
    width: 700
    height: 460
    minimumWidth: 560
    minimumHeight: 380
    title: "Framework Test Console"
    color: "#f4f1ea"

    RuntimeBridge {
        id: runtimeBridge
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 20

        Label {
            text: "Framework Test Console"
            color: "#1e2a2f"
            font.pixelSize: 30
            font.bold: true
        }

        Label {
            text: "Runtime lifecycle and module integration"
            color: "#657277"
            font.pixelSize: 15
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 104
            radius: 8
            color: "#ffffff"
            border.color: "#d9d4c9"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 18

                Rectangle {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    radius: 8
                    color: runtimeBridge.state === "Running" ? "#2e8b6d" : "#c58a3a"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Runtime state"
                        color: "#657277"
                    }

                    Label {
                        text: runtimeBridge.state
                        color: "#1e2a2f"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }

                ColumnLayout {
                    spacing: 4

                    Label { text: "ExampleModule"; color: "#657277" }
                    Label {
                        text: runtimeBridge.moduleRegistered ? "Registered" : "Not registered"
                        color: runtimeBridge.moduleRegistered ? "#2e8b6d" : "#657277"
                        font.bold: true
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: "Start runtime"
                enabled: runtimeBridge.state !== "Running"
                onClicked: runtimeBridge.start()
            }

            Button {
                text: "Stop runtime"
                enabled: runtimeBridge.state === "Running"
                onClicked: runtimeBridge.stop()
            }

            Button {
                text: "Clear error"
                enabled: runtimeBridge.lastError.length > 0
                onClicked: runtimeBridge.clearError()
            }

            Button {
                text: "Reset runtime"
                onClicked: runtimeBridge.reset()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: runtimeBridge.lastError.length > 0 ? "#fff0ed" : "#e7eee9"
            border.color: runtimeBridge.lastError.length > 0 ? "#df8d7b" : "#b9d2c3"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8

                Label {
                    text: runtimeBridge.lastError.length > 0 ? "Last error" : "Diagnostics"
                    color: "#657277"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                      text: runtimeBridge.lastError.length > 0
                          ? runtimeBridge.lastError
                          : "Ready to exercise module registration, initialization, start and stop. Reset starts a fresh runtime."
                    color: runtimeBridge.lastError.length > 0 ? "#9b3e32" : "#315b4b"
                    wrapMode: Text.Wrap
                }

                Label {
                    Layout.fillWidth: true
                    text: "The UI is an optional Qt6 adapter; the framework runtime remains Qt-free."
                    color: "#657277"
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}