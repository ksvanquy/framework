Kiến trúc & Sơ đồ phân tầng (Layering Architecture)
Kiến trúc tuân thủ nghiêm ngặt nguyên tắc Dependency một chiều (Strict Unidirectional Dependency) từ trên xuống dưới:
┌────────────────────────────────────────────────────────┐
│ 1. Application Layer (Main entry, App composition)     │
└───────────────────────────┬────────────────────────────┘
                            │ uses
                            ▼
┌────────────────────────────────────────────────────────┐
│ 2. UI / Presentation Layer (Optional Qt6 / QML adapter)│
└───────────────────────────┬────────────────────────────┘
                            │ binds
                            ▼
┌────────────────────────────────────────────────────────┐
│ 3. Runtime & ModuleManager Layer (Lifecycle, DI, Graph)│
└───────────────────────────┬────────────────────────────┘
                            │ hosts
                            ▼
┌────────────────────────────────────────────────────────┐
│ 4. Modules & Plugins Layer (Business features)         │
└───────────────────────────┬────────────────────────────┘
                            │ consumes
                            ▼
┌────────────────────────────────────────────────────────┐
│ 5. Services Layer (Config, EventBus, Storage, Logger...)│
└───────────────────────────┬────────────────────────────┘
                            │ implements / relies on
                            ▼
┌────────────────────────────────────────────────────────┐
│ 6. Core Layer (Types, Error/Result, Interfaces, IDs)   │
└───────────────────────────┬────────────────────────────┘
                            │ uses
                            ▼
┌────────────────────────────────────────────────────────┐
│ 7. Platform / C++ STL Layer (OS, Filesystem, Threads)  │
└────────────────────────────────────────────────────────>

Nguyên tắc vàng: Tầng dưới tuyệt đối không được biết hay include header của tầng trên. (Ví dụ: Core không biết Services, Services không biết Modules).

3. Dependency Rules & Boundaries
Core: Hoàn toàn thuần khiết (Pure C++ STL). Không chứa logic nghiệp vụ, không biết gì về Qt, SQLite, Network hay Plugin.

Services: Cung cấp các service dạng interface tại Core và implementation tại Services. Các module tương tác với nhau hoặc hệ thống thông qua Services (ví dụ: IEventBus, ILogger).

Runtime: Nhạc trưởng điều phối. Chịu trách nhiệm đọc cấu hình, nạp module, giải quyết đồ thị phụ thuộc (Dependency Graph), và kích hoạt chu trình khởi động/dừng.

Plugins: Là các Dynamic Library (.dll / .so / .dylib) tuân thủ ABI contract nghiêm ngặt của framework, được load động bởi Runtime thông qua IPlugin interface.

4. Class / Interface Chính (Conceptual Overview)
IModule: Giao diện cơ sở cho mọi module/plugin (initialize(), start(), stop()).

ModuleManager: Chịu trách nhiệm quản lý state machine của lifecycle module và topological sort dựa trên dependency.

IServiceLocator / ServiceRegistry: Quản lý các cross-cutting services (đơn giản, tường minh, tránh Service Locator anti-pattern quá mức bằng cách ưu tiên Constructor Injection trong nội bộ module).

Result<T> & Error: Mô hình trả về lỗi an toàn, không dùng exception cho lỗi nghiệp vụ thông thường.

5. Ownership & Lifetime Strategy
Toàn cục (Global/Framework Lifetime): Các Services cốt lõi (Logger, Config, Diagnostics) sống suốt vòng đời của Application Runtime.

Theo Module (Module Lifetime): Các object nội bộ của module được sở hữu độc quyền bởi class instance của module đó (std::unique_ptr). Khi module STOPPED và UNLOADED, mọi tài nguyên phải được thu hồi tự động.

Subscription/Callback Safety: Sử dụng cơ chế token-based cho EventBus hoặc Signal-Slot thủ công. Khi một object (hoặc module) bị hủy, token tự động hủy đăng ký (unsubscribe) để triệt tiêu hoàn toàn dangling callback và use-after-free.

6. Lifecycle State Machine (Tóm tắt)
Mỗi module di chuyển qua các trạng thái tuần tự:
DISCOVERED ➔ LOADED ➔ INITIALIZED ➔ STARTED ➔ RUNNING ➔ STOPPING ➔ STOPPED ➔ UNLOADED.
ModuleManager kiểm tra điều kiện chuyển đổi trạng thái và tự động sắp xếp thứ tự khởi động (đúng chiều dependency) và tắt (ngược chiều dependency).

7. Error Handling Strategy
Sử dụng Result<T> chứa giá trị hoặc Error struct (gồm ErrorCode, std::string message, và SourceLocation).

Phân loại lỗi rõ ràng: Operational Error (trả về Result), Programmer Error (dùng assert hoặc Panic), Fatal Framework Error (abort/terminate an toàn kèm log chẩn đoán).

8. Debug Strategy (Structured Logging)
Mọi component xuất log theo định dạng thống nhất có gắn nhãn ngữ cảnh:
[Timestamp] [ThreadID] [Level] [Category:SubCategory] Message

Ví dụ: [Runtime][ModuleManager] Loading module: NetworkModule (v1.0.0)

Giúp developer truy vết chính xác object nào đang ở state nào, API nào đang được gọi ngay từ cái nhìn đầu tiên trên terminal hoặc file log.

9. Directory Structure (Cấu trúc thư mục chi tiết)

framework/
├── CMakeLists.txt
├── core/
│   ├── include/core/
│   │   ├── types.h
│   │   ├── error.h
│   │   ├── result.h
│   │   ├── interfaces.h
│   │   └── module_id.h
│   └── src/
├── services/
│   ├── include/services/
│   │   ├── iconfig.h
│   │   ├── ievent_bus.h
│   │   ├── icommand_bus.h
│   │   ├── ischeduler.h
│   │   ├── istorage.h
│   │   ├── ilogger.h
│   │   └── idiagnostics.h
│   └── src/
├── runtime/
│   ├── include/runtime/
│   │   ├── application.h
│   │   ├── module_manager.h
│   │   └── imodule.h
│   └── src/
├── modules/
│   └── example_module/
├── plugins/
│   └── plugin_sdk/
├── ui/
│   └── qt6/
│       ├── viewmodels/
│       └── adapters/
├── tests/
└── examples/

10. Test Strategy (Chiến lược kiểm thử)
Unit Test: Kiểm thử độc lập từng tầng core, services, runtime sử dụng GoogleTest hoặc Catch2. Core và Services có thể test 100% không cần GUI hay OS giả lập phức tạp.

Integration Test: Kiểm thử kịch bản nạp/gỡ module, kiểm tra phát hiện dependency vòng tròn (circular dependency) của ModuleManager.

11. Các rủi ro kiến trúc & Giải pháp (Architectural Risks)
Rủi ro Circular Dependency giữa các Module:

Giải pháp: ModuleManager bắt buộc phải chạy thuật toán Topological Sort ở giai đoạn LOADED -> INITIALIZED. Nếu phát hiện chu trình phụ thuộc, Runtime sẽ từ chối khởi động và log chi tiết đường dẫn gây ra chu trình.

Rủi ro ABI Break khi đổi Plugin:

Giải pháp: Giữ Plugin API tối giản, sử dụng Pure Virtual Interfaces (class IPlugin) kết hợp với hàm create_plugin() / destroy_plugin() xuất ra bằng extern "C" với noexcept.