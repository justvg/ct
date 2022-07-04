#include <vulkan.h>

#define VMA_VULKAN_VERSION 1001000
#define VMA_IMPLEMENTATION
#include "ThirdParty/VMA/vk_mem_alloc.h"

#ifndef ENGINE_RELEASE
#define VkCheck(Call) \
				{ \
					VkResult Result = Call; \
					Assert(Result == VK_SUCCESS); \
				}
#else
#define VkCheck(Call) Call
#endif

// NOTE(georgii): Can't be just changed. Will introduce bugs. But we are fine with FramesInFlight=2.
const uint32_t FramesInFlight = 2;

struct SImage
{
	VkImage Image;
	VkImageView View;
	VmaAllocation Allocation;
	VkFormat Format;
	uint32_t Width, Height;
};

struct SBuffer
{
	VkBuffer Buffer;
	VmaAllocation Allocation;
	uint64_t Size;
	void* Data;
};

struct SSwapchain
{
	VkSwapchainKHR VkSwapchain;
	uint32_t ImageCount;
	VkImage Images[8];

	uint32_t Width, Height;

	VkPresentModeKHR PresentMode;
};

VkInstance CreateInstance()
{
	VkApplicationInfo AppInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	AppInfo.pApplicationName = "Engine";
	AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.pEngineName = "Engine";
	AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.apiVersion = VK_API_VERSION_1_1; // NOTE(georgii): If you wanna change this, don't forget to change VMA_VULKAN_VERSION

	const char* Extensions[] = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifndef ENGINE_RELEASE
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};

	VkInstanceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	CreateInfo.pApplicationInfo = &AppInfo;
	CreateInfo.enabledExtensionCount = ArrayCount(Extensions);
	CreateInfo.ppEnabledExtensionNames = Extensions;

#ifndef ENGINE_RELEASE
	uint32_t LayerCount = 0;
	vkEnumerateInstanceLayerProperties(&LayerCount, 0);

	VkLayerProperties* Layers = (VkLayerProperties*) malloc(sizeof(VkLayerProperties) * LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, Layers);

	bool bValidationLayerAvailable = false;
	for (uint32_t I = 0; I < LayerCount; I++)
	{
		if (CompareStrings(Layers[I].layerName, "VK_LAYER_KHRONOS_validation"))
		{
			bValidationLayerAvailable = true;
		}
	}
	free(Layers);

	if (!bValidationLayerAvailable)
	{
		SLogger::Log("Vulkan validation layer is not available.\n", LoggerVerbosity_Debug);
	}

	const char* ValidationLayers[] =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	if (bValidationLayerAvailable)
	{
		CreateInfo.enabledLayerCount = ArrayCount(ValidationLayers);
		CreateInfo.ppEnabledLayerNames = ValidationLayers;
	}
#endif

	VkInstance Instance = 0;
	VkResult Result = vkCreateInstance(&CreateInfo, 0, &Instance);
	switch (Result)
	{
		case VK_SUCCESS:
		{
			SLogger::Log("Vulkan instance initialized.\n", LoggerVerbosity_Release);
		} break;

		case VK_ERROR_INITIALIZATION_FAILED:
		{
			SLogger::Log("Can't initialize Vulkan instance. VK_ERROR_INITIALIZATION_FAILED\n", LoggerVerbosity_Release);
		} break;

		case VK_ERROR_LAYER_NOT_PRESENT:
		{
			SLogger::Log("Can't initialize Vulkan instance. VK_ERROR_LAYER_NOT_PRESENT\n", LoggerVerbosity_Release);
		} break;

		case VK_ERROR_EXTENSION_NOT_PRESENT:
		{
			SLogger::Log("Can't initialize Vulkan instance. VK_ERROR_EXTENSION_NOT_PRESENT\n", LoggerVerbosity_Release);
		} break;

		case VK_ERROR_INCOMPATIBLE_DRIVER:
		{
			SLogger::Log("Can't initialize Vulkan instance. VK_ERROR_INCOMPATIBLE_DRIVER\n", LoggerVerbosity_Release);
		} break;
	}		

	Assert(Instance);
	return Instance;
}

