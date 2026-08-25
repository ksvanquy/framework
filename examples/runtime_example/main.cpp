#include "example_module/example_module.h"
#include "runtime/application.h"
#include "runtime/module_manager.h"
#include "runtime/runtime.h"

#include <memory>

class RuntimeExample final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime& runtime) override {
        return runtime.moduleManager().registerModule(
            std::make_unique<framework::modules::ExampleModule>(runtime.logger(), runtime.eventBus()));
    }
};

int main(int argc, char* argv[]) {
    RuntimeExample application;
    return application.exec(argc, argv);
}