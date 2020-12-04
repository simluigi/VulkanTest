/*======================================================================
Vulkan Tutorial
Author:			Sim Luigi
Last Modified:	2020.12.04

Current Page:
https://vulkan-tutorial.com/en/Drawing_a_triangle/Drawing/Rendering_and_presentation
Drawing: Swap Chain Recreation (Complete!)
Comments: Currently at Render Passes

<< 気づいたこと・メモ >>

＊　Vulkan上のオブジェクトを生成するとき、ほとんど以下の段階で実装します。
	1.) オブジェクト情報構造体「CreateInfo」を作成します。
	2.) 構造体の種類を選択します。（ImageViewの場合、VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO、など）
	3.) 構造体の設定・フラッグを定義します。
	4.) CreateInfo構造体に基づいてオブジェクト自体を生成します。
		例:　VkResult = vkCreateImageView(m_LogicalDevice...)

＊	Vulkan専用のデバッグ機能「Validation Layer」「バリデーションレイヤー」について：
	複雑ですが、簡単にイメージすると、画像編集ソフト（Photoshop、GIMPなど）のレイヤーと同じように重ねているの方が想像しやすいです。
	
	例：処理A
	　→レイヤー①：正常
		→レイヤー②：正常
		　→レイヤー③：エラー（デバッグメッセンジャー表示、エラーハンドリング）


=======================================================================*/

#define GLFW_INCLUDE_VULKAN		// VulkanSDKをGLFWと一緒にインクルードします。
#include <GLFW/glfw3.h>			// replaces #include <vulkan/vulkan.h> and automatically bundles it with glfw include

#include <iostream>
#include <vector>
#include <map>  
#include <optional>			// C++17 and above
#include <cstring>
#include <algorithm>		// std::min/max : chooseSwapExtent()
#include <cstdint>			// UINT32_MAX   : in chooseSwapExtent()
#include <stdexcept>		// std::runtime error、など
#include <cstdlib>			// EXIT_SUCCESS・EXIT_FAILURE
#include <set>
#include <fstream>			// シェーダーのバイナリデータを読み込む　for loading shader binary data

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;		// 同時に処理されるフレームの最大数
										// how many frames should be processed concurrently 

const std::vector<const char*> validationLayers =	// Vulkanのバリデーションレイヤー：SDK上のエラーチェック仕組み
													// Vulkan Validation layers: SDK's own error checking implementation
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions =	// Vulkanエクステンション（今回はSwapChain)
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME					// 誤字を避けるためのマクロ定義
};


#ifdef NDEBUG										// NDEBUG = Not Debug	
const bool enableValidationLayers = false;			// バリデーションレイヤー無効
#else
const bool enableValidationLayers = true;			// バリデーションレイヤー有効
#endif

// デバッグ機能生成・削除関数はクラス外又はスタティックで設定しないといけません。
// create/destroy debug functions must either be a static class function or function outside the class

