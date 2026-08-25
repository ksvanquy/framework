# Kiến trúc C++ Application Framework

## 1. Trạng thái và mục tiêu

Đây là kiến trúc đã chốt cho framework C++20 trong repository. Mục tiêu của baseline là cung cấp một nền tảng modular, dễ debug và dễ test để nhiều ứng dụng C++ có thể dùng chung runtime, lifecycle và các service cơ bản.

Framework ưu tiên dependency rõ ràng, ownership rõ ràng và lifecycle có thể quan sát. Không đưa abstraction hoặc dependency vào framework chỉ để dự phòng cho một ứng dụng cụ thể.

Các mục tiêu đã hoàn thành:

- Core nhỏ, ổn định và độc lập với Qt6.
- Runtime có `Application`, `Runtime` và `ModuleManager`.
- Module có dependency graph và lifecycle được kiểm soát.
- Plugin boundary tối thiểu qua C ABI.
- Service implementations mặc định có thể test trực tiếp.
- Failure path, lifetime và cleanup được kiểm tra bằng GoogleTest/CTest.

## 2. Nguyên tắc phụ thuộc

```text
Application
    |
    +-- UI / Presentation (Qt6/QML tùy chọn)
    |
    v
Runtime / ModuleManager
    |
    v
Modules / Plugins
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

Dependency chỉ đi xuống. Các dependency sau bị cấm:

```text
Core -> Services / Runtime / Modules / Qt6
Services -> Application / UI / ModuleManager internals
Storage implementation -> UI
Plugin SDK -> Qt6
```

Core không biết Qt6, QML, SQLite, HTTP implementation, UI hoặc plugin implementation. Qt6 chỉ xuất hiện trong adapter/presentation target tùy chọn.

`Framework module` và `C++ module` là hai khái niệm khác nhau:

- **Framework module** là object runtime implement `IModule`, có lifecycle và do `ModuleManager` quản lý.
- **C++ module** là tính năng ngôn ngữ/compiler như `export module` và `import`; nó không phải cơ chế plugin runtime của framework.

## 3. Các tầng và trách nhiệm

### 3.1 Core

Core chứa các contract nền tảng, không chứa implementation nghiệp vụ:

- `Error`, `ErrorCode`, `Result<T>` và `Result<void>`.
- Module ID, types và các interface dùng chung.
- Các kiểu dữ liệu lifecycle khi cần.

Core chỉ phụ thuộc C++ STL và platform primitives phù hợp.

### 3.2 Services

Services cung cấp capability dùng chung qua interface và implementation:

| Interface | Trách nhiệm |
| --- | --- |
| `ILogger` | Ghi log theo level/category |
| `IEventBus` | Publish/subscribe event, subscription RAII |
| `IConfig` | Đọc configuration |
| `ICommandBus` | Gửi command tới handler |
| `IScheduler` | Lập lịch task lặp |
| `IStorage` | Lưu/đọc key-value |
| `IDiagnostics` | Chụp runtime snapshot |

Default implementations nằm trong `services/default_services.h`: `ConsoleLogger`, `InMemoryConfig`, `InMemoryEventBus`, `InMemoryCommandBus`, `InMemoryStorage`, `ThreadScheduler` và `BasicDiagnostics`.

Các implementation mặc định phục vụ composition và test. `InMemoryConfig`/`InMemoryStorage` không phải persistence production; ứng dụng có thể thay bằng implementation riêng mà không sửa Core.

### 3.3 Runtime

Runtime là tầng điều phối và sở hữu:

- service instances;
- `RuntimeContext`;
- `ModuleManager`;
- lifecycle của application và module;
- plugin load/register/unload.

`RuntimeContext` chứa non-owning references:

```cpp
struct RuntimeContext {
    services::ILogger& logger;
    services::IEventBus& eventBus;
    services::IConfig& config;
    services::ICommandBus& commandBus;
    services::IScheduler& scheduler;
    services::IStorage& storage;
    services::IDiagnostics& diagnostics;
};
```

Runtime phải sống lâu hơn mọi module đang sử dụng context. Constructor injection được ưu tiên khi module chỉ cần một hoặc hai service; `RuntimeContext` phù hợp khi module cần nhiều service.

### 3.4 Modules và Plugins

Module chứa feature hoặc nghiệp vụ. Built-in module được sở hữu bằng `std::unique_ptr<IModule>`. Plugin module được quản lý cùng dynamic-library handle và destroy function để allocator boundary an toàn.

Module không truy cập implementation nội bộ của `ModuleManager`. Module chỉ dùng public contract và các service được inject.

## 4. Lifecycle và dependency graph

Lifecycle đầy đủ:

```text
Discovered -> Loaded -> Initialized -> Started -> Running
Running -> Stopping -> Stopped -> Unloaded
```

Contract hiện tại:

| Operation | State đầu vào | Kết quả thành công |
| --- | --- | --- |
| Register | `Discovered`, `Loaded` | Module được đăng ký |
| Initialize | `Discovered`, `Loaded`, `Stopped` | `Initialized` |
| Start | `Initialized` | `Started` hoặc `Running` |
| Stop | `Started`, `Running` | `Stopped` |
| Unload plugin | `Discovered`, `Loaded`, `Initialized`, `Stopped` | Module bị remove, plugin `Unloaded` |

Sau khi stop thành công, module có thể initialize/start lại trên cùng instance. Điều này cho phép chu kỳ:

```text
initialize -> start -> stop -> initialize -> start
```

`ModuleManager` phải:

- initialize/start dependency trước dependent module;
- stop theo thứ tự ngược dependency;
- từ chối start module chưa initialize;
- phát hiện missing dependency, self-dependency và circular dependency;
- rollback các module đã start nếu module tiếp theo start thất bại;
- tiếp tục cleanup các module còn lại khi một stop operation thất bại;
- từ chối state transition không hợp lệ.

`canInitialize()`, `canStart()` và `canStop()` trong [runtime/include/runtime/imodule.h](runtime/include/runtime/imodule.h) là nguồn contract chung cho module và manager.

## 5. Application lifecycle

`Application` là composition layer tối giản:

```text
onConfigureModules()
    -> Runtime::initialize()
    -> Runtime::start()
    -> onRun()
    -> Runtime::stop()
