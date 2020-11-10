/*======================================================================
Vulkan Tutorial
Author:			Sim Luigi
Last Modified:	2020.11.10

Current Page:
https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
Selecting a Physical Device
=======================================================================*/

#define GLFW_INCLUDE_VULKAN		// replaces #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>			// and automatically bundles it with glfw include

#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>		// reporting and propagating errors
#include <cstdlib>			// EXIT_SUCCESS and EXIT_FAILURE

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const std::vector<const char*> validationLayers =	// vector of Vulkan validation layers 
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG	// NDEBUG = Not Debug	
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif


// create/destroy debug functions must either be a static class function or function outside the class

VkResult CreateDebugUtilsMessengerEXT(		// extension function; not automatically loaded. Need to specify address via vkGetInstanceProcedureAddr
	VkInstance instance,	
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

class HelloTriangleApplication 
{	
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	GLFWwindow*		m_Window;
	VkInstance		m_Instance;

	VkDebugUtilsMessengerEXT	m_DebugMessenger;

	void initWindow()
	{
		glfwInit();			// initialize glfw

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// do not create an OpenGL context
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);			// no window resizing for now
		
		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);		// create window with above hint conditions
	}
	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
	}
	void mainLoop()
	{
		while (glfwWindowShouldClose(m_Window) == false)
		{
			glfwPollEvents();	// Update/event checker; also later : render a single frame
		}
	}
	void cleanup()
	{
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		}

		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);	// uninit window
		glfwTerminate();				// uninit glfw
	}

	void createInstance()	// may provide useful information to the Vulkan driver
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};								// Vulkan application info struct
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		std::vector<const char*> extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers)		// if validation layers is on (debug mode), include validation layer names in instantiation
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);	// store result if desired, otherwise call straight w/o result variable
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,			// severity of the message; can actually be used in comparison operation
																		// i.e. if (messageSeverity >= VK_DEUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) { //show }
		VkDebugUtilsMessageTypeFlagsEXT messageType,					
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,		// VkDebugUtilsMesengerCallbackDataEXT struct containing message itself
		void* pUserData)												// pointer that was specified during the setup of callback, allowing you to pass your own data to it
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;	// boolean indicating if Vulkan call that triggered the validation layer message should be aborted
							// if callback returns true, call is aborted with VK_ERROR_VALIDATION_FAILED_EXT error. 
							// Should always return VK_FALSE unless testing validation layer itself.
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};	// empty struct
		
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;

	}

	void setupDebugMessenger()
	{
		if (enableValidationLayers == false)	// only works in debug mode
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)	// nullptr = optional allocator callback
		{
			throw std::runtime_error("Failed to set up debug messenger!");
		}
	}

	std::vector<const char*> getRequiredExtensions()	// returns the list of extensions based on whether validation layers are enabled or not
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers == true)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool checkValidationLayerSupport()												// check if ALL the layers in validationLayers exist in the availableLayers list
	{
		uint32_t layerCount;										
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);					// get number of validation layers

		std::vector<VkLayerProperties> availableLayers(layerCount);					// create vector of validation layers based on number 
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());	// query data of validation layers to the vector

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const VkLayerProperties& layerProperties : availableLayers)		// go through all available layers
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)				// if layer name matches existing validation layer name
				{
					layerFound = true;												// set layerFound to true (return value)
					break;
				}
			}

			if (layerFound == false)	// as long as layer is found in the list, loop will continue until all validation layers have been verified.
			{
				return false;
			}
		}
		return true;
	}	
};


int main(void)
{
	HelloTriangleApplication app;
	try
	{
		app.run();
	} 
	catch (const std::exception& e)
	{ 
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}