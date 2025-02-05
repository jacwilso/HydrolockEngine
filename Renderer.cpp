#include "Renderer.h"

#include <assert.h>
#include <iostream>
#include <chrono>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <Middleware/stb_image.h>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

#include "Engine.h"
#include "Window.h"
#include "Utilities/Defines.h"
#include "Shaders/ShaderStructures.h"
#include "VulkanUtilities.h"
#include "Math/vec4.h"

#define BASE_SUBPASS 0

const Vertex vertices[] =
{
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    // {{-0.25f, -0.25f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // {{ 0.75f, -0.25f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    // {{ 0.75f,  0.75f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    // {{-0.25f,  0.75f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
};

const uint16_t indices[] = { 
    0, 1, 2, 2, 3, 0,
    // 4, 5, 6, 6, 7, 4,
};

bool framebufferResized = false; // TODO: remove

// RENDERER
void Renderer::init()
{
    /// VULKAN
    createVulkanInstance();

    // Surface
    assert( glfwCreateWindowSurface(m_instance, Engine::m_window.Get(), nullptr, &m_surface) == VK_SUCCESS );

    createVulkanDevice();
    createVulkanSwapChain();
    createVulkanPipeline();
    // Command Pool // TODO: move back into buffers with fullscreen
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = m_graphicsFamily; // TODO: should this be m_graphicsQueue
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_commandPool) == VK_SUCCESS );
#if TRANSFER_FAMILY
    poolCreateInfo.queueFamilyIndex = queueFamily.transferFamily; // TODO: should this be m_transferQueue
    assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_transferCommandPool) == VK_SUCCESS ); // TODO: currently doesn't work
#endif
    createVulkanBuffers();
    createVulkanDrawCmds();

    createImguiContext();
    
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
    }
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
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
        if (isDeviceSuitable(device, m_surface))
        {
            m_physicalDevice = device;
            break;
        }
    }
    assert( m_physicalDevice != VK_NULL_HANDLE );

    // Logical Device
    QueueFamilyIndices queueFamily = findQueueFamilies(m_physicalDevice, m_surface);
    m_graphicsFamily = queueFamily.graphicsFamily;
    assert( queueFamily.graphicsFamily == queueFamily.presentFamily ); // TODO: rather than forming a set with multiple queues

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily.graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
#if EDITOR
    deviceFeatures.fillModeNonSolid = VK_TRUE;
#endif
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = sizeof(requiredExtensions) / sizeof(char*);
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions;
    // TODO: backwards compatible set enabledLayerCount & ppEnabledLayeredNames

    assert( vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) == VK_SUCCESS );

    vkGetDeviceQueue(m_device, queueFamily.graphicsFamily,  0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, queueFamily.presentFamily,   0, &m_presentQueue);
#if TRANSFER_FAMILY
    vkGetDeviceQueue(m_device, queueFamily.transferFamily,  0, &m_transferQueue);
#endif
}

void Renderer::createVulkanSwapChain()
{
    // Swap Chain
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice, m_surface);
    VkSurfaceFormatKHR surfaceFormat = selectSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatCount);
    VkPresentModeKHR presentMode = selectSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModeCount);
    int frameWidth, frameHeight;
    glfwGetFramebufferSize(Engine::m_window.Get(), &frameWidth, &frameHeight); // TODO: should I be updating the window size here too?
    VkExtent2D extent = selectSwapExtent(swapChainSupport.capabilities, frameWidth, frameHeight);

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

    // if (queueFamily.graphicsFamily != queueFamily.presentFamily) // TODO: asserts if not true
    // {
    //     swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //     swapChainCreateInfo.queueFamilyIndexCount = 2;
    //     const uint32_t queueFamilyIndices[] = {queueFamily.graphicsFamily, indices.presentFamily};
    //     swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    // } else

#if TRANSFER_FAMILY
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
#else
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
#endif
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    
    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // TODO: can pass old swap here
    assert( vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain) == VK_SUCCESS );

    // Swap Chain Images
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, nullptr);
    m_swapChainImages = new VkImage[m_swapChainImageCount];
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, m_swapChainImages);

    // TODO: do they need to be member vars?
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    m_swapChainImageViews = new VkImageView[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        createImageView(m_device, m_swapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, m_swapChainImageViews[i]);
    }
}

