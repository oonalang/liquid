#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include "Triggerbot1657ExportNames.h"
#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5u
#endif

namespace Triggerbot1657 { namespace ManagedRuntime {



struct Api {
    void* (*domainGet)()=nullptr;
    const void** (*domainAssemblies)(const void*,std::size_t*)=nullptr;
    const void* (*assemblyImage)(const void*)=nullptr;
    const char* (*imageName)(const void*)=nullptr;
    void* (*classFromName)(const void*,const char*,const char*)=nullptr;
    const void* (*classMethods)(void*,void**)=nullptr;
    const char* (*methodName)(const void*)=nullptr;
    std::uint32_t (*methodParamCount)(const void*)=nullptr;
    const char* (*methodParamName)(const void*,std::uint32_t)=nullptr;
    std::uint32_t (*methodFlags)(const void*,std::uint32_t*)=nullptr;
    const void* (*virtualMethod)(void*,const void*)=nullptr;
    void* (*invoke)(const void*,void*,void**,void**)=nullptr;
    void* (*unbox)(void*)=nullptr;

    std::uint32_t MissingMask() const {
        const bool present[]={domainGet!=nullptr,domainAssemblies!=nullptr,assemblyImage!=nullptr,imageName!=nullptr,
            classFromName!=nullptr,classMethods!=nullptr,methodName!=nullptr,methodParamCount!=nullptr,methodParamName!=nullptr,
            methodFlags!=nullptr,virtualMethod!=nullptr,invoke!=nullptr,unbox!=nullptr};
        std::uint32_t mask=0;
        for (unsigned i=0;i<kRuntimeExportCount;++i) if (!present[i]) mask|=1u<<i;
        return mask;
    }
    bool Complete() const {
        return domainGet && domainAssemblies && assemblyImage && imageName &&
            classFromName && classMethods && methodName && methodParamCount &&
            methodParamName && methodFlags && virtualMethod && invoke && unbox;
    }
};

enum class Error { None, MissingExports, MetadataUnavailable, MethodUnavailable,
                   VirtualMethodUnavailable, InvocationException, InvalidResult };
struct BoolResult { bool succeeded=false; bool value=false; };

class Adapter {
public:
    explicit Adapter(Api exports={}) : api(exports) {}
    void Bind(Api exports) {
        api=exports;
        linecast=start=stop=nullptr;
        error=Error::None;
    }
    Error LastError() const { return error; }
    bool HasExports() const { return api.Complete(); }

    BoolResult Linecast(void* from,void* to,void* hit) {
        if (!from || !to || !hit) { error=Error::InvalidResult; return {}; }
        if (!EnsureMethods()) return {};
        void* arguments[]={from,to,hit};
        void* exception=nullptr;
        void* result=api.invoke(linecast,nullptr,arguments,&exception);
        if (exception) { error=Error::InvocationException; return {}; }
        if (!result) { error=Error::InvalidResult; return {}; }
        void* value=api.unbox(result);
        if (!value) { error=Error::InvalidResult; return {}; }
        error=Error::None;
        return {true,*static_cast<const bool*>(value)};
    }
    bool StartFire(void* controller) { return InvokeFire(controller,false,false); }
    bool StopFire(void* controller,bool immediate) { return InvokeFire(controller,true,immediate); }

private:
    Api api;
    const void* linecast=nullptr;
    const void* start=nullptr;
    const void* stop=nullptr;
    Error error=Error::None;

    const void* FindImage(const char* name) {
        void* domain=api.domainGet();
        if (!domain) return nullptr;
        std::size_t count=0;
        const void** assemblies=api.domainAssemblies(domain,&count);
        if (!assemblies) return nullptr;
        for (std::size_t i=0;i<count;++i) {
            const void* image=assemblies[i] ? api.assemblyImage(assemblies[i]) : nullptr;
            const char* actual=image ? api.imageName(image) : nullptr;
            if (actual && std::strcmp(actual,name)==0) return image;
        }
        return nullptr;
    }
    const void* FindMethod(void* klass,const char* name,const char* const* parameters,
                           std::uint32_t count,bool isStatic) {
        if (!klass) return nullptr;
        void* iterator=nullptr;
        while (const void* method=api.classMethods(klass,&iterator)) {
            const char* actual=api.methodName(method);
            if (!actual || std::strcmp(actual,name)!=0 || api.methodParamCount(method)!=count) continue;
            std::uint32_t implementationFlags=0;
            if (((api.methodFlags(method,&implementationFlags)&0x10)!=0)!=isStatic) continue;
            bool matches=true;
            for (std::uint32_t i=0;i<count;++i) {
                const char* parameter=api.methodParamName(method,i);
                if (!parameter || std::strcmp(parameter,parameters[i])!=0) { matches=false; break; }
            }
            if (matches) return method;
        }
        return nullptr;
    }
    bool EnsureMethods() {
        if (!api.Complete()) { error=Error::MissingExports; return false; }
        if (linecast && start && stop) return true;
        const void* unity=FindImage("UnityEngine.dll");
        const void* game=FindImage("Assembly-CSharp.dll");
        if (!unity || !game) { error=Error::MetadataUnavailable; return false; }
        void* physics=api.classFromName(unity,"UnityEngine","Physics");
        void* controller=api.classFromName(game,"GameEngine","PlayerController");
        const char* linecastParameters[]={"start","end","hitInfo"};
        const char* stopParameters[]={"isImmidiately"};
        const void* foundLinecast=FindMethod(physics,"Linecast",linecastParameters,3,true);
        const void* foundStart=FindMethod(controller,"StartFire",nullptr,0,false);
        const void* foundStop=FindMethod(controller,"StopFire",stopParameters,1,false);
        if (!foundLinecast || !foundStart || !foundStop) { error=Error::MethodUnavailable; return false; }
        linecast=foundLinecast; start=foundStart; stop=foundStop;
        error=Error::None;
        return true;
    }
    bool InvokeFire(void* controller,bool releasing,bool immediate) {
        if (!controller) { error=Error::InvalidResult; return false; }
        if (!EnsureMethods()) return false;
        const void* method=api.virtualMethod(controller,releasing ? stop : start);
        if (!method) { error=Error::VirtualMethodUnavailable; return false; }
        void* arguments[]={&immediate};
        void* exception=nullptr;
        api.invoke(method,controller,releasing ? arguments : nullptr,&exception);
        if (exception) { error=Error::InvocationException; return false; }
        error=Error::None;
        return true; // A void method normally returns no boxed result.
    }
};

inline bool RuntimeLibraryName(const char* path) {
    if (!path) return false;
    const char* slash=std::strrchr(path,'/');
    const char* name=slash ? slash+1 : path;
    return std::strcmp(name,"libunity.so")==0 || std::strcmp(name,"libil2cpp.so")==0;
}





struct LoadedModule {
    std::string path;
    std::uintptr_t base=0;
    std::uintptr_t low=0;
    std::uintptr_t high=0;
    std::uintptr_t dynamic=0;
};
inline bool Inside(std::uintptr_t address,std::size_t size,const LoadedModule& module) {
    if (address<module.low || address>module.high) return false;
    return size<=module.high-address;
}
inline std::vector<LoadedModule> LoadedRuntimeModules() {
    std::vector<LoadedModule> modules;
    dl_iterate_phdr([](dl_phdr_info* info,std::size_t,void* context) {
        if (!info || !RuntimeLibraryName(info->dlpi_name)) return 0;
        auto& modules=*static_cast<std::vector<LoadedModule>*>(context);
        LoadedModule module;
        module.path=info->dlpi_name;
        module.base=static_cast<std::uintptr_t>(info->dlpi_addr);
        bool mapped=false;
        for (ElfW(Half) i=0;i<info->dlpi_phnum;++i) {
            const ElfW(Phdr)& header=info->dlpi_phdr[i];
            if (header.p_type==PT_DYNAMIC)
                module.dynamic=module.base+header.p_vaddr;
            if (header.p_type!=PT_LOAD) continue;
            const std::uintptr_t start=module.base+header.p_vaddr;
            const std::uintptr_t end=start+header.p_memsz;
            if (!mapped) { module.low=start; module.high=end; mapped=true; }
            else {
                if (start<module.low) module.low=start;
                if (end>module.high) module.high=end;
            }
        }
        if (mapped && module.dynamic) modules.push_back(module);
        return 0;
    },&modules);
    return modules;
}


inline std::uintptr_t Relocated(std::uintptr_t value,const LoadedModule& module) {
    if (value>=module.low && value<module.high) return value;
    return module.base+value;
}

inline std::size_t SymbolCountFromGnuHash(const std::uint32_t* hash,const LoadedModule& module) {
    if (!hash || !Inside(reinterpret_cast<std::uintptr_t>(hash),4*sizeof(std::uint32_t),module))
        return 0;
    const std::size_t bucketsCount=hash[0];
    const std::size_t minimum=hash[1];
    const std::size_t bloomSize=hash[2];
    if (!bucketsCount || bucketsCount>(1u<<24)) return 0;
    const std::uintptr_t buckets=reinterpret_cast<std::uintptr_t>(hash)+4*sizeof(std::uint32_t)+
        bloomSize*sizeof(ElfW(Addr));
    if (!Inside(buckets,bucketsCount*sizeof(std::uint32_t),module)) return 0;
    const auto* bucket=reinterpret_cast<const std::uint32_t*>(buckets);

    const std::uintptr_t chains=buckets+bucketsCount*sizeof(std::uint32_t)-
        minimum*sizeof(std::uint32_t);
    std::size_t count=minimum;
    for (std::size_t i=0;i<bucketsCount;++i) {
        std::size_t index=bucket[i];
        if (index<minimum) continue;
        for (;;) {
            const std::uintptr_t entry=chains+index*sizeof(std::uint32_t);
            if (!Inside(entry,sizeof(std::uint32_t),module)) break;
            const std::uint32_t value=*reinterpret_cast<const std::uint32_t*>(entry);
            ++index;
            if (value&1u) break;
        }
        if (index>count) count=index;
    }
    return count;
}
inline Api ResolveExports(const LoadedModule& module) {
    Api api{};
    if (!module.dynamic) return api;
    const ElfW(Sym)* table=nullptr;
    const char* strings=nullptr;
    std::size_t stringsSize=0,symbolCount=0;
    std::size_t entrySize=sizeof(ElfW(Sym));
    const std::uint32_t* hash=nullptr;
    const std::uint32_t* gnuHash=nullptr;
    const auto* dynamic=reinterpret_cast<const ElfW(Dyn)*>(module.dynamic);
    for (;;++dynamic) {
        if (!Inside(reinterpret_cast<std::uintptr_t>(dynamic),sizeof(ElfW(Dyn)),module)) break;
        if (dynamic->d_tag==DT_NULL) break;
        switch (dynamic->d_tag) {
            case DT_SYMTAB: table=reinterpret_cast<const ElfW(Sym)*>(Relocated(dynamic->d_un.d_ptr,module)); break;
            case DT_STRTAB: strings=reinterpret_cast<const char*>(Relocated(dynamic->d_un.d_ptr,module)); break;
            case DT_STRSZ: stringsSize=dynamic->d_un.d_val; break;
            case DT_SYMENT: entrySize=dynamic->d_un.d_val; break;
            case DT_HASH: hash=reinterpret_cast<const std::uint32_t*>(Relocated(dynamic->d_un.d_ptr,module)); break;
            case DT_GNU_HASH: gnuHash=reinterpret_cast<const std::uint32_t*>(Relocated(dynamic->d_un.d_ptr,module)); break;
            default: break;
        }
    }
    if (!table || !strings || !stringsSize || entrySize!=sizeof(ElfW(Sym))) return api;
    if (!Inside(reinterpret_cast<std::uintptr_t>(strings),stringsSize,module)) return api;
    if (hash) {
        if (!Inside(reinterpret_cast<std::uintptr_t>(hash),2*sizeof(std::uint32_t),module)) return api;
        symbolCount=hash[1];
    } else if (gnuHash) {
        symbolCount=SymbolCountFromGnuHash(gnuHash,module);
    }
    if (!symbolCount || symbolCount>(1u<<24) ||
        !Inside(reinterpret_cast<std::uintptr_t>(table),symbolCount*sizeof(ElfW(Sym)),module)) return api;
    auto find=[&](const char* name)->void* {
        if (!name) return nullptr;
        const std::size_t length=std::strlen(name);
        for (std::size_t i=1;i<symbolCount;++i) {
            const ElfW(Sym)& symbol=table[i];
            if (symbol.st_shndx==SHN_UNDEF || !symbol.st_name || !symbol.st_value) continue;
            if (symbol.st_name>=stringsSize || length>=stringsSize-symbol.st_name) continue;
            const char* actual=strings+symbol.st_name;
            if (actual[length]!='\0' || std::memcmp(actual,name,length)!=0) continue;
            return reinterpret_cast<void*>(module.base+symbol.st_value);
        }
        return nullptr;
    };
#define TRIGGERBOT_EXPORT(member,index) \
    api.member=reinterpret_cast<decltype(api.member)>(find(kRuntimeExportNames[index]))
    TRIGGERBOT_EXPORT(domainGet,0);
    TRIGGERBOT_EXPORT(domainAssemblies,1);
    TRIGGERBOT_EXPORT(assemblyImage,2);
    TRIGGERBOT_EXPORT(imageName,3);
    TRIGGERBOT_EXPORT(classFromName,4);
    TRIGGERBOT_EXPORT(classMethods,5);
    TRIGGERBOT_EXPORT(methodName,6);
    TRIGGERBOT_EXPORT(methodParamCount,7);
    TRIGGERBOT_EXPORT(methodParamName,8);
    TRIGGERBOT_EXPORT(methodFlags,9);
    TRIGGERBOT_EXPORT(virtualMethod,10);
    TRIGGERBOT_EXPORT(invoke,11);
    TRIGGERBOT_EXPORT(unbox,12);
#undef TRIGGERBOT_EXPORT
    return api;
}
inline std::atomic<std::uint32_t> missingExportMask{(1u<<kRuntimeExportCount)-1};
inline std::atomic<bool> runtimeLibraryOpened{false};
inline Api LoadExports() {
    static Api ready{};
    static void* retained=nullptr;
    if (ready.Complete()) return ready;
    Api best{};
    unsigned bestCount=0;
    bool found=false;
    const auto modules=LoadedRuntimeModules();
    for (const auto& module:modules) {
        Api resolved=ResolveExports(module);
        unsigned count=0;
        for (unsigned i=0;i<kRuntimeExportCount;++i)
            if (!(resolved.MissingMask()&(1u<<i))) ++count;
        found=true;
        if (count>bestCount) { best=resolved; bestCount=count; }
        if (resolved.Complete()) {
#if defined(RTLD_NOLOAD)


            if (!retained) retained=dlopen(module.path.c_str(),RTLD_NOW|RTLD_NOLOAD);
#endif
            ready=resolved;
            runtimeLibraryOpened.store(true,std::memory_order_relaxed);
            missingExportMask.store(0,std::memory_order_relaxed);
            return ready;
        }
    }
    runtimeLibraryOpened.store(found,std::memory_order_relaxed);
    missingExportMask.store(best.MissingMask(),std::memory_order_relaxed);
    return best;
}
inline Adapter adapter;
inline void RetryExports() {
    if (adapter.HasExports()) return;
    using Clock=std::chrono::steady_clock;
    static Clock::time_point next{};
    const auto now=Clock::now();
    if (now<next) return;
    next=now+std::chrono::seconds(1);
    adapter.Bind(LoadExports());
}

}} // namespace Triggerbot1657::ManagedRuntime
