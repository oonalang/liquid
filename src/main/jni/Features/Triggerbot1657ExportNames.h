#pragma once
namespace Triggerbot1657 {
inline constexpr const char* kRuntimeExportNames[]={
    "il2cpp_domain_get",
    "il2cpp_domain_get_assemblies",
    "il2cpp_assembly_get_image",
    "il2cpp_image_get_name",
    "il2cpp_class_from_name",
    "il2cpp_class_get_methods",
    "il2cpp_method_get_name",
    "il2cpp_method_get_param_count",
    "il2cpp_method_get_param_name",
    "il2cpp_method_get_flags",
    "il2cpp_object_get_virtual_method",
    "il2cpp_runtime_invoke",
    "il2cpp_object_unbox",
};
inline constexpr unsigned kRuntimeExportCount=sizeof(kRuntimeExportNames)/sizeof(*kRuntimeExportNames);
}
