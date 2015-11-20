#include <mbgl/util/thread_context.hpp>

namespace mbgl {
namespace util {

class MainThreadContextRegistrar {
public:
    MainThreadContextRegistrar() : context("Main", ThreadType::Main, ThreadPriority::Regular) {
        ThreadContext::current = &context;
    }

    ~MainThreadContextRegistrar() {
        ThreadContext::current = nullptr;
    }

private:
    ThreadContext context;
};

ThreadContext::ThreadContext(const std::string& name_, ThreadType type_, ThreadPriority priority_)
    : name(name_),
      type(type_),
      priority(priority_) {
}

__thread ThreadContext *ThreadContext::current;

// Will auto register the main thread context
// at startup. Must be instantiated after the
// ThreadContext::current object.
MainThreadContextRegistrar registrar;

}
}
