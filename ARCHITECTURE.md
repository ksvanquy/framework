# C++ Application Framework Architecture

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
| Initialize | `Discovered`, `Loaded` | `Initialized` |
| Start | `Initialized` | `Started` hoac `Running` |
| Stop | `Started`, `Running` | `Stopped` |
| Unload plugin | `Discovered`, `Loaded`, `Initialized`, `Stopped` | removed, then `Unloaded` |

`canInitialize()`, `canStart()` va `canStop()` trong `runtime/imodule.h` la nguon contract dung chung cho module va manager.

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
