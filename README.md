# C++ Application Framework

Framework C++20 dạng modular để xây dựng các ứng dụng desktop hoặc service có lifecycle rõ ràng, dependency dễ kiểm soát và khả năng mở rộng bằng module/plugin.

Trạng thái scope: **đã chốt**. Repository này là baseline hoàn chỉnh của framework lõi, không phải một dự án ứng dụng cụ thể.

## Phạm vi đã hoàn thành

- `Core` độc lập với Qt6, UI, database và network implementation.
- `Runtime` điều phối application lifecycle.
- `ModuleManager` quản lý đăng ký, dependency, initialize/start/stop và rollback khi start thất bại.
- Built-in module và external plugin qua C ABI.
- Bảy service mặc định: logger, config, event bus, command bus, scheduler, storage và diagnostics.
- `Result<T>`/`Error` cho operational error.
- Qt6/QML adapter tùy chọn, không làm Qt trở thành dependency của Core.
- CMake package export và package consumer test.
- Unit, integration, stress và system test với GoogleTest/CTest.

## Kiến trúc ở mức cao

```text
Application
    |
    +-- Qt6/QML adapter (tùy chọn)
    |
    v
Runtime / ModuleManager
    |
    +-- Built-in modules
    +-- External plugins
    |
    v
Services
    |
    v
Core
    |
    v
C++ STL / OS / Platform
```

Dependency chỉ đi từ tầng cao xuống tầng thấp. Runtime sở hữu services và module manager; module sở hữu state nghiệp vụ của mình; `RuntimeContext` chỉ chứa non-owning references.

Xem đầy đủ contract, lifecycle, plugin ABI và ownership tại [ARCHITECTURE.md](ARCHITECTURE.md).

## Yêu cầu môi trường

- Windows với Visual Studio 2022/MSVC hoặc compiler C++20 tương đương.
- CMake 3.20 trở lên.
- Qt 6.11 chỉ cần khi build adapter Qt6/QML.

## Build và test

Build mặc định gồm examples, plugins và tests:

```powershell
cmake -S . -B build `
    -DBUILD_TESTING=ON `
    -DBUILD_EXAMPLES=ON `
    -DBUILD_PLUGINS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Baseline đã xác minh:

```text
GoogleTest: 67 tests
CTest:      68 tests
Kết quả:    68/68 pass
```

Chạy riêng executable test:

```powershell
.\build\tests\Debug\framework_core_tests.exe
```

Build các target chính:

```text
framework_core
framework_services
framework_runtime
framework_example_module
framework_core_tests
framework_miniapp
framework_runtime_example
```

## Quick start

Một application tối thiểu kế thừa `Application`, đăng ký module trong `onConfigureModules()` và trả exit code từ `onRun()`:

```cpp
#include "runtime/application.h"
#include "runtime/runtime.h"

class MyApplication final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(
        framework::runtime::Runtime& runtime) override {
        // Đăng ký built-in module của ứng dụng tại đây.
        (void)runtime;
        return {};
    }

    int onRun() override {
        return 0;
    }
};

int main(int argc, char* argv[]) {
    MyApplication application;
    return application.exec(argc, argv);
}
```

Lifecycle được framework điều phối theo thứ tự:

```text
onConfigureModules
    -> initialize
    -> start
    -> onRun
    -> stop
```

Ví dụ hoàn chỉnh nằm trong [examples/miniapp/main.cpp](examples/miniapp/main.cpp).

## Plugin

Plugin export ba symbol C ABI:

```cpp
extern "C" const PluginDescriptor* get_plugin_descriptor() noexcept;
extern "C" runtime::IModule* create_plugin_module() noexcept;
extern "C" void destroy_plugin_module(runtime::IModule*) noexcept;
```

`PluginLoader` kiểm tra symbol, descriptor, API/ABI version, dependency metadata và sự khớp giữa descriptor với module. Khi unload, framework stop module, destroy module qua destroy function rồi mới đóng dynamic library.

Chi tiết API nằm trong [plugins/plugin_sdk/plugin_api.h](plugins/plugin_sdk/plugin_api.h).

## Qt6/QML tùy chọn

Qt6 chỉ được build khi cần:

```powershell
cmake -S . -B build-qt `
    -DBUILD_QT6=ON `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
cmake --build build-qt --config Debug
```

Adapter Qt6/QML nằm trong `ui/qt6`. Core, Services và plugin SDK không include Qt6.

## Ví dụ ứng dụng

Blueprint cho ứng dụng Edge TTS Studio được ghi tại [docs/edge-tts-studio-guide.md](docs/edge-tts-studio-guide.md). Tài liệu này minh họa cách đặt Edge TTS client, audio adapter và UI ở tầng ứng dụng thay vì đưa chúng vào Core.

## Cấu trúc repository

```text
core/                  Error, Result, types, contracts nền tảng
services/              Service interfaces và default implementations
runtime/               Application, Runtime, ModuleManager, PluginLoader
modules/example_module Module built-in mẫu
plugins/               Plugin SDK và plugin fixtures
examples/              Runtime example và MiniApp
ui/qt6/                Adapter Qt6/QML tùy chọn
tests/                 Unit, integration, stress, system, package tests
docs/                  Tài liệu sử dụng và test report nội bộ
```

## Tài liệu

- [Kiến trúc đã chốt](ARCHITECTURE.md)
- [Hướng dẫn Edge TTS Studio](docs/edge-tts-studio-guide.md)
- [Test report nội bộ](docs/test-report.md)
- [MiniApp example](docs/miniapp.md)

## Giới hạn của scope

Framework không cung cấp sẵn Edge TTS client, audio playback, HTTP transport, SQLite implementation hoặc UI application hoàn chỉnh. Đây là các adapter/feature thuộc từng application. C++20 Modules cũng không được dùng làm runtime plugin system.