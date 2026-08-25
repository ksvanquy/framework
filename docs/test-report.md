# Test Report nội bộ

Tai lieu nay chot pham vi test noi bo cho framework truoc khi mo rong them module, service hoac UI. Muc tieu la khoa cac contract anh huong den do tin cay, lifetime va kha nang debug; khong theo duoi coverage phan tram.

## Trang thai hien tai

- `framework_core_tests`: 67 GoogleTest cases.
- `MiniAppSmoke`: 1 CTest smoke test.
- Ket qua xac minh gan nhat: **68/68 CTest pass**.
- Build mac dinh: `BUILD_TESTING=ON`, `BUILD_PLUGINS=ON`, `BUILD_EXAMPLES=ON`.
- Qt6 la optional; build Qt6 hien la smoke/manual test, khong phai release gate mac dinh.

Lenh baseline:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON -DBUILD_PLUGINS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

## Release gates

Tat ca gate P0 phai pass truoc khi merge thay doi runtime, plugin hoac service. Gate P1 bat buoc khi thay doi dung boundary tuong ung.

### P0: bat buoc

| Area | Internal test | Expected contract | Status |
| --- | --- | --- | --- |
| Core | `ErrorTest.*`, `ResultTest.*`, `CoreIntegrationTest.*`, `CoreSystemTest.*` | Khong mat error/value; move-only va public headers hoat dong | PASS |
| Lifecycle | `RuntimeTest.ModuleWithoutDependenciesCanInitializeAndStart`, `ExampleModuleFollowsCompleteLifecycle`, `RuntimeCanRestartAfterStop` | `initialize -> start -> stop -> initialize -> start` hop le | PASS |
| Dependency | `MissingDependencyIsRejected`, `SelfDependencyIsRejected`, `CircularDependencyIsRejected`, `IndependentModulesHaveDeterministicOrder` | Loi ro rang; forward order cho init/start, reverse order cho stop | PASS |
| Failure | `InitializeFailureLeavesKnownStates`, `FailedStartRollsBackModulesAlreadyStarted`, `StopFailureStillStopsRemainingModules` | State sau failure duoc xac dinh; tiep tuc cleanup; tra first error | PASS |
| Ownership | `LifetimeIntegrationTest.EventSubscriptionIsRemovedWhenModuleIsDestroyed` | Khong con callback tro vao module da huy | PASS |
| Plugin | `LoadsValidPluginAndOwnsModuleLifetime`, version tests, `RejectsMissingRequiredExport`, `RejectsCreateReturningNull` | Plugin khong hop le bi tu choi truoc khi dang ky | PASS |
| Plugin lifetime | `UnloadsPluginAfterStoppingItsModule`, `RuntimeStopUnloadsRegisteredPlugin` | Stop/destroy module truoc unload dynamic library | PASS |
| Application | `RunsAndReturnsApplicationExitCode`, `PropagatesConfigureFailure`, `StopsRuntimeWhenRunFails`, `PropagatesInitializeFailure`, `PropagatesStartFailure`, `PropagatesStopFailure` | Run failure giu exit code va van cleanup; lifecycle failure map exit code va giu `lastError()` | PASS |

### P1: bat buoc theo thay doi

| Area | Internal test |
| --- | --- |
| EventBus | Nhieu subscriber, unsubscribe truoc/sau publish, callback tu unsubscribe |
| CommandBus | Missing handler, duplicate handler, handler error |
| Config/Storage | Missing key, overwrite, empty key, read/write consistency |
| Scheduler | Cancellation, destructor join, callback khong chay sau cancel |
| Diagnostics/Logger | Snapshot/provider, category/level va concurrent output |
| Packaging | Install package, consumer configure/build/run voi target `Framework::` |
| Qt6 | QML load, state signal, error signal, start/stop lap lai |

