#include "Renderer.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.h"

// VULKAN
const char* const requiredExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const int MAX_FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

void AvailableExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    VkExtensionProperties extensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
    std::cout << "Available extensions:" << std::endl;
    for (const auto& extension : extensions)
    {
        std::cout << "\t" << extension.extensionName << std::endl;
    }
}

void constructDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

#define GRAPHICS_FAMILY 0b00000001
#define PRESENT_FAMILY  0b00000010
struct QueueFamilyIndices 
{
    uint8_t bitmask = 0;
    uint32_t graphicsFamily;
    uint32_t presentFamily;

    bool isComplete()
    {
        return (bitmask & GRAPHICS_FAMILY) == GRAPHICS_FAMILY &&
               (bitmask & PRESENT_FAMILY) == PRESENT_FAMILY;
    }    
};

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    QueueFamilyIndices indices; // TODO: pass in like variable

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            indices.bitmask |= GRAPHICS_FAMILY;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
            indices.bitmask |= PRESENT_FAMILY;
        }

        if (indices.isComplete()) break;
        i++;
    }

    return indices;
}

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formatCount;
    VkPresentModeKHR *presentModes;
    uint32_t presentModeCount;
};

SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, nullptr);
    if (details.formatCount > 0)
    {
        details.formats = new VkSurfaceFormatKHR[details.formatCount];
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
    } else
    {
        details.formats = nullptr;
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, nullptr);
    if (details.presentModeCount > 0)
    {
        details.presentModes = new VkPresentModeKHR[details.presentModeCount];
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
    } else
    {
        details.presentModes = nullptr;
    }

    return details;
}

bool isDeviceSutiable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);
    for (const auto& required : requiredExtensions)
    {
        bool found = false;
        for (const auto& available : availableExtensions)
        {
            if (strcmp(required, available.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    return indices.isComplete() && swapChainSupport.presentModeCount > 0;

    // TODO: don't care about GPU right now
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

VkSurfaceFormatKHR selectSwapSurfaceFormat(const VkSurfaceFormatKHR* const availableFormats, int availableFormatCount)
{
    for (int i = 0; i < availableFormatCount; ++i)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8_UNORM &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR selectSwapPresentMode(const VkPresentModeKHR* const availablePresentModes, int availablePresentModeCount)
{
    for (int i = 0; i < availablePresentModeCount; ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    VkExtent2D extent = {width, height};
    extent.width = max(capabilities.minImageExtent.width, 
        min(capabilities.maxImageExtent.width, width));
    extent.height = max(capabilities.minImageExtent.height, 
        min(capabilities.maxImageExtent.height, height));
    return extent;
}

VkShaderModule createShaderModule(const VkDevice& device, const char* const shaderPath)
{
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
    assert( file.is_open() );
    size_t size = (size_t)file.tellg();
    // TODO: https://stackoverflow.com/questions/6320264/how-to-align-pointer#comment62262961_6320314
    char* buffer = new char[size];
    // std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer, size);
    // file.read(buffer.data(), size);
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    // TODO: this will cause errors https://stackoverflow.com/questions/22770255/can-i-cast-an-unsigned-char-to-an-unsigned-int
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    // createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    VkShaderModule module;
    assert( vkCreateShaderModule(device, &createInfo, nullptr, &module) == VK_SUCCESS );

    delete[] buffer;
    return module;
}

// RENDERER
void Renderer::run()
{
    init();
    mainLoop();
    cleanup();
}

void Renderer::init()
{
    /// GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_windowWidth = 800;
    m_windowHeight = 600;
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Bending", nullptr, nullptr);
    
    /// VULKAN
    createVulkanInstance();

    // Surface
    assert( glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) == VK_SUCCESS );

    createVulkanDevice();
    createVulkanPipeline();
    createVulkanBuffers();
    
    // Semaphores + Fences
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    m_currentFrame = 0;
    m_imageAcquired = new VkSemaphore[MAX_FRAMES_IN_FLIGHT];
    m_renderCompleted = new VkSemaphore[MAX_FRAMES_IN_FLIGHT];
    m_framesInFlight = new VkFence[MAX_FRAMES_IN_FLIGHT];
    m_imagesInFlight = new VkFence[m_swapChainImageCount];
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        assert( vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_imageAcquired[i])   == VK_SUCCESS );
        assert( vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderCompleted[i]) == VK_SUCCESS );
        assert( vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_framesInFlight[i]) == VK_SUCCESS );
        m_imagesInFlight[i] = VK_NULL_HANDLE;
    }
}

void Renderer::createVulkanInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // TODO: application setup info
    appInfo.pApplicationName = "Bending";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // TODO: get from defines
    appInfo.pEngineName = "Bending";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0; // TODO: not 1_1?

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#if DEBUG && DEBUG_RENDERER
    uint32_t extensionCount = glfwExtensionCount + 1;
    const char* extensions[extensionCount];
    for (int i = 0; i < glfwExtensionCount; ++i)
    {
        extensions[i] = glfwExtensions[i];
    }
    extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
    uint32_t extensionCount = glfwExtensionCount;
    const char** extensions = glfwExtensions;
#endif
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    
#if DEBUG && DEBUG_RENDERER
    const char* const validationLayers[] = {
        "VK_LAYER_KHRONOS_validation" // TODO: not found on macOS w/ MoltenVK
    };
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        assert( layerFound && "Validation layers requested, but not available!" ); // TODO: 
    }
    createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(const char*);
    createInfo.ppEnabledLayerNames = validationLayers;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
