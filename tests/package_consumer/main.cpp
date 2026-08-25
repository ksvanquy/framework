#include "runtime/runtime.h"

int main() {
    framework::runtime::Runtime runtime;
    return runtime.logger().log(
        framework::services::LogLevel::Debug, "PackageConsumer", "Framework package loaded"), 0;
}