// デバッグメッセンジャー生成
VkResult CreateDebugUtilsMessengerEXT(	// エクステンション関数：自動的にロードされていません。
										// 関数アドレスをvkGetInstanceProcedureAddr()で特定できます。
										// extension function; not automatically loaded. Need to specify address via	
										// vkGetInstanceProcedureAddr()
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

// デバッグメッセンジャー削除
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

// Vulkan上のあらゆる処理はキューで処理されています。処理によってキューの種類も異なります。
struct QueueFamilyIndices
{
	// optional: 情報が存在しているかしていないかのコンテナ（どちらでも可能）
	// optional: optional contained value; value MAY or MAY NOT be present. (C++17)
	std::optional<uint32_t> graphicsFamily;		// グラフィック系キュー　graphics family	
	std::optional<uint32_t> presentFamily;		// プレゼンテーション（描画）キュー　presentation family

	bool isComplete()							// キューの各値がちゃんと存在しているか　check if value exists for all values
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// SwapChain詳細
// Swap Chain Details
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// Main class
// メインクラス
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

	GLFWwindow*					m_Window;			// WINDOWSではなくGLFW;　クロスプラットフォームの実装

	VkInstance					m_Instance;			// インスタンス：アプリケーションとSDKのつながり
	VkDebugUtilsMessengerEXT	m_DebugMessenger;	// デバッグコールバック
	VkSurfaceKHR				m_Surface;			// GLFW -> Windows System Integration -> Window

	VkPhysicalDevice			m_PhysicalDevice = VK_NULL_HANDLE;	// 物理デバイス（グラフィックスカード）

	VkDevice					m_LogicalDevice;	// 物理デバイスとのシステムインターフェース

	VkQueue						m_GraphicsQueue;	// グラフィックス専用キュー
	VkQueue						m_PresentQueue;		// プレゼンテーション専用キュー

	VkSwapchainKHR				m_SwapChain;				// 表示する予定の画像のキュー
	std::vector<VkImage>		m_SwapChainImages;			// キュー画像
	VkFormat					m_SwapChainImageFormat;		// 画像フォーマット
	VkExtent2D					m_SwapChainExtent;			// extent : 画像レゾルーション（通常、ウィンドウと同じ）

	std::vector<VkImageView>	m_SwapChainImageViews;		// VkImageのハンドル；画像を使用する際にアクセスする（ビューそのもの）
	std::vector<VkFramebuffer>	m_SwapChainFramebuffers;	// SwapChainのフレームバッファ

	VkRenderPass				m_RenderPass;				// レンダーパス
	VkPipelineLayout			m_PipelineLayout;			// グラフィックスパイプラインレイアウト
	VkPipeline					m_GraphicsPipeline;			// グラフィックスパイプライン自体

	// Vulkanのあらゆるコマンド（描画、メモリ転送）などが関数で直接呼ばれません。オブジェクトと同じように
	// コマンドバッファーを生成して、そのコマンドをコマンドバッファーに登録（格納）します。
	// 複雑なセットアップの代わりに、コマンドが事前に処理できます。
	VkCommandPool					m_CommandPool;		// CommandPool : コマンドバッファーのメモリ管理
	std::vector<VkCommandBuffer>	m_CommandBuffers;	// CommandPoolを削除された同時にコマンドバッファを削除されますので
														// コマンドバッファーのクリーンアップは不要です。
	// Semaphore：簡単に「シグナル」。処理を同期するために利用します。
	// Fence: GPU-CPUの間の同期機能；ゲート見たいなストッパーである。
	std::vector<VkSemaphore>		m_ImageAvailableSemaphores;		// イメージ描画準備完了セマフォ
	std::vector<VkSemaphore>		m_RenderFinishedSemaphores;		// レンダリング完了セマフォ
	std::vector<VkFence>			m_InFlightFences;				// 起動中のフェンス
	std::vector<VkFence>			m_ImagesInFlight;				// 処理中の画像
	size_t							m_CurrentFrame = 0;				// 現在こフレームカウンター
	bool							m_FramebufferResized = false;	// ウウィンドウサイズが変更したか

	void initWindow()
	{
		glfwInit();			// GLFW初期化

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// OPENGLコンテクストを作成しない！
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);			// ユーザーでウィンドウリサイズ有効

		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);		// 上記のパラメータでウィンドウを生成します
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->m_FramebufferResized = true;
	}

	// Vulkanの初期化
	void initVulkan()
	{
		createInstance();			// インスタンス生成			
		setupDebugMessenger();		// デバッグコールバック設定
		createSurface();			// ウインドウサーフェス生成
		pickPhysicalDevice();		// Vulkan対象グラフィックスカードの選択
		createLogicalDevice();		// グラフィックスカードとインターフェースするデバイス設定
		createSwapChain();			// SwapChain生成
		createImageViews();			// SwapChain用の画像ビュー生成
		createRenderPass();			// レンダーパス
		createGraphicsPipeline();	// グラフィックスパイプライン生成
		createFramebuffers();		// フレームバッファ生成
		createCommandPool();		// コマンドバッファーを格納するプールを生成
		createCommandBuffers();		// コマンドバッファー生成
		createSyncObjects();
	}

	// メインループ
	void mainLoop()
	{
		while (glfwWindowShouldClose(m_Window) == false)
		{
			glfwPollEvents();	// Update/event checker; also later : render a single frame
			drawFrame();
		}
		
		vkDeviceWaitIdle(m_LogicalDevice);	// let logical device finish operations before exiting the main loop 
	}

	// SwapChainを再生成する前に古いSwapChainを削除する関数
	// before recreating swap chain, call this to clean up older versions of it
	void cleanupSwapChain()
	{
		for (VkFramebuffer framebuffer : m_SwapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);
		}

		// Frees up command buffers without destroying the command pool; 
		// reuse the existing pool to allocate new command buffers
		vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()),
			m_CommandBuffers.data());

		vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);

		for (VkImageView imageView : m_SwapChainImageViews)
		{
			vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(m_LogicalDevice, m_SwapChain, nullptr);
	}

	// 後片づけ
	void cleanup()
	{
		cleanupSwapChain();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{		
			vkDestroySemaphore(m_LogicalDevice, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_LogicalDevice, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_LogicalDevice, m_InFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);

		vkDestroyDevice(m_LogicalDevice, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);	// uninit window
		glfwTerminate();				// uninit glfw
	}

	// SwapChainがウィンドウサーフェスに対応していない場合（ウインドウリサイズ）、SwapChainを再生成
	// 必要があります。SwapChain又はウィンドウサイズに依存するオブジェクトはSwapChainと同時に
	// 再生成しないといけません。

	// Recreate swap chain and all creation functions for the object that depend on the swap chain or the window size.
	// This step is to be implemented when the swap chain is no longer compatible with the window surface;
	// i.e. window size changing (and thus the extent values are no longer consistent)
	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0)						// ウィンドウが最小化状態の場合 window is minimized
		{
			glfwGetFramebufferSize(m_Window, &width, &height);	// 最小化の場外から解除するまでウインドウ処理を一旦停止する
			glfwWaitEvents();									// window paused until window in foreground
		}

		vkDeviceWaitIdle(m_LogicalDevice);		// 使用中のリソースの処理を終了まで待つこと。
												// do not touch resources that are still in use, wait for them to complete.

		cleanupSwapChain();			// SwapChainの後片付け

		createSwapChain();			// SwapChain自体を再生成
		createImageViews();			// SwapChain内の画像に依存する
		createRenderPass();			// SwapChain内の画像のフォーマットに依存する
		createGraphicsPipeline();	// ビューポート、Scissorに依存する
		createFramebuffers();		// SwapChain内の画像に依存する
		createCommandBuffers();		// SwapChain内の画像に依存する
	}

	// インスタンス生成 （Vulkanドライバーに重要な情報を渡す）
	// may provide useful information to the Vulkan driver
	void createInstance()	
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) // デバッグモードの設定でバリデーションレイヤー機能がサポートされない場合
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};	// Vulkanアプリケーション情報構造体を生成 application info struct
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

		// デバッグモードオンの場合、バリデーションレイヤー名を定義する
		// if validation layers is on (debug mode), include validation layer names in instantiation
		if (enableValidationLayers)		
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

		// 構造体による情報に基づいてインスタンスを生成します。
		// Create Instance based on the info struct
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);	
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	// デバッグコールバック
	// messageSeverity: エラーの重要さ；比較オペレータで比べられます。
	// 例：if (messageSeverity >= VK_DEUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) の場合、そのWARNINGのレベル以上のみを表示する。

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,		
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,		// VkDebugUtilsMesengerCallbackDataEXT メッセージ自体の構造体
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
		if (enableValidationLayers == false)	// デバッグモードではない場合、無視　only works in debug mode
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)	// nullptr = optional allocator callback
		{
			throw std::runtime_error("Failed to set up debug messenger!");
		}
	}

	// サーフェス生成（GLFW）
	// Surface Creation
	void createSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	// 物理デバイス（グラフィックスカードを選択）
	// Select compatible GPU
	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);	// Vulkan対応のデバイス（GPU)を数える
		if (deviceCount == 0)		// 見つからなかった場合、エラー表示
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);						// 数えたGPUに基づいて物理デバイスベクトルを生成
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());	// デバイス情報をベクトルに代入

		for (const VkPhysicalDevice device : devices)		// 見つかったデバイスはやろうとしている処理に適切かどうかを確認します
		{													// evaluate each physical device if suitable for the operation to perform
			if (isDeviceSuitable(device) == true)
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	// ロジカルデバイス生成
	void createLogicalDevice()				
	{
		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);	// ロジカルデバイスキュー準備　Preparing logical device queue

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;		// ロジカルデバイスキュー生成情報
		std::set<uint32_t> uniqueQueueFamilies =
		{ indices.graphicsFamily.value(), indices.presentFamily.value() };	//　キュー種類（現在：グラフィックス、プレゼンテーション）

		float queuePriority = 1.0f;		// 優先度；0.0f（低）～1.0f（高）
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};


		VkDeviceCreateInfo createInfo{};	// ロジカルデバイス生成情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();	// パラメータのポインター　pointer to the logical device queue info (above)

		createInfo.pEnabledFeatures = &deviceFeatures;		// currently empty (will revisit later)

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// デバイス専用バリデーションレイヤーとインスタンス専用バリデーションレイヤーの区別は最新バージョンでなくなりました。
		// が、念のため古いバージョンを使っている場合、コンパティビリティのために以下の設定を定義します。

		// Setting up device validation layers: This part is actually only for compatibility with older versions of Vulkan.
		// Recent implementations have removed the distinction between instance and device-specific validation layers so
		// enabledLayerCount and ppEnabledLayerNames are ignored by up-to-date implementations.
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		// 上記のパラメータに基づいて実際のロジカルデバイスを生成する
		// Creating the logical device itself
		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);		//　グラフィックスキュー graphics queue
		vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily.value(), 0, &m_PresentQueue);		//　プレゼンテーションキュー presentation queue
	}

	// SwapChain: 画像の切り替え
	void createSwapChain()
	{
		// GPUのSwapChainサポート情報を読み込む
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// +1の理由：
		// SwapChainから次のイメージを獲得する前に、たまにはドライバーの内部処理の終了を待たないといけません。
		// そこで、余裕な1つ（以上）のイメージを用意できるように設定するのはおすすめです。

		// why +1?
		// sometimes we may have to wait on the driver to perform internal operations before we can acquire
		// another image to render to. Therefore it is recommended to request at least one more image than the minimum.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;


		if (swapChainSupport.capabilities.maxImageCount > 0					// zero here means there is no maximum!
			&& imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};		// SwapChain生成情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;	// stereoscopic3D 以外なら「１」 
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // SwapChainを利用する処理　what operations to use swap chain for 

		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)	//　グラフィックスとプレゼンテーションキューが異なる場合
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; 
			// イメージ共有モード　Exclusive （排他的）：1つのイメージが複数のキューに所属できます。
			// concurrent: can be shared across multiple queue families
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else  // グラフィックスとプレゼンテーションキューが同じ（現代のデバイスはこういう風に設定されています）。
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	
			// イメージ共有モード　Exclusive （排他的）：1つのイメージが1つのキュー種類しか所属しません。他のキュー種類に使用すると
			// 所属をはっきり切り替えないといけません。パフォーマンス面で最適。
			// exclusive: image is owned by one queue family at a time;	must be explicitly transferred before use in another queue
			// family. Offers the best performance.
			createInfo.queueFamilyIndexCount = 0;		// optional
			createInfo.pQueueFamilyIndices = nullptr;	// optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// アルファチャンネル：不透明
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;	// TRUE : オクルージョンされたピクセルの色を無視　don't care about color of obscured pixels

		createInfo.oldSwapchain = VK_NULL_HANDLE;	// 一つのSwapChainのみ　for now, only create one swap chain

		// 上記の情報に基づいてSwapChainを生成します。
		if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, nullptr);	// SwapChainのイメージ数を特定
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, m_SwapChainImages.data());	// 情報をm_SwapChainImagesに代入

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;
	}

	void createImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());		// イメージカウントによってベクトルサイズを変更する
																	// allocate enough size to fit all image views 					
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};		// イメージビュー生成構造体
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;	// 1D/3D/キューブマップなど
			createInfo.format = m_SwapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// デフォルト設定　default settings
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			
			// 上記の構造体の情報に基づいて実際のイメージビューを生成する
			if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create image views!");
			}
		}
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapChainImageFormat;	// format of color attachment must match format of swap chain images
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	// no multisampling yet

		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		// what to do with data before rendering

		// VK_ATTACHMENT_LOAD_OP_LOAD		: Preserve existing contents of attachment
		// VK_ATTACHMENT_LOAD_OP_CLEAR		: Clear the values to a constant at the start (in this case, clear to black)
		// VK_ATTACHMENT_LOAD_OP_DONT_CARE	: Existing contents are undefined; we don't care about them

		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;		// what to do with data after rendering

		// VK_ATTACHMENT_STORE_OP_STORE		: Rendered contents will be stored in memory and can be read later
		// VK_ATTACHMENT_STORE_OP_DONT_CARE	: Contents of the framebuffer will be undefined ater the rendering operation

		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// not using stencil buffer at the moment
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// which layout image will have prior to render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// layout to automatically transition to after pass finishes

		// Common Layouts:
		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation


		VkAttachmentReference colorAttachmentReference{};
		colorAttachmentReference.attachment = 0;	// attachment reference index; 0 = first index (only one attachment for now)
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;	// subpass index 0 (our first and only subpass)
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		// 上記の構造体の情報に基づいて実際のレンダーパスを生成する
		if (vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void createGraphicsPipeline()
	{
		const std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
		const std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// Shader Stages

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;		// enum for programmable stages in Graphics Pipeline: Intro
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";					// entrypoint function name

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Fixed Functions
		// 1.) Vertex Input 

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		// 2.) Input Assembly

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// 3.) Viewports and Scissors

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChainExtent.width;
		viewport.height = (float)m_SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};						// scissor rectangle: defines in which regions pixels will actually be stored
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChainExtent;		// currently set to draw the entire framebuffer

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// 4.) Rasterizer

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;			// VK_TRUE: fragments beyond near and far planes are clamped to them
														// rather than discarding them; useful with shadow mapping
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;	// FILL = fill area of polygon with fragments
														// LINE = polygon edges drawn as lines (i.e. wireframe)
														// POINT = polygon vertices are drawn as points
														// using anything other than FILL requires enabling a GPU feature
		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;		// determines front-facing by vertex order (CLOCKWISE or COUNTER_CLOCKWISE) 
		rasterizer.depthBiasEnable = VK_FALSE;		// VK_TRUE: adjusting depth values i.e. for shadow mapping
		rasterizer.depthBiasConstantFactor = 0.0f;	// optional
		rasterizer.depthBiasClamp = 0.0f;			// optional
		rasterizer.depthBiasSlopeFactor = 0.0f;		// optional

		// 5.) Multisampling 

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;				// optional
		multisampling.pSampleMask = nullptr;				// optional
		multisampling.alphaToCoverageEnable = VK_FALSE;		// optional
		multisampling.alphaToOneEnable = VK_FALSE;			// optional

		// 6.) Depth and Stencil Testing (revisit later)

		// 7.) Color Blending

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
											| VK_COLOR_COMPONENT_G_BIT 
											| VK_COLOR_COMPONENT_B_BIT 
											| VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				// optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				// optional

		// Alpha Blending Implementation: (pseudocode)
		// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
		// finalAlpha.a = newAlpha.a;
		//
		//colorBlendAttachment.blendEnable = VK_TRUE;
		//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		colorBlending.logicOpEnable = VK_FALSE;			// VK_TRUE for bitwise combination color blending mode
		colorBlending.logicOp = VK_LOGIC_OP_COPY;		// optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;		// optional
		colorBlending.blendConstants[1] = 0.0f;		// optional
		colorBlending.blendConstants[2] = 0.0f;		// optional
		colorBlending.blendConstants[3] = 0.0f;		// optional

		// 8.) Dynamic State (revisit later)

		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;


		// 9.) Pipeline Layout (empty for now, revisit later)

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;				// optional
		pipelineLayoutInfo.pSetLayouts = nullptr;			// optional
		pipelineLayoutInfo.pushConstantRangeCount = 0;		// optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr;	// optional

		if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// 10.) Graphics Pipeline creation: Putting everything together to create the pipeline!!
		//		Combine all the objects: Shader Stages, Fixed-function states, Pipeline Layout, Render Passes

		// ...but before that, we need to create a Pipeline info struct:

		// reference the array of VkPipelineShaderStageCreateInfo structs
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		// reference all the structures described in the fixed-function stage
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;			// optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;				// optional

		// pipelineInfo.flags - none at the moment(see below: basePipelineHandle and basePipelineIndex)

		// pipeline layout: Vulkan handle, NOT struct pointer
		pipelineInfo.layout = m_PipelineLayout;

		// reference to the render pass and the subpass index
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		// these values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag 
		// is also specified in the flags field of VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// optional; derive new pipeline from existing pipeline
		pipelineInfo.basePipelineIndex = -1;				// optional; derive new pipeline from existing pipeline index

		// Creating the actual graphics pipeline:
		// Second argument (VK_NULL_HANDLE): pipeline cache (revisit later)
		if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);
	}

	void createFramebuffers()	
	{
		m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = 
			{
				m_SwapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;			// framebuffer must be compatible with render pass
			framebufferInfo.attachmentCount = 1;				// render pass pAtachment array
			framebufferInfo.pAttachments = attachments;			// render pass pAtachment array
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;							// swap chain images are single images = value 1

			if (vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();	// drawing commands: graphics queue family chosen
		poolInfo.flags = 0;		// optional, revisit later

		if (vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool!");
		}
	}

	void createCommandBuffers()
	{
		m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;					
		// PRIMARY:		can be submitted to queue for execution, but cannot be called from other command buffers
		// SECONDARY:	cannot be submitted directly, but can be called from primary command buffers

		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers!");
		}

		// Starting command buffer recording
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;	// optional
			beginInfo.pInheritanceInfo = nullptr;		// optional, only for secondary command buffers (which state to inherit from)

			if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to begin recording command buffer!");
			}

			// Starting a render pass
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_SwapChainExtent;	// match render area to size of attachments for best performance

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;						// clear values for VK_ATTACHMENT_LOAD_OP_CLEAR
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);
			// first	: commandBuffer
			// second	: vertexCount: even without vertex buffer, still drawing 3 vertices (triangle)
			// third	: instanceCount: used for instanced rendering, otherwise 1)
			// fourth	: firstInstance: used as offset for instanced rendering, defines lowest value of gl_InstanceIndex 

			vkCmdEndRenderPass(m_CommandBuffers[i]);

			if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to record command buffer!");
			}
		}
	}

	void createSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImagesInFlight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS
			 || vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS
			 || vkCreateFence(m_LogicalDevice, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	void drawFrame()
	{
		vkWaitForFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		
		VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		// check if swap chain is out of date
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}


		// check if a previous frame is using this image (i.e. there is its frame to wait on)
		if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_LogicalDevice, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		// mark the image as now being in use by this frame
		m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;	// which semaphore to wait on before execution
		submitInfo.pWaitDstStageMask = waitStages;		// which stage(s) of the pipeline to wait

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores; // which semaphores to signal once command buffer(s) have finished execution

		vkResetFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame]);

		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		// submit the result back to the swap chain to have it show on screen
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.pResults = nullptr;		// optional; array of VkResult values to check for each swap chain if 
											// presentation was successful (when using multiple swap chains).

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR 
		 || result == VK_SUBOPTIMAL_KHR
		 || m_FramebufferResized == true)
		{
			m_FramebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;	// advance to next frame
	}

	VkShaderModule createShaderModule(const std::vector<char>& code)	// creates VkShaderModule from buffer 
	{	
		VkShaderModuleCreateInfo createInfo{};	// specify a pointer to the bugger with the bytecode and length
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();

		// size of bytecode is in bytes, but bytecode pointer is uint32_t pointer rather than a char pointer.
		// thus, we need to recast the pointer as below.
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());	

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB					// if sRGB format is found 
				&& availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)	// B8G8R8A8 is one of the most common ones
			{
				return availableFormat;
			}
		}
		return  availableFormats[0];	// otherwise, just use the first format that is specified
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)	// used to implement triple buffering (less latency)
			{
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; // if VK_PRESENT_MODE_MAILBOX_KHR is not available, use a guaranteed available mode
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites)	// extent = resolution of the swap chain images
	{
		//Vulkan works with pixel units, but the screen resolution (WIDTH, HEIGHT) is in screen coordinates.
		//Use glfwGetFramebufferSize to query the resolution of the window in pixel before matching to min/max image extent

		//maximum value of uint32_t; special value to indicate that we will be picking the resolution that best matches the window
		//within the minImageExtent and maxImageExtent

		if (capabilites.currentExtent.width != UINT32_MAX)
		{
			return capabilites.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(m_Window, &width, &height);

			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(capabilites.minImageExtent.width,
				std::min(capabilites.maxImageExtent.width, actualExtent.width));

			actualExtent.height = std::max(capabilites.minImageExtent.height,
				std::min(capabilites.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);	//basic surface capabilities

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);		// query supported surface formats
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);	// resize to hold all available formats
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);		// currently set to find queue family that supports VK_QUEUE_GRAPHICS_BIT

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;

		// isComplete(): returns true for GPUs with geometry shader support
		// sample if wanting to narrow down to geometry shaders:
		// return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;	

		// checkDeviceExtensionSupport: currently set up for swapchain extension

		// swapChainAdequate: at least one supported image format and one supported presentation mode given the window surface
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const VkExtensionProperties& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);	// erase if required extension is in the vector
		}
		bool isEmpty = requiredExtensions.empty();
		//std::cout << "\nisEmpty = " << isEmpty << std::endl;
		return isEmpty;	// if all the required extension were present (and thus erased), returns true
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());	// assign data to queueFamilies

		int i = 0;
		for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			if (presentSupport = VK_TRUE)		// do not use "== true" for VkBool32
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())	// if queueFamily is found, exit early
			{
				break;
			}
			i++;
		}
		return indices;
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

	static std::vector<char> readFile(const std::string& fileName)
	{
		std::ifstream file(fileName, std::ios::ate			// ate = start reading at EOF 
			| std::ios::binary);	// binary = read file as binary file (avoid text transformations)

		if (file.is_open() == false)
		{
			throw std::runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();		// telg() = returns input stream position. 
		std::vector<char> buffer(fileSize);			// reading at EOF essentially gives us the size of the file for the buffer

		file.seekg(0);								// return to beginning of file
		file.read(buffer.data(), fileSize);			// read(x, y) = read up to count y and assign to buffer x
													// in this case, read all the bytes at once

		// std::cout << fileSize << std::endl;		// check file byte size with actual file (properties)

		file.close();

		return buffer;
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