void Renderer::createVulkanPipeline()
{
    // Shaders
    VkShaderModule vert = createShaderModule(m_device, "Shaders/Pipelines/Default/Default.vert.spv");
    VkShaderModule frag = createShaderModule(m_device, "Shaders/Pipelines/Default/Default.frag.spv");

    VkPipelineShaderStageCreateInfo vertCreateInfo = {};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertCreateInfo.module = vert;
    vertCreateInfo.pName = "main";

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

    VkVertexInputBindingDescription bindingDesc = Vertex::bindingDesc();
    VkVertexInputAttributeDescription vertexInputDesc[] = { 
        Vertex::positionAttribute(),
        Vertex::colorAttribute(),
        Vertex::uvAttribute(),
    };

    VkPipelineVertexInputStateCreateInfo vertInputCreateInfo = {};
    vertInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertInputCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertInputCreateInfo.vertexAttributeDescriptionCount = sizeof(vertexInputDesc) / sizeof(VkVertexInputAttributeDescription);
    vertInputCreateInfo.pVertexAttributeDescriptions = vertexInputDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    {
        // Uniform Buffer
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        // Sampler
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding imageLayoutBinding = {};
        imageLayoutBinding.binding = 2;
        imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imageLayoutBinding.descriptorCount = 64;
        imageLayoutBinding.pImmutableSamplers = nullptr;
        imageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding, imageLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
        layoutCreateInfo.pBindings = bindings;
        assert( vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorLayout) == VK_SUCCESS );
    }

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
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f; // > 1.0f requires wideLines
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = -1.0f;
    depthStencilCreateInfo.maxDepthBounds = 1.0f;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE; // TODO: Stencil

    VkPipelineColorBlendAttachmentState blendAttachment = {};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                     VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT |
                                     VK_COLOR_COMPONENT_A_BIT;
    // blendAttachment.blendEnable = VK_FALSE;
    // blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    // blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    // blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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

    VkPushConstantRange vsPushConstantRange = {};
    vsPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vsPushConstantRange.offset = 0;
    vsPushConstantRange.size = sizeof(VsPushConstants);

    VkPushConstantRange fsPushConstantRange = {};
    fsPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsPushConstantRange.offset = sizeof(VsPushConstants);
    fsPushConstantRange.size = sizeof(FsPushConstants);
    VkPushConstantRange pushConstantRange[] = { vsPushConstantRange, fsPushConstantRange };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = sizeof(pushConstantRange) / sizeof(VkPushConstantRange);
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRange;
    assert( vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS );

    // Render Pass
    VkAttachmentDescription colorAttachmentDesc = {};
    colorAttachmentDesc.format = m_swapChainImageFormat;
    colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachmentDesc = {};
    depthAttachmentDesc.format = findDepthFormat(m_physicalDevice); // TODO: why call this twice
    depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference swapAttachmentRef = {};
    swapAttachmentRef.attachment = 0;
    swapAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 1;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference inputAttachmentRef = {};
    inputAttachmentRef.attachment = 1;
    inputAttachmentRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference transitionAttachmentRef = {};
    transitionAttachmentRef.attachment = 2;
    transitionAttachmentRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inputAttachmentRefs[] = { inputAttachmentRef, transitionAttachmentRef };
    
    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDescription swapSubpassDesc = {};
    swapSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    swapSubpassDesc.colorAttachmentCount = 1;
    swapSubpassDesc.pColorAttachments = &swapAttachmentRef;
    swapSubpassDesc.inputAttachmentCount = sizeof(inputAttachmentRefs) / sizeof(VkAttachmentReference);
    swapSubpassDesc.pInputAttachments = inputAttachmentRefs;

    VkSubpassDescription subpassDescs[] = { subpassDesc, swapSubpassDesc };

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = BASE_SUBPASS;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency transitionSubpassDependency = {};
    transitionSubpassDependency.srcSubpass = BASE_SUBPASS;
    transitionSubpassDependency.dstSubpass = BASE_SUBPASS + 1;
    transitionSubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transitionSubpassDependency.srcAccessMask = 0;
    transitionSubpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    transitionSubpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkSubpassDependency readSubpassDependency = {};
    readSubpassDependency.srcSubpass = BASE_SUBPASS + 1;
    readSubpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    readSubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    readSubpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    readSubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    readSubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency subpassDependencies[] = { subpassDependency, transitionSubpassDependency, readSubpassDependency };

    VkAttachmentDescription attachments[] = { colorAttachmentDesc, colorAttachmentDesc, depthAttachmentDesc };
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = sizeof(attachments) / sizeof(VkAttachmentDescription);
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = sizeof(subpassDescs) / sizeof(VkSubpassDescription);
    renderPassCreateInfo.pSubpasses = subpassDescs;
    renderPassCreateInfo.dependencyCount = sizeof(subpassDependencies) / sizeof(VkSubpassDependency);
    renderPassCreateInfo.pDependencies = subpassDependencies;
    assert( vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_renderPass.scene) == VK_SUCCESS );

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
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &blendCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.renderPass = m_renderPass.scene;
    pipelineCreateInfo.subpass = BASE_SUBPASS;
    pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
#if EDITOR
    // TODO: https://github.com/LunarG/VulkanSamples/blob/master/API-Samples/pipeline_cache/pipeline_cache.cpp

    // VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    // pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    // pipelineCacheCreateInfo.pNext = nullptr;

    // assert( vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache) == VK_SUCCESS );
    // TODO: store pipeline cache as member variable, write to file w/ vkGetPipelineCacheData