Trang thai service P1: **PASS** cho cac contract deterministic va scheduler basic da co test truc tiep trong `services_test.cpp`. Concurrency stress va policy exception mo rong van nam ngoai bo test toi thieu.

## P0 tests: test gi, test nhu nao, ket qua

### 1. RuntimeTest.InitializeFailureLeavesKnownStates

- Test gi: failure khi initialize module sau module da initialize thanh cong.
- Test nhu nao: dung `TestModule` co the chu dong fail trong `initialize()`; gọi `ModuleManager::initializeAll()`.
- Ket qua: module truoc giu `Initialized`, module loi giu `Discovered`, error goc duoc tra ve.
- Ket qua hien tai: **PASS**.

### 2. RuntimeTest.StopFailureStillStopsRemainingModules

- Test gi: stop failure cua dependent module khong duoc ngan cleanup dependency.
- Test nhu nao: dung `TestModule` co `stop()` that bai; ghi lai event va state cua cac module.
- Ket qua: tiep tuc stop theo reverse dependency order, tra first error, module loi giu `Started`.
- Ket qua hien tai: **PASS**.

### 3. RuntimeTest.SelfDependencyIsRejected

- Test gi: module phu thuoc chinh ID cua no.
- Test nhu nao: dang ky module `self` voi dependency `self`, sau do goi `initializeAll()`.
- Ket qua: tra `StateError`, message chua `self -> self`, module van `Discovered`.
- Ket qua hien tai: **PASS**.

### 4. RuntimeTest.IndependentModulesHaveDeterministicOrder

- Test gi: thu tu lifecycle cua cac module khong co dependency.
- Test nhu nao: dang ky `charlie`, `alpha`, `bravo`; ghi event trong initialize/start/stop.
- Ket qua: initialize/start theo `alpha -> bravo -> charlie`, stop theo thu tu nguoc.
- Ket qua hien tai: **PASS**.

### 5. PluginLoaderTest.RejectsMalformedDescriptor

- Test gi: descriptor null, ID/name rong, dependency pointer null, dependency rong/trung.
- Test nhu nao: load 5 shared-library fixture descriptor malformed, moi fixture bao phu mot nhanh validation.
- Ket qua: ca 5 fixture bi tu choi voi `InvalidArgument`, khong tao module.
- Ket qua hien tai: **PASS**.

### 6. PluginLoaderTest.RejectsMissingRequiredExport

- Test gi: thieu tung C export `get_plugin_descriptor`, `create_plugin_module`, `destroy_plugin_module`.
- Test nhu nao: load ba shared-library fixture, moi fixture bo mot export bang compile definition.
- Ket qua: `PluginLoadFailed`, message chi ro symbol thieu, handle duoc dong khi load that bai.
- Ket qua hien tai: **PASS**.

### 7. PluginLoaderTest.RejectsCreateReturningNull

- Test gi: export day du nhung `create_plugin_module()` tra `nullptr`.
- Test nhu nao: load `framework_null_module_plugin`, fixture co descriptor/API/ABI hop le.
- Ket qua: `PluginLoadFailed`, message `Plugin module creation failed`, khong tao `LoadedPlugin`.
- Ket qua hien tai: **PASS**.

### 8. ApplicationIntegrationTest.StopsRuntimeWhenRunFails

- Test gi: `onRun()` tra exit code loi sau khi runtime da start.
- Test nhu nao: application dang ky `StopTrackingModule`, `onRun()` tra `23`, sau do goi `exec()`.
- Ket qua: runtime van stop module, `exec()` tra `23`, `lastError()` van null.
- Ket qua hien tai: **PASS**.

### 9. ApplicationIntegrationTest.PropagatesInitializeFailure

- Test gi: initialize module that bai.
- Test nhu nao: module tra `InternalError` trong `initialize()`.
- Ket qua: `exec()` tra `-2`, `lastError()` giu error, khong goi stop.
- Ket qua hien tai: **PASS**.

### 10. ApplicationIntegrationTest.PropagatesStartFailure