#ifndef ENGINE_RELEASE
VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjectType, uint64_t Object, size_t Location, int32_t MessageCode, const char* LayerPrefix, const char* Message, void* UserData)
{
	if (Flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		return VK_FALSE;

	const char* Type = (Flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "ERROR" : 
					   (Flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) ? "DEBUG" :
					   (Flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) ? "WARNING" : "INFO";

	char Text[4096];
	snprintf(Text, sizeof(Text), "\n\n%s: %s\n\n", Type, Message);

	PlatformOutputDebugString(Text);

	if (Flags & (VK_DEBUG_REPORT_ERROR_BIT_EXT)) 
	{
		SLogger::Log(Text, LoggerVerbosity_Debug);
		Assert(!"Validation error encountered!");
	}

	return VK_FALSE;
}

VkDebugReportCallbackEXT RegisterDebugCallback(VkInstance Instance)
{
	VkDebugReportCallbackCreateInfoEXT CreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
	CreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	CreateInfo.pfnCallback = DebugReportCallback;

	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(Instance, "vkCreateDebugReportCallbackEXT");

	VkDebugReportCallbackEXT Callback = 0;
	if (vkCreateDebugReportCallbackEXT)
	{
		VkCheck(vkCreateDebugReportCallbackEXT(Instance, &CreateInfo, 0, &Callback));
		Assert(Callback);
	}
	else
	{
		Assert(!"Can't get the address of vkCreateDebugReportCallbackEXT. So can't register Vulkan debug callback.");
	}	

	return Callback;
}
#endif

struct SVulkanVersion
{
	uint8_t Major;
	uint8_t Minor;
};

SVulkanVersion GetVulkanVersion()
{
	SVulkanVersion Version = {};

	uint32_t VersionEncoded = 0;
	VkCheck(vkEnumerateInstanceVersion(&VersionEncoded));

	Version.Major = uint8_t(VK_VERSION_MAJOR(VersionEncoded));
	Version.Minor = uint8_t(VK_VERSION_MINOR(VersionEncoded));

	return Version;
}

VkSampleCountFlagBits GetMaxUsableSamplerCount(const VkPhysicalDeviceProperties& PhysicalDeviceProps)
{
	VkSampleCountFlags SampleCount = PhysicalDeviceProps.limits.framebufferColorSampleCounts & PhysicalDeviceProps.limits.framebufferDepthSampleCounts;
	if (SampleCount & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (SampleCount & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (SampleCount & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (SampleCount & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (SampleCount & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (SampleCount & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

uint32_t GetGraphicsFamilyIndex(VkPhysicalDevice PhysicalDevice)
{
	uint32_t QueueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, 0);

	VkQueueFamilyProperties QueueFamilyProperties[64];
	Assert(QueueFamilyPropertyCount <= ArrayCount(QueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueFamilyProperties);

	uint32_t FamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	for (uint32_t I = 0; I < QueueFamilyPropertyCount; I++)
	{
		if (QueueFamilyProperties[I].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			FamilyIndex = I;
			break;
		}
	}

	return FamilyIndex;
}

bool SupportsPresentation(VkPhysicalDevice PhysicalDevices, uint32_t FamilyIndex)
{
	bool bResult = vkGetPhysicalDeviceWin32PresentationSupportKHR(PhysicalDevices, FamilyIndex);
	return bResult;
}

VkPhysicalDevice PickPhysicalDevice(VkInstance Instance)
{
	uint32_t PhysicalDeviceCount = 0;
	VkCheck(vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, 0));

	VkPhysicalDevice PhysicalDevices[32];
	Assert(PhysicalDeviceCount <= ArrayCount(PhysicalDevices));
	VkCheck(vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices));

	VkPhysicalDevice Discrete = 0;
	VkPhysicalDevice Fallback = 0;
	for (uint32_t I = 0; I < PhysicalDeviceCount; I++)
	{
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(PhysicalDevices[I], &Properties);

		uint32_t GraphicsFamilyIndex = GetGraphicsFamilyIndex(PhysicalDevices[I]);
		if (GraphicsFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
			continue;

		if (!SupportsPresentation(PhysicalDevices[I], GraphicsFamilyIndex))
			continue;

		if (!Discrete && (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
		{
			Discrete = PhysicalDevices[I];
		}
		else if (!Fallback)
		{
			Fallback = PhysicalDevices[I];
		}
	}

	VkPhysicalDevice PhysicalDevice = Discrete ? Discrete : Fallback;
	if (PhysicalDevice)
	{
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);

		char Info[128];
		snprintf(Info, sizeof(Info), "Selected GPU: %s\n", Properties.deviceName);
		SLogger::Log(Info, LoggerVerbosity_Release);
	}
	else
	{
		SLogger::Log("Can't pick GPU.\n", LoggerVerbosity_Release);
	}

	Assert(PhysicalDevice);
	return PhysicalDevice;
}

VkDevice CreateDevice(VkPhysicalDevice PhysicalDevice, uint32_t FamilyIndex, bool& bOutIndirectCountKHR, bool& bOutIndirectCountAMD, bool& bOutMinMaxSampler)
{
	VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	QueueCreateInfo.queueFamilyIndex = FamilyIndex;
	QueueCreateInfo.queueCount = 1;
	float Priority = 1.0f;
	QueueCreateInfo.pQueuePriorities = &Priority;

	uint32_t AvailableExtensionCount = 0;
	VkCheck(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &AvailableExtensionCount, 0));

	VkExtensionProperties* AvailableExtensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * AvailableExtensionCount);
	VkCheck(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &AvailableExtensionCount, AvailableExtensions));

	bool bSwapchainKHR = false;
	bool bIndirectCountKHR = false;
	bool bIndirectCountAMD = false;
	bool bMinMaxSampler = false;
	for (uint32_t I = 0; I < AvailableExtensionCount; I++)
	{
		if (CompareStrings(AvailableExtensions[I].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
		{
			bSwapchainKHR = true;
		}
		else if (CompareStrings(AvailableExtensions[I].extensionName, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME))
		{
			bIndirectCountKHR = true;
		}
		else if (CompareStrings(AvailableExtensions[I].extensionName, VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME))
		{
			bIndirectCountAMD = true;
		}
		else if (CompareStrings(AvailableExtensions[I].extensionName, VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME))
		{
			bMinMaxSampler = true;
		}
	}
	free(AvailableExtensions);

	if (!bMinMaxSampler)
	{
		SLogger::Log("MinMax sampler extension is not available.\n", LoggerVerbosity_Release);
	}

	if (!bIndirectCountKHR && !bIndirectCountAMD)
	{
		SLogger::Log("DrawIndirectCount extension is not available.\n", LoggerVerbosity_Release);
	}

	Assert(bSwapchainKHR);
	Assert(bIndirectCountKHR || bIndirectCountAMD);

	VkDevice Device = 0;
	if (bSwapchainKHR)
	{
		uint32_t ExtensionCount = 1;
		const char* Extensions[3] = {};
		Extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

		if (bMinMaxSampler)
		{
			Extensions[ExtensionCount++] = VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME;
		}

		if (bIndirectCountKHR || bIndirectCountAMD)
		{
			Extensions[ExtensionCount++] = bIndirectCountKHR ? VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME : (bIndirectCountAMD ? VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME : 0);
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};
		DeviceFeatures.multiDrawIndirect = true;
		DeviceFeatures.fillModeNonSolid = true;

		VkPhysicalDeviceShaderDrawParametersFeatures DeviceShaderDrawParametersFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES };
		DeviceShaderDrawParametersFeatures.shaderDrawParameters = true;

		VkDeviceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		CreateInfo.pNext = &DeviceShaderDrawParametersFeatures;
		CreateInfo.queueCreateInfoCount = 1;
		CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
		CreateInfo.enabledExtensionCount = ExtensionCount;
		CreateInfo.ppEnabledExtensionNames = Extensions;
		CreateInfo.pEnabledFeatures = &DeviceFeatures;

		VkCheck(vkCreateDevice(PhysicalDevice, &CreateInfo, 0, &Device));
		Assert(Device);

		SLogger::Log("Vulkan device created.\n", LoggerVerbosity_Release);
	}
	else
	{
		SLogger::Log("VK_KHR_SWAPCHAIN_EXTENSION_NAME is now available, no reason to create a device.\n", LoggerVerbosity_Release);
	}

	bOutIndirectCountKHR = bIndirectCountKHR;
	bOutIndirectCountAMD = bIndirectCountAMD;
	bOutMinMaxSampler = bMinMaxSampler;

	return Device;
}

VkSurfaceKHR CreateSurface(VkInstance Instance, HINSTANCE HInstance, HWND Window)
{
	VkWin32SurfaceCreateInfoKHR CreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    CreateInfo.hinstance = HInstance;
	CreateInfo.hwnd = Window;

	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(Instance, "vkCreateWin32SurfaceKHR");
	Assert(vkCreateWin32SurfaceKHR);

	VkSurfaceKHR Surface = 0;
	if (vkCreateWin32SurfaceKHR)
	{
		VkCheck(vkCreateWin32SurfaceKHR(Instance, &CreateInfo, 0, &Surface));
		Assert(Surface);

		if (!Surface)
		{
			SLogger::Log("Can't create surface.\n", LoggerVerbosity_Release);
		}
	}
	else
	{
		SLogger::Log("Can't get vkCreateWin32SurfaceKHR address.\n", LoggerVerbosity_Release);
	}

	return Surface;
}

VkBool32 SurfaceSupportsPresentation(VkPhysicalDevice PhysicalDevice, uint32_t FamilyIndex, VkSurfaceKHR Surface)
{
	VkBool32 SupportsPresentation = false;
	VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &SupportsPresentation));

	return SupportsPresentation;
}

VkFormat GetSwapchainFormat(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
	uint32_t FormatsCount = 0;
	VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatsCount, 0));

	VkSurfaceFormatKHR Formats[512];
	Assert(FormatsCount <= ArrayCount(Formats));
	VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatsCount, Formats));

	VkFormat Result = Formats[0].format;
	if ((FormatsCount == 1) && (Formats[0].format == VK_FORMAT_UNDEFINED))
	{
		Result = VK_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		for (uint32_t I = 0; I < FormatsCount; I++)
		{
			if ((Formats[I].format == VK_FORMAT_R8G8B8A8_UNORM) || (Formats[I].format == VK_FORMAT_B8G8R8A8_UNORM))
			{
				Result = Formats[I].format;
				break;
			}
		}
	}

	return Result;
}

VkFormat FindDepthFormat(VkPhysicalDevice PhysicalDevice)
{
	VkFormat Formats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	for (uint32_t I = 0; I < ArrayCount(Formats); I++)
	{
		VkFormatProperties FormatProps;
		vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Formats[I], &FormatProps);

		if (FormatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			return Formats[I]; 
	}

	Assert(!"Can't find depth format!");
	return VK_FORMAT_UNDEFINED;
}

VmaAllocator CreateVulkanMemoryAllocator(VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device)
{
	VmaAllocatorCreateInfo CreateInfo = {};
	CreateInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT; // I hope, we are fine with this flag set.
	CreateInfo.instance = Instance;
	CreateInfo.physicalDevice = PhysicalDevice;
	CreateInfo.device = Device;

	VmaAllocator Allocator = 0;
	VkCheck(vmaCreateAllocator(&CreateInfo, &Allocator));
	Assert(Allocator);

	return Allocator;
}

VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, uint32_t MipLevel, uint32_t MipCount, VkImageAspectFlags AspectFlags)
{
	VkImageViewCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	CreateInfo.image = Image;
	CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	CreateInfo.format = Format;
	CreateInfo.subresourceRange.aspectMask = AspectFlags;
	CreateInfo.subresourceRange.baseMipLevel = MipLevel;
	CreateInfo.subresourceRange.levelCount = MipCount;
	CreateInfo.subresourceRange.layerCount = 1;

	VkImageView ImageView = 0;
	VkCheck(vkCreateImageView(Device, &CreateInfo, 0, &ImageView));
	Assert(ImageView);

	return ImageView;
}

