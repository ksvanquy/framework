# Hướng dẫn xây dựng Edge TTS Studio

Tài liệu này hướng dẫn tạo một ứng dụng `Edge TTS Studio` bằng C++20 Application Framework trong repository này. Ứng dụng có thể:

- nhập văn bản và cấu hình giọng đọc;
- gửi yêu cầu đến Edge TTS qua một adapter riêng;
- lưu audio và lịch sử phiên đọc;
- phát sự kiện tiến trình cho UI;
- chạy tác vụ nền và báo cáo lỗi qua các service của framework.

Framework cung cấp runtime, module lifecycle và các service cơ bản. Edge TTS client, audio playback và UI là phần của ứng dụng, không nằm trong Core.

## 1. Kiến trúc mục tiêu

```text
Qt6/QML UI (optional)
        |
        v
StudioApplication
        |
        v
TtsStudioModule
        |
        +-- EdgeTtsClient       (HTTP/TTS adapter)
        +-- AudioPlayer         (OS/Qt adapter)
        +-- HistoryRepository   (IStorage adapter)
        |
        v
RuntimeContext
        |
        +-- ILogger
        +-- IEventBus
        +-- IConfig
        +-- ICommandBus
        +-- IScheduler
        +-- IStorage
        +-- IDiagnostics
        |
        v
Core
```

Nguyên tắc quan trọng:

1. `Core` không biết Qt6, QML, HTTP hay Edge TTS.
2. Module chỉ phụ thuộc vào interface và `RuntimeContext`, không truy cập `ModuleManager` internals.
3. Adapter bên ngoài chịu trách nhiệm cho network, file format và audio device.
4. Mọi callback vào module phải được hủy bằng `SubscriptionToken` trước khi module bị destroy.
5. Luôn dùng `Result<T>` cho lỗi vận hành thay vì ném exception qua boundary của framework.

Chi tiết về dependency và ownership nằm trong [ARCHITECTURE.md](../ARCHITECTURE.md).

## 2. Chuẩn bị môi trường

Cần có:

- Windows 10/11, Visual Studio 2022 và MSVC;
- CMake 3.20 trở lên;
- C++20 compiler;
- GoogleTest nếu build test;
- Qt 6.11 nếu muốn build UI QML.

Build ban đầu:

```powershell
cmake -S . -B build -DBUILD_EXAMPLES=ON -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Framework hiện có các target dùng trực tiếp:

- `framework_core`: `Error`, `Result<T>`, types và interface cơ bản;
- `framework_services`: các interface và default in-memory services;
- `framework_runtime`: `Application`, `Runtime`, `ModuleManager`, plugin loader;
- `framework_example_module`: module mẫu để tham khảo.

## 3. Tạo module TTS Studio

Tạo thư mục ứng dụng riêng, ví dụ:

```text
apps/edge_tts_studio/
    CMakeLists.txt
    main.cpp
    include/edge_tts_studio/tts_studio_module.h
    src/tts_studio_module.cpp
    src/edge_tts_client.h
    src/edge_tts_client.cpp
```

Module nên giữ state của một phiên đọc, ví dụ:

```cpp
struct SynthesisRequest {
    std::string text;
    std::string voice;
    std::string outputPath;
};

class EdgeTtsClient {
public:
    framework::core::Result<std::string> synthesize(
        const SynthesisRequest& request);
};
```

`EdgeTtsClient` là adapter ứng dụng. Nó có thể dùng thư viện HTTP và parser phù hợp, nhưng không nên được thêm vào `core/` hoặc `services/`. Adapter cần đóng gói:

- validate text, voice và output path;
- tạo request đến endpoint Edge TTS mà ứng dụng chọn;
- ghi audio vào file tạm thời an toàn;
- trả về `Result<string>` chứa đường dẫn file audio;
- chuyển lỗi HTTP, timeout và parse response thành `Error`.

Không hard-code credential trong source. Nếu endpoint hoặc credential cần cấu hình, đọc từ `IConfig` hoặc biến môi trường của ứng dụng.

## 4. Kết nối module với RuntimeContext

Module có thể nhận các service cần thiết qua constructor. Khi cần nhiều service, dùng `RuntimeContext` trong `initialize`:

```cpp
class TtsStudioModule final : public framework::runtime::IModule {
public:
    explicit TtsStudioModule(framework::runtime::RuntimeContext& context)
        : context_(context) {}