- Test gi: start module that bai sau khi initialize thanh cong.
- Test nhu nao: module tra `InternalError` trong `start()`.
- Ket qua: `exec()` tra `-3`, `lastError()` giu error, khong goi stop.
- Ket qua hien tai: **PASS**.

### 11. ApplicationIntegrationTest.PropagatesStopFailure

- Test gi: stop module that bai sau khi application da start.
- Test nhu nao: module tra `InternalError` trong `stop()`.
- Ket qua: `exec()` tra `-4`, `lastError()` giu error, va stop duoc goi.
- Ket qua hien tai: **PASS**.

## Cac gap con lai trong P0

P0 hien da co test cho malformed descriptor va cac failure path cua Application. Cac contract ngoai danh sach nay van can duoc chot truoc khi them assertion moi, dac biet la unload plugin tu state `Initialized`, exception trong callback, va threading model cua service.

## Cach chay va ket qua baseline

Chay toan bo CTest:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Ket qua gan nhat:

```text
41/41 tests passed
0 failed
```

Chay mot nhom GoogleTest:

```powershell
.\build\tests\Debug\framework_core_tests.exe --gtest_filter="RuntimeTest.*"
.\build\tests\Debug\framework_core_tests.exe --gtest_filter="PluginLoaderTest.*"
.\build\tests\Debug\framework_core_tests.exe --gtest_filter="ApplicationIntegrationTest.*"
```

Chay rieng tung P0 test:

```powershell
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=RuntimeTest.InitializeFailureLeavesKnownStates
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=RuntimeTest.StopFailureStillStopsRemainingModules
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=RuntimeTest.SelfDependencyIsRejected
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=RuntimeTest.IndependentModulesHaveDeterministicOrder
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=PluginLoaderTest.RejectsMissingRequiredExport
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=PluginLoaderTest.RejectsCreateReturningNull
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=PluginLoaderTest.RejectsMalformedDescriptor
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=ApplicationIntegrationTest.StopsRuntimeWhenRunFails
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=ApplicationIntegrationTest.PropagatesInitializeFailure
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=ApplicationIntegrationTest.PropagatesStartFailure
.\build\tests\Debug\framework_core_tests.exe --gtest_filter=ApplicationIntegrationTest.PropagatesStopFailure
```

## Contract can chot truoc khi viet assertion

Mot so behavior hien chua duoc xem la bug hay contract. Khong nen viet test theo phan doan:

- Initialize failure da chot: module da initialize giu `Initialized`, module that bai giu `Discovered`; framework khong rollback ngam vi `IModule` chua co `uninitialize()`.
- Stop failure da chot: tiep tuc stop cac module con lai theo reverse dependency order; tra first error, module that bai giu state hien tai.
- Co cho phep unload plugin tu state `Initialized` hay chi tu `Stopped`?
- Callback service duoc phep nem exception hay framework phai catch?
- Service nao thread-safe; scheduler va event dispatch dung thread nao?
- Application `onRun()` exception duoc map thanh exit code nao?

Quy tac: moi cau hoi tren phai co mot dong trong public contract/documentation truoc khi them test P0 tuong ung.

## Khong nam trong scope release gate

- Benchmark throughput cua EventBus/Scheduler.
- Stress test hang gio hoac fuzz plugin ABI.
- Test pixel/UI layout Qt6 tren moi platform.
- C++20 Modules build.
- Test implementation cua SQLite/network khi framework chua cung cap implementation.

## CI gates

```text
1. Debug build + CTest baseline
2. Package consumer configure/build/run
3. Ubuntu sanitizer build + CTest
4. Ubuntu clang-tidy build
5. Qt6 configure/build + startup smoke khi Qt6 duoc bat
```

Mot thay doi chi duoc xem la san sang khi gate lien quan den boundary thay doi deu pass. Test fail phai bao gom state hien tai, module ID, operation va error code/message de giu kha nang debug.