SImage CreateImage(VkDevice Device, VmaAllocator MemoryAllocator, VkFormat Format, uint32_t Width, uint32_t Height, uint32_t MipLevel, uint32_t MipCount, VkImageUsageFlags UsageFlags, VkImageAspectFlags AspectFlags, VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_1_BIT)
{
	VkImageCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	CreateInfo.imageType = VK_IMAGE_TYPE_2D;
	CreateInfo.format = Format;
	CreateInfo.extent.width = Width;
	CreateInfo.extent.height = Height;
	CreateInfo.extent.depth = 1;
	CreateInfo.mipLevels = MipCount;
	CreateInfo.arrayLayers = 1;
	CreateInfo.samples = SampleCount;
	CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	CreateInfo.usage = UsageFlags;
	CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo AllocationCreateInfo = {};
	AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImage Image = 0;
	VmaAllocation Allocation = 0;
	VkCheck(vmaCreateImage(MemoryAllocator, &CreateInfo, &AllocationCreateInfo, &Image, &Allocation, 0));
	Assert(Image);
	Assert(Allocation);

	VkImageView View = CreateImageView(Device, Image, Format, MipLevel, MipCount, AspectFlags);

	SImage Result = {};
	Result.Image = Image;
	Result.View = View;
	Result.Allocation = Allocation;
	Result.Format = Format;
	Result.Width = Width;
	Result.Height = Height;

	return Result;
}