#endif

    // General
    assert( vkCreateGraphicsPipelines(m_device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipeline.scene) == VK_SUCCESS );

    vkDestroyShaderModule(m_device, vert, nullptr);
    vkDestroyShaderModule(m_device, frag, nullptr);

    pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    pipelineCreateInfo.basePipelineHandle = m_pipeline.scene;
    pipelineCreateInfo.basePipelineIndex = -1;

    // Composition
    {
        VkGraphicsPipelineCreateInfo compositionPipelineCreateInfo = pipelineCreateInfo;
        compositionPipelineCreateInfo.subpass = BASE_SUBPASS + 1;
        compositionPipelineCreateInfo.pDepthStencilState = nullptr;

        vert = createShaderModule(m_device, "Shaders/Basics/Fullscreen.vert.spv");
        frag = createShaderModule(m_device, "Shaders/Pipelines/Background/Gradient.frag.spv");
        // frag = createShaderModule(m_device, "Shaders/Pipelines/Background/Solid.frag.spv");
        shaderStages[0].module = vert;
        shaderStages[1].module = frag;
        // TODO: enable different types of backgrounds (changable at runtime?)

        VkSpecializationMapEntry colorCountMapEntry = {};
        colorCountMapEntry.constantID = 0;
        colorCountMapEntry.size = sizeof(int);
        colorCountMapEntry.offset = 0;
        int colorCount = m_colorCount;

        VkSpecializationInfo specializationInfo = {};
        specializationInfo.mapEntryCount = 1;
        specializationInfo.pMapEntries = &colorCountMapEntry;
        specializationInfo.dataSize = sizeof(int);
        specializationInfo.pData = &colorCount;

        VkPipelineShaderStageCreateInfo d_fragStageCreateInfo = fragStageCreateInfo;
        d_fragStageCreateInfo.pSpecializationInfo = &specializationInfo;
        d_fragStageCreateInfo.module = frag;
        VkPipelineShaderStageCreateInfo d_shaderStages[] = {
            shaderStages[0],
            d_fragStageCreateInfo
        };
        compositionPipelineCreateInfo.pStages = d_shaderStages;

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo d_vertInputCreateInfo = {};
        d_vertInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        compositionPipelineCreateInfo.pVertexInputState = &d_vertInputCreateInfo;

        {
            VkDescriptorSetLayoutBinding colorLayoutBinding = {};
            colorLayoutBinding.binding = 0;
            colorLayoutBinding.descriptorCount = 1;
            colorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            colorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding uboLayoutBinding = {};
            uboLayoutBinding.binding = 1;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            uboLayoutBinding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding bindings[] = { colorLayoutBinding, uboLayoutBinding };
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
            layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
            layoutCreateInfo.pBindings = bindings;
            assert( vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_compositionDescriptorLayout) == VK_SUCCESS );
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &m_compositionDescriptorLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        assert( vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_compositionPipelineLayout) == VK_SUCCESS );
        compositionPipelineCreateInfo.layout = m_compositionPipelineLayout;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo d_rasterizerCreateInfo = rasterizerCreateInfo;
        d_rasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
        d_rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        compositionPipelineCreateInfo.pRasterizationState = &d_rasterizerCreateInfo;

        assert( vkCreateGraphicsPipelines(m_device, nullptr, 1, &compositionPipelineCreateInfo, nullptr, &m_pipeline.composition) == VK_SUCCESS );

        vkDestroyShaderModule(m_device, vert, nullptr);
        vkDestroyShaderModule(m_device, frag, nullptr);
    }

#if EDITOR
    // Wireframe
    {
        VkGraphicsPipelineCreateInfo wireframeCreateInfo = pipelineCreateInfo;

        // Shaders
        vert = createShaderModule(m_device, "Shaders/Pipelines/Wireframe/Wireframe.vert.spv");
        frag = createShaderModule(m_device, "Shaders/Pipelines/Wireframe/Wireframe.frag.spv");
        shaderStages[0].module = vert;
        shaderStages[1].module = frag;

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo d_vertInputCreateInfo = vertInputCreateInfo;
        VkVertexInputAttributeDescription positionAttribute = Vertex::positionAttribute();
        d_vertInputCreateInfo.vertexAttributeDescriptionCount = 1;
        d_vertInputCreateInfo.pVertexAttributeDescriptions = &positionAttribute;
        wireframeCreateInfo.pVertexInputState = &d_vertInputCreateInfo;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo d_rasterizerCreateInfo = rasterizerCreateInfo;
        d_rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
        wireframeCreateInfo.pRasterizationState = &d_rasterizerCreateInfo;

        assert( vkCreateGraphicsPipelines(m_device, nullptr, 1, &wireframeCreateInfo, nullptr, &m_pipeline.wireframe) == VK_SUCCESS );

        vkDestroyShaderModule(m_device, vert, nullptr);
        vkDestroyShaderModule(m_device, frag, nullptr);
    }
#endif
}

