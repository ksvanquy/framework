# Blueprint ứng dụng Qt6/QML

Tài liệu này mô tả cách xây dựng một ứng dụng UI Qt6/QML trên C++ Application Framework hiện tại. Ví dụ trong repository là `framework_qt6_example`, một test console dùng UI để điều khiển lifecycle của Runtime và `ExampleModule`.

Qt6/QML là lớp presentation tùy chọn. `Core`, `Services`, plugin SDK và phần lớn Runtime không được phụ thuộc Qt6.

## 1. Kiến trúc ứng dụng

```text
QML Main.qml
    |
    | Q_PROPERTY / Q_INVOKABLE / signals
    v
RuntimeBridge : QObject
    |
    v
Runtime / ModuleManager
    |
    +-- ExampleModule
    +-- Core services
```

Trách nhiệm của từng phần:

- `Main.qml`: layout, trạng thái hiển thị và thao tác người dùng.
- `RuntimeBridge`: adapter QObject, chuyển API C++/framework thành API QML.
- `Runtime`: lifecycle và module orchestration.
- `ExampleModule`: business/module behavior.
- `Core` và `Services`: logic không biết Qt.

Source tham khảo:

- [ui/qt6/main.cpp](../ui/qt6/main.cpp)
- [ui/qt6/adapters/runtime_bridge.h](../ui/qt6/adapters/runtime_bridge.h)
- [ui/qt6/adapters/runtime_bridge.cpp](../ui/qt6/adapters/runtime_bridge.cpp)
- [ui/qt6/Main.qml](../ui/qt6/Main.qml)
- [ui/qt6/CMakeLists.txt](../ui/qt6/CMakeLists.txt)

## 2. Qt entry point

`main.cpp` chỉ chịu trách nhiệm khởi tạo Qt application, tạo QML engine và load QML module:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("Framework.Qt6", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return application.exec();
}
```

Không khởi tạo service global trong `main.cpp`. Runtime và module nên được tạo trong bridge hoặc composition layer để lifecycle có owner rõ ràng.

## 3. RuntimeBridge làm adapter

`RuntimeBridge` kế thừa `QObject` và dùng QML integration:

```cpp
class RuntimeBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool moduleRegistered READ moduleRegistered
               NOTIFY moduleRegisteredChanged)

public:
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void clearError();
};
```

Quy ước bridge:

- `Q_PROPERTY` dùng cho state mà QML cần render.
- `Q_INVOKABLE` dùng cho command từ UI gửi xuống C++.
- `signals` dùng để thông báo thay đổi hoặc lỗi.
- `std::unique_ptr<runtime::Runtime>` bảo đảm Runtime có owner duy nhất.
- Không đưa QObject hoặc kiểu Qt vào Core/Services interface.

## 4. Start, stop và reset

Khi QML gọi `start()`, bridge:

1. đăng ký `ExampleModule` nếu chưa đăng ký;
2. gọi `Runtime::initialize()`;
3. gọi `Runtime::start()`;
4. xóa lỗi cũ và phát state `Running`.

Khi gọi `stop()`, bridge gọi `Runtime::stop()`, xóa lỗi nếu thành công và phát state `Stopped`.

`reset()` dừng Runtime hiện tại, tạo Runtime mới, đặt lại cờ `moduleRegistered` và xóa lỗi. Đây là thao tác hữu ích cho test console và các màn hình retry, nhưng không phải lúc nào cũng cần trong app production.

Ví dụ điều khiển trong QML:

```qml
RuntimeBridge {
    id: runtimeBridge
}

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
```

UI nên lấy trạng thái từ property thay vì tự giữ một bản sao không đồng bộ với Runtime.

## 5. Hiển thị lỗi và diagnostics

Bridge chuyển `core::Error::message()` thành `QString` và phát `errorOccurred` khi có lỗi:

```qml
Label {
    text: runtimeBridge.lastError.length > 0
        ? runtimeBridge.lastError
        : "Ready"
    wrapMode: Text.Wrap
}

Connections {
    target: runtimeBridge
    function onErrorOccurred(message) {
        console.log(message)
    }
}
```

Không nên để QML tự diễn giải `ErrorCode` hoặc truy cập implementation của service. Nếu cần thông báo thân thiện, bridge có thể map error code thành view model hoặc message riêng ở tầng UI.

## 6. QML module và CMake

CMake hiện dùng `qt_add_qml_module` để đóng gói QML và QObject adapter:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_add_executable(framework_qt6_example
    main.cpp
)

qt_add_qml_module(framework_qt6_example
    URI Framework.Qt6
    VERSION 1.0
    SOURCES
        adapters/runtime_bridge.cpp
        adapters/runtime_bridge.h
    QML_FILES
        Main.qml
)

target_link_libraries(framework_qt6_example PRIVATE
    framework_runtime
    framework_example_module
    Qt6::Quick
)
```