```

`onConfigureModules()` trả `Result<void>` để lỗi đăng ký không bị bỏ qua. `Application::exec()` map lỗi theo bước:

| Bước lỗi | Exit code mặc định |
| --- | --- |
| Configure | `-1` |
| Initialize | `-2` |
| Start | `-3` |
| Stop khi `onRun()` trả 0 | `-4` |
| Stop khi `onRun()` đã trả lỗi | Giữ exit code của `onRun()` |

Lỗi gốc được lưu trong `lastError()`.

## 6. Plugin ABI và unload safety

Plugin SDK nằm trong [plugins/plugin_sdk/plugin_api.h](plugins/plugin_sdk/plugin_api.h). Plugin phải export:

```cpp
extern "C" const PluginDescriptor* get_plugin_descriptor() noexcept;
extern "C" runtime::IModule* create_plugin_module() noexcept;
extern "C" void destroy_plugin_module(runtime::IModule*) noexcept;
```

`PluginDescriptor` gồm ID, name, semantic version, API/ABI version và dependency metadata.

`PluginLoader` validate:

- dynamic library load được;
- ba export bắt buộc tồn tại;
- descriptor không null, ID/name không rỗng;
- API và ABI version tương thích;
- dependency pointer/count hợp lệ, không rỗng và không trùng;
- `descriptor.id` khớp `module.info().id`;
- `create_plugin_module()` không trả null.

Thứ tự cleanup bắt buộc:

```text
stop module
    -> cancel scheduler task / unsubscribe event
    -> destroy module bằng destroy_plugin_module()
    -> unload dynamic library