void Renderer::createVulkanBuffers()
{
    m_attachments.create(m_device, m_physicalDevice, m_swapChainExtent, m_swapChainImageFormat);

    // Framebuffer
    m_sceneFramebuffers = new VkFramebuffer[m_swapChainImageCount];
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = m_renderPass.scene;
    framebufferCreateInfo.width = m_swapChainExtent.width;
    framebufferCreateInfo.height = m_swapChainExtent.height;
    framebufferCreateInfo.layers = 1;
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        VkImageView attachments[] = { 
            m_swapChainImageViews[i],
            m_attachments.colorView,
            m_attachments.depthView 
        };

        framebufferCreateInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
        framebufferCreateInfo.pAttachments = attachments;
        assert( vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_sceneFramebuffers[i]) == VK_SUCCESS );
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    void* data;
    const int BufferMemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Texture Image
    m_texImage = new VkImage[64];
    m_texImageMemory = new VkDeviceMemory[64];
    m_texImageView = new VkImageView[64];
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* texPixels = stbi_load("Resources/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize texSize = texWidth * texHeight * 4;
        assert( texPixels );
        createBuffer(m_device, m_physicalDevice,
                    texSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    BufferMemoryProperty,
                    stagingBuffer, stagingBufferMemory);
        vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
        memcpy(data, texPixels, texSize);
        vkUnmapMemory(m_device, stagingBufferMemory);
        stbi_image_free(texPixels);

        createImage(m_device, m_physicalDevice, texWidth, texHeight, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
            m_texImage[0], m_texImageMemory[0]);

    #if TRANSFER_FAMILY
        copyImage(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_texImage[0], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #else
        copyImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_texImage[0], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #endif
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);

        createImageView(m_device, m_texImage[0], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_texImageView[0]);
    }
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* texPixels = stbi_load("Resources/sprite.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize texSize = texWidth * texHeight * 4;
        assert( texPixels );
        createBuffer(m_device, m_physicalDevice,
                    texSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    BufferMemoryProperty,
                    stagingBuffer, stagingBufferMemory);
        vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
        memcpy(data, texPixels, texSize);
        vkUnmapMemory(m_device, stagingBufferMemory);
        stbi_image_free(texPixels);

        createImage(m_device, m_physicalDevice, texWidth, texHeight, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
            m_texImage[1], m_texImageMemory[1]);

    #if TRANSFER_FAMILY
        copyImage(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_texImage[0], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #else
        copyImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_texImage[1], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #endif
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);

        createImageView(m_device, m_texImage[1], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_texImageView[1]);
    }
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* texPixels = stbi_load("Resources/texture2.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize texSize = texWidth * texHeight * 4;
        assert( texPixels );
        createBuffer(m_device, m_physicalDevice,
                    texSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    BufferMemoryProperty,
                    stagingBuffer, stagingBufferMemory);
        vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
        memcpy(data, texPixels, texSize);
        vkUnmapMemory(m_device, stagingBufferMemory);
        stbi_image_free(texPixels);

        createImage(m_device, m_physicalDevice, texWidth, texHeight, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
            m_texImage[2], m_texImageMemory[2]);

    #if TRANSFER_FAMILY
        copyImage(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_texImage[0], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #else
        copyImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_texImage[2], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #endif
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);

        createImageView(m_device, m_texImage[2], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_texImageView[2]);
    }

    // Sampler
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    assert( vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &m_texSampler) == VK_SUCCESS );

    // Vertex + Index Buffer
    size_t bufferSize = sizeof(vertices) + sizeof(indices);

    createBuffer(m_device, m_physicalDevice,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // TODO: VK_MEMORY_PROPERTY_HOST_COHERENT_BIT worse performance. use flushing
        BufferMemoryProperty,
        stagingBuffer, stagingBufferMemory);
    
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    data = static_cast<char*>(data) + sizeof(vertices);
    memcpy(data, indices, sizeof(indices));
    vkUnmapMemory(m_device, stagingBufferMemory);

    createBuffer(m_device, m_physicalDevice,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vertexIndexBuffer, m_vertexIndexBufferMemory);
#if TRANSFER_FAMILY
    copyBuffer(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_vertexIndexBuffer, bufferSize);
#else
    copyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_vertexIndexBuffer, bufferSize);