`QML_ELEMENT` cho phép `RuntimeBridge` được dùng trực tiếp trong module `Framework.Qt6`. Khi thêm bridge mới, thêm source vào `SOURCES`, thêm `QML_ELEMENT` và khai báo property/signal cần thiết.

## 7. Configure và build

Giả sử Qt được cài tại `C:/Qt/6.11.2/msvc2022_64`:

```powershell
cmake -S . -B build-qt `
    -DBUILD_QT6=ON `
    -DBUILD_EXAMPLES=ON `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64

cmake --build build-qt --config Debug --target framework_qt6_example
```

Chạy ứng dụng:

```powershell
.\build-qt\ui\qt6\Debug\framework_qt6_example.exe
```

## 8. Qt deployment trên Windows

Target có post-build command gọi `windeployqt` khi công cụ được tìm thấy trong Qt installation. Công cụ này copy Qt DLL, plugin và QML imports cần thiết cạnh executable.

Nếu chạy app báo thiếu DLL, kiểm tra:

```powershell
Get-Command windeployqt
Test-Path C:/Qt/6.11.2/msvc2022_64/bin/windeployqt.exe
```

Có thể chạy thủ công:

```powershell
C:/Qt/6.11.2/msvc2022_64/bin/windeployqt.exe `
    --qmldir ui/qt6 `
    build-qt/ui/qt6/Debug/framework_qt6_example.exe
```

Không commit DLL được deploy vào source tree. Build directory là artifact của môi trường chạy.

## 9. Mở rộng thành UI ứng dụng thật

Khi thay test console bằng app thực tế, giữ bridge mỏng và đưa use case vào module/service:

```text
QML view
    -> UI bridge/view model
        -> Runtime command/event boundary
            -> application module
                -> service interfaces / adapters
```

Ví dụ với Edge TTS Studio:

- QML quản lý text editor, voice selector, progress và playback controls;
- `TtsStudioBridge` chuyển thao tác UI thành command/use case;
- `TtsStudioModule` điều phối synthesis;
- `EdgeTtsClient` xử lý HTTP/TTS ở tầng application;
- `IEventBus` phát progress/completion;
- `IStorage` lưu history;
- Qt audio adapter xử lý playback.

Network, audio, database và QML không được đưa vào `core/`. Chi tiết blueprint Edge TTS nằm trong [edge-tts-studio-guide.md](edge-tts-studio-guide.md).

## 10. Testing

Tách test theo boundary:

- **Core/Runtime tests**: chạy không cần Qt, kiểm tra lifecycle, dependency, rollback và error paths.
- **Bridge tests**: kiểm tra property, signal, start/stop/reset và chuyển lỗi.
- **QML tests**: kiểm tra binding, button state, error rendering và load module.
- **Smoke test**: khởi động executable và xác nhận QML root object được load.

Runtime tests hiện có thể chạy độc lập với Qt:

```powershell
cmake --build build --config Debug --target framework_core_tests
ctest --test-dir build -C Debug --output-on-failure
```

Qt app cần được configure trong `build-qt` trước khi chạy UI test. Không gọi Edge TTS production endpoint trong test; dùng fake client hoặc fake transport.

## 11. Checklist

- [ ] Core và Services không include Qt6.
- [ ] Mọi QObject bridge có ownership và parent/lifetime rõ ràng.
- [ ] QML chỉ gọi API public qua property, invokable và signal.
- [ ] Runtime error được chuyển thành UI state có thể hiển thị.
- [ ] `stop()` được gọi trước khi Runtime hoặc bridge bị destroy.
- [ ] Subscription token và scheduler token không còn callback tới object đã hủy.
- [ ] QML module URI/version khớp với `engine.loadFromModule()`.
- [ ] `windeployqt` được chạy cho build Windows phân phối.
- [ ] UI test không thay thế cho runtime/unit test không cần Qt.

## Tài liệu liên quan

- [Kiến trúc framework](../ARCHITECTURE.md)
- [MiniApp blueprint](miniapp.md)
- [Edge TTS Studio blueprint](edge-tts-studio-guide.md)
- [Test report nội bộ](test-report.md)