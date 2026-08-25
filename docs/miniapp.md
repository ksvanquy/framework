# MiniApp Blueprint

`MiniApp` là ví dụ nhỏ nhất cho cách ghép một application bằng framework. Ví dụ này không có UI và không gọi network; mục tiêu là minh họa composition layer, module lifecycle, event bus và logging.

## Mục tiêu

Khi chạy MiniApp, framework sẽ:

1. tạo `Runtime` và các default services;
2. đăng ký `ExampleModule`;
3. subscribe các event `example.started` và `example.stopped`;
4. initialize rồi start module;
5. chạy `onRun()`;
6. stop module và tự giải phóng các subscription.

Sơ đồ dependency:

```text
MiniApplication
	   |
	   v
Application::exec
	   |
	   v
Runtime / ModuleManager
	   |
	   +-- ExampleModule
	   +-- RuntimeContext
			  +-- Logger
			  +-- EventBus
```

## Source chính

File mẫu: [examples/miniapp/main.cpp](../examples/miniapp/main.cpp)

Target được định nghĩa tại [examples/miniapp/CMakeLists.txt](../examples/miniapp/CMakeLists.txt):

```cmake
add_executable(framework_miniapp
	main.cpp
)

target_link_libraries(framework_miniapp PRIVATE
	framework_runtime
	framework_example_module
)
```

`framework_miniapp` chỉ cần link Runtime và module mẫu. Runtime đã kéo theo Core và Services cần thiết.

## 1. Kế thừa Application

Application của MiniApp kế thừa `framework::runtime::Application`:

```cpp
class MiniApplication final : public framework::runtime::Application {
protected:
	framework::core::Result<void> onConfigureModules(
		framework::runtime::Runtime& runtime) override;

	int onRun() override;
};
```

`Application::exec()` điều phối lifecycle cố định:

```text
onConfigureModules
	-> Runtime::initialize
	-> Runtime::start
	-> onRun
	-> Runtime::stop
```

Mỗi bước có thể trả lỗi bằng `Result<void>`. Exit code lỗi được giữ ở `lastError()` của `Application`.

## 2. Đăng ký event subscription

Trong `onConfigureModules()`, MiniApp subscribe hai event do `ExampleModule` phát ra:

```cpp
startedSubscription_ = runtime.context().eventBus.subscribe(
	"example.started", [this](const void*) {
		onModuleEvent("Example module started");
	});

stoppedSubscription_ = runtime.context().eventBus.subscribe(
	"example.stopped", [this](const void*) {
		onModuleEvent("Example module stopped");
	});
```

`subscribe()` trả về `std::unique_ptr<SubscriptionToken>`. MiniApp giữ token như member để subscription sống cùng application và tự unsubscribe khi object bị hủy.

Luôn kiểm tra token trước khi tiếp tục composition:

```cpp
if (startedSubscription_ == nullptr || stoppedSubscription_ == nullptr) {
	return framework::core::Error(
		framework::core::ErrorCode::InternalError,
		"Miniapp could not subscribe to module events");
}
```

Callback không nên giữ pointer tới state đã bị destroy. Với module/plugin, hãy reset token trước khi unload module.

## 3. Đăng ký built-in module

MiniApp đăng ký module bằng `ModuleManager` và chuyển các service cần thiết qua constructor:

```cpp
return runtime.moduleManager().registerModule(
	std::make_unique<framework::modules::ExampleModule>(
		runtime.context().logger,
		runtime.context().eventBus));
```

`ModuleManager` sở hữu module sau khi đăng ký. Module được initialize/start theo dependency order và stop theo thứ tự ngược lại.

Đây là pattern nên dùng cho application thật:

- application tạo composition;
- runtime sở hữu lifecycle;
- module sở hữu state nghiệp vụ;
- service interface được inject qua constructor hoặc `RuntimeContext`.

## 4. Chạy application

`onRun()` ghi một log và trả exit code thành công:

```cpp
int onRun() override {
	runtime_.context().logger.log(
		framework::services::LogLevel::Info,
		"Application:Miniapp",
		"Application is running");
	return 0;
}
```

Hàm `main()` chỉ tạo application và gọi `exec()`:

```cpp
int main(int argc, char* argv[]) {
	MiniApplication application;
	return application.exec(argc, argv);
}
```

Business logic không nên đặt trong `main()`. Hãy đặt logic vào module hoặc service adapter tương ứng.

## 5. Build và chạy

Configure repository:

```powershell
cmake -S . -B build `
	-DBUILD_EXAMPLES=ON `
	-DBUILD_PLUGINS=ON `
	-DBUILD_TESTING=ON
```

Build MiniApp:

```powershell
cmake --build build --config Debug --target framework_miniapp
```

Chạy trên Windows:

```powershell
.\build\examples\miniapp\Debug\framework_miniapp.exe
```

Output dự kiến:

```text
Application is running
Example module started
Example module stopped
```

Exit code thành công là `0`.

## 6. Smoke test

CMake đăng ký MiniApp như một CTest smoke test khi `BUILD_TESTING=ON`:

```cmake
if(BUILD_TESTING)
	add_test(NAME MiniAppSmoke COMMAND framework_miniapp)
endif()
```

Chạy riêng smoke test:

```powershell
ctest --test-dir build -C Debug -R MiniAppSmoke --output-on-failure
```

Chạy toàn bộ test suite:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## 7. Mở rộng blueprint

Để biến MiniApp thành một ứng dụng cụ thể, giữ nguyên composition pattern và thay module mẫu bằng module nghiệp vụ:

```text
Application
	-> register application modules
	-> inject RuntimeContext/services
	-> start UI hoặc service loop
```

Ví dụ với Edge TTS Studio:

- `TtsStudioModule` xử lý use case tổng hợp giọng nói;
- `EdgeTtsClient` là HTTP/TTS adapter ở tầng application;
- `IEventBus` phát progress và completion event;
- `IScheduler` xử lý cleanup file tạm;
- `IStorage` lưu history;
- Qt6/QML chỉ nằm ở UI adapter tùy chọn.

Xem blueprint chi tiết tại [docs/edge-tts-studio-guide.md](edge-tts-studio-guide.md) và kiến trúc tổng thể tại [ARCHITECTURE.md](../ARCHITECTURE.md).

## Giới hạn của MiniApp

MiniApp cố ý không phải production application. Nó không cung cấp:

- UI hoặc Qt event loop;
- HTTP/network client;
- persistent configuration hoặc database;
- audio playback;
- plugin loading trong chính ví dụ.

Các capability này thuộc application hoặc adapter riêng, không nên đưa vào Core.