void DestroyImage(VkDevice Device, VmaAllocator MemoryAllocator, SImage& Image)
{
	vkDestroyImageView(Device, Image.View, 0);
	vmaDestroyImage(MemoryAllocator, Image.Image, Image.Allocation);

	Image.Image = 0;
	Image.View = 0;
	Image.Allocation = 0;
}

VkSampler CreateSampler(VkDevice Device, VkFilter Filter, VkSamplerAddressMode AddressMode, float MaxLod = 0.0f, VkSamplerReductionMode ReductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, VkSamplerMipmapMode MipmapFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST)
{
	VkSamplerCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	CreateInfo.magFilter = Filter;
	CreateInfo.minFilter = Filter;
	CreateInfo.mipmapMode = MipmapFilter;
	CreateInfo.addressModeU = AddressMode;
	CreateInfo.addressModeV = AddressMode;
	CreateInfo.addressModeW = AddressMode;
	CreateInfo.minLod = 0.0f;
	CreateInfo.maxLod = MaxLod;

	VkSamplerReductionModeCreateInfo ReductionCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO };
	if (ReductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
	{
		ReductionCreateInfo.reductionMode = ReductionMode;
		CreateInfo.pNext = &ReductionCreateInfo;
	}

	VkSampler Sampler = 0;
	VkCheck(vkCreateSampler(Device, &CreateInfo, 0, &Sampler));
	Assert(Sampler);

	return Sampler;
}

