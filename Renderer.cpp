#include "Renderer.h"

#include <assert.h>
#include <iostream>
#include <chrono>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.h"
#include "Shaders/ShaderStructures.h"
#include "GLFWUtilities.h"
#include "VulkanUtilities.h"

const Vertex vertices[] =
{
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

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
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Bending", nullptr, nullptr); // TODO: full screen only
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    
    /// VULKAN
    createVulkanInstance();

    // Surface
    assert( glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) == VK_SUCCESS );

    createVulkanDevice();
    createVulkanSwapChain();
    createVulkanPipeline();
    // Command Pool // TODO: move back into buffers with fullscreen
    QueueFamilyIndices queueFamily = findQueueFamilies(m_physicalDevice, m_surface); // TODO: this is the 3rd time this is called on init (at least)
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = queueFamily.graphicsFamily; // TODO: should this be m_graphicsQueue
    poolCreateInfo.flags = 0;
    assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_commandPool) == VK_SUCCESS );
#if TRANSFER_FAMILY
    poolCreateInfo.queueFamilyIndex = queueFamily.transferFamily; // TODO: should this be m_transferQueue
    assert( vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_transferCommandPool) == VK_SUCCESS ); // TODO: currently doesn't work
#endif
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
        if (isDeviceSutiable(device, m_surface))
        {
            m_physicalDevice = device;
            break;
        }
    }
    assert( m_physicalDevice != VK_NULL_HANDLE );

    // Logical Device
    QueueFamilyIndices queueFamily = findQueueFamilies(m_physicalDevice, m_surface);
    assert( queueFamily.graphicsFamily == queueFamily.presentFamily ); // TODO: rather than forming a set with multiple queues

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily.graphicsFamily;
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
    glfwGetFramebufferSize(m_window, &frameWidth, &frameHeight); // TODO: should I be updating the window size here too?
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
    VkVertexInputBindingDescription bindingDesc = Vertex::bindingDesc();
    vertInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertInputCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertInputCreateInfo.vertexAttributeDescriptionCount = Vertex::attributeDescCount;
    vertInputCreateInfo.pVertexAttributeDescriptions = Vertex::attributeDesc();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Uniform Buffer
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1; // TODO: how does this map if only 1 binding
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &uboLayoutBinding;
    assert( vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorLayout) == VK_SUCCESS );

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
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
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

    // Vertex + Index Buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    size_t bufferSize = sizeof(vertices) + sizeof(indices);

    createBuffer(m_device, m_physicalDevice,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // TODO: VK_MEMORY_PROPERTY_HOST_COHERENT_BIT worse performance. use flushing
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);
    
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    data = static_cast<char*>(data) + sizeof(vertices);
    memcpy(data, indices, sizeof(indices));
    vkUnmapMemory(m_device, stagingBufferMemory);

    createBuffer(m_device, m_physicalDevice,
        bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vertexIndexBuffer,
        m_vertexIndexBufferMemory);
#if TRANSFER_FAMILY
    copyBuffer(m_device, m_commandPool, m_transferQueue, stagingBuffer, m_vertexIndexBuffer, bufferSize);
#else
    copyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_vertexIndexBuffer, bufferSize);
#endif
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // Uniform Buffer
    VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers = new VkBuffer[m_swapChainImageCount];
    m_uniformBuffersMemory = new VkDeviceMemory[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        createBuffer(m_device, m_physicalDevice,
            uniformBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }

    VkDescriptorPoolSize descPoolSize = {};
    descPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descPoolSize.descriptorCount = m_swapChainImageCount;

    VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
    descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCreateInfo.poolSizeCount = 1;
    descPoolCreateInfo.pPoolSizes = &descPoolSize;
    descPoolCreateInfo.maxSets = m_swapChainImageCount;
    assert (vkCreateDescriptorPool(m_device, &descPoolCreateInfo, nullptr, &m_descriptorPool) == VK_SUCCESS );

    VkDescriptorSetLayout layouts[m_swapChainImageCount];
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        layouts[i] = m_descriptorLayout;
    }
    VkDescriptorSetAllocateInfo descSetAllocInfo = {};
    descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetAllocInfo.descriptorPool = m_descriptorPool;
    descSetAllocInfo.descriptorSetCount = m_swapChainImageCount;
    descSetAllocInfo.pSetLayouts = layouts;
    m_descriptorSets = new VkDescriptorSet[m_swapChainImageCount];
    assert( vkAllocateDescriptorSets(m_device, &descSetAllocInfo, m_descriptorSets) == VK_SUCCESS );
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        VkDescriptorBufferInfo descBufferInfo = {};
        descBufferInfo.buffer = m_uniformBuffers[i];
        descBufferInfo.offset = 0;
        descBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descWrite = {};
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = m_descriptorSets[i];
        descWrite.dstBinding = 0;
        descWrite.dstArrayElement = 0;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.descriptorCount = 1;
        descWrite.pBufferInfo = &descBufferInfo;
        descWrite.pImageInfo = nullptr;
        descWrite.pTexelBufferView = nullptr;
        vkUpdateDescriptorSets(m_device, 1, &descWrite, 0, nullptr);
    }

    // Command Buffer
    m_commandBuffers = new VkCommandBuffer[m_swapChainImageCount];
    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = m_swapChainImageCount;
    assert( vkAllocateCommandBuffers(m_device, &cmdBufferAllocInfo, m_commandBuffers) == VK_SUCCESS );

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

        VkBuffer vertexBuffers[] = { m_vertexIndexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(m_commandBuffers[i], m_vertexIndexBuffer, sizeof(vertices), VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);
        vkCmdDrawIndexed(m_commandBuffers[i], sizeof(indices) / sizeof(indices[0]), 1, 0, 0, 0);
        // vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);
        assert( vkEndCommandBuffer(m_commandBuffers[i]) == VK_SUCCESS );
    }
}

void Renderer::recreateVulkanSwapChain() // TODO: remove when just fullscreen
{
    assert( false );
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device);

    // Cleanup
    vkFreeCommandBuffers(m_device, m_commandPool, m_swapChainImageCount, m_commandBuffers);
    cleanupVulkanSwapChain();

    createVulkanSwapChain();
    createVulkanPipeline();
    createVulkanBuffers();
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
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAcquired[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) // TODO: remove when just fullscreen
    {
        recreateVulkanSwapChain();
        return;
    }
    assert( result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR );

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[imageIndex] = m_framesInFlight[m_currentFrame];

    update(imageIndex);

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
    mat4 model = mat4::eulerRotation(0.0f, 0.0f, time);
    ubo.modelViewProjection = proj * view * model;
    ubo.time = time;

    void* data;
    vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(UniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
}

void Renderer::cleanupVulkanSwapChain()
{
    // Buffers
    // TODO: doesn't need to be redone on recreateSwapChain
    vkDestroyBuffer(m_device, m_vertexIndexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexIndexBufferMemory, nullptr);

    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
    }
    delete[] m_uniformBuffers;
    delete[] m_uniformBuffersMemory;
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    delete[] m_descriptorSets;

    delete[] m_commandBuffers;
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyFramebuffer(m_device, m_swapChainFramebuffers[i], nullptr);
    }
    delete[] m_swapChainFramebuffers;
    // Pipeline
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorLayout, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    // Swap Chain
    for (int i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
    }
    delete[] m_swapChainImageViews;
    delete[] m_swapChainImages;
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
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