```

Không unload plugin khi module, callback, scheduler task, function pointer hoặc data pointer còn trỏ vào dynamic library.

`SubscriptionToken` là move-only RAII token. Token phải được module giữ trong member có lifetime phù hợp và reset trước khi giải phóng state mà callback sử dụng.

## 7. Error handling và logging

Operational error dùng `Result<T>`/`Error` với các nhóm chính:

- `InvalidArgument`;
- `NotFound`;
- `AlreadyExists`;
- `StateError`;
- `PluginLoadFailed`;
- `InternalError`.

Không dùng exception cho lỗi nghiệp vụ thông thường. Boundary của framework phải chuyển exception từ handler/callback không kiểm soát thành behavior có contract.

Log theo category có cấu trúc:

```text
[Runtime.ModuleManager] Registering module
[Module:Example] Starting
[Application:Miniapp] Application is running
```

Không ghi credential hoặc toàn bộ nội dung văn bản người dùng vào log mặc định.

## 8. CMake và packaging

Các option chính:

```text
BUILD_TESTING
BUILD_EXAMPLES
BUILD_PLUGINS
BUILD_QT6
ENABLE_WARNINGS
ENABLE_CLANG_TIDY
ENABLE_SANITIZERS
```

Target framework:

```cmake
find_package(Framework CONFIG REQUIRED)
target_link_libraries(app PRIVATE Framework::framework_runtime)
```

Package export các target `framework_core`, `framework_services`, `framework_runtime` và `framework_example_module`. Package consumer được kiểm tra riêng trong `tests/package_consumer`.

Qt6 không nằm trong package dependency của Core/Services/Runtime; adapter Qt6 được build qua `BUILD_QT6=ON`.

## 9. Cấu trúc repository

```text
core/                  Core contracts và types
services/              Service interfaces/implementations
runtime/               Runtime, Application, ModuleManager, PluginLoader
modules/example_module Module built-in mẫu
plugins/plugin_sdk/    Plugin C ABI
plugins/example_plugin Plugin và validation fixtures
examples/              Runtime example và MiniApp
ui/qt6/                Qt6/QML adapter tùy chọn
tests/                 Unit, integration, stress, system, package
	docs/                  User guide và internal test report
```

## 10. Test baseline và release gate

Baseline hiện tại:

```text
framework_core_tests: 67 GoogleTest cases
MiniAppSmoke:          1 CTest test
Tổng:                  68/68 CTest pass
```

Các nhóm contract đã được kiểm tra:

- `Error`/`Result` và public Core headers;
- lifecycle, restart, dependency order và failure rollback;
- plugin version, descriptor, export và module lifetime;
- Application configure/initialize/start/run/stop failure;
- lifetime của event subscription;
- Config, Storage, EventBus, CommandBus, Diagnostics, Scheduler và Logger;
- package consumer, stress/system checks và MiniApp smoke.

Lệnh release gate:

```powershell
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Qt6 là optional/manual gate khi `BUILD_QT6=ON`, không phải dependency của baseline Core.

## 11. Ngoài scope đã chốt

Các phần sau thuộc application hoặc roadmap riêng, không phải trách nhiệm của framework baseline:

- Edge TTS client và HTTP protocol implementation;
- audio playback device;
- SQLite hoặc persistence implementation cụ thể;
- UI/UX hoàn chỉnh của từng ứng dụng;
- benchmark throughput và stress test hàng giờ;
- C++20 Modules thay cho C++ header truyền thống;
- production authentication/credential management của từng backend.

Blueprint ứng dụng Edge TTS Studio nằm tại [docs/edge-tts-studio-guide.md](docs/edge-tts-studio-guide.md).# C++ Application Framework Architecture

## 1. Muc tieu

Framework nay duoc thiet ke de xay dung nhieu ung dung C++ co cau truc modular, de mo rong va de debug.

Muc tieu chinh:

- Quan ly lifecycle cua framework module bang `ModuleManager`.
- Ho tro built-in module va external plugin.
- Phan tach ro ownership, dependency va lifetime.
- Su dung `Result<T>` va `Error` cho operational error.
- Giữ Core nho, on dinh va khong phu thuoc UI.
- Qt6/QML chi la adapter tuy chon o tang presentation.
- Khong dua C++20 Modules vao runtime plugin system.

Trong tai lieu nay, hai khai niem sau la khac nhau:

- **Framework module**: object runtime implement `IModule`, co lifecycle va duoc `ModuleManager` quan ly.
- **C++ module**: tinh nang cua ngon ngu/compiler nhu `export module` va `import`.

## 2. Layering

Dependency chi di mot chieu tu tang cao xuong tang thap:

```text
Application
		|
		v
UI / Presentation (optional Qt6/QML)
		|
		v
Runtime / ModuleManager
		|
		v
Modules / Plugins
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

### 2.1 Core

Core la tang thap nhat cua framework. Core chi biet C++ STL va cac abstraction co ban:

- `Error`, `ErrorCode`, `Result<T>`.
- Core types, IDs va interface co ban.
- Cac kieu du lieu lifecycle khi phu hop.

Core khong duoc biet ve:

- Qt6, QML hoac UI.
- SQLite, network implementation hoac storage implementation.
- Runtime, ModuleManager, plugin implementation.

### 2.2 Services

Services cung cap capability dung chung thong qua interface va implementation:

- `ILogger`
- `IEventBus`
- `IConfig`
- `ICommandBus`
- `IScheduler`
- `IStorage`
- `IDiagnostics`

Service implementation duoc Runtime tao va so huu. Module nhan service bang constructor injection hoac `RuntimeContext`.

### 2.3 Runtime

Runtime la tang dieu phoi:

- Quan ly application lifecycle.
- Quan ly `ModuleManager`.
- Resolve dependency graph va topological order.
- Initialize/start/stop module.
- Quan ly plugin load, registration va unload.
- Cung cap `RuntimeContext` cho cac component can nhieu service.

### 2.4 Modules va Plugins

Module chua business feature. Plugin la module duoc cung cap tu dynamic library (`.dll`, `.so` hoac `.dylib`).

Built-in module duoc so huu bang `std::unique_ptr<IModule>`. Plugin module duoc quan ly boi `LoadedPlugin` va `destroy_plugin_module()` de dam bao dung ABI va allocator.

### 2.5 UI / Presentation

Qt6/QML nam o tang tren cung va chi phu thuoc Runtime/Modules. Qt6 khong duoc tro thanh dependency cua Core, Services hoac Plugin SDK.

## 3. Dependency rules

Khong cho phep cac dependency sau:

```text
Core -> Services / Modules / Runtime / Qt6
Services -> Modules / Runtime / Application
Storage implementation -> UI
Module -> ModuleManager internals
```

Interface nen dat o tang thap nhat hop ly. Implementation dat o tang cao hon. Module khong truy cap implementation noi bo cua ModuleManager.

Khong tao circular dependency giua module. `ModuleManager` phai tu choi graph co chu ky truoc khi initialize.

## 4. Runtime context

Framework su dung `RuntimeContext` thay cho global service locator:

```cpp
struct RuntimeContext {
		services::ILogger& logger;
		services::IEventBus& eventBus;
		services::IConfig& config;
		services::ICommandBus& commandBus;
		services::IScheduler& scheduler;
		services::IStorage& storage;
		services::IDiagnostics& diagnostics;
};
```

`RuntimeContext` chi chua non-owning references. Runtime so huu service va phai song lau hon moi module su dung context.

Constructor injection van duoc uu tien khi module chi can mot hoac hai service. Context phu hop cho application composition va module can nhieu service.

## 5. Module lifecycle

Lifecycle day du:

```text
Discovered -> Loaded -> Initialized -> Started -> Running
Running -> Stopping -> Stopped -> Unloaded
```

Contract hien tai:

| Operation | State dau vao | State thanh cong |
| --- | --- | --- |
| Register | `Discovered`, `Loaded` | registered |
| Initialize | `Discovered`, `Loaded`, `Stopped` | `Initialized` |
| Start | `Initialized` | `Started` hoac `Running` |
| Stop | `Started`, `Running` | `Stopped` |
| Unload plugin | `Discovered`, `Loaded`, `Initialized`, `Stopped` | removed, then `Unloaded` |

`canInitialize()`, `canStart()` va `canStop()` trong `runtime/imodule.h` la nguon contract dung chung cho module va manager.

Sau khi stop thanh cong, module co the initialize va start lai tren cung instance. Vi vay application co the lap lai chu trinh `initialize -> start -> stop` ma khong can reset Runtime.

`ModuleManager` dam bao:

- Dependency initialize/start truoc dependent module.
- Stop theo thu tu nguoc dependency.
- Khong start module chua initialize.
- Tu choi state transition khong hop le.
- Rollback cac module da start neu mot module start that bai.
- Detect circular va missing dependency.

## 6. Plugin ABI

Plugin boundary su dung C ABI toi gian trong `plugins/plugin_sdk/plugin_api.h`.

`PluginDescriptor` gom:

- `id`
- `name`
- semantic version
- `apiVersion`
- `abiVersion`
- dependency array va `dependencyCount`

Plugin phai export ba symbol:

```cpp
extern "C" const PluginDescriptor* get_plugin_descriptor() noexcept;
extern "C" runtime::IModule* create_plugin_module() noexcept;
extern "C" void destroy_plugin_module(runtime::IModule*) noexcept;
```

`PluginLoader` validate:

- Dynamic library co the load.
- Ba export symbol ton tai.
- Descriptor khong null, `id` va `name` khong rong.
- API version va ABI version tuong thich.
- Dependency metadata hop le, khong null va khong trung.
- `descriptor.id` khop voi `module.info().id`.

Plugin khong duoc expose framework internals khong can thiet qua ABI.

## 7. Ownership va lifetime

Ownership chinh:

```text
Runtime
	owns services and ModuleManager