#endif
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // Uniform Buffer
    m_uniformBuffers = new VkBuffer[m_swapChainImageCount];
    m_uniformBuffersMemory = new VkDeviceMemory[m_swapChainImageCount];
    m_colorBuffers = new VkBuffer[m_swapChainImageCount];
    m_colorBuffersMemory = new VkDeviceMemory[m_swapChainImageCount];
    VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);
    VkDeviceSize colorBufferSize = sizeof(vec4) * m_colorCount;
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        createBuffer(m_device, m_physicalDevice,
            uniformBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            BufferMemoryProperty,
            m_uniformBuffers[i], m_uniformBuffersMemory[i]);
        
        createBuffer(m_device, m_physicalDevice,
            colorBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            BufferMemoryProperty,
            m_colorBuffers[i], m_colorBuffersMemory[i]);
    }

    {
        VkDescriptorPoolSize uboDescPoolSize = {};
        uboDescPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescPoolSize.descriptorCount = m_swapChainImageCount;

        VkDescriptorPoolSize samplerDescPoolSize = {};
        samplerDescPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerDescPoolSize.descriptorCount = m_swapChainImageCount;

        VkDescriptorPoolSize imageDescPoolSize = {};
        imageDescPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imageDescPoolSize.descriptorCount = m_swapChainImageCount * 64;

        VkDescriptorPoolSize descPoolSize[] = { uboDescPoolSize, samplerDescPoolSize, imageDescPoolSize };
        VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
        descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolCreateInfo.poolSizeCount = sizeof(descPoolSize) / sizeof(VkDescriptorPoolSize);
        descPoolCreateInfo.pPoolSizes = descPoolSize;
        descPoolCreateInfo.maxSets = m_swapChainImageCount;
        assert (vkCreateDescriptorPool(m_device, &descPoolCreateInfo, nullptr, &m_descriptorPool) == VK_SUCCESS );
    }

    {
        VkDescriptorPoolSize inputDescPoolSize = {};
        inputDescPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputDescPoolSize.descriptorCount = m_swapChainImageCount;

        VkDescriptorPoolSize uboDescPoolSize = {};
        uboDescPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescPoolSize.descriptorCount = m_swapChainImageCount;

        VkDescriptorPoolSize descPoolSize[] = { inputDescPoolSize, uboDescPoolSize };
        VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
        descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolCreateInfo.poolSizeCount = sizeof(descPoolSize) / sizeof(VkDescriptorPoolSize);
        descPoolCreateInfo.pPoolSizes = descPoolSize;
        descPoolCreateInfo.maxSets = m_swapChainImageCount;
        assert (vkCreateDescriptorPool(m_device, &descPoolCreateInfo, nullptr, &m_compositionDescriptorPool) == VK_SUCCESS );
    }

    VkDescriptorSetLayout layouts[m_swapChainImageCount];
    VkDescriptorSetLayout compositionLayouts[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        layouts[i] = m_descriptorLayout;
        compositionLayouts[i] = m_compositionDescriptorLayout;
    }
    {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {};
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.descriptorPool = m_descriptorPool;
        descSetAllocInfo.descriptorSetCount = m_swapChainImageCount;
        descSetAllocInfo.pSetLayouts = layouts;
        m_descriptorSets = new VkDescriptorSet[m_swapChainImageCount];
        assert( vkAllocateDescriptorSets(m_device, &descSetAllocInfo, m_descriptorSets) == VK_SUCCESS );
    }
    {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {};
        descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAllocInfo.descriptorPool = m_compositionDescriptorPool;
        descSetAllocInfo.descriptorSetCount = m_swapChainImageCount;
        descSetAllocInfo.pSetLayouts = compositionLayouts;
        m_compositionDescriptorSets = new VkDescriptorSet[m_swapChainImageCount];
        assert( vkAllocateDescriptorSets(m_device, &descSetAllocInfo, m_compositionDescriptorSets) == VK_SUCCESS );
    }

    VkDescriptorImageInfo samplerImageInfo = {};
    samplerImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerImageInfo.imageView = VK_NULL_HANDLE;
    samplerImageInfo.sampler = m_texSampler;

    VkDescriptorImageInfo imagesImageInfo[64];
    for (int i = 0; i < m_instances; ++i)
    {
        imagesImageInfo[i] = {};
        imagesImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesImageInfo[i].imageView = m_texImageView[i];
        imagesImageInfo[i].sampler = nullptr;
    }
    for (int i = m_instances; i < 64; ++i)
    {
        imagesImageInfo[i] = {};
        imagesImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesImageInfo[i].imageView = m_texImageView[0];
        imagesImageInfo[i].sampler = nullptr;
    }

    VkDescriptorImageInfo compositionColorImageInfo = {};
    compositionColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    compositionColorImageInfo.imageView = m_attachments.colorView;
    compositionColorImageInfo.sampler = nullptr;

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        {
            VkDescriptorBufferInfo descBufferInfo = {};
            descBufferInfo.buffer = m_uniformBuffers[i];
            descBufferInfo.offset = 0;
            descBufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet uboWriteDescSet = {};
            uboWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWriteDescSet.dstSet = m_descriptorSets[i];
            uboWriteDescSet.dstBinding = 0;
            uboWriteDescSet.dstArrayElement = 0;
            uboWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWriteDescSet.descriptorCount = 1;
            uboWriteDescSet.pBufferInfo = &descBufferInfo;
            uboWriteDescSet.pImageInfo = nullptr;
            uboWriteDescSet.pTexelBufferView = nullptr;
            uboWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet samplerWriteDescSet = {};
            samplerWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWriteDescSet.dstSet = m_descriptorSets[i];
            samplerWriteDescSet.dstBinding = 1;
            samplerWriteDescSet.dstArrayElement = 0;
            samplerWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerWriteDescSet.descriptorCount = 1;
            samplerWriteDescSet.pImageInfo = &samplerImageInfo;
            samplerWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet imageWriteDescSet = {};
            imageWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWriteDescSet.dstSet = m_descriptorSets[i];
            imageWriteDescSet.dstBinding = 2;
            imageWriteDescSet.dstArrayElement = 0;
            imageWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageWriteDescSet.descriptorCount = 64;
            imageWriteDescSet.pImageInfo = imagesImageInfo;
            imageWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet writeDescSet[] = { uboWriteDescSet, samplerWriteDescSet, imageWriteDescSet };
            vkUpdateDescriptorSets(m_device, sizeof(writeDescSet) / sizeof(VkWriteDescriptorSet), writeDescSet, 0, nullptr);
        }

        {
            VkWriteDescriptorSet inputWriteDescSet = {};
            inputWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            inputWriteDescSet.dstSet = m_compositionDescriptorSets[i];
            inputWriteDescSet.dstBinding = 0;
            inputWriteDescSet.dstArrayElement = 0;
            inputWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            inputWriteDescSet.descriptorCount = 1;
            inputWriteDescSet.pImageInfo = &compositionColorImageInfo;
            inputWriteDescSet.pNext = nullptr;

            VkDescriptorBufferInfo descBufferInfo = {};
            descBufferInfo.buffer = m_colorBuffers[i];
            descBufferInfo.offset = 0;
            descBufferInfo.range = sizeof(vec4) * m_colorCount;

            VkWriteDescriptorSet uboWriteDescSet = {};
            uboWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWriteDescSet.dstSet = m_compositionDescriptorSets[i];
            uboWriteDescSet.dstBinding = 1;
            uboWriteDescSet.dstArrayElement = 0;
            uboWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWriteDescSet.descriptorCount = 1;
            uboWriteDescSet.pBufferInfo = &descBufferInfo;
            uboWriteDescSet.pImageInfo = nullptr;
            uboWriteDescSet.pTexelBufferView = nullptr;
            uboWriteDescSet.pNext = nullptr;
            
            VkWriteDescriptorSet writeDescSet[] = { inputWriteDescSet, uboWriteDescSet };
            vkUpdateDescriptorSets(m_device, sizeof(writeDescSet) / sizeof(VkWriteDescriptorSet), writeDescSet, 0, nullptr);
        }
    }

    // Command Buffer
    m_commandBuffers = new VkCommandBuffer[m_swapChainImageCount];
    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = m_swapChainImageCount;
    assert( vkAllocateCommandBuffers(m_device, &cmdBufferAllocInfo, m_commandBuffers) == VK_SUCCESS );
}

