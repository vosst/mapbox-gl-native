#ifndef MBGL_UTIL_THREAD_CONTEXT
#define MBGL_UTIL_THREAD_CONTEXT

#include <cstdint>
#include <string>
#include <thread>

namespace mbgl {

class FileSource;

namespace util {

class GLObjectStore;

enum class ThreadPriority : bool {
    Regular,
    Low,
};

enum class ThreadType : uint8_t {
    Main,
    Map = Main,
    Worker,
    Unknown,
};

struct ThreadContext {
public:
    ThreadContext(const std::string& name, ThreadType type, ThreadPriority priority);

    static bool currentlyOn(ThreadType type) {
        return current->type == type;
    }

    static std::string getName() {
        if (current != nullptr) {
            return current->name;
        } else {
            return "Unknown";
        }
    }

    static ThreadPriority getPriority() {
        if (current != nullptr) {
            return current->priority;
        } else {
            return ThreadPriority::Regular;
        }
    }

    static FileSource* getFileSource() {
        if (current != nullptr) {
            return current->fileSource;
        } else {
            return nullptr;
        }
    }

    static void setFileSource(FileSource* fileSource) {
        if (current != nullptr) {
            current->fileSource = fileSource;
        } else {
            throw new std::runtime_error("Current thread has no current ThreadContext.");
        }
    }

    static GLObjectStore* getGLObjectStore() {
        if (current != nullptr) {
            return current->glObjectStore;
        } else {
            return nullptr;
        }
    }

    static void setGLObjectStore(GLObjectStore* glObjectStore) {
        if (current != nullptr) {
            current->glObjectStore = glObjectStore;
        } else {
            throw new std::runtime_error("Current thread has no current ThreadContext.");
        }
    }

private:
    std::string name;
    ThreadType type;
    ThreadPriority priority;

    FileSource* fileSource = nullptr;
    GLObjectStore* glObjectStore = nullptr;

    static __thread ThreadContext *current;

    friend class MainThreadContextRegistrar;
    template <class Object> friend class Thread;
};

}
}

#endif