#endif
    
#if DEBUG && DEBUG_RENDERER // TODO: better macro
    VkDebugUtilsMessengerCreateInfoEXT debugInstanceCreateInfo;
    constructDebugMessengerCreateInfo(debugInstanceCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInstanceCreateInfo;
#else
    createInfo.pNext = nullptr;
#endif

    assert( vkCreateInstance(&createInfo, nullptr, &m_instance) == VK_SUCCESS ); // TODO: custom allocator
    assert( m_instance );

#if DEBUG && DEBUG_RENDERER // TODO: better macro
    AvailableExtensions();

    VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo;
    constructDebugMessengerCreateInfo(debugMessageCreateInfo);

    assert( CreateDebugUtilsMessengerEXT(m_instance, &debugMessageCreateInfo, nullptr, &m_debugMessenger) == VK_SUCCESS );
#endif
}

void Renderer::createVulkanDevice()
{
    // Physical Device
    m_physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    assert( deviceCount > 0 );
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices);
    for (const auto& device : devices)
    {
        if (isDeviceSutiable(device, m_surface))
        {
            m_physicalDevice = device;
            break;
        }
    }
    assert( m_physicalDevice != VK_NULL_HANDLE );

    // Logical Device
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);
    assert( indices.graphicsFamily == indices.presentFamily ); // TODO: rather than forming a set with multiple queues

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = sizeof(requiredExtensions) / sizeof(char*);
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions;
    // TODO: backwards compatible set enabledLayerCount & ppEnabledLayeredNames

    assert( vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) == VK_SUCCESS );

    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily,  0, &m_presentQueue);

    // Swap Chain
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice, m_surface);
    VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatCount);
    VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModeCount);
    VkExtent2D extent = selectSwapExtent(swapChainSupport.capabilities, m_windowWidth, m_windowHeight);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    delete[] swapChainSupport.formats; // TODO: should this go in a destructor?
    delete[] swapChainSupport.presentModes;

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // if (indices.graphicsFamily != indices.presentFamily) // TODO: asserts if not true
    // {
    //     swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //     swapChainCreateInfo.queueFamilyIndexCount = 2;
    //     const uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    //     swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    // } else

    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    
    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    assert( vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain) == VK_SUCCESS );

    // Swap Chain Images
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, nullptr);
    m_swapChainImages = new VkImage[m_swapChainImageCount];
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, m_swapChainImages);

    // TODO: do they need to be member vars?
    m_swapCahinImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    m_swapChainImageViews = new VkImageView[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = m_swapChainImages[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = surfaceFormat.format;
        
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        assert( vkCreateImageView(m_device, &viewCreateInfo, nullptr, &m_swapChainImageViews[i]) == VK_SUCCESS );
    }
}