VkImageMemoryBarrier CreateImageMemoryBarrier(VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImage Image, VkImageAspectFlags AspectMask, uint32_t MipLevel = 0, uint32_t MipLevelCount = VK_REMAINING_MIP_LEVELS)
{
	VkImageMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	Barrier.srcAccessMask = SrcAccessMask;
	Barrier.dstAccessMask = DstAccessMask;
	Barrier.oldLayout = OldLayout;
	Barrier.newLayout = NewLayout;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = Image;
	Barrier.subresourceRange.aspectMask = AspectMask;
	Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	Barrier.subresourceRange.baseMipLevel = MipLevel;
	Barrier.subresourceRange.levelCount = MipLevelCount;

	return Barrier;
}

VkBufferMemoryBarrier CreateBufferMemoryBarrier(VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, SBuffer Buffer, uint64_t Size)
{
	VkBufferMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	Barrier.srcAccessMask = SrcAccessMask;
	Barrier.dstAccessMask = DstAccessMask;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.buffer = Buffer.Buffer;
	Barrier.offset = 0;
	Barrier.size = Size;

	return Barrier;
}

void UploadImageFromBuffer(VkDevice Device, VkCommandPool CommandPool, VkCommandBuffer CommandBuffer, VkQueue Queue, const SImage& Image, uint32_t Width, uint32_t Height, const SBuffer& StagingBuffer, void *Data, uint64_t Size)
{
	Assert(StagingBuffer.Data);
	memcpy(StagingBuffer.Data, Data, Size);

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCheck(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

	VkImageMemoryBarrier TransferBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1);
	vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &TransferBarrier);

	VkBufferImageCopy CopyRegion = {};
    CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	CopyRegion.imageSubresource.layerCount = 1;
    CopyRegion.imageExtent.width = Width;
    CopyRegion.imageExtent.height = Height;
	CopyRegion.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(CommandBuffer, StagingBuffer.Buffer, Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

	VkImageMemoryBarrier ReadBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Image.Image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1);
	vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &ReadBarrier);

	VkCheck(vkEndCommandBuffer(CommandBuffer));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;
	VkCheck(vkQueueSubmit(Queue, 1, &SubmitInfo, 0));

	VkCheck(vkDeviceWaitIdle(Device));
}

uint32_t GetMipsCount(uint32_t Width, uint32_t Height)
{
	float Log = Floor(Max(Log2(float(Width)), Log2(float(Height))));
	uint32_t MipsCount = (uint32_t)Log + 1;

	return MipsCount;
}

VkFramebuffer CreateFramebuffer(VkDevice Device, VkRenderPass RenderPass, const VkImageView *Attachments, uint32_t AttachmentCount, uint32_t Width, uint32_t Height)
{
	VkFramebufferCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	CreateInfo.renderPass = RenderPass;
	CreateInfo.attachmentCount = AttachmentCount;
	CreateInfo.pAttachments = Attachments;
	CreateInfo.width = Width;
	CreateInfo.height = Height;
	CreateInfo.layers = 1;

	VkFramebuffer Framebuffer = 0;
	VkCheck(vkCreateFramebuffer(Device, &CreateInfo, 0, &Framebuffer));
	Assert(Framebuffer);

	return Framebuffer;
}