void Renderer::recreateVulkanSwapChain() // TODO: remove when just fullscreen
{
    assert( false );
    // int width, height;
    // glfwGetFramebufferSize(Engine::m_window.Get(), &width, &height);
    // while (width == 0 || height == 0)
    // {
    //     glfwGetFramebufferSize(Engine::m_window.Get(), &width, &height);
    //     glfwWaitEvents();
    // }

    // vkDeviceWaitIdle(m_device);

    // // Cleanup
    // vkFreeCommandBuffers(m_device, m_commandPool, m_swapChainImageCount, m_commandBuffers);
    // cleanupVulkanSwapChain();

    // createVulkanSwapChain();
    // createVulkanPipeline();
    // createVulkanBuffers();
    // createVulkanDrawCmds();

    // recreateImguiSwapChain();
}

void Renderer::update()
{
    renderImgui();
    vkWaitForFences(m_device, 1, &m_framesInFlight[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t frameIndex;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAcquired[m_currentFrame], VK_NULL_HANDLE, &frameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) // TODO: remove when just fullscreen
    {
        recreateVulkanSwapChain();
        return;
    }
    assert( result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR );

    if (m_imagesInFlight[frameIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[frameIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[frameIndex] = m_framesInFlight[m_currentFrame];

    update(frameIndex); // TODO: does this have to wait here? Can this happen before the wait?
    updateImgui(frameIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkCommandBuffer commandBuffers[] = { m_commandBuffers[frameIndex], m_imguiCommandBuffers[frameIndex] };
    VkSemaphore waitSemaphores[] = { m_imageAcquired[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = sizeof(commandBuffers) / sizeof(VkCommandBuffer);
    submitInfo.pCommandBuffers = commandBuffers;

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
    presentInfo.pImageIndices = &frameIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) // TODO: remove when just fullscreen
    {
        framebufferResized = false;
        recreateVulkanSwapChain();
        result = VK_SUCCESS;
    }
    assert( result == VK_SUCCESS );

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::update(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo;
    mat4 proj = mat4::projection(3.141519 * 0.25f, 
        m_swapChainExtent.width / (float) m_swapChainExtent.height,
        0.1f, 100.0f);
    mat4 view = mat4::translate(0.0f, 0.0f, -2.0f);
    // mat4 model = mat4::eulerRotation(0.0f, 0.0f, time);
    // mat4 model = mat4::eulerRotation(0.0f, 0.0f, angle);
    mat4 model = mat4::Identity;
    ubo.modelViewProjection = proj * view * model;
    ubo.time = time;

    void* data;
    vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(UniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);

    int test = 100 * time;
    vec4 colors[] = {
        vec4(float(test % 255)/255.0f,float(test % 255)/255.0f,float(test % 255)/255.0f,1.0f),
        // vec4(127.0f/255.0f,127.0f/255.0f,213.0f/255.0f),
        vec4(134.0f/255.0f,168.0f/255.0f,231.0f/255.0f,1.0f),
        vec4(145.0f/255.0f,234.0f/255.0f,228.0f/255.0f,1.0f)
    };
    vkMapMemory(m_device, m_colorBuffersMemory[currentImage], 0, sizeof(colors), 0, &data);
    memcpy(data, colors, sizeof(colors));
    vkUnmapMemory(m_device, m_colorBuffersMemory[currentImage]);
}

void Renderer::cleanupVulkanSwapChain()
{
    // Buffers
    // TODO: doesn't need to be redone on recreateSwapChain
    m_attachments.destroy(m_device);

    vkDestroySampler(m_device, m_texSampler, nullptr);
    for (int i = 0; i < m_instances; ++i)
    {
        vkDestroyImageView(m_device, m_texImageView[i], nullptr);
        vkDestroyImage(m_device, m_texImage[i], nullptr);
        vkFreeMemory(m_device, m_texImageMemory[i], nullptr);
    }
    vkDestroyBuffer(m_device, m_vertexIndexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexIndexBufferMemory, nullptr);

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(m_device, m_colorBuffers[i], nullptr);
        vkFreeMemory(m_device, m_colorBuffersMemory[i], nullptr);
    }
    delete[] m_uniformBuffers;
    delete[] m_uniformBuffersMemory;
    delete[] m_colorBuffers;
    delete[] m_colorBuffersMemory;
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    delete[] m_descriptorSets;
    vkDestroyDescriptorPool(m_device, m_compositionDescriptorPool, nullptr);
    delete[] m_compositionDescriptorSets;

    delete[] m_commandBuffers;
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyFramebuffer(m_device, m_sceneFramebuffers[i], nullptr);
    }
    delete[] m_sceneFramebuffers;
    // Pipeline
    m_pipeline.destroy(m_device);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_compositionDescriptorLayout, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_device, m_compositionPipelineLayout, nullptr);
    
    // Swap Chain
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
    }
    delete[] m_swapChainImageViews;
    delete[] m_swapChainImages;
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

    cleanupImguiContext();
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
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    cleanupVulkanSwapChain();
    m_renderPass.destroy(m_device);
    // Device
    vkDestroyDevice(m_device, nullptr);
    // Instance
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
#if DEBUG && DEBUG_RENDERER
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
    vkDestroyInstance(m_instance, nullptr);
}

void Renderer::createVulkanDrawCmds()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VsPushConstants vsPushConstants[m_instances];
    FsPushConstants fsPushConstants[m_instances];
    model[0] = mat4::translate(-0.5f, -0.5f, -0.5f);
    model[1] = mat4::Identity;
    model[2] = mat4::translate(0.5f, 0.5f, -0.5f);
    for (int i = 0; i < m_instances; ++i)
    {
        vsPushConstants[i].instanceID = i;
        vsPushConstants[i].model = model[i];
        fsPushConstants[i].instanceID = i;
    }

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        assert( vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) == VK_SUCCESS );

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass.scene;
        renderPassInfo.framebuffer = m_sceneFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        VkClearValue clearColor = {};
        clearColor.color = {1.0f, 1.0f, 0.0f, 0.0f};
        VkClearValue clearDepth = {};
        clearDepth.depthStencil = {1.0f, 0};
        VkClearValue clearColors[] = { clearColor, clearColor, clearDepth }; // Needs to be same order as attachments

        renderPassInfo.clearValueCount = sizeof(clearColors) / sizeof(VkClearValue);
        renderPassInfo.pClearValues = clearColors;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkBuffer vertexBuffers[] = { m_vertexIndexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_vertexIndexBuffer, sizeof(vertices), VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

#if EDITOR
        // Wireframe
        if (renderMode == 0 || renderMode == 2 )
        {
            vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.wireframe);
            for (int j = 0; j < m_instances; ++j)
            {
                vkCmdPushConstants(m_commandBuffers[i], m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VsPushConstants), &vsPushConstants[j]);
                vkCmdPushConstants(m_commandBuffers[i], m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VsPushConstants), sizeof(FsPushConstants), &fsPushConstants[j]);
                vkCmdDrawIndexed(m_commandBuffers[i], sizeof(indices) / sizeof(indices[0]), 1, 0, 0, 0);
            }
        }
