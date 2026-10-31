#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include "Features/Triggerbot1657Runtime.h"

using namespace Triggerbot1657::ManagedRuntime;
struct Method {
    const char* name;
    std::vector<const char*> parameters;
    bool isStatic;
};
struct Class { std::vector<Method*> methods; };
struct Image { const char* name; Class* klass; };
struct Vector { float x,y,z; };
struct Hit { void* collider=nullptr; };
struct BoxedBool { bool value=false; };

static Method wrongStatic{"Linecast",{"start","end","hitInfo"},false};
static Method wrongOverload{"Linecast",{"start","end","layerMask"},true};
static Method wrongOrder{"Linecast",{"end","start","hitInfo"},true};
static Method wrongCount{"Linecast",{"start","end"},true};
static Method linecastMethod{"Linecast",{"start","end","hitInfo"},true};
static Method startMethod{"StartFire",{},false};
static Method stopMethod{"StopFire",{"isImmidiately"},false};
static Method derivedStart{"DerivedStartFire",{},false};
static Method derivedStop{"DerivedStopFire",{"isImmidiately"},false};
static Class physics{{&wrongStatic,&wrongOverload,&wrongOrder,&wrongCount,&linecastMethod}};
static Class controllerClass{{&startMethod,&stopMethod}};
static Image unityImage{"UnityEngine.dll",&physics};
static Image gameImage{"Assembly-CSharp.dll",&controllerClass};
static const void* assemblies[]={&unityImage,&gameImage};
static int domainToken=0,controllerToken=0,colliderToken=0,exceptionToken=0;
static bool domainReady=true,raiseException=false,nullResult=false,nullUnbox=false,virtualReady=true;
static bool collision=false,expectedImmediate=false;
static int invokeCalls=0,startCalls=0,stopCalls=0;
static BoxedBool boxed;
static Vector* expectedFrom=nullptr;
static Vector* expectedTo=nullptr;
static Hit* expectedHit=nullptr;

static void* DomainGet() { return domainReady ? &domainToken : nullptr; }
static const void** DomainAssemblies(const void* domain,std::size_t* count) {
    assert(domain==&domainToken); *count=2; return assemblies;
}
static const void* AssemblyImage(const void* assembly) { return assembly; }
static const char* ImageName(const void* image) { return static_cast<const Image*>(image)->name; }
static void* ClassFromName(const void* image,const char* space,const char* name) {
    if (image==&unityImage && std::strcmp(space,"UnityEngine")==0 && std::strcmp(name,"Physics")==0) return &physics;
    if (image==&gameImage && std::strcmp(space,"GameEngine")==0 && std::strcmp(name,"PlayerController")==0) return &controllerClass;
    return nullptr;
}
static const void* ClassMethods(void* klass,void** iterator) {
    auto* value=static_cast<Class*>(klass);
    const auto index=reinterpret_cast<std::uintptr_t>(*iterator);
    if (index>=value->methods.size()) return nullptr;
    *iterator=reinterpret_cast<void*>(index+1);
    return value->methods[index];
}
static const char* MethodName(const void* method) { return static_cast<const Method*>(method)->name; }
static std::uint32_t ParamCount(const void* method) { return static_cast<const Method*>(method)->parameters.size(); }
static const char* ParamName(const void* method,std::uint32_t index) { return static_cast<const Method*>(method)->parameters[index]; }
static std::uint32_t MethodFlags(const void* method,std::uint32_t* implementation) {
    *implementation=0;
    return static_cast<const Method*>(method)->isStatic ? 0x10 : 0;
}
static const void* VirtualMethod(void* object,const void* method) {
    assert(object==&controllerToken);
    if (!virtualReady) return nullptr;
    if (method==&startMethod) return &derivedStart;
    assert(method==&stopMethod); return &derivedStop;
}
static void* Invoke(const void* method,void* object,void** parameters,void** exception) {
    ++invokeCalls;
    if (raiseException) { *exception=&exceptionToken; return nullptr; }
    *exception=nullptr;
    if (method==&linecastMethod) {
        assert(object==nullptr);

        assert(parameters && parameters[0]==expectedFrom && parameters[1]==expectedTo && parameters[2]==expectedHit);
        expectedHit->collider=&colliderToken;
        boxed.value=collision;
        return nullResult ? nullptr : &boxed;
    }
    assert(object==&controllerToken);
    if (method==&derivedStart) { assert(parameters==nullptr); ++startCalls; }
    else {
        assert(method==&derivedStop && parameters);
        assert(*static_cast<bool*>(parameters[0])==expectedImmediate);
        ++stopCalls;
    }
    return nullptr; // Successful void invocation has no boxed result.
}
static void* Unbox(void* object) {
    assert(object==&boxed);
    return nullUnbox ? nullptr : &boxed.value;
}
static Api Exports() {
    return {&DomainGet,&DomainAssemblies,&AssemblyImage,&ImageName,&ClassFromName,
        &ClassMethods,&MethodName,&ParamCount,&ParamName,&MethodFlags,
        &VirtualMethod,&Invoke,&Unbox};
}

int main() {
    Vector from{1,2,3},to{4,5,6};
    Hit hit{};
    expectedFrom=&from; expectedTo=&to; expectedHit=&hit;
    Adapter adapter(Exports());
    auto query=[&](){return adapter.Linecast(&from,&to,&hit);};
    auto result=query();
    assert(result.succeeded && !result.value && hit.collider==&colliderToken);
    collision=true; result=query();
    assert(result.succeeded && result.value);

    raiseException=true; result=query();
    assert(!result.succeeded && adapter.LastError()==Error::InvocationException);
    raiseException=false; nullResult=true; result=query();
    assert(!result.succeeded && adapter.LastError()==Error::InvalidResult);
    nullResult=false; nullUnbox=true; result=query();
    assert(!result.succeeded && adapter.LastError()==Error::InvalidResult);
    nullUnbox=false; assert(query().succeeded && adapter.LastError()==Error::None);

    assert(adapter.StartFire(&controllerToken) && startCalls==1);
    expectedImmediate=true;
    assert(adapter.StopFire(&controllerToken,true) && stopCalls==1);
    virtualReady=false;
    const auto beforeMissingVirtual=invokeCalls;
    assert(!adapter.StartFire(&controllerToken));
    assert(adapter.LastError()==Error::VirtualMethodUnavailable && invokeCalls==beforeMissingVirtual);
    virtualReady=true; raiseException=true;
    assert(!adapter.StartFire(&controllerToken) && adapter.LastError()==Error::InvocationException);
    assert(!adapter.StopFire(&controllerToken,true) && adapter.LastError()==Error::InvocationException);
    raiseException=false;

    Api missing=Exports(); missing.invoke=nullptr; adapter.Bind(missing);
    const auto beforeMissingExport=invokeCalls;
    assert(!query().succeeded && adapter.LastError()==Error::MissingExports);
    assert(invokeCalls==beforeMissingExport);
    adapter.Bind(Exports()); domainReady=false;
    assert(!query().succeeded && adapter.LastError()==Error::MetadataUnavailable);
    domainReady=true; assert(query().succeeded); // Deferred lookup retries after assemblies load.

    adapter.Bind(Exports());
    linecastMethod.parameters[2]="unrelated";
    assert(!query().succeeded && adapter.LastError()==Error::MethodUnavailable);
    linecastMethod.parameters[2]="hitInfo";
    assert(query().succeeded); // Count alone never selects the layer-mask overload.
    assert(!adapter.Linecast(nullptr,&to,&hit).succeeded && adapter.LastError()==Error::InvalidResult);
}