VkSwapchainKHR CreateSwapchain(VkDevice Device, VkSurfaceKHR Surface, VkFormat Format, const VkSurfaceCapabilitiesKHR& SurfaceCaps, VkPresentModeKHR PresentMode, VkSwapchainKHR OldSwapchain = 0)
{
	VkCompositeAlphaFlagBitsKHR CompositeAlpha = (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :
												 (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
												 (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
 												  VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	VkSwapchainCreateInfoKHR CreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	CreateInfo.surface = Surface;
	CreateInfo.minImageCount = Max(2u, SurfaceCaps.minImageCount);
	CreateInfo.imageFormat = Format;
	CreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	CreateInfo.imageExtent = SurfaceCaps.currentExtent;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	CreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	CreateInfo.compositeAlpha = CompositeAlpha;
	CreateInfo.presentMode = PresentMode;
	CreateInfo.oldSwapchain = OldSwapchain;

	VkSwapchainKHR Swapchain = 0;
	VkCheck(vkCreateSwapchainKHR(Device, &CreateInfo, 0, &Swapchain));

	return Swapchain;
}

SSwapchain CreateSwapchain(VkDevice Device, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, VkFormat ColorFormat, VkPresentModeKHR PresentMode, VkSwapchainKHR OldSwapchain = 0)
{
	SSwapchain Swapchain = {};

	VkSurfaceCapabilitiesKHR SurfaceCaps;
	VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCaps));

	Swapchain.VkSwapchain = CreateSwapchain(Device, Surface, ColorFormat, SurfaceCaps, PresentMode, OldSwapchain);
	Assert(Swapchain.VkSwapchain);

	uint32_t ImageCount = 0;
	VkCheck(vkGetSwapchainImagesKHR(Device, Swapchain.VkSwapchain, &ImageCount, 0));

	Assert(ImageCount <= ArrayCount(Swapchain.Images));
	VkCheck(vkGetSwapchainImagesKHR(Device, Swapchain.VkSwapchain, &ImageCount, Swapchain.Images));

	Swapchain.ImageCount = ImageCount;
	Swapchain.Width = SurfaceCaps.currentExtent.width;
	Swapchain.Height = SurfaceCaps.currentExtent.height;
	Swapchain.PresentMode = PresentMode;

	return Swapchain;
}

void DestroySwapchain(SSwapchain Swapchain, VkDevice Device)
{
	vkDestroySwapchainKHR(Device, Swapchain.VkSwapchain, 0);
}

bool ResizeSwapchainIfChanged(SSwapchain& Swapchain, VkDevice Device, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, VkFormat ColorFormat, const VkSurfaceCapabilitiesKHR& SurfaceCaps)
{
	bool bResized = false;

	if ((Swapchain.Width != SurfaceCaps.currentExtent.width) || (Swapchain.Height != SurfaceCaps.currentExtent.height))
	{
		VkCheck(vkDeviceWaitIdle(Device));

		SSwapchain OldSwapchain = Swapchain;
		Swapchain = CreateSwapchain(Device, PhysicalDevice, Surface, ColorFormat, Swapchain.PresentMode, Swapchain.VkSwapchain);

		DestroySwapchain(OldSwapchain, Device);

		bResized = true;
	}

	return bResized;
}

bool ChangeVSyncIfNeeded(SSwapchain& Swapchain, VkDevice Device, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, VkFormat ColorFormat, VkPresentModeKHR PresentMode)
{
	bool bChanged = false;

	if (Swapchain.PresentMode != PresentMode)
	{
		VkCheck(vkDeviceWaitIdle(Device));

		SSwapchain OldSwapchain = Swapchain;
		Swapchain = CreateSwapchain(Device, PhysicalDevice, Surface, ColorFormat, PresentMode, Swapchain.VkSwapchain);

		DestroySwapchain(OldSwapchain, Device);

		bChanged = true;
	}

	return bChanged;
}

VkCommandPool CreateCommandPool(VkDevice Device, VkCommandPoolCreateFlags Flags, uint32_t FamilyIndex)
{
	VkCommandPoolCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	CreateInfo.flags = Flags;
	CreateInfo.queueFamilyIndex = FamilyIndex;

	VkCommandPool CommandPool = 0;
	VkCheck(vkCreateCommandPool(Device, &CreateInfo, 0, &CommandPool));
	Assert(CommandPool);

	return CommandPool;
}

void AllocateCommandBuffers(VkDevice Device, VkCommandPool CommandPool, VkCommandBuffer* CommandBuffers, uint32_t Count)
{
	VkCommandBufferAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	AllocateInfo.commandPool = CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = Count;

	VkCheck(vkAllocateCommandBuffers(Device, &AllocateInfo, CommandBuffers));

	for (uint32_t I = 0; I < Count; I++)
	{
		Assert(CommandBuffers[I]);
	}
}

VkSemaphore CreateSemaphore(VkDevice Device)
{
	VkSemaphoreCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore Semaphore = 0;
	VkCheck(vkCreateSemaphore(Device, &CreateInfo, 0, &Semaphore));

	return Semaphore;
}

VkFence CreateFence(VkDevice Device)
{
	VkFenceCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };	
	CreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence Fence = 0;
	VkCheck(vkCreateFence(Device, &CreateInfo, 0, &Fence));
	Assert(Fence);

	return Fence;
}

