/*======================================================================
Vulkan Tutorial
Author:			Sim Luigi
Last Modified:	2020.12.09

Current Page:
https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_pool_and_sets
Uniform Buffers: Descriptor Pool and Sets(complete!)

2020.12.06:		今までのソースコードに日本語コメント欄を追加しました。

<< 気づいたこと・メモ >>　※間違っている場合、教えていただければ幸いです。

＊　VulkanAPIのネーミングコンベンションについて：
	
	小文字「 k 」： 関数          vkCreateInstance()   
	大文字「 K 」： データ型      VkInstance
	全て大文字　 ： マクロ        VK_KHR_SWAPCHAIN_EXTENSION_NAME

	Vk/Vk/VK: Vulkan
	KHR     : Khronos （Vulkan、OpenCLのディヴェロッパー

＊　Vulkan上のオブジェクトを生成するとき、ほとんど以下の段階で実装します。

                                                                  例：インスタンス	
	1.) オブジェクト情報構造体「CreateInfo」を作成します。        VkInstanceCreateInfo createInfo{};
	2.) 構造体の種類を選択します。                                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	3.) 構造体の設定・フラッグを定義します。                      createInfo.flags = 0;
	4.) CreateInfo構造体に基づいてオブジェクト自体を生成します。  vkCreateInstance(&createInfo, nullptr, &m_Instance):

＊	Vulkan専用のデバッグ機能「Validation Layer」「バリデーションレイヤー」について：
	複雑ですが、簡単にイメージすると、画像編集ソフト（Photoshop、GIMPなど）のレイヤーと同じように重ねているの方が想像しやすいです。
	
	例：処理A
	　→レイヤー①：正常
		→レイヤー②：正常
		　→レイヤー③：エラー（デバッグメッセンジャー表示、エラーハンドリング）

＊　Vulkanのシェーダーコードについて：
	GLSLやHLSLなどのハイレベルシェーダー言語（HLSLはまさにHighLevelShadingLanguage)と違って、
	VulkanのシェーダーコードはSPIR-Vというバイトコードフォーマットで定義されています。

	GPUによるハイレベルシェーダーコードの読み方が異なる場合があり、
	あるGPUでシェーダーがうまく動いても他のGPUで同じ結果が出せるとは限りません。
	機械語のローレベルランゲージではないが、HLSLと機械語の間のインターメディエイトレベル
	言語としてクロスプラットフォーム処理に向いています。

	なお、GLSLなどのシェーダーはSPIR-Vに変換するツールを使えばVulkanで使えるようになるはずです。

＊	Vulkanのあらゆるコマンド（描画、メモリ転送）などが関数で直接呼ばれません。オブジェクトと同じように
	コマンドバッファーを生成して、そのコマンドをコマンドバッファーに登録（格納）します。
	複雑なセットアップの代わりに、コマンドが事前に処理できます。

＊	フラグメントシェーダー：　ピクセルシェーダーと同じです。

＊　VulkanのBool関数に限って、true・falseではなく、VK_TRUE・VK_FALSEを使用することです。
	同じく、Vulkan専用 VK_SUCCESS、VK_ERROR_○○コールバックも用意されています。

＊　ステージングバッファーの使用の理由
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT： 一番理想的なメモリーフォーマットが、ローカルメモリーが直接アクセスできません。
	そこで、そのメモリータイプが使えるCPU上の「ステージングバッファー」を生成し、頂点配列（データ）に渡します。
	そしてステージングバッファーの情報をローカルメモリーに格納されている頂点バッファーに移します。

=======================================================================*/

#define GLFW_INCLUDE_VULKAN    // VulkanSDKをGLFWと一緒にインクルードします。
#include <GLFW/glfw3.h>        // replaces #include <vulkan/vulkan.h> and automatically bundles it with glfw include

#define GLM_FORCE_RADIANS                   // glm::rotate関数をラジアンで処理する設定
#include <glm/glm.hpp>                      // 
#include <glm/gtc/matrix_transform.hpp>     // モデルトランスフォーム              glm::rotate, glm::scale  
                                            // ビュートランスフォーム              glm::lookAt
                                            // プロジェクショントランスフォーム    glm::perspective

#include <chrono>       // 時間管理；フレームレートに関わらずジオメトリーが90°回転させます
                        // time manager: will allow us to rotate geometry 90 degress regardless of frame rate

#include <iostream>     // 基本I/O
#include <vector>       // std::vector
#include <set>          // std::set
#include <array>        // std::array
#include <map>          // std::map
#include <cstring>      // strcpy, など
#include <optional>     // C++17 and above
#include <algorithm>    // std::min/max : chooseSwapExtent()
#include <cstdint>      // UINT32_MAX   : in chooseSwapExtent()
#include <stdexcept>    // std::runtime error、など
#include <cstdlib>      // EXIT_SUCCESS・EXIT_FAILURE : main()
#include <fstream>      // シェーダーのバイナリデータを読み込む　for loading shader binary data
#include <glm/glm.hpp>  // glm::vec2, vec3 : Vertex構造体

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// 同時に処理されるフレームの最大数 
// how many frames should be processed concurrently 
const int MAX_FRAMES_IN_FLIGHT = 2;		

// Vulkanのバリデーションレイヤー：SDK上のエラーチェック仕組み
// Vulkan Validation layers: SDK's own error checking implementation
const std::vector<const char*> validationLayers =				
{
	"VK_LAYER_KHRONOS_validation"
};

// Vulkanエクステンション（今回はSwapChain)
const std::vector<const char*> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME    // 誤字を避けるためのマクロ定義
};

// NDEBUG = Not Debug	
#ifdef NDEBUG					
// バリデーションレイヤー無効
const bool enableValidationLayers = false;
#else
// バリデーションレイヤー有効
const bool enableValidationLayers = true;
#endif

// デバッグメッセンジャー生成
// エクステンション関数：自動的にロードされていません。
// 関数アドレスをvkGetInstanceProcedureAddr()で特定できます。
// extension function; not automatically loaded. Need to specify address via	
// vkGetInstanceProcedureAddr()
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr
	(instance, "vkCreateDebugUtilsMessengerEXT");
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

struct Vertex
{
	// glmライブラリーがシェーダーコードに合ってるC++データ型を用意してくれます。
	glm::vec2 pos;		
	glm::vec3 color;

	// シェーダー情報がGPUに読み込まれた後、頂点シェーダーに渡す関数2つ
	// Two functions to tell Vulkan how to pass the shader data once it's been uploaded to the GPU
 
