#include <cassert>
#include <chrono>
#include <thread>
#include "Features/Triggerbot1657Runtime.h"

using namespace Triggerbot1657;
using namespace Triggerbot1657::ManagedRuntime;
int main(int argc,char** argv) {
    assert(argc==3); // Partial libunity.so and complete libil2cpp.so fixtures.
    assert(RuntimeLibraryName("/a/b/libunity.so"));
    assert(RuntimeLibraryName("/a/b/libil2cpp.so"));
    assert(!RuntimeLibraryName("/a/b/libunity.so.backup"));
    RetryExports();
    assert(!adapter.HasExports());
    assert(!runtimeLibraryOpened.load());
    void* partial=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    assert(partial);
    auto api=LoadExports();
    assert(!api.Complete());
    assert(api.MissingMask()==(1u<<11));
    assert(missingExportMask.load()==(1u<<11));
    assert(runtimeLibraryOpened.load());
    assert(std::strcmp(kRuntimeExportNames[11],"il2cpp_runtime_invoke")==0);
    void* full=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);
    assert(full);

    std::this_thread::sleep_for(std::chrono::milliseconds(1050));
    RetryExports();
    assert(adapter.HasExports());
    assert(missingExportMask.load()==0);
    api=LoadExports();

    assert(reinterpret_cast<void*>(api.domainGet)==dlsym(full,"il2cpp_domain_get"));
    assert(reinterpret_cast<void*>(api.invoke)==dlsym(full,"il2cpp_runtime_invoke"));
    dlclose(partial);
    dlclose(full);
    assert(LoadExports().Complete()); // Selected module handle remains retained.
}