ModuleManager
	owns built-in module proxies and plugin owners
LoadedPlugin
	owns module handle, destroy function and dynamic library handle
Module
	owns its private resources
```

Thu tu cleanup plugin bat buoc:

```text
stop module
		-> cancel task / unsubscribe callback
		-> destroy module via destroy_plugin_module()
		-> unload dynamic library
```

`SubscriptionToken` la RAII token. Khi module bi destroy, token tu unsubscribe de tranh dangling callback.

Khong duoc unload plugin khi van con:

- Module object dang song.
- EventBus subscription dang tro vao plugin.
- Scheduler task co callback vao plugin.
- Function pointer hoac data pointer tro vao dynamic library.

## 8. Application API

`Application` la composition layer toi gian:

```text
onConfigureModules()
		-> Runtime::initialize()
		-> Runtime::start()
		-> onRun()
		-> Runtime::stop()
```

`onConfigureModules()` tra ve `Result<void>` de loi register module khong bi bo qua. `Application::exec()` tra exit code rieng cho configure, initialize, start va stop failure, dong thoi luu `lastError()`.

## 9. Logging va error handling

Moi component log voi category co cau truc:

```text
[Runtime.ModuleManager] Registering module
[Module:Example] Starting
[Application:Miniapp] Application is running
```

Operational error dung `Result<T>` va `Error`. Cac nhom loi chinh:

- `InvalidArgument`
- `NotFound`
- `AlreadyExists`
- `StateError`
- `PluginLoadFailed`
- `InternalError`

Khong dung exception cho loi nghiep vu thong thuong.

## 10. CMake, packaging va CI

CMake options:

```text
BUILD_TESTING
BUILD_EXAMPLES
BUILD_PLUGINS
BUILD_QT6
ENABLE_WARNINGS
ENABLE_CLANG_TIDY
ENABLE_SANITIZERS
```

Package export cac target:

```cmake
find_package(Framework CONFIG REQUIRED)
target_link_libraries(app PRIVATE Framework::framework_runtime)
```

CI thuc hien:

- Build va test tren Ubuntu va Windows.
- Install package va build package consumer doc lap.
- Chay AddressSanitizer va UndefinedBehaviorSanitizer tren Ubuntu.
- Chay clang-tidy tren Ubuntu.

## 11. Cau truc thu muc

```text
framework/
├── core/
├── services/
├── runtime/
├── modules/
│   └── example_module/
├── plugins/
│   ├── plugin_sdk/
│   └── example_plugin/
├── examples/
│   ├── runtime_example/
│   └── miniapp/
├── ui/
│   └── qt6/
│       ├── adapters/
│       └── viewmodels/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── package_consumer/
│   ├── stress/
│   └── system/
├── cmake/
├── CMakeLists.txt
└── ARCHITECTURE.md
```

## 12. Test strategy

### Unit tests

- Core `Error` va `Result`.
- Service implementations.
- Lifecycle contract va ModuleManager.
- Plugin descriptor validation.

### Integration tests

- Dependency ordering.
- Circular/missing dependency.
- Plugin load/register/start/stop/unload.
- Event subscription cleanup.
- Application success va failure path.

### Stress va system tests

- Lap lai construction/error handling.
- Kiem tra public Core headers.
- Kiem tra executable miniapp.
- Chay sanitizer de phat hien memory va undefined behavior.

## 13. Qt6/QML adapter

Qt6/QML la optional va duoc build bang:

```powershell
cmake -S . -B build-qt `
	-DBUILD_QT6=ON `
	-DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
cmake --build build-qt --config Debug
```

`RuntimeBridge` la `QObject` typed QML, expose state va cac operation start/stop. Adapter su dung Runtime/ExampleModule nhung khong dua Qt vao Core, Services hoac Runtime.

## 14. Nguyen tac phat trien

Truoc khi them feature moi:

1. Xac dinh tang so huu abstraction.
2. Kiem tra dependency chi di xuong.
3. Chot ownership va cleanup order.
4. Them unit test cho contract.
5. Them integration/lifetime test neu co boundary module, plugin, thread hoac callback.
6. Chay full CTest, warnings, sanitizer va static analysis truoc khi merge.