void Renderer::createVulkanPipeline()
{
    // Shaders
    VkShaderModule vert = createShaderModule(m_device, "Shaders/vert.spv");
    VkShaderModule frag = createShaderModule(m_device, "Shaders/frag.spv");

    VkPipelineShaderStageCreateInfo vertCreateInfo = {};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertCreateInfo.module = vert;
    vertCreateInfo.pName = "main";
    // vertCreateInfo.pSpecializationInfo // shader constants

    VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
    fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageCreateInfo.module = frag;
    fragStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = 
    {
        vertCreateInfo,
        fragStageCreateInfo
    };

    VkPipelineVertexInputStateCreateInfo vertInputCreateInfo = {};
    vertInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport + Scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = m_swapChainExtent.width;
    viewport.height = m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // TODO: wireframe possibility
    rasterizerCreateInfo.lineWidth = 1.0f; // > 1.0f requires wideLines
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // Depth + Stencil
    // VkPipelineDepthStencilStateCreateInfo

    VkPipelineColorBlendAttachmentState blendAttachment = {};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                     VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT |
                                     VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // blendAttachment.blendEnable = VK_TRUE;
    // blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
    blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendCreateInfo.logicOpEnable = VK_FALSE;
    blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    blendCreateInfo.attachmentCount = 1;
    blendCreateInfo.pAttachments = &blendAttachment;
    blendCreateInfo.blendConstants[0] = 0.0f;
    blendCreateInfo.blendConstants[1] = 0.0f;
    blendCreateInfo.blendConstants[2] = 0.0f;
    blendCreateInfo.blendConstants[3] = 0.0f;

    // VkPipelineDynamicStateCreateInfo // TODO: dynamic state

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    assert( vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS );

    // Render Pass
    VkAttachmentDescription colorAttachmentDesc = {};
    colorAttachmentDesc.format = m_swapCahinImageFormat;
    colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentDesc;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDesc;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;
    assert( vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_renderPass) == VK_SUCCESS );

    // Pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &blendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.renderPass = m_renderPass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // VK_PIPELINE_CREATE_DERIVATIVE_BIT
    pipelineCreateInfo.basePipelineIndex = -1;

    assert( vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) == VK_SUCCESS );

    vkDestroyShaderModule(m_device, vert, nullptr);
    vkDestroyShaderModule(m_device, frag, nullptr);
}

void Renderer::createVulkanBuffers()
{
    // Framebuffer
    m_swapChainFramebuffers = new VkFramebuffer[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        VkImageView attachments[] = { m_swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = m_swapChainExtent.width;
        framebufferCreateInfo.height = m_swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        assert( vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_swapChainFramebuffers[i]) == VK_SUCCESS );
    }

    // Command Pool
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice, m_surface); // TODO: this is the 3rd time this is called on init (at least)
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolCreateInfo.flags = 0;
    assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_commandPool) == VK_SUCCESS );

    // Command Buffer
    m_commandBuffers = new VkCommandBuffer[m_swapChainImageCount];
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = m_swapChainImageCount;
    assert( vkAllocateCommandBuffers(m_device, &allocateInfo, m_commandBuffers) == VK_SUCCESS );

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        assert( vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) == VK_SUCCESS );

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);
        assert( vkEndCommandBuffer(m_commandBuffers[i]) == VK_SUCCESS );
    }
}

void Renderer::mainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        render();
    }
}

void Renderer::render()
{
    vkWaitForFences(m_device, 1, &m_framesInFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAcquired[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[imageIndex] = m_framesInFlight[m_currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAcquired[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_renderCompleted[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_device, 1, &m_framesInFlight[m_currentFrame]);
    assert( vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_framesInFlight[m_currentFrame]) == VK_SUCCESS );

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup()
{
    // Vulcan
    // Synchronization
    vkDeviceWaitIdle(m_device);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(m_device, m_imageAcquired[i], nullptr);
        vkDestroySemaphore(m_device, m_renderCompleted[i], nullptr);
        vkDestroyFence(m_device, m_framesInFlight[i], nullptr);
    }
    delete[] m_imageAcquired;
    delete[] m_renderCompleted;
    delete[] m_framesInFlight;
    delete[] m_imagesInFlight;

    // Buffers
    delete[] m_commandBuffers;
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyFramebuffer(m_device, m_swapChainFramebuffers[i], nullptr);
    }
    delete[] m_swapChainFramebuffers;
    // Pipeline
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
    }
    delete[] m_swapChainImageViews;
    delete[] m_swapChainImages;
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    // Device
    vkDestroyDevice(m_device, nullptr);
    // Instance
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
#if DEBUG && DEBUG_RENDERER
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);

    // GLFW
    glfwDestroyWindow(m_window);
    glfwTerminate();
}