VkQueryPool CreateQueryPool(VkDevice Device, uint32_t QueriesCount)
{
    VkQueryPoolCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    CreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    CreateInfo.queryCount = QueriesCount;

    VkQueryPool QueryPool = 0;
    VkCheck(vkCreateQueryPool(Device, &CreateInfo, 0, &QueryPool));
    Assert(QueryPool);

	return QueryPool;
}

VkShaderModule LoadShader(VkDevice Device, const char* Path)
{
	SReadEntireFileResult FileData = ReadEntireFile(Path);
	Assert(FileData.Size % 4 == 0);

	VkShaderModuleCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	CreateInfo.codeSize = FileData.Size;
	CreateInfo.pCode = (uint32_t*)FileData.Memory;

	VkShaderModule ShaderModule = 0;
	VkCheck(vkCreateShaderModule(Device, &CreateInfo, 0, &ShaderModule));
	Assert(ShaderModule);

	free(FileData.Memory);

	return ShaderModule;
}

SBuffer CreateBuffer(VmaAllocator MemoryAllocator, VkDeviceSize Size, VkBufferUsageFlags BufferUsage, VmaMemoryUsage MemoryUsage)
{
	VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferCreateInfo.size = Size;
	BufferCreateInfo.usage = BufferUsage;

	VmaAllocationCreateInfo AllocationCreateInfo = {};
	AllocationCreateInfo.usage = MemoryUsage;

	SBuffer Buffer = {};
	VkCheck(vmaCreateBuffer(MemoryAllocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer.Buffer, &Buffer.Allocation, 0));
	Assert(Buffer.Buffer);
	Assert(Buffer.Allocation);

	void* Data = 0;
	if ((MemoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU) || (MemoryUsage == VMA_MEMORY_USAGE_CPU_ONLY))
		vmaMapMemory(MemoryAllocator, Buffer.Allocation, &Data);
	Buffer.Size = Size;
	Buffer.Data = Data;

	return Buffer;
}

void UploadBuffer(VkDevice Device, VkCommandPool CommandPool, VkCommandBuffer CommandBuffer, VkQueue Queue, const SBuffer& Buffer, const SBuffer& StagingBuffer, void *Data, uint64_t Size)
{
	Assert(StagingBuffer.Data);
	memcpy(StagingBuffer.Data, Data, Size);

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCheck(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

	VkBufferCopy CopyRegion = { 0, 0, Size };
	vkCmdCopyBuffer(CommandBuffer, StagingBuffer.Buffer, Buffer.Buffer, 1, &CopyRegion);

	VkCheck(vkEndCommandBuffer(CommandBuffer));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;
	VkCheck(vkQueueSubmit(Queue, 1, &SubmitInfo, 0));

	VkCheck(vkDeviceWaitIdle(Device));
}

void UpdateBuffer(VkCommandBuffer CommandBuffer, const SBuffer& Buffer, uint64_t DstOffset, const SBuffer& StagingBuffer, uint64_t SrcOffset, void *Data, uint64_t Size)
{
	Assert(SrcOffset + Size <= StagingBuffer.Size);

	Assert(StagingBuffer.Data);
	memcpy((uint8_t *)StagingBuffer.Data + SrcOffset, Data, Size);

	VkBufferCopy CopyRegion = { SrcOffset, DstOffset, Size };
	vkCmdCopyBuffer(CommandBuffer, StagingBuffer.Buffer, Buffer.Buffer, 1, &CopyRegion);
}

VkDescriptorPool CreateDescriptorPool(VkDevice Device)
{
	VkDescriptorPoolSize PoolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 30 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	CreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	CreateInfo.maxSets = 80;
	CreateInfo.poolSizeCount = ArrayCount(PoolSizes);
	CreateInfo.pPoolSizes = PoolSizes;

	VkDescriptorPool DescriptorPool = 0;
	VkCheck(vkCreateDescriptorPool(Device, &CreateInfo, 0, &DescriptorPool));
	Assert(DescriptorPool);

	return DescriptorPool;
}

VkDescriptorSet CreateDescriptorSet(VkDevice Device, VkDescriptorPool DescriptorPool, VkDescriptorSetLayout DescriptorSetLayout)
{
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescriptorSetAllocateInfo.descriptorPool = DescriptorPool;
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pSetLayouts = &DescriptorSetLayout;

	VkDescriptorSet DescriptorSet = 0;
	VkCheck(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &DescriptorSet));
	Assert(DescriptorSet);

	return DescriptorSet;
}

void UpdateDescriptorSetBuffer(VkDevice Device, VkDescriptorSet DescriptorSet, uint32_t Binding, VkDescriptorType DescriptorType, SBuffer Buffer, VkDeviceSize BufferRange)
{
	VkDescriptorBufferInfo BufferInfo = {};
	BufferInfo.buffer = Buffer.Buffer;
	BufferInfo.range = BufferRange;

	VkWriteDescriptorSet DescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	DescriptorWrite.dstSet = DescriptorSet;
	DescriptorWrite.dstBinding = Binding;
	DescriptorWrite.descriptorCount = 1;
	DescriptorWrite.descriptorType = DescriptorType;
	DescriptorWrite.pBufferInfo = &BufferInfo;
	
	vkUpdateDescriptorSets(Device, 1, &DescriptorWrite, 0, 0);
}

void UpdateDescriptorSetImage(VkDevice Device, VkDescriptorSet DescriptorSet, uint32_t Binding, VkDescriptorType DescriptorType, VkSampler Sampler, VkImageView ImageView, VkImageLayout ImageLayout)
{
	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.sampler = Sampler;
	ImageInfo.imageView = ImageView;
	ImageInfo.imageLayout = ImageLayout;

	VkWriteDescriptorSet DescriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	DescriptorWrite.dstSet = DescriptorSet;
	DescriptorWrite.dstBinding = Binding;
	DescriptorWrite.descriptorCount = 1;
	DescriptorWrite.descriptorType = DescriptorType;
	DescriptorWrite.pImageInfo = &ImageInfo;

	vkUpdateDescriptorSets(Device, 1, &DescriptorWrite, 0, 0);
}

VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(uint32_t Binding, VkDescriptorType DescriptorType, VkShaderStageFlags StageFlags)
{
	VkDescriptorSetLayoutBinding SetLayoutBinding = {};
	SetLayoutBinding.binding = Binding;
	SetLayoutBinding.descriptorType = DescriptorType;
	SetLayoutBinding.descriptorCount = 1;
	SetLayoutBinding.stageFlags = StageFlags;

	return SetLayoutBinding;
}

VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice Device, uint32_t SetBindingsCount, const VkDescriptorSetLayoutBinding* SetBindings, VkDescriptorSetLayoutBindingFlagsCreateInfo* SetLayoutBindingFlags = 0)
{
	VkDescriptorSetLayoutCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	CreateInfo.pNext = SetLayoutBindingFlags;
	CreateInfo.flags = SetLayoutBindingFlags ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
	CreateInfo.bindingCount = SetBindingsCount;
	CreateInfo.pBindings = SetBindings;

	VkDescriptorSetLayout SetLayout = 0;
	VkCheck(vkCreateDescriptorSetLayout(Device, &CreateInfo, 0, &SetLayout));
	Assert(SetLayout);

	return SetLayout;
}

VkPipelineLayout CreatePipelineLayout(VkDevice Device, uint32_t SetLayoutCount, const VkDescriptorSetLayout* SetLayouts, uint32_t PushConstantsSize = 0, VkShaderStageFlags PushConstantsShaderStage = VK_SHADER_STAGE_ALL)
{
	VkPipelineLayoutCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	CreateInfo.setLayoutCount = SetLayoutCount;
	CreateInfo.pSetLayouts = SetLayouts;

	if (PushConstantsSize > 0)
	{
		VkPushConstantRange PushConstants = {};
		PushConstants.stageFlags = PushConstantsShaderStage;
		PushConstants.offset = 0;
		PushConstants.size = PushConstantsSize;

		CreateInfo.pushConstantRangeCount = 1;
		CreateInfo.pPushConstantRanges = &PushConstants;
	}

	VkPipelineLayout PipelineLayout = 0;
	VkCheck(vkCreatePipelineLayout(Device, &CreateInfo, 0, &PipelineLayout));
	Assert(PipelineLayout);

	return PipelineLayout;
}

VkPipeline CreateComputePipeline(VkDevice Device, VkPipelineLayout PipelineLayout, VkShaderModule CS)
{
	VkPipelineShaderStageCreateInfo ShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	ShaderStage.flags;
	ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	ShaderStage.module = CS;
	ShaderStage.pName = "main";
	
	VkComputePipelineCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	CreateInfo.stage = ShaderStage;
	CreateInfo.layout = PipelineLayout;

	VkPipeline ComputePipeline = 0;
	VkCheck(vkCreateComputePipelines(Device, 0, 1, &CreateInfo, 0, &ComputePipeline));
	Assert(ComputePipeline);

	return ComputePipeline;
}