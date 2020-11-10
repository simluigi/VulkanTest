/*======================================================================
Vulkan Tutorial
Author:			Sim Luigi
Last Modified:	2020.11.08

Current Page:
https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Validation_layers
Message Callback portion
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

		VkApplicationInfo appInfo{};	// Vulkan application info struct
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t		glfwExtensionCount = 0;
		const char**	glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);	// GLFW function that returns extension(s) it needs

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);	// store result
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);	// get the number of extensions

		std::vector<VkExtensionProperties> extensions(extensionCount);				// create a vector of extensions
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());	// query the extension details to the vector

		std::cout << "available extensions:\n";
		for (const VkExtensionProperties& extension : extensions)
		{
			std::cout << 't' << extension.extensionName << 'n';
		}

		if (enableValidationLayers)		// if validation layers is on (debug mode), include validation layer names in instantiation
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
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