#endif

        // General
        if (renderMode < 2)
        {
            vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.scene);
            for (int j = 0; j < m_instances; ++j)
            {
                vkCmdPushConstants(m_commandBuffers[i], m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VsPushConstants), &vsPushConstants[j]);
                vkCmdPushConstants(m_commandBuffers[i], m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VsPushConstants), sizeof(FsPushConstants), &fsPushConstants[j]);
                vkCmdDrawIndexed(m_commandBuffers[i], sizeof(indices) / sizeof(indices[0]), 1, 0, 0, 0);
            }        
        }

        // Composite
        vkCmdNextSubpass(m_commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.composition);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_compositionPipelineLayout, 0, 1, &m_compositionDescriptorSets[i], 0, nullptr);
        vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);
        assert( vkEndCommandBuffer(m_commandBuffers[i]) == VK_SUCCESS );
    }
}

// TODO: remove

void Renderer::cycleMode()
{
    renderMode = ++renderMode % 3;
    vkQueueWaitIdle(m_graphicsQueue);
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkResetCommandBuffer(m_commandBuffers[i], 0);
    }
    createVulkanDrawCmds();
}

void Renderer::loadImageInstance(char filePath[64], float position[3])
{
    {
        const int BufferMemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        void* data;
        
        int texWidth, texHeight, texChannels;
        stbi_uc* texPixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize texSize = texWidth * texHeight * 4;
        assert( texPixels );
        createBuffer(m_device, m_physicalDevice,
                    texSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    BufferMemoryProperty,
                    stagingBuffer, stagingBufferMemory);
        vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
        memcpy(data, texPixels, texSize);
        vkUnmapMemory(m_device, stagingBufferMemory);
        stbi_image_free(texPixels);

        createImage(m_device, m_physicalDevice, texWidth, texHeight, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
            m_texImage[m_instances], m_texImageMemory[m_instances]);

    #if TRANSFER_FAMILY
        copyImage(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_texImage[m_instances], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #else
        copyImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_texImage[m_instances], texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
    #endif
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);

        createImageView(m_device, m_texImage[m_instances], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_texImageView[m_instances]);
    }

    model[m_instances] = mat4::translate(position[0], position[1], position[2]);

    m_instances++;
    vkQueueWaitIdle(m_graphicsQueue);

    VkDescriptorImageInfo compositionColorImageInfo = {};
    compositionColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    compositionColorImageInfo.imageView = m_attachments.colorView;
    compositionColorImageInfo.sampler = nullptr;

    VkDescriptorImageInfo samplerImageInfo = {};
    samplerImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerImageInfo.imageView = VK_NULL_HANDLE;
    samplerImageInfo.sampler = m_texSampler;

    VkDescriptorImageInfo imagesImageInfo[64];
    for (int i = 0; i < m_instances + 1; ++i)
    {
        imagesImageInfo[i] = {};
        imagesImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesImageInfo[i].imageView = m_texImageView[i];
        imagesImageInfo[i].sampler = nullptr;
    }
    for (int i = m_instances; i < 64; ++i)
    {
        imagesImageInfo[i] = {};
        imagesImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesImageInfo[i].imageView = m_texImageView[0];
        imagesImageInfo[i].sampler = nullptr;
    }

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        {
            VkDescriptorBufferInfo descBufferInfo = {};
            descBufferInfo.buffer = m_uniformBuffers[i];
            descBufferInfo.offset = 0;
            descBufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet uboWriteDescSet = {};
            uboWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWriteDescSet.dstSet = m_descriptorSets[i];
            uboWriteDescSet.dstBinding = 0;
            uboWriteDescSet.dstArrayElement = 0;
            uboWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWriteDescSet.descriptorCount = 1;
            uboWriteDescSet.pBufferInfo = &descBufferInfo;
            uboWriteDescSet.pImageInfo = nullptr;
            uboWriteDescSet.pTexelBufferView = nullptr;
            uboWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet samplerWriteDescSet = {};
            samplerWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWriteDescSet.dstSet = m_descriptorSets[i];
            samplerWriteDescSet.dstBinding = 1;
            samplerWriteDescSet.dstArrayElement = 0;
            samplerWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerWriteDescSet.descriptorCount = 1;
            samplerWriteDescSet.pImageInfo = &samplerImageInfo;
            samplerWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet imageWriteDescSet = {};
            imageWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWriteDescSet.dstSet = m_descriptorSets[i];
            imageWriteDescSet.dstBinding = 2;
            imageWriteDescSet.dstArrayElement = 0;
            imageWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageWriteDescSet.descriptorCount = 64;
            imageWriteDescSet.pImageInfo = imagesImageInfo;
            imageWriteDescSet.pNext = nullptr;

            VkWriteDescriptorSet writeDescSet[] = { uboWriteDescSet, samplerWriteDescSet, imageWriteDescSet };
            vkUpdateDescriptorSets(m_device, sizeof(writeDescSet) / sizeof(VkWriteDescriptorSet), writeDescSet, 0, nullptr);
        }

        {
            // TODO: add color gradient

            VkWriteDescriptorSet inputWriteDescSet = {};
            inputWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            inputWriteDescSet.dstSet = m_compositionDescriptorSets[i];
            inputWriteDescSet.dstBinding = 0;
            inputWriteDescSet.dstArrayElement = 0;
            inputWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            inputWriteDescSet.descriptorCount = 1;
            inputWriteDescSet.pImageInfo = &compositionColorImageInfo;
            inputWriteDescSet.pNext = nullptr;
            
            VkWriteDescriptorSet writeDescSet[] = { inputWriteDescSet };
            vkUpdateDescriptorSets(m_device, sizeof(writeDescSet) / sizeof(VkWriteDescriptorSet), writeDescSet, 0, nullptr);
        }
    }

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkResetCommandBuffer(m_commandBuffers[i], 0);
    }
    createVulkanDrawCmds();
}