	// ① 頂点バインディング：情報の読み込む率　（stride, 頂点ごと・インスタンスごと）
	// Vertex Binding: Which rate to load data from memory throughout the vertices (stride, by vertex or by instance)
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;                                // 配列1つだけ（インデックス 0）
		bindingDescription.stride = sizeof(Vertex);                    // stride: 次の要素までのバイト数  number of bytes from one entry to the next
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;    // RATE_INSTANCE（インスタンスレンダリング）

		return bindingDescription;
	}

	// ②アトリビュートデスクリプター：頂点バインディングから読み込んだ頂点データの扱い
	// Attribute Descriptor: how to handle vertex input
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		// attributeDescriptions[0]: 位置情報　Position
		// bindingDescriptionと同じ値: 頂点シェーダー (location = 0) in	
		attributeDescriptions[0].binding = 0;							
		attributeDescriptions[0].location = 0;																							
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;		
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		// フォーマットについて：	// 普通に以下の組み合わせで宣言されています。															
		// float : VK_FORMAT_R32_SFLOAT
		// vec2  : VK_FORMAT_R32G32_SFLOAT
		// vec3  : VK_FORMAT_R32G32B32_SFLOAT
		// vec4  : VK_FORMAT_R32G32B32A32_SFLOAT
																		
		// カラータイプ（SFLOAT、UINT、SINT)もShaderInputに合わせる必要があります。
		// ivec2  : VK_FORMAT_R32G32_SINT		（int 32ビット vec2)
		// uvec4  : VK_FORMAT_R32G32B32A32_UINT （unsigned 32ビット vec4)
		// double : VK_FORMAT_R64_SFLOAT		（ダブルプレシジョン64ビット float
		
		// attributeDescriptions[1]: カラー情報　Color　（上記とほぼ同じ）
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

// UBO (UniformBufferObject): マトリクス変換情報・MVP Transform
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};


// 頂点データインターリーブ
// Vertex Attribute Interleave
const std::vector<Vertex> vertices = 
{
	{{-0.5f, -0.5f,}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f,}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f,}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f,}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = // uint16_t for now since using less than 65535 unique vertices
{
	0, 1, 2, 2, 3, 0
};

// Vulkan上のあらゆる処理はキューで処理されています。処理によってキューの種類も異なります。
struct QueueFamilyIndices
{
	// optional: 情報が存在しているかしていないかのコンテナ（どちらでも可能）
	// optional contained value; value MAY or MAY NOT be present. (C++17)

	std::optional<uint32_t> graphicsFamily;   // グラフィック系キュー
	std::optional<uint32_t> presentFamily;    // プレゼント（描画）キュー

	// キューの各値がちゃんと存在しているか　check if value exists for all values
	bool isComplete()							
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

	GLFWwindow* m_Window;     // WINDOWSではなくGLFW;　クロスプラットフォーム対応
	VkInstance  m_Instance;   // インスタンス：アプリケーションとSDKのつながり
	
	VkDebugUtilsMessengerEXT m_DebugMessenger;   // デバッグコールバック
	
	VkSurfaceKHR m_Surface;    // GLFW -> WSI (Windows System Integration) -> ウィンドウ生成

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;            // 物理デバイス（GPU・グラフィックスカード）
	VkDevice         m_LogicalDevice;                              // 物理デバイスとのシステムインターフェース

	VkQueue m_GraphicsQueue;    // グラフィックス専用キュー
	VkQueue m_PresentQueue;     // プレゼント（描画）専用キュー

	VkSwapchainKHR m_SwapChain;                // 表示する予定の画像のキュー
	std::vector<VkImage> m_SwapChainImages;	   // キュー画像
	VkFormat m_SwapChainImageFormat;           // 画像フォーマット
	VkExtent2D m_SwapChainExtent;              // extent : 画像レゾルーション（通常、ウィンドウと同じ）

	std::vector<VkImageView> m_SwapChainImageViews;        // VkImageのハンドル；画像を使用する際にアクセスする（ビューそのもの）
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;    // SwapChainのフレームバッファ

	VkRenderPass             m_RenderPass;             // レンダーパス
	VkDescriptorSetLayout    m_DescriptorSetLayout;    // でスクリプターセットレイアウト
	VkPipelineLayout         m_PipelineLayout;         // グラフィックスパイプラインレイアウト
	VkPipeline               m_GraphicsPipeline;       // グラフィックスパイプライン自体

	VkCommandPool                   m_CommandPool;       // CommandPool : コマンドバッファー、そしてその割り当てたメモリ管理、
	std::vector<VkCommandBuffer>    m_CommandBuffers;		

	VkDescriptorPool                m_DescriptorPool;    // DescriptorPool : デスクリプターセット、そしてその割り当てたメモリ管理
	std::vector<VkDescriptorSet>    m_DescriptorSets;

	VkBuffer       m_VertexBuffer;          // 頂点バッファー
	VkDeviceMemory m_VertexBufferMemory;    // 頂点バッファーメモリー割り当て
	VkBuffer       m_IndexBuffer;           // インデックスバッファー
	VkDeviceMemory m_IndexBufferMemory;     // インデックスバッファーメモリー割り当て

	std::vector<VkBuffer>          m_UniformBuffers;
	std::vector<VkDeviceMemory>    m_UniformBuffersMemory;

	// Semaphore：簡単に「シグナル」。処理を同期するために利用します。
	// Fence: GPU-CPUの間の同期機能；ゲート見たいなストッパーである。
	std::vector<VkSemaphore>    m_ImageAvailableSemaphores;    // イメージ描画準備完了セマフォ
	std::vector<VkSemaphore>    m_RenderFinishedSemaphores;    // レンダリング完了セマフォ
	std::vector<VkFence>        m_InFlightFences;              // 起動中のフェンス
	std::vector<VkFence>        m_ImagesInFlight;              // 処理中の画像
	size_t                      m_CurrentFrame = 0;            // 現在こフレームカウンター
	
	bool m_FramebufferResized = false;    // ウウィンドウサイズが変更したか

	void initWindow()
	{
		glfwInit();    // GLFW初期化

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);    // OPENGLコンテクストを作成しない！
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);       // ユーザーでウィンドウリサイズ有効

		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);    // 上記のパラメータでウィンドウを生成します
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
	}

	// ウィンドウリサイズコールバック
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->m_FramebufferResized = true;
	}

	// Vulkanの初期化
	void initVulkan()
	{
		createInstance();			    // インスタンス生成			
		setupDebugMessenger();          // デバッグコールバック設定
		createSurface();                // ウインドウサーフェス生成
		pickPhysicalDevice();           // Vulkan対象グラフィックスカードの選択
		createLogicalDevice();          // グラフィックスカードとインターフェースするデバイス設定
		createSwapChain();              // SwapChain生成
		createImageViews();             // SwapChain用の画像ビュー生成
		createRenderPass();             // レンダーパス
		createDescriptorSetLayout();    // リソースでスクリプターレイアウト 
		createGraphicsPipeline();       // グラフィックスパイプライン生成
		createFramebuffers();           // フレームバッファ生成
		createCommandPool();            // コマンドバッファーを格納するプールを生成
		createVertexBuffer();           // 頂点バッファー生成
		createIndexBuffer();		    // インデックスバッファー生成
		createUniformBuffers();         // ユニフォームバッファー生成
		createDescriptorPool();         // デスクリプターセットを格納するプールを生成
		createDescriptorSets();         // デスクリプターセットを生成
		createCommandBuffers();         // コマンドバッファー生成
		createSyncObjects();            // 処理同期オブジェクト生成
	}

	// メインループ
	void mainLoop()
	{
		while (glfwWindowShouldClose(m_Window) == false)
		{
			glfwPollEvents();    // イベント待機  Update/event checker
			drawFrame();         // フレーム描画
		}
		
		// プログラム終了（後片付け）の前に、既に動いている処理を済ませます。
		// let logical device finish operations before exiting the main loop 
		vkDeviceWaitIdle(m_LogicalDevice);	
	}

	// SwapChainを再生成する前に古いSwapChainを削除する関数
	// before recreating swap chain, call this to clean up older versions of it
	void cleanupSwapChain()
	{
		for (VkFramebuffer framebuffer : m_SwapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_LogicalDevice, framebuffer, nullptr);
		}

		// コマンドプールを削除せずコマンドバッファーを開放。既存のプールを新しいコマンドバッファーで使います。
		// Frees up command buffers without destroying the command pool; reuse the existing pool to allocate new command buffers
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

		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			vkDestroyBuffer(m_LogicalDevice, m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_LogicalDevice, m_UniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);

	}

	// 後片づけ
	// 3つ目のnullptr: 任意のコールバック引数
	void cleanup()
	{
		cleanupSwapChain();

		vkDestroyDescriptorSetLayout(m_LogicalDevice, m_DescriptorSetLayout, nullptr);

		vkDestroyBuffer(m_LogicalDevice, m_IndexBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(m_LogicalDevice, m_VertexBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, m_VertexBufferMemory, nullptr);

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
		while (width == 0 || height == 0)                       // ウィンドウが最小化状態の場合 window is minimized
		{
			glfwGetFramebufferSize(m_Window, &width, &height);  // 最小化の場外から解除するまでウインドウ処理を一旦停止する
			glfwWaitEvents();                                   // window paused until window in foreground
		}

		vkDeviceWaitIdle(m_LogicalDevice);    // 使用中のリソースの処理を終了まで待つこと。do not touch resources that are still in use, wait for them to complete.

		cleanupSwapChain();         // SwapChainの後片付け

		createSwapChain();          // SwapChain自体を再生成
		createImageViews();         // SwapChain内の画像に依存する
		createRenderPass();         // SwapChain内の画像のフォーマットに依存する
		createGraphicsPipeline();   // ビューポート、Scissorに依存する
		createFramebuffers();       // SwapChain内の画像に依存する
		createUniformBuffers();     // SwapChain内の画像に依存する
		createDescriptorPool();     // SwapChain内の画像に依存する
		createDescriptorSets();     // SwapChain内の画像に依存する
		createCommandBuffers();     // SwapChain内の画像に依存する
	}

	// Vulkanインスタンス生成 （Vulkanドライバーに重要な情報を渡します）
	// may provide useful information to the Vulkan driver
	void createInstance()	
	{
		if (enableValidationLayers && !checkValidationLayerSupport())    // デバッグモードの設定でバリデーションレイヤー機能がサポートされない場合
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};    // Vulkanアプリケーション情報構造体を生成
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

		// 上記の構造体の情報に基づいて実際のインスタンスを生成します。
		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
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
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,    // VkDebugUtilsMesengerCallbackDataEXT メッセージ自体の構造体
		void* pUserData)                                              // pointer that was specified during the setup of callback, allowing you to pass your own data to it
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		// boolean indicating if Vulkan call that triggered the validation layer message should be aborted
		// if callback returns true, call is aborted with VK_ERROR_VALIDATION_FAILED_EXT error. 
		// Should always return VK_FALSE unless testing validation layer itself.
		return VK_FALSE;
	}

	// デバッグメッセージ構造体に必要なエクステンションを代入する関数
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};    // デバッグメッセンジャー情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;

	}

	void setupDebugMessenger()
	{
		if (enableValidationLayers == false)    // デバッグモードではない場合、無視します  Only works in debug mode
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
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
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);    // Vulkan対応のデバイス（GPU)を数える
		if (deviceCount == 0)                                             // 見つからなかった場合、エラー表示
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);                     // 数えたGPUに基づいて物理デバイスベクトルを生成
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());   // デバイス情報をベクトルに代入

		for (const VkPhysicalDevice device : devices)     // 見つかったデバイスはやろうとしている処理に適切かどうかを確認します
		{                                                 // evaluate each physical device if suitable for the operation to perform
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
		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);    // ロジカルデバイスキュー準備　Preparing logical device queue

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;               // ロジカルデバイスキュー生成情報
		std::set<uint32_t> uniqueQueueFamilies =
		{ indices.graphicsFamily.value(), indices.presentFamily.value() };	 // キュー種類（現在：グラフィックス、プレゼンテーション）

		float queuePriority = 1.0f;    // 優先度；0.0f（低）～1.0f（高）
		
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

		VkDeviceCreateInfo createInfo{};    // ロジカルデバイス生成情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();    // パラメータのポインター　pointer to the logical device queue info (above)

		createInfo.pEnabledFeatures = &deviceFeatures;             // currently empty (will revisit later)

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

		// 上記のパラメータに基づいて実際のロジカルデバイスを生成します。
		// Creating the logical device itself
		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);    //　グラフィックスキュー graphics queue
		vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily.value(), 0, &m_PresentQueue);      //　プレゼンテーションキュー presentation queue
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
		if (swapChainSupport.capabilities.maxImageCount > 0                 // zero here means there is no maximum!
			&& imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};    // SwapChain生成情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;                                // stereoscopic3D 以外なら「１」 
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // SwapChainを利用する処理　what operations to use swap chain for 

		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)    // グラフィックスとプレゼンテーションキューが異なる場合
		{
			// イメージ共有モードについて： Regarding imageSharingMode
			
			// VK_SHARING_MODE_EXCLUSIVE （排他的）：1つのイメージが1つのキュー種類しか所属できません
			// 他のキュー種類に使用すると 所属をはっきり切り替えないといけません。パフォーマンス面で最適
			// exclusive: image is owned by one queue family at a time;	must be explicitly transferred before use in another queue
			// family. Offers the best performance.

			// VK_SHARING_MODE_CONCURRENT（同時）　：1つのイメージが複数のキュー種類に所属できます
			// concurrent: can be shared across multiple queue families
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; 
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else    // グラフィックスとプレゼンテーションキューが同じ（現代のデバイスはこういう風に設定されています）。
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	
			createInfo.queueFamilyIndexCount = 0;        // 任意 optional
			createInfo.pQueueFamilyIndices = nullptr;    // 任意 optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // アルファチャンネル：不透明
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;               // TRUE : オクルージョンされたピクセルの色を無視　don't care about color of obscured pixels
		createInfo.oldSwapchain = VK_NULL_HANDLE;	// 一つのSwapChainのみ　for now, only create one swap chain

		// 上記の情報に基づいてSwapChainを生成します。
		if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, nullptr);                    // SwapChainのイメージ数を獲得
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, m_SwapChainImages.data());   // 情報をm_SwapChainImagesに代入

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;
	}

	void createImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());    // イメージカウントによってベクトルサイズを変更する allocate enough size to fit all image views 					
		
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};                            // イメージビュー生成構造体
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                   // 1D/3D/キューブマップなど
			createInfo.format = m_SwapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;       // デフォルト設定　default settings
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			
			// 上記の構造体の情報に基づいて実際のイメージビューを生成します。
			if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create image views!");
			}
		}
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};           // カラーアタッチメント
		colorAttachment.format  = m_SwapChainImageFormat;    // SwapChainフォーマットと同じ　format of color attachment = format of swap chain images
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;     // マルチサンプリングなし

		colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;    // レンダリング前の情報はどうするフラッグ　
		                                                          // what to do with data before rendering

		// VK_ATTACHMENT_LOAD_OP_LOAD       : 既存の情報を保存　Preserve existing contents of attachment
		// VK_ATTACHMENT_LOAD_OP_CLEAR      : クリアする（現在：黒）Clear the values to a constant at the start (in this case, clear to black)
		// VK_ATTACHMENT_LOAD_OP_DONT_CARE  : 情報を無視　Existing contents are undefined; we don't care about them

		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	// レンダリング後の情報はどうするフラッグ what to do with data after rendering

		// VK_ATTACHMENT_STORE_OP_STORE     : 情報を保存　Rendered contents will be stored in memory and can be read later
		// VK_ATTACHMENT_STORE_OP_DONT_CARE : 情報を無視　Contents of the framebuffer will be undefined ater the rendering operation

		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;     // ステンシルバッファーを使っていない
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;    // not using stencil buffer

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;        // レンダリング前のイメージレイアウト image layout before render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    // レンダリング前のイメージレイアウト layout to automatically transition to after pass
		
		// 一般のレイアウト種 Common Layouts:
		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : カラーアタッチメントとして使う    Images used as color attachment
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR          : SwapChainで描画する               Images to be presented in the swap chain
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL     : メモリーコピー演算として使う      Images to be used as destination for a memory copy operation

		// アタッチメントレフレックスインデックス
		VkAttachmentReference colorAttachmentReference{};
		colorAttachmentReference.attachment = 0;    // attachment reference index; 0 = first index (only one attachment for now)
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// サブパス
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentReference;

		// サブパス依存関係
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;    // サブパスインデックス 0 subpass index 0 (our first and only subpass)
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// レンダーパス情報構造体生成
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		// 上記の構造体の情報に基づいて実際のレンダーパスを生成します。
		if (vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	// リソースレイアウト：どんなリソース（バッファー、イメージ情報など）をグラフィックスパイプラインにアクセスを許可するか
	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};    // UniversalBufferObjectレイアウトバインディング情報構造体
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;               // MVPトランスフォームは1つのバッファーオブジェクトに格納されています
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;               // 任意 optional, image sampling用
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;    // 参照できるシェーダーステージ（現在、頂点シェーダーでスクリプター）
		                                                             // 全てのステージの場合、VK_SHADER_STAGE_ALL_GRAPHICS
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	// グラフィックスパイプライン生成
	void createGraphicsPipeline()
	{
		const std::vector<char> vertShaderCode = readFile("shaders/vert.spv");    // 頂点シェーダー外部ファイルの読み込み
		const std::vector<char> fragShaderCode = readFile("shaders/frag.spv");    // フラグメントシェーダー外部ファイルの読み込み

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);     // 頂点シェーダーモジュール生成（頂点データ、色データ含め）
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);     // フラグメントシェーダーモジュール生成

		// シェーダステージ：パイプラインでシェーダーを利用する段階	
		// Shader Stages: Assigning shader code to its specific pipeline stage
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};                    // 頂点シェーダーステージ情報構造体
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;                   // enum for programmable stages in Graphics Pipeline: Intro
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";                                       // 頂点シェーダーエントリーポイント関数名(shaders.vert)

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};                    // フラグメントシェーダーステージ情報構造体
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";                                       // フラグメントシェーダーエントリーポイント関数名(shaders.frag)

		// パイプライン生成のタイミングで使える形にします。
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };    // シェーダー構造体配列

		// パイプライン生成の際に必要な段階 necessary steps in creating a graphics pipeline

		// 1.) 頂点インプット：頂点シェーダーに渡される頂点情報のフォーマット
		// Vertex Input: Format of the vertex data to be passed to the vertex shader

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};    // 頂点インプット情報構造体
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// Vertex構造体のVertexBindingDescriptionとVertexAttributeDescriptionに参照します
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// 2.) インプットアセンブリー： 頂点からどんなジオメトリーが描画されるか、そしてトポロジーメンバー設定
		// Input Assembly: What kind of geometry will be drawn from the vertices, topology member settings

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;    // POINT_LIST、LINE_LIST、LINE_STRIP、TRIANGLE_STRIP
		inputAssembly.primitiveRestartEnable = VK_FALSE;                 //　STRIPの場合、ラインと三角を分散できます

		// 3.) ビューポート・シザー四角
		// Viewports and Scissor Rectangles

		// ビューポート：イメージからフレームバッファーまでの「トランスフォーム」        Viewport: 'transformation' from the image to the framebuffer
		// シザー四角：ピクセルデータが格納される領域；画面上で描画される「フィルター」  Scissor rectangle: 'filter' in which region pixels will be stored. 

		VkViewport viewport{};                                // ビューポート情報構造体
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChainExtent.width;      // 描画レゾルーションと同じ
		viewport.height = (float)m_SwapChainExtent.height;    // 描画レゾルーションと同じ
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};                     // シザー四角情報構造体				
		scissor.offset = { 0, 0 };              // オフセットなし
		scissor.extent = m_SwapChainExtent;     // フレームバッファー全体を描画する設定 set to draw the entire framebuffer

		VkPipelineViewportStateCreateInfo viewportState{};    // ビューポートステート（状態）情報構造体
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;                 // ビューポートのポインター
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;                   // シザー四角のポインター

		// 4.) ラスタライザー：頂点シェーダーからのジオメトリー（シェープ）をフラグメント（ピクセル）に変換して色を付けます。 
		// Rasterizer: Takes geomerty shaped from the vertex shader and turns it into fragments (pixels) to be colored by the fragment shader.

		VkPipelineRasterizationStateCreateInfo rasterizer{};    // ラスタライザー情報構造体
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;			
		// VK_TRUE: ニア―・ファー領域のフラグメントが捨てずに境界にクランプされます。シャドウマッピングに便利。
		// VK_TRUE: fragments beyond near and far planes are clamped to them rather than discarding them; useful with shadow mapping

		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// FILL: フラグメントに埋め込む fill area of polygon with fragments
		// LINE: ラインで描画（ワイヤーフレーム）　polygon edges drawn as lines (i.e. wireframe)
		// POINT: 頂点で描画　polygon vertices are drawn as points
		// ※FILL以外の場合、特定なGPU機能をオンにする必要があります。Using anything other than FILL requires enabling a GPU feature.
		
		rasterizer.lineWidth = 1.0f;    // ラインの厚さ（ピクセル単位）Line thickness (in pixels)
		// ※1.0以上の厚さの場合、特定なGPU機能をオンにする必要があります。Using line width greater than 1.0f requires enabling a GPU feature.

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;                // カリング設定（通常：BackfaceCulling)
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;     // 頂点の順番により表面・裏面を判断する設定（時計回り・反時計回り）
		                                                            // determines front-facing by vertex order (CLOCKWISE or COUNTER_CLOCKWISE) 
		rasterizer.depthBiasEnable = VK_FALSE;        // VK_TRUE: Depth値調整（シャドウマッピング）Adjusting depth values i.e. for shadow mapping
		rasterizer.depthBiasConstantFactor = 0.0f;    // 任意 optional
		rasterizer.depthBiasClamp = 0.0f;             // 任意 optional
		rasterizer.depthBiasSlopeFactor = 0.0f;       // 任意 optional

		// 5.) マルチサンプリング（アンチエイリアシング用） 
		// Multisampling (method to perform anti-aliasing)

		VkPipelineMultisampleStateCreateInfo multisampling{};    // マルチサンプリング情報構造体
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;                   // 無効
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;     // 1ビットのみ（無効）
		multisampling.minSampleShading = 1.0f;              // 任意 optional
		multisampling.pSampleMask = nullptr;                // 任意 optional
		multisampling.alphaToCoverageEnable = VK_FALSE;     // 任意 optional
		multisampling.alphaToOneEnable = VK_FALSE;          // 任意 optionall

		// 6.)　デプス・ステンシル（現在なし） 
		// Depth and Stencil Testing (revisit later)

		// 7.) カラーブレンディング：①フレームバッファーの既存の色と混ぜるか、②前と新しい値をBitwise演算で合成するか
		// Color Blending: Either ① mix the old value (in the framebuffer) and new value, or ② perform a bitwise operation with the two values
			
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};         // カラーブレンド情報構造体
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT      // ①のブレンディング方式
											| VK_COLOR_COMPONENT_G_BIT 
											| VK_COLOR_COMPONENT_B_BIT 
											| VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;                        // 下記のコメント欄をご覧ください。
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;     // 任意 optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;    // 任意 optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;                // 任意 optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;     // 任意 optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;    // 任意 optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;                // 任意 optional

		// blendEnableがVK_TRUEの場合のブレンディング設定（疑似言語）
		// if (blendEnable) 
		//	{
		//		finalColor.rgb = (srcColorBlendFactor * newColor.rgb) < colorBlendOp > (dstColorBlendFactor * oldColor.rgb);
		//		finalColor.a = (srcAlphaBlendFactor * newColor.a) < alphaBlendOp > (dstAlphaBlendFactor * oldColor.a);
		// }
		// else 
		// {
		//		finalColor = newColor;
		// }
		// finalColor = finalColor & colorWriteMask;

		// アルファブレンディングの例（疑似言語）
		// Alpha Blending Implementation: (pseudocode)
		// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
		// finalAlpha.a = newAlpha.a;

		// アルファブレンディング設定：
		//colorBlendAttachment.blendEnable = VK_TRUE;
		//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
		VkPipelineColorBlendStateCreateInfo colorBlending{};    // パイプラインカラーブレンドステート情報構造体
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;       // VK_TRUE: ②のベンディング方式
		colorBlending.logicOp = VK_LOGIC_OP_COPY;     // 任意 optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;       // 任意 optional
		colorBlending.blendConstants[1] = 0.0f;       // 任意 optional
		colorBlending.blendConstants[2] = 0.0f;       // 任意 optional
		colorBlending.blendConstants[3] = 0.0f;       // 任意 optional

		// 8.) ダイナミックステート（後で詳しく調べます）
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		// 9.) パイプラインレイアウト（後で詳しく調べます）
		// Pipeline Layout (empty for now, revisit later)

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};     // パイプラインレイアウト情報構造体
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;               
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;     // でスクリプターセットレイアウト
		pipelineLayoutInfo.pushConstantRangeCount = 0;               // 任意 optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr;            // 任意 optional

		// 上記の構造体の情報に基づいて実際のパイプラインレイアウトを生成します。
		if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// 10.) グラフィックスパイプライン：全部の段階を組み合わせてパイプラインを生成します。
		// が、その前にいつものようにグラフィックスパイプライン情報構造体を生成する必要があります。
		// Graphics Pipeline creation: Putting everything together to create the pipeline!!
		// ...but before that, we need to create a Pipeline info struct:

		VkGraphicsPipelineCreateInfo pipelineInfo{};    // パイプライン情報構造体
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;                    // シェーダーステージに合わせる	 Make sure this info is aligned with Shader Stages above
		pipelineInfo.pStages = shaderStages;            // シェーダーステージ配列のポインター

		// 今までの段階をパイプライン構造体情報ポインターに参照します。
		// reference all the structures described in the previous stages
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;        // 任意 optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;             // 任意 optional

		// pipelineInfo.flags - 現在なし。下記の basePipelineHandleと　basePipelineHandleIndexをご覧ください。
		// none at the moment; see basePipelineHandle and basePipelineIndex below

		// パイプラインレイアウト：構造体ポインターではなく、Vulkanハンドル
		// pipeline layout: Vulkan handle, NOT struct pointer
		pipelineInfo.layout = m_PipelineLayout;

		// レンダーパスとサブパスの参照
		// reference to the render pass and the subpass index
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		// pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BITの場合：
		// 既存のパイプラインからの情報を新たなパイプラインに使用します。
		// these values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag 
		// is also specified in the flags field of VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;    // 任意 optional
		pipelineInfo.basePipelineIndex = -1;                 // 任意 optional

		// 上記の構造体の情報に基づいて、ようやく実際のグラフィックスパイプラインが生成できます。
		// 2つ目の引数：パイプラインキャッシュ(後で調べます）
		// Creating the actual graphics pipeline from data struct
		// Second argument: pipeline cache (revisit later)
		if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics pipeline!");
		}

		// 用済みのシェーダーモジュールを削除します。
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
			framebufferInfo.renderPass = m_RenderPass;          // フレームバッファーと相当する設定 compatibility with framebuffer settings
			framebufferInfo.attachmentCount = 1;                // レンダーパスの値 render pass pAtachment
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;                         // 1: 1つのイメージしかない場合 Swap chain images as single images

			if (vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	// コマンドバッファーを使うためのコマンドプールの生成します
	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};    // コマンドプール情報構造体
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();    // 描画するためグラフィックスキューを選択します
		                                                                          // drawing commands: graphics queue family chosen
		poolInfo.flags = 0;    // optional 任意

		// 上記の構造体の情報に基づいて実際のコマンドプールを生成します。
		if (vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics command pool!");
		}
	}

	// 頂点バッファー生成
	void createVertexBuffer()
	{
		// 頂点単位 ＊　配列の要素数
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		// ステージングバッファー：CPUメモリー上臨時バッファー。頂点データに渡され、最終的な頂点バッファーに渡します。
		// Staging buffer: temporary buffer in CPU memory that takes in vertex array and sends it to the final vertex buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(
			bufferSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;	// voidポインター；今回メモリーマップに使用します
		
		// 情報をメモリーにマップ
		vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);
		
		// 頂点バッファーを生成します
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,		// 一番理想的なメモリーフォーマットが、普通にCPUがアクセスできません。
			m_VertexBuffer,
			m_VertexBufferMemory);

		// 頂点データをステージングバッファーから頂点バッファーに移す
		copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

		// 用済みのステージングバッファーとメモリーの後片付け
		vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, stagingBufferMemory, nullptr);
	}

	// インデックスバッファー生成：頂点バッファーとほぼ同じ（違いは番後　①、②で表示されています
	void createIndexBuffer()
	{
		// インデックス単位　＊　配列の要素数
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();    // 変更点　①、②

		// ステージングバッファー：頂点バッファーと同じ
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;	
		vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);        // 変更点　③ vertices.data() --> indices.data()
		vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);

		// インデックスバッファーを生成します
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,    // 変更点　④ (VK_BUFFER_USAGE_VERTEX_BITからINDEX_BITに)
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_IndexBuffer,            // 変更点　⑤  インデックスバッファー
			m_IndexBufferMemory);     // 変更点　⑥　インデックスバッファーメモリー

		// インデックスデータをステージングバッファーからインデックスバッファーに移す
		copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);    // 変更点　⑦　コピー先をインデックスバッファーに

		// 用済みのステージングバッファーとメモリーの後片付け
		vkDestroyBuffer(m_LogicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice, stagingBufferMemory, nullptr);
	}

	// ユニフォームバッファー：シェーダー用のUBOデータ
	void createUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(m_SwapChainImages.size());
		m_UniformBuffersMemory.resize(m_SwapChainImages.size());

		// フレームずつUBOトランスフォームをを適用する
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_UniformBuffers[i],
				m_UniformBuffersMemory[i]
			);
		}
	}

	// でスクリプターセットを直接生成できません。コマンドバッファーのコマンドプールと同じように、まずは
	// でスクリプタープールを生成する必要があります
	void createDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};    // 各フレームに1つのデスクリプターを用意します
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

		VkDescriptorPoolCreateInfo poolInfo{};    // デスクリプタープール生成情報構造体
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());

		if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	// デスクリプターセット（トランスフォーム情報）生成
	void createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(m_SwapChainImages.size(), m_DescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;    // デスクリプタープール
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(m_SwapChainImages.size());
		if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);    // 完全に上書き：VK_WHOLE_SIZEと同じ

			VkWriteDescriptorSet descriptorWrite{};    // デスクリプターの設定・コンフィグレーション情報構造体
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0;                // ユニフォームバッファーバインディングインデックス「0」
			descriptorWrite.dstArrayElement = 0;           // 配列を使っていない場合、「0」

			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;

			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;          // 任意 optional
			descriptorWrite.pTexelBufferView = nullptr;    // 任意 optional

			// デスクリプターセットを更新します
			vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}

	// 汎用バッファー生成関数
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};                          // バッファー情報構造体
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;                                 // ユースケース：頂点バッファー
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;       // 共有モード（SwapChain生成と同じ）：今回グラフィックスキュー専用

		// 上記の構造体に基づいて実際のバッファーを生成します。
		if (vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};    // メモリー割り当て情報構造体
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		// 上記の構造体に基づいて実際のメモリー確保処理を実行します。
		if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate vertex buffer memory!");
		}

		// 確保されたメモリー割り当てを頂点バッファーにバインドします
		vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0);
	}

	// バッファーコピー関数
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		// 情報コピー処理の準備：コマンドバッファー生成
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};    // コマンドバッファー開始情報構造体
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;    // 使用回数：1回のみ

		// コマンドバッファー（処理記録）開始
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		// コピー領域確保
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;    // 任意 optional
		copyRegion.dstOffset = 0;    // 任意 optional
		copyRegion.size = size;

		// コピー元のバッファーの中身をコピー先のバッファーにコピーするコマンドを記録します
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		// コマンドバッファー記録終了
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};    // サブミット（キューに入れる）情報構造体
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// コマンドバッファー情報をキューにサブミットします
		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

		vkQueueWaitIdle(m_GraphicsQueue);    // 処理終了まで待機
		vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);    // コマンドバッファーを開放します
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;    // メモリータイプ、メモリーヒープ
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

		// 適切なメモリータイプを獲得
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i)    // corresponding bit is set to 1
				&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties)     // bitwise AND 論理積 == 引数 properties
			{
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type!");    // 適切なメモリータイプが見つからなかった場合
	}

	// コマンドプールの情報からコマンドバッファー生成
	void createCommandBuffers()
	{
		m_CommandBuffers.resize(m_SwapChainFramebuffers.size());    // フレームバッファーサイズに合わせる

		VkCommandBufferAllocateInfo allocInfo{};                    // メモリー割り当て情報構造体
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;		
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;					
		// PRIMARY:	キューに直接渡せますが、他のコマンドバッファーから呼び出せません。
		// can be submitted to queue for execution, but cannot be called from other command buffers
		// SECONDARY:　キューに直接渡せませんが、プライマリーコマンドバッファーから呼び出せます。
		// cannot be submitted directly, but can be called from primary command buffers
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers!");
		}

		// コマンドバッファー登録開始 Starting command buffer recording
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo{};       // コマンドバッファー開始情報構造体
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;                        // 任意　optional
			beginInfo.pInheritanceInfo = nullptr;       // 継承：SECONDARYの場合のみ（どのコマンドバッファーから呼び出すか）
			                                            // only for secondary command buffers (which state to inherit from)

			if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to begin recording command buffer!");
			}

			// レンダーパス開始
			// Starting a render pass
			VkRenderPassBeginInfo renderPassInfo{};		// レンダーパス情報構造体
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };

			// パフォーマンスの最適化のため、レンダー領域をアタッチメントサイズに合わせます。
			// match render area to size of attachments for best performance
			renderPassInfo.renderArea.extent = m_SwapChainExtent;	

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };    // 黒
			renderPassInfo.clearValueCount = 1;                      // VK_ATTACHMENT_LOAD_OP_CLEARのクリア値 (clearColor)
			renderPassInfo.pClearValues = &clearColor;

			// 実際のレンダーパスを開始します
			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);	

			// グラフィックスパイプラインとつなぎます
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			// 頂点バッファー情報をバインドしたら描画の準備は完成です
			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

			// インデックスバッファー
			vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);    // VK_INDEX_TYPE_UINT32

			// デスクリプターセットをバインドします
			vkCmdBindDescriptorSets(
				m_CommandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_PipelineLayout,
				0,
				1,
				&m_DescriptorSets[i],
				0,
				nullptr)
				;

			// 描画コマンド（インデックスバッファー）
			vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			// 引数①：コマンドバッファー
			//     ②：頂点数（頂点バッファーなしでも頂点を描画しています。）
			//     ③：インスタンス数（インスタンスレンダリング用）
			//     ④：インデックスバッファーの最初点からのオフセット
			//     ④：インデックスバッファーに足すオフセット (使い道はまだ不明）
			//     ④：インスタンスのオフセット（インスタンスレンダリング用）

			// arguments
			// first    : commandBuffer
			// second   : vertexCount  : even without vertex buffer, still drawing 3 vertices (triangle)
			// third    : instanceCount: used for instanced rendering, otherwise 1)
			// fourth   : firstIndexOffset : offset to start of index buffer (1 means GPU reads from second index)
			// fifth    : indexAddOffset   : offset to add to indices (not sure what this is for)
			// sixth    : instanceOffset   : used in instanced rendering

			// レンダーパスを終了します
			vkCmdEndRenderPass(m_CommandBuffers[i]);

			if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to record command buffer!");
			}
		}
	}

	// 処理同期の専用オブジェクト生成
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

	
	void updateUniformBuffer(uint32_t currentImage)
	{
		//// startTime、currentTimeの実際のデータ型: static std::chrono::time_point<std::chrono::steady_clock> 
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		
		UniformBufferObject ubo{};  // MVPトランスフォーム情報構造体

		//// M(Model: 毎フレーム、Z軸に90°回転させる
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));   
		//
		// V(View): 引数　eye位置, center位置, up軸　（現在上から45°）
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		// P(Projection): 引数　45°バーティカルFoV, アスペクト比、ニア、ファービュープレーン
		// arguments: field-of-view, aspect ratio, near and far view planes 
		ubo.proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);

		//// 元々GLMはOpelGLに対応するために設計されています（Y軸のクリップ座標が逆さになっています）。
		//// Vulkanに対応するためにクリップ座標のY軸を「元に戻す」訳です。こうしないと描画はひっくり返す状態になってしまいます。

		//// GLM was initially developed for OpenGL where the Y-coordinate of the clip coordinates is reversed.  
		//// To compensate, multiply the Y-coordinate of the projection matrix by -1 (reverting back to normal).  
		//// Not doing this results in an upside-down render.
		ubo.proj[1][1] *= -1;

		//// UBO情報を現在のユニフォームバッファーにうつします
		void* data;
		vkMapMemory(m_LogicalDevice, m_UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_LogicalDevice, m_UniformBuffersMemory[currentImage]);
	}

	// 描画関数
	void drawFrame()
	{
		// フェンス処理を待ちます
		vkWaitForFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		// SwapChainがすたれた場合  （すたれた）
		// check if swap chain is out of date
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();    // SwapChainを再生成
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		// ユニフォームバッファー更新
		updateUniformBuffer(imageIndex);

		// 現在の画像が以前のフレームで使われているか（フェンスを待っているか）
		// check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_LogicalDevice, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}

		// 現在の画像が現在のフレームで使われているように示す。
		// mark the image as now being in use by this frame
		m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSubmitInfo submitInfo{};    // キュー同期・提出情報構造体
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;    // 実行前に待つセマフォ　semaphore to wait on before execution
		submitInfo.pWaitDstStageMask = waitStages;      // 待たせるパイプラインステージ　stage(s) of the pipeline to wait

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;    // 終了後のときに起動するセマフォ  semaphores to signal once command buffer(s) have finished execution

		vkResetFences(m_LogicalDevice, 1, &m_InFlightFences[m_CurrentFrame]);

		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};    // プレゼント情報構造体
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;	
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };      // SwapChain構造体
		presentInfo.swapchainCount = 1;                     // SwapChain数（現在：１）
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// 任意（複数のSwapChainの場合、各SwapChainの結果を格納する配列）
		// array of VkResult values to check for each swap chain if presentation was successful if using multiple swap chains
		presentInfo.pResults = nullptr;		
											
		// リザルトをSwapChainに渡して描画します  submit the result back to the swap chain to have it show on screen
		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR    // SwapChainが廃れた
		 || result == VK_SUBOPTIMAL_KHR           // SwapChainが最適化されていない
		 || m_FramebufferResized == true)         // フレームバッファーのサイズが変更された
		{
			m_FramebufferResized = false;
			recreateSwapChain();                  // SwapChainを再生成します
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;    // 次のフレームに移動　advance to next frame
	}


	// シェーダーコードをパイプラインに使用する際にシェーダーモジュールにwrapする必要があります。
	VkShaderModule createShaderModule(const std::vector<char>& code)	
	{	
		// specify a pointer to the buffer with the bytecode and length

		VkShaderModuleCreateInfo createInfo{};    // シェーダーモジュール情報構造体
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;		
		createInfo.codeSize = code.size();        // サイズ

		// バイトコードのサイズはバイトで表しますが、バイトコードポインターはuint32_t型です。
		// そこで、ポインターをrecastする必要があります。
		// size of bytecode is in bytes, but bytecode pointer is uint32_t pointer rather than a char pointer.
		// thus, we need to recast the pointer as below.
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());	

		// 上記の構造体に基づいてシェーダーモジューを生成します。
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}
		return shaderModule;
	}

	// サーフェスフォーマット選択
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB    // sRGBが使えるなら洗濯します。
				&& availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)	 
			{
				return availableFormat;
			}
		}
		// それ以外、最初見つかったフォーマットを使ってもいいです otherwise, just use the first format that is specified
		return  availableFormats[0];	
										
	}

	// スワッププレゼントモード
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)    // トリプルバッファリング　triple buffering (less latency)
			{
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;    // if VK_PRESENT_MODE_MAILBOX_KHR is not available, use a guaranteed available mode
	}

	// レゾルーション設定
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites)    // extent = resolution of the swap chain images
	{
		// Vulkanがピクセル単位で設定されていますがスクリーンレゾルーションは2D座標で表しています。
		// ピクセル単位でのスクリーンレゾルーションを獲得するためにglfwGetFramebufferSize()を使う必要があります。
		// Vulkan works with pixel units, but the screen resolution (WIDTH, HEIGHT) is in screen coordinates.
		// Use glfwGetFramebufferSize() to query the resolution of the window in pixel before matching to min/max image extent

		// UINT32_MAX: uint32_tの最大値 maximum value of uint32_t
		// ここで使うと、自動的に一番理想なレゾルーションを選択してくれます。
		// special value to indicate that we will be picking the resolution that best matches the window
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

	// SwapChainサポート確認
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);    // サーフェスケーパビリティ surface capabilities

		uint32_t formatCount;                                                                   // 適用できるフォーマットカウント
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);         // 対応しているサーフェスフォーマットを確認 	
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);    // 適用できるフォーマットを格納できるようにリサイズします。
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());    // データを代入
		}

		uint32_t presentModeCount;                                                              // 適用できるプレゼトモードカウント
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &presentModeCount, nullptr);    // 対応しているプレゼンテーションモードを確認
		if (presentModeCount != 0)
		{
			// 適用できるプレゼンテーションモードを格納できるようにリサイズします。
			details.presentModes.resize(presentModeCount);    
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());    // データを代入
		}
		return details;
	}

	// GPUがやりたい処理に適切かの確認
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);     // VK_QUEUE_GRAPHICS_BITを探しています

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;     // 最低限1つのイメージフォーマットと1つのプレゼンテーションモードが特定できましたか
		                                    // At least one supported image format and one supported presentation mode given the window surface
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// ジオメトリーシェーダーのみを選択したい場合；　sample if wanting to narrow down to geometry shaders:
		// return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;		
		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	// ロジカルデバイスがエクステンションに対応できるかの確認
	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;	// エクステンションカウント
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);	// 使用可能エクステンションのベクトル

		// 全てのエクステンションプロパティをavailableExtensionsに代入します
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// 必要なエクステンションのベクトル
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const VkExtensionProperties& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);    // 必要なエクステンションを見つかった場合、setから削除します。
			                                                      // erase if required extension is in the vector
		}
		
		bool isEmpty = requiredExtensions.empty();    // 全ての必要なエクステンションが見つかった場合、True	
		return isEmpty;                               // if all the required extension were present (and thus erased), returns true
	}

	// キュー種類選択
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);                 // キュー種類を特定

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);                         // キュー種類カウントによりベクトルを生成します
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());    // 情報をベクトルに代入

		int i = 0;
		for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			if (presentSupport = VK_TRUE)		
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())    // キュー種類が見つかった場合、break / If queueFamily is found, exit early
			{
				break;
			}
			i++;
		}
		return indices;
	}

	// 必要なエクステンションの一覧（デバッグ機能がオン・オフによって異なります）
	// returns the list of extensions based on whether validation layers are enabled or not
	std::vector<const char*> getRequiredExtensions()	
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

	// バリデーションレイヤーがサポートされているかの確認
	bool checkValidationLayerSupport()		
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);                   // バリデーションレイヤー数を特定

		std::vector<VkLayerProperties> availableLayers(layerCount);                 // カウントによりベクトルを生成します
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());    // 情報をベクトルに代入

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const VkLayerProperties& layerProperties : availableLayers)
			{
				// レイヤー名が既存のバリデーションレイヤーに合ってる場合 if layer name matches existing validation layer name
				if (strcmp(layerName, layerProperties.layerName) == 0)						
				{
					layerFound = true;		// 見つかったフラッグ
					break;
				}
			}

			// 今までのレイヤーが既存のレイヤーリストに見つかった限り、全てのレイヤーが確認するまでループが続きます
			// as long as layer is found in the list, loop will continue until all validation layers have been verified
			if (layerFound == false)        
			{							
				return false;
			}
		}
		return true;
	}

	// ファイル読み込み
	static std::vector<char> readFile(const std::string& fileName)
	{
		std::ifstream file(fileName, std::ios::ate    // ate: 尻尾からファイルを読む　start reading at EOF 
			| std::ios::binary);                      // binary: バイナリファイルとして扱う read file as binary file (avoid text transformations)

		if (file.is_open() == false)
		{
			throw std::runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();     // telg(): インプットストリーム位置を戻す returns input stream position. 
		std::vector<char> buffer(fileSize);         // 尻尾(EOF: End of File) から読み込むと実際のファイルサイズを特定できます。
		                                            // EOF essentially gives us the size of the file for the buffer

		file.seekg(0);                              // ファイルの頭に戻る　return to beginning of file
		file.read(buffer.data(), fileSize);         // read(x, y): yまで読み込み、xバッファーにアサインします 
		                                            // read up to count y and assign to buffer x

		// 今回、全てのバイトを一気に読み込む in this case, read all the bytes at once

		// std::cout << fileSize << std::endl;		// ファイルプロパティーでサイズが合ってるかが確認できます。
													// check file byte size with actual file (properties)
		file.close();

		return buffer;
	}
};

// メイン関数
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