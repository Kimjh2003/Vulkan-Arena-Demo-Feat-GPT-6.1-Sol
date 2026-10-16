#include "vulkan_context.h"
#include "vulkan/include/vulkan_common.hpp"
#include <android/log.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace graphics::vk {
void check(VkResult r,const char* op){if(r!=VK_SUCCESS)throw vk3d::VulkanError(r,op);}
static VKAPI_ATTR VkBool32 VKAPI_CALL message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* data,void*) {
    __android_log_print(severity>=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT?ANDROID_LOG_ERROR:ANDROID_LOG_WARN,
                        "ArenaVulkan","Validation: %s",data->pMessage);
    return VK_FALSE;
}
Context::Context(ANativeWindow* window){try{
    std::vector<const char*> extensions={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    std::vector<const char*> layers;
    bool debugUtils=false,syncValidation=false;
#ifndef NDEBUG
    {uint32_t count=0;check(vkEnumerateInstanceLayerProperties(&count,nullptr),"instance layers");
    std::vector<VkLayerProperties> available(count);check(vkEnumerateInstanceLayerProperties(&count,available.data()),"instance layers");
    for(const auto& layer:available)if(!std::strcmp(layer.layerName,"VK_LAYER_KHRONOS_validation"))layers.push_back("VK_LAYER_KHRONOS_validation");
    check(vkEnumerateInstanceExtensionProperties(nullptr,&count,nullptr),"instance extensions");
    std::vector<VkExtensionProperties> ext(count);check(vkEnumerateInstanceExtensionProperties(nullptr,&count,ext.data()),"instance extensions");
    for(const auto& x:ext)if(!std::strcmp(x.extensionName,VK_EXT_DEBUG_UTILS_EXTENSION_NAME))debugUtils=true;
    for(auto layer:layers){
        check(vkEnumerateInstanceExtensionProperties(layer,&count,nullptr),"layer extensions");
        ext.resize(count);check(vkEnumerateInstanceExtensionProperties(layer,&count,ext.data()),"layer extensions");
        for(const auto& x:ext){
            if(!std::strcmp(x.extensionName,VK_EXT_DEBUG_UTILS_EXTENSION_NAME))debugUtils=true;
            if(!std::strcmp(x.extensionName,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))syncValidation=true;
        }
    }
    if(debugUtils)extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if(syncValidation)extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }
#endif
    VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.pApplicationName="Vulkan Arena";
    app.applicationVersion=VK_MAKE_VERSION(0,2,0);app.pEngineName="Arena";app.apiVersion=VK_API_VERSION_1_0;
    VkInstanceCreateInfo ci{};ci.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;ci.pApplicationInfo=&app;
    VkValidationFeatureEnableEXT syncFeature=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validationFeatures{};validationFeatures.sType=VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount=1;validationFeatures.pEnabledValidationFeatures=&syncFeature;
    if(syncValidation)ci.pNext=&validationFeatures;
    ci.enabledExtensionCount=extensions.size();ci.ppEnabledExtensionNames=extensions.data();
    ci.enabledLayerCount=layers.size();ci.ppEnabledLayerNames=layers.data();
    check(vkCreateInstance(&ci,nullptr,&instance),"Vulkan instance");
    if(!layers.empty()&&debugUtils){
        auto create=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
        if(create){VkDebugUtilsMessengerCreateInfoEXT info{};info.sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            info.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            info.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            info.pfnUserCallback=message;check(create(instance,&info,nullptr,&debug),"debug messenger");}
    }
    VkAndroidSurfaceCreateInfoKHR sc{};sc.sType=VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;sc.window=window;
    check(vkCreateAndroidSurfaceKHR(instance,&sc,nullptr,&surface),"Android Vulkan surface");
    uint32_t count=0;check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"physical devices");
    std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"physical devices");
    for(auto candidate:devices){
        uint32_t ec=0;check(vkEnumerateDeviceExtensionProperties(candidate,nullptr,&ec,nullptr),"device extensions");
        std::vector<VkExtensionProperties> ex(ec);check(vkEnumerateDeviceExtensionProperties(candidate,nullptr,&ec,ex.data()),"device extensions");
        if(std::none_of(ex.begin(),ex.end(),[](const auto& e){return !std::strcmp(e.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME);}))continue;
        uint32_t qc=0;vkGetPhysicalDeviceQueueFamilyProperties(candidate,&qc,nullptr);std::vector<VkQueueFamilyProperties> qs(qc);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate,&qc,qs.data());
        for(uint32_t i=0;i<qc;++i){VkBool32 present=0;check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,surface,&present),"surface support");
            if(present&&qs[i].queueCount&&(qs[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){
                physical=candidate;family=i;timestampBits=qs[i].timestampValidBits;break;}}
        if(physical)break;
    }
    if(!physical)throw std::runtime_error("No Vulkan graphics/present queue");
    vkGetPhysicalDeviceProperties(physical,&properties);VkPhysicalDeviceFeatures supported{};vkGetPhysicalDeviceFeatures(physical,&supported);
    enabled.textureCompressionASTC_LDR=supported.textureCompressionASTC_LDR;
    enabled.textureCompressionETC2=supported.textureCompressionETC2;enabled.samplerAnisotropy=supported.samplerAnisotropy;
    const char* required[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    float priority=1;VkDeviceQueueCreateInfo qi{};qi.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{};di.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;
    di.pEnabledFeatures=&enabled;di.enabledExtensionCount=1;di.ppEnabledExtensionNames=required;
    check(vkCreateDevice(physical,&di,nullptr,&device),"Vulkan device");vkGetDeviceQueue(device,family,0,&queue);
    __android_log_print(ANDROID_LOG_INFO,"ArenaVulkan","Device: %s Vulkan %u.%u, ASTC=%u ETC2=%u anisotropy=%u validation=%zu sync=%d",
        properties.deviceName,VK_VERSION_MAJOR(properties.apiVersion),VK_VERSION_MINOR(properties.apiVersion),enabled.textureCompressionASTC_LDR,enabled.textureCompressionETC2,enabled.samplerAnisotropy,layers.size(),syncValidation);
}catch(...){release();throw;}}
Context::~Context(){release();}
void Context::release() noexcept {
    if(device){vkDeviceWaitIdle(device);vkDestroyDevice(device,nullptr);device={};}
    if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);
    if(debug){auto destroy=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));if(destroy)destroy(instance,debug,nullptr);}
    if(instance)vkDestroyInstance(instance,nullptr);
}
}