    framework::core::ModuleInfo info() const override;
    framework::core::Result<void> initialize() override;
    framework::core::Result<void> start() override;
    framework::core::Result<void> stop() override;

private:
    framework::runtime::RuntimeContext& context_;
    std::unique_ptr<framework::services::SubscriptionToken> synthesizeToken_;
};
```

Trong `initialize`, use-case có thể xử lý command từ UI hoặc controller. Ví dụ handler này dùng với một command registrar của ứng dụng:

```cpp
auto result = commandRegistrar_.registerHandler("tts.synthesize", [this](const void* data) {
    if (data == nullptr) {
        return framework::core::Error(
            framework::core::ErrorCode::InvalidArgument,
            "Synthesis request is null");
    }

    const auto& request = *static_cast<const SynthesisRequest*>(data);
    return synthesize(request);
});
if (!result) {
    return result;
}
```

Tên command là contract giữa presentation và module. UI không cần biết Edge TTS client được tạo như thế nào.

> Lưu ý: `ICommandBus` hiện tại chỉ có `send` trên interface; `registerHandler` là API của `InMemoryCommandBus`. Vì vậy đoạn trên là mẫu cho một `commandRegistrar_` được inject riêng, không phải trực tiếp là `context_.commandBus`. Trong app production, có thể đăng ký handler tại composition layer hoặc bổ sung một registration interface trước khi module dùng command bus.

## 5. Phát sự kiện tiến trình

Dùng `IEventBus` cho các sự kiện không cần giá trị trả về:

```cpp
struct SynthesisProgress {
    std::string requestId;
    int percent = 0;
    std::string state;
};

context_.eventBus.publish("tts.progress", &progress);
```

UI hoặc module khác subscribe:

```cpp
progressToken_ = context.eventBus.subscribe("tts.progress", [this](const void* data) {
    const auto& progress = *static_cast<const SynthesisProgress*>(data);
    updateProgress(progress);
});
```

Giữ `progressToken_` trong object có lifetime dài hơn callback. Token tự unsubscribe khi bị hủy; trong `stop`, có thể gọi `reset()` sớm để chủ động kết thúc subscription.

Không truyền pointer tới object ngắn hạn qua event bus. Nếu event được xử lý bất đồng bộ, hãy copy payload sang một object có ownership rõ ràng.

## 6. Chạy tác vụ nền bằng Scheduler

`IScheduler` phù hợp cho poll trạng thái, cleanup file tạm và các tác vụ lặp lại. Ví dụ:

```cpp
cleanupToken_ = context_.scheduler.scheduleInterval(
    std::chrono::minutes(5),
    [this] { removeExpiredTemporaryFiles(); });
```

Callback phải an toàn khi module đang stop. Hủy token trước khi giải phóng state mà callback sử dụng:

```cpp
framework::core::Result<void> TtsStudioModule::stop() {
    cleanupToken_.reset();
    progressToken_.reset();
    return {};
}
```

Không dùng fixed sleep để đồng bộ business logic hoặc test. Trong test, dùng `condition_variable`, `atomic` và timeout có giới hạn.

## 7. Lưu cấu hình và lịch sử

`IConfig` phù hợp để đọc cấu hình nhỏ như voice mặc định, rate và volume:

```cpp
auto voice = context_.config.getString("tts.voice");
if (!voice) {
    // Chọn default của ứng dụng khi key chưa tồn tại.
}
```

`IStorage` phù hợp cho metadata lịch sử:

```cpp
context_.storage.set(historyKey, audioPath);
auto audioPath = context_.storage.get(historyKey);
```

Default services là in-memory, nên dữ liệu mất khi process kết thúc. `IConfig` hiện tại là read-only interface; việc ghi cấu hình nên được thực hiện bởi configuration provider của app hoặc implementation concrete được app sở hữu. Khi cần persistence thật, tạo implementation `IConfig`/`IStorage` mới, ví dụ JSON, SQLite hoặc file system. Giữ implementation đó ở tầng Services/Application, không đưa dependency database vào Core.

## 8. Logger và Diagnostics

Log theo category có cấu trúc:

```cpp
context_.logger.log(
    framework::services::LogLevel::Info,
    "TtsStudio.Synthesis",
    "Synthesis request started");
```

Dùng `IDiagnostics` để cung cấp snapshot cho màn hình About/Diagnostics:

```cpp
context_.diagnostics.setProvider([this] {
    return framework::services::DiagnosticSnapshot{
        "Running",
        {"TtsStudio", "EdgeTtsAdapter"}};
});
```

Không ghi text văn bản người dùng vào log mặc định. Nếu cần debug request, log request id, voice và kích thước text thay vì nội dung nhạy cảm.

## 9. Application composition

`Application` điều phối chu kỳ:

```cpp
class StudioApplication final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(
        framework::runtime::Runtime& runtime) override {
        return runtime.moduleManager().registerModule(
            std::make_unique<TtsStudioModule>(runtime.context()));
    }

    int onRun() override {
        return 0;
    }
};

int main(int argc, char* argv[]) {
    StudioApplication application;
    return application.exec(argc, argv);
}
```

Trình tự lifecycle là:

```text
onConfigureModules
    -> Runtime::initialize
    -> Runtime::start
    -> onRun
    -> Runtime::stop
```

`onConfigureModules`, `initialize`, `start` và `stop` đều phải trả lỗi rõ ràng. Nếu một bước thất bại, `Application::exec` trả exit code âm và lưu `lastError()`.

Với Qt/QML, `onRun` thường khởi động event loop của UI adapter. Qt chỉ nên xuất hiện trong `ui/qt6` và target app Qt, không xuất hiện trong module domain hoặc Core.

## 10. CMake cho ứng dụng

Ứng dụng executable có thể bắt đầu với CMake như sau:

```cmake
add_executable(edge_tts_studio
    main.cpp
    src/tts_studio_module.cpp
    src/edge_tts_client.cpp
)

target_link_libraries(edge_tts_studio PRIVATE
    framework_runtime
)
```

Nếu tách module thành library:

```cmake
add_library(edge_tts_studio_module
    src/tts_studio_module.cpp
    src/edge_tts_client.cpp
)

target_link_libraries(edge_tts_studio_module PRIVATE
    framework_runtime
)

target_link_libraries(edge_tts_studio PRIVATE
    edge_tts_studio_module
)
```

Nếu ứng dụng nằm trong cùng repository, thêm subdirectory trước target app. Nếu dùng package đã install, dùng các target namespace từ `FrameworkTargets.cmake` và cấu hình `CMAKE_PREFIX_PATH` theo package install của framework.

## 11. Test plan cho Edge TTS Studio

Tách test thành ba lớp:

### Unit test

- validate `SynthesisRequest`;
- map HTTP/parse error thành `ErrorCode`;
- command `tts.synthesize` gọi đúng use case;
- event progress có payload đúng;
- scheduler cleanup không chạy sau cancel;
- history repository xử lý missing key.

### Integration test

- module register/start/stop với `Runtime`;
- runtime restart: `initialize -> start -> stop -> initialize -> start`;
- adapter dùng fake HTTP server, không phụ thuộc network thật;
- audio file được tạo và có thể đọc lại;
- lỗi một module không để lại subscription hoặc worker.

### UI test

- nhập text và chọn voice;
- disable nút Speak khi request đang chạy;
- hiển thị progress và lỗi;
- play file vừa tạo;
- mở lại history.

Chạy test framework và test app:

```powershell
cmake --build build --config Debug --target framework_core_tests
ctest --test-dir build -C Debug --output-on-failure
```

Không nên gọi Edge TTS production endpoint trong CTest. Dùng fake client hoặc fake HTTP transport để test deterministic, nhanh và không cần credential.

## 12. Checklist trước khi merge

- [ ] Core không có include Qt6, HTTP client hoặc database.
- [ ] Module có `initialize`, `start`, `stop` và xử lý lỗi.
- [ ] Mọi subscription và scheduler token được giữ bằng RAII.
- [ ] Không có callback nào dùng object đã bị destroy.
- [ ] Credential không nằm trong source hoặc test fixture.
- [ ] Request timeout và lỗi network được xử lý.
- [ ] File tạm được cleanup khi cancel và khi stop.
- [ ] Unit test không dùng fixed sleep để chờ callback.
- [ ] Integration test có runtime restart và failure path.
- [ ] `ctest --test-dir build -C Debug --output-on-failure` pass.

## Tài liệu liên quan

- [Architecture](../ARCHITECTURE.md)
- [Test report nội bộ](test-report.md)
- [Mini application example](miniapp.md)
- [Runtime example source](../examples/miniapp/main.cpp)
- [Qt6/QML adapter](../ui/qt6/CMakeLists.txt)