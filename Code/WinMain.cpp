#define VK_USE_PLATFORM_WIN32_KHR
#include "Platform.h"
#include "CPUProfiler.h"
#include "GPUProfiler.h"

#ifndef ENGINE_RELEASE
#include "ThirdParty\DearImGui\imgui.cpp"
#include "ThirdParty\DearImGui\imgui_widgets.cpp"
#include "ThirdParty\DearImGui\imgui_tables.cpp"
#include "ThirdParty\DearImGui\imgui_draw.cpp"
#include "ThirdParty\DearImGui\imgui_impl_win32.cpp"
#include "ThirdParty\DearImGui\imgui_impl_vulkan.cpp"
#endif

#include "Engine.cpp"

#include <Windows.h>
#include <mmsystem.h>

#ifndef ENGINE_RELEASE
VkRenderPass InitializeDearImgui(HWND Window, VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device, VkCommandBuffer CommandBuffer,
						 		 uint32_t FamilyIndex, VkQueue Queue, VkPipelineCache PipelineCache, const SSwapchain& Swapchain, VkFormat SwapchainFormat)
{
	// Create descriptor pool for DearImgui
	VkDescriptorPoolSize PoolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo CreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	CreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	CreateInfo.maxSets = 1000;
	CreateInfo.poolSizeCount = ArrayCount(PoolSizes);
	CreateInfo.pPoolSizes = PoolSizes;

	VkDescriptorPool ImguiDescrPool = 0;
	VkCheck(vkCreateDescriptorPool(Device, &CreateInfo, 0, &ImguiDescrPool));
	Assert(ImguiDescrPool);

	// Create render pass for DearImgui
	VkAttachmentDescription Attachment = {};
	Attachment.format = SwapchainFormat;
	Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	Attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	Attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachment;

	VkRenderPassCreateInfo RenderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	RenderPassCreateInfo.attachmentCount = 1;
	RenderPassCreateInfo.pAttachments = &Attachment;
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &Subpass;

	VkRenderPass RenderPass = 0;
	VkCheck(vkCreateRenderPass(Device, &RenderPassCreateInfo, 0, &RenderPass));
	Assert(RenderPass);

	ImGui_ImplWin32_Init(Window);
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance = Instance;
    InitInfo.PhysicalDevice = PhysicalDevice;
    InitInfo.Device = Device;
    InitInfo.QueueFamily = FamilyIndex;
    InitInfo.Queue = Queue;
    InitInfo.PipelineCache = PipelineCache;
    InitInfo.DescriptorPool = ImguiDescrPool;
    InitInfo.MinImageCount = Swapchain.ImageCount;
    InitInfo.ImageCount = Swapchain.ImageCount;
	ImGui_ImplVulkan_Init(&InitInfo, RenderPass);

	// Upload IMGUI font textures
	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkCheck(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

    ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);

	vkEndCommandBuffer(CommandBuffer);

	VkSubmitInfo SubmitInfo  = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;
	vkQueueSubmit(Queue, 1, &SubmitInfo, 0);

	vkDeviceWaitIdle(Device);
    
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	return RenderPass;
}
#endif

void PlatformGetAllFilenamesFromDir(const char* WildcardPath, char* Filenames, uint32_t Stride, uint32_t& FileCount)
{
	WIN32_FIND_DATAA FindData; 
    HANDLE FileHandle = FindFirstFileA(WildcardPath, &FindData); 
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			memcpy(Filenames, FindData.cFileName, StringLength(FindData.cFileName));

			FileCount++;
			Filenames += Stride;
		} while (FindNextFileA(FileHandle, &FindData));
		FindClose(FileHandle);
	}
}

struct SPlatformData
{
	HWND Window;

	double MouseRestoreX, MouseRestoreY;
	double MouseLastX, MouseLastY;
};

static SPlatformData GlobalPlatformData;
static bool bGlobalRunning = true;
static bool bGlobalWindowFocused = true;
static bool bGlobalShowMouse = true;
static bool bGlobalShowMouseBeforeMinimization = false;
static bool bGlobalCursorWasJustDisabled = false;
static bool bGlobalShowCursor = true;
static bool bGlobalShowCursorBeforeMinimization = false;

static int64_t GlobalPerfCounterFrequency;

static bool bGlobalFullscreen = true;
static VkPresentModeKHR GlobalPresentMode = VK_PRESENT_MODE_FIFO_KHR;

void PlatformQuitGame()
{
	bGlobalRunning = false;
}

bool PlatformGetFullscreen()
{
	return bGlobalFullscreen;
}

void PlatformChangeFullscreen(bool bFullscreen)
{
	bGlobalFullscreen = bFullscreen;
}

bool PlatformGetVSync()
{
	return (GlobalPresentMode == VK_PRESENT_MODE_FIFO_KHR) ? true : false;
}

void PlatformChangeVSync(bool bEnabled)
{
	GlobalPresentMode = bEnabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
}

struct SWindowUserPtr
{
	SPlatformData* PlatformData;
	SGameInput* GameInput;
};

inline LARGE_INTEGER WinGetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline float WinGetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    float Result = (End.QuadPart - Start.QuadPart) / float(GlobalPerfCounterFrequency);
    return Result;
}

struct SSoundBuffer
{
	bool bWasPrepared;
	WAVEHDR WaveHeader;
};

struct SWinSoundBackend
{
	bool bInitialized;
	HWAVEOUT WaveOut;

	uint32_t CurrentBuffer;
	SSoundBuffer Buffers[64];

	uint32_t ChannelCount;
	uint32_t SamplesPerSec;
	uint32_t BitsPerChannel;

	float TargetLatency;
	uint64_t SamplesWritten;
};

SWinSoundBackend WinInitializeSoundBackend()
{
	SWinSoundBackend SoundBackend = {};

	SoundBackend.ChannelCount = 2;
	SoundBackend.SamplesPerSec = 44100;
	SoundBackend.BitsPerChannel = 16;
	// TODO(georgii): Review latency!
	SoundBackend.TargetLatency = 50.0f;

	WAVEFORMATEX WaveFormat = {};
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.nChannels = (WORD) SoundBackend.ChannelCount;
	WaveFormat.nSamplesPerSec = (DWORD) SoundBackend.SamplesPerSec;
	WaveFormat.wBitsPerSample = (WORD) SoundBackend.BitsPerChannel;
	WaveFormat.nAvgBytesPerSec = (WaveFormat.nSamplesPerSec * WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
	WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;

	if (waveOutOpen(&SoundBackend.WaveOut, WAVE_MAPPER, &WaveFormat, 0, 0, 0) != MMSYSERR_NOERROR)
	{
		SLogger::Log("Can't initialize sound backend.\n", LoggerVerbosity_Release);
	}
	else
	{
		const uint32_t SamplesPerBuffer = uint32_t(SoundBackend.SamplesPerSec * (SoundBackend.TargetLatency / 1000.0f));
		for (uint32_t I = 0; I < ArrayCount(SoundBackend.Buffers); I++)
		{
			SoundBackend.Buffers[I].WaveHeader.lpData = (LPSTR) malloc(SamplesPerBuffer * (SoundBackend.ChannelCount * (SoundBackend.BitsPerChannel / 8)));
		}

		SoundBackend.bInitialized = true;
		SLogger::Log("Sound backend initialized.\n", LoggerVerbosity_Release);
	}

	return SoundBackend;
}

void WinGetCursorPos(HWND Window, double* XPos, double* YPos)
{
    POINT Pos;
    if (GetCursorPos(&Pos))
    {
        ScreenToClient(Window, &Pos);

        *XPos = Pos.x;
        *YPos = Pos.y;
    }
}

void WinSetCursorPos(SPlatformData* PlatformData, int XPos, int YPos)
{
	POINT Pos = { XPos, YPos };

    // Store the new position so it can be recognized later
    PlatformData->MouseLastX = Pos.x;
    PlatformData->MouseLastY = Pos.y;

    ClientToScreen(PlatformData->Window, &Pos);
    SetCursorPos(Pos.x, Pos.y);
}

void WinEnableCursor(SPlatformData* PlatformData, SGameInput* GameInput, bool bShowCursor = true)
{
	if (!bGlobalShowMouse)
	{
		ClipCursor(0);
		SetCursorPos(int(PlatformData->MouseRestoreX), int(PlatformData->MouseRestoreY));

		bGlobalShowMouse = true;
		GameInput->bShowMouse = true;
	}

	if (bShowCursor != bGlobalShowCursor)
	{
		ShowCursor(bShowCursor);
		bGlobalShowCursor = bShowCursor;
	}
}

void WinDisableCursor(SPlatformData* PlatformData, SGameInput* GameInput)
{
	WinGetCursorPos(PlatformData->Window, &GameInput->MouseX, &GameInput->MouseY);
	GameInput->MouseLastX = GameInput->MouseX;
	GameInput->MouseLastY = GameInput->MouseY;

	POINT MouseRestoreP;
	GetCursorPos(&MouseRestoreP);
	PlatformData->MouseRestoreX = MouseRestoreP.x;
	PlatformData->MouseRestoreY = MouseRestoreP.y;

	RECT Rect;
	GetClientRect(PlatformData->Window, &Rect);
	
	POINT Pos = { Rect.right / 2, Rect.bottom / 2 };
	PlatformData->MouseLastX = Pos.x;
    PlatformData->MouseLastY = Pos.y;

	ClientToScreen(PlatformData->Window, &Pos);
	SetCursorPos(Pos.x, Pos.y);

	if (bGlobalShowMouse)
	{
		bGlobalShowMouse = false;
		GameInput->bShowMouse = false;
		bGlobalCursorWasJustDisabled = true;
	}

	if (bGlobalShowCursor)
	{
		ShowCursor(FALSE);
		bGlobalShowCursor = false;
	}
	
	ClientToScreen(PlatformData->Window, (POINT*)&Rect.left);
	ClientToScreen(PlatformData->Window, (POINT*)&Rect.right);
	ClipCursor(&Rect);
}

void PlatformOutputDebugString(const char* String)
{
	OutputDebugStringA(String);
}

void PlatformDisableCursor(SGameInput* GameInput)
{
	WinDisableCursor(&GlobalPlatformData, GameInput);
}

void PlatformEnableCursor(SGameInput* GameInput)
{
	WinEnableCursor(&GlobalPlatformData, GameInput);
}

void PlatformToggleCursor(SGameInput* GameInput, bool bEnable, bool bShowCursor)
{
	if (bEnable)
	{
		WinEnableCursor(&GlobalPlatformData, GameInput, bShowCursor);
	}
	else if (!bEnable)
	{
		WinDisableCursor(&GlobalPlatformData, GameInput);
	}
}

bool PlatformIsCursorEnabled()
{
	bool bResult = bGlobalShowMouse;
	return bResult;
}

bool PlatformIsCursorShowed()
{
	return bGlobalShowCursor;
}

void WinProcessKeyboardMessage(SButton& Button, bool IsDown)
{
	if (Button.IsDown != IsDown)
	{
		Button.IsDown = IsDown;
		Button.HalfTransitionCount++;
	}
}

static HWND GlobalWindow;
static WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
void WinToggleFullscreen(HWND Window)
{
    // NOTE(georgii): This follows Raymond Chen's prescription for fullscreen toggling, see:
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	static bool bFullscreen = false;

	// NOTE(georgii): This is here just to update GlobalWindowPosititon every time
	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW) 
	{
		GetWindowPlacement(Window, &GlobalWindowPosition);
	}

	if (bFullscreen != bGlobalFullscreen)
	{
		if (Style & WS_OVERLAPPEDWINDOW) 
		{
			MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
			if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
				GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) 
			{
				SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(Window, HWND_TOP,
							MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
							MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
							MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
							SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

				bFullscreen = true;
			}
		}
		else 
		{
			SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(Window, &GlobalWindowPosition);
			SetWindowPos(Window, NULL, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
						SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

			bFullscreen = false;
		}
	}
}

SWindowPlacementInfo PlatformGetWindowPlacement()
{
	SWindowPlacementInfo PlacementInfo = { &GlobalWindowPosition, sizeof(GlobalWindowPosition) };

	return PlacementInfo;
}

void PlatformSetWindowPlacement(SWindowPlacementInfo PlacementInfo)
{
	DWORD Style = GetWindowLong(GlobalWindow, GWL_STYLE);
	SetWindowLong(GlobalWindow, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
	SetWindowPlacement(GlobalWindow, (const WINDOWPLACEMENT*) PlacementInfo.InfoPointer);
	SetWindowPos(GlobalWindow, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

LRESULT CALLBACK WinMainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    SWindowUserPtr* WindowUserPtr = (SWindowUserPtr*) GetWindowLongPtrA(Window, GWLP_USERDATA);
	SPlatformData* PlatformData = 0;
	SGameInput* GameInput = 0;
	if(WindowUserPtr)
	{
		PlatformData = WindowUserPtr->PlatformData;
		GameInput = WindowUserPtr->GameInput;
	}

#ifndef ENGINE_RELEASE
    ImGuiIO& IO = ImGui::GetIO();
    ImGui_ImplWin32_Data* BackendData = ImGui_ImplWin32_GetBackendData();
#endif

    LRESULT Result = 0;
    switch (Message)
    {
		case WM_SETFOCUS:
		{
			bGlobalWindowFocused = true;

			if (GameInput)
			{
				PlatformToggleCursor(GameInput, bGlobalShowMouseBeforeMinimization, bGlobalShowCursorBeforeMinimization);
			}

			// NOTE(georgii): Handle Alt + Tab bug.
			WinProcessKeyboardMessage(GameInput->Buttons[Button_Alt], false);
	        
#ifndef ENGINE_RELEASE
			IO.AddFocusEvent(Message == WM_SETFOCUS);
#endif
		} break;

		case WM_KILLFOCUS:
		{
			bGlobalWindowFocused = false;

			bGlobalShowMouseBeforeMinimization = bGlobalShowMouse;
			bGlobalShowCursorBeforeMinimization = bGlobalShowCursor;
			if (PlatformData && GameInput)
			{
				WinEnableCursor(PlatformData, GameInput);
			}

			// NOTE(georgii): Handle Alt + Tab bug.
			WinProcessKeyboardMessage(GameInput->Buttons[Button_Alt], false);

#ifndef ENGINE_RELEASE
			IO.AddFocusEvent(Message == WM_SETFOCUS);
#endif
		} break;

        case WM_DESTROY:
        {
            bGlobalRunning = false;
        } break;
        
        case WM_CLOSE:
        {
            bGlobalRunning = false;
        } break;

		case WM_MOUSEMOVE:
		{
			if(PlatformData && GameInput && bGlobalWindowFocused)
			{
				if (!bGlobalCursorWasJustDisabled)
				{
					const int X = ((int)(short)LOWORD(LParam));
					const int Y = ((int)(short)HIWORD(LParam));

					double DeltaX = X - PlatformData->MouseLastX;
					double DeltaY = Y - PlatformData->MouseLastY;

					PlatformData->MouseLastX = X;
					PlatformData->MouseLastY = Y;

					if (!bGlobalShowMouse)
					{
						GameInput->MouseX += DeltaX;
						GameInput->MouseY += DeltaY;

						GameInput->MouseDeltaX = float(GameInput->MouseX - GameInput->MouseLastX);
						GameInput->MouseDeltaY = float(GameInput->MouseY - GameInput->MouseLastY);

						GameInput->MouseLastX = GameInput->MouseX;
						GameInput->MouseLastY = GameInput->MouseY;
					}
				}
				else
				{
					bGlobalCursorWasJustDisabled = false;
				}
			}

#ifndef ENGINE_RELEASE
			BackendData->MouseHwnd = Window;
			if (!BackendData->MouseTracked)
			{
				TRACKMOUSEEVENT TrackEvent = { sizeof(TrackEvent), TME_LEAVE, Window, 0 };
				TrackMouseEvent(&TrackEvent);
				BackendData->MouseTracked = true;
			}
#endif
		} break;
		
#ifndef ENGINE_RELEASE
		case WM_MOUSELEAVE:
		{
	        if (BackendData->MouseHwnd == Window)
				BackendData->MouseHwnd = NULL;
			BackendData->MouseTracked = false;
		} break;

		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		{
			int Button = 0;
			if ((Message == WM_LBUTTONDOWN) || (Message == WM_LBUTTONDBLCLK)) { Button = 0; }
			if ((Message == WM_RBUTTONDOWN) || (Message == WM_RBUTTONDBLCLK)) { Button = 1; }
			if ((Message == WM_MBUTTONDOWN) || (Message == WM_MBUTTONDBLCLK)) { Button = 2; }
			if ((Message == WM_XBUTTONDOWN) || (Message == WM_XBUTTONDBLCLK)) { Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? 3 : 4; }
			if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
				SetCapture(Window);
			IO.MouseDown[Button] = true;
		} break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			int Button = 0;
			if (Message == WM_LBUTTONUP) { Button = 0; }
			if (Message == WM_RBUTTONUP) { Button = 1; }
			if (Message == WM_MBUTTONUP) { Button = 2; }
			if (Message == WM_XBUTTONUP) { Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? 3 : 4; }
			IO.MouseDown[Button] = false;
			if (!ImGui::IsAnyMouseDown() && GetCapture() == Window)
				ReleaseCapture();
		} break;

    	case WM_MOUSEWHEEL:
		{
        	IO.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(WParam) / (float)WHEEL_DELTA;
            
            if (GameInput)
            {
                GameInput->MouseWheelDelta += (float)GET_WHEEL_DELTA_WPARAM(WParam) / (float)WHEEL_DELTA;
            }
		} break;

    	case WM_MOUSEHWHEEL:
		{
        	IO.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(WParam) / (float)WHEEL_DELTA;
		} break;
#endif

        case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			if(GameInput)
			{
				uint32_t VKCode = (uint32_t)WParam;

				bool WasDown = ((LParam & (1 << 30)) != 0);
				bool IsDown = ((LParam & (1 << 31)) == 0);
				bool bAltDown = ((LParam & (1 << 29)) != 0);
                
				if (WasDown != IsDown)
				{ 
					if (VKCode == 'W')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_W], IsDown);
					}
					else if (VKCode == 'A')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_A], IsDown);
					}
					else if (VKCode == 'S')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_S], IsDown);
					}
					else if (VKCode == 'D')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_D], IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Space], IsDown);
					}
					else if (VKCode == VK_F1)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_F1], IsDown);
					}
                    else if (VKCode == VK_F2)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_F2], IsDown);
					}
					else if (VKCode == VK_CONTROL)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Control], IsDown);
					}
					else if (VKCode == VK_MENU)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Alt], IsDown);
					}
					else if (VKCode == VK_SHIFT)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Shift], IsDown);
					}
					else if (VKCode == VK_DELETE)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Delete], IsDown);
					}
					else if (VKCode == VK_RETURN)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Enter], IsDown && !bAltDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Escape], IsDown);
					}
					else if (VKCode == 'Z')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Z], IsDown);
					}
					else if (VKCode == 'R')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_R], IsDown);
					}
					else if (VKCode == 'C')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_C], IsDown);
					}
					else if (VKCode == 'X')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_X], IsDown);
					}
					else if (VKCode == 'B')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_B], IsDown);
					}
					else if (VKCode == 'Q')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_Q], IsDown);
					}
					else if (VKCode == 'E')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_E], IsDown);
					}
					else if (VKCode == 'F')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_F], IsDown);
					}
					else if (VKCode == 'G')
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_G], IsDown);
					}
					else if (VKCode == VK_UP)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_ArrowUp], IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_ArrowDown], IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_ArrowLeft], IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						WinProcessKeyboardMessage(GameInput->Buttons[Button_ArrowRight], IsDown);
					}

					if (IsDown)
					{
						if (bAltDown)
						{
							if (VKCode == VK_F4)
							{
								bGlobalRunning = false;
							}
						}
					}
				}
			}

#ifndef ENGINE_RELEASE
			bool Down = (Message == WM_KEYDOWN || Message == WM_SYSKEYDOWN);
			if (WParam < 256)
				IO.KeysDown[WParam] = Down;
			if (WParam == VK_CONTROL)
				IO.KeyCtrl = Down;
			if (WParam == VK_SHIFT)
				IO.KeyShift = Down;
			if (WParam == VK_MENU)
				IO.KeyAlt = Down;
#endif
		} break;

#ifndef ENGINE_RELEASE
		case WM_CHAR:
		{
        	if ((WParam > 0) && (WParam < 0x10000))
	            IO.AddInputCharacterUTF16((unsigned short)WParam);
		} break;

    	case WM_SETCURSOR:
		{
        	if ((LOWORD(LParam) == HTCLIENT) && ImGui_ImplWin32_UpdateMouseCursor())
            	Result = 1;
		} break;

    	case WM_DEVICECHANGE:
		{
	        if ((UINT)WParam == DBT_DEVNODES_CHANGED)
	            BackendData->WantUpdateHasGamepad = true;
		} break;
#endif
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

int CALLBACK WinMain(HINSTANCE HInstance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
#ifdef ENGINE_RELEASE
	SLogger::Initialize(LoggerVerbosity_Release);
#elif defined(ENGINE_PROFILE)
	SLogger::Initialize(LoggerVerbosity_Debug);
#else
	SLogger::Initialize(LoggerVerbosity_SuperDebug);
#endif

    WNDCLASSA WindowClass = {};
	WindowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = WinMainWindowCallback;
    WindowClass.hInstance = HInstance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "EngineWindowClass";

#ifndef ENGINE_RELEASE
	// Initialize DearImgui
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO(); (void)IO;
	ImGui::StyleColorsDark();
#endif

    if (RegisterClassA(&WindowClass))
	{
		SLogger::Log("Window class registered.\n", LoggerVerbosity_Release);

		HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Color Thief", WS_OVERLAPPEDWINDOW, 
									  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, HInstance, 0);

		if (Window)
		{
			SLogger::Log("Window created.\n", LoggerVerbosity_Release);

			GlobalWindow = Window;

			GlobalPlatformData.Window = Window;
			
			SGameInput GameInput = {};
			SWindowUserPtr WindowUserPtr = { &GlobalPlatformData, &GameInput };
			SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)&WindowUserPtr);

			SWinSoundBackend WinSound = WinInitializeSoundBackend();

			LARGE_INTEGER PerfCounterFrequencyResult;
			QueryPerformanceFrequency(&PerfCounterFrequencyResult);
			GlobalPerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

			SGameMemory GameMemory = {};
			GameMemory.StorageSize = Gigabytes(1);
			GameMemory.Storage = malloc(GameMemory.StorageSize);
			if (GameMemory.Storage)
			{
				SLogger::Log("Game memory allocated.\n\n", LoggerVerbosity_Release);

#if ENGINE_RELEASE
				uint32_t InternalWidthSaved = 0;
				uint32_t InternalHeightSaved = 0;

				SReadEntireFileResult GeneralSaveFile = ReadEntireFile("Saves\\GeneralSave");
				if (GeneralSaveFile.Memory && GeneralSaveFile.Size)
				{
					uint8_t* SaveFilePointer = (uint8_t*) GeneralSaveFile.Memory;

					uint32_t LastLevelNameLength = 0;
					memcpy(&LastLevelNameLength, SaveFilePointer, sizeof(uint32_t));
					SaveFilePointer += sizeof(uint32_t);
					SaveFilePointer += LastLevelNameLength;

					bool bFullscreen;
					memcpy(&bFullscreen, SaveFilePointer, sizeof(bool));
					SaveFilePointer += sizeof(bool);

					SaveFilePointer += sizeof(bool);
					SaveFilePointer += sizeof(bool);
					SaveFilePointer += sizeof(int32_t);

					memcpy(&InternalWidthSaved, SaveFilePointer, sizeof(uint32_t));
					SaveFilePointer += sizeof(uint32_t);

					memcpy(&InternalHeightSaved, SaveFilePointer, sizeof(uint32_t));
					SaveFilePointer += sizeof(uint32_t);
					
					SaveFilePointer += sizeof(uint32_t);
					SaveFilePointer += sizeof(uint32_t);

					uint64_t WindowPlacementSize;
					memcpy(&WindowPlacementSize, SaveFilePointer, sizeof(uint64_t));
					SaveFilePointer += sizeof(uint64_t);

					void *WindowPlacement = malloc(WindowPlacementSize);
					memcpy(WindowPlacement, SaveFilePointer, WindowPlacementSize);
					SaveFilePointer += WindowPlacementSize;

					PlatformChangeFullscreen(bFullscreen);
					if (!bFullscreen)
					{
						SWindowPlacementInfo WindowPlacementInfo = { WindowPlacement, WindowPlacementSize };
						PlatformSetWindowPlacement(WindowPlacementInfo);
					}
					free(WindowPlacement);
				}
#endif

				WinToggleFullscreen(Window);

				// NOTE(georgii): Fix for vkEnumeratePhysicalDevices bug on laptops with AMD and NVIDIA GPUs
				// https://github.com/KhronosGroup/Vulkan-Loader/issues/552
				SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");

				memset(GameMemory.Storage, 0, GameMemory.StorageSize);

				SVulkanVersion VulkanVersion = GetVulkanVersion();
				Assert((VulkanVersion.Major >= 1) && (VulkanVersion.Minor >= 1));

				char VulkanVersionOutput[128] = {};
				snprintf(VulkanVersionOutput, sizeof(VulkanVersionOutput), "Available Vulkan SDK version: %d.%d\n", VulkanVersion.Major, VulkanVersion.Minor);
				SLogger::Log(VulkanVersionOutput, LoggerVerbosity_Release);

				VkInstance Instance = CreateInstance();
#ifndef ENGINE_RELEASE
				RegisterDebugCallback(Instance);
#endif

				VkPhysicalDevice PhysicalDevice = PickPhysicalDevice(Instance);

				VkPhysicalDeviceProperties PhysicalDeviceProps = {};
				vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProps);

				uint32_t GraphicsFamilyIndex = GetGraphicsFamilyIndex(PhysicalDevice);
				Assert(GraphicsFamilyIndex != VK_QUEUE_FAMILY_IGNORED);

				bool bIndirectCountKHR = false;
				bool bIndirectCountAMD = false;
				bool bMinMaxSampler = false;
				VkDevice Device = CreateDevice(PhysicalDevice, GraphicsFamilyIndex, bIndirectCountKHR, bIndirectCountAMD, bMinMaxSampler);

				VkSurfaceKHR Surface = CreateSurface(Instance, HInstance, Window);
				Assert(SurfaceSupportsPresentation(PhysicalDevice, GraphicsFamilyIndex, Surface));

				VkQueue GraphicsQueue = 0;
				vkGetDeviceQueue(Device, GraphicsFamilyIndex, 0, &GraphicsQueue);

				VkFormat SwapchainFormat = GetSwapchainFormat(PhysicalDevice, Surface);
				VkFormat DepthFormat = FindDepthFormat(PhysicalDevice);

				VmaAllocator MemoryAllocator = CreateVulkanMemoryAllocator(Instance, PhysicalDevice, Device);
				SSwapchain Swapchain = CreateSwapchain(Device, PhysicalDevice, Surface, SwapchainFormat, GlobalPresentMode);
				SLogger::Log("Swapchain created.\n", LoggerVerbosity_Release);

				VkCommandPool CommandPool = CreateCommandPool(Device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, GraphicsFamilyIndex);

				VkCommandBuffer CommandBuffers[FramesInFlight] = {};
				AllocateCommandBuffers(Device, CommandPool, CommandBuffers, ArrayCount(CommandBuffers));
				SLogger::Log("Command pool and command buffers allocated.\n", LoggerVerbosity_Release);

				VkSemaphore AcquireSemaphores[FramesInFlight] = {};
				VkSemaphore ReleaseSemaphores[FramesInFlight] = {};
				VkFence FrameFences[FramesInFlight] = {};
				for (uint32_t I = 0; I < FramesInFlight; I++)
				{
					AcquireSemaphores[I] = CreateSemaphore(Device);
					ReleaseSemaphores[I] = CreateSemaphore(Device);
					FrameFences[I] = CreateFence(Device);
				}

				INIT_GPU_PROFILER(Instance, Device, PhysicalDeviceProps.limits.timestampPeriod);

				SVulkanContext Vulkan = {};
				Vulkan.Device = Device;
				Vulkan.MemoryAllocator = MemoryAllocator;
				Vulkan.bMinMaxSampler = bMinMaxSampler;
				Vulkan.CommandPool = CommandPool;
				Vulkan.GraphicsQueue = GraphicsQueue;
				Vulkan.SwapchainFormat = SwapchainFormat;
				Vulkan.DepthFormat = DepthFormat;
				Vulkan.InternalWidth = Swapchain.Width;
				Vulkan.InternalHeight = Swapchain.Height;
				Vulkan.Width = Swapchain.Width; 
				Vulkan.Height = Swapchain.Height;

#if ENGINE_RELEASE
				if ((InternalWidthSaved > 0) && (InternalHeightSaved > 0))
				{
					Vulkan.InternalWidth = InternalWidthSaved;
					Vulkan.InternalHeight = InternalHeightSaved;
				}
#endif

#ifndef ENGINE_RELEASE
				VkRenderPass ImguiRenderPass = InitializeDearImgui(Window, Instance, PhysicalDevice, Device, CommandBuffers[0], GraphicsFamilyIndex, GraphicsQueue, 0, Swapchain, SwapchainFormat);
				SLogger::Log("DearImGui initialized.\n\n", LoggerVerbosity_Debug);
				Vulkan.ImguiRenderPass = ImguiRenderPass;
#endif

				// Try to get pointer to vkCmdDrawIndexedIndirectCount
				if (bIndirectCountKHR)
				{
					Vulkan.vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR) vkGetInstanceProcAddr(Instance, "vkCmdDrawIndexedIndirectCountKHR");
				}
				else if (bIndirectCountAMD)
				{
					Vulkan.vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD) vkGetInstanceProcAddr(Instance, "vkCmdDrawIndexedIndirectCountAMD");
				}

				ShowWindow(Window, SW_SHOW);

				uint32_t FrameID = 0;
				float FrameCpuTime = 0.0f;
				while (bGlobalRunning)
				{
					BEGIN_PROFILER_BLOCK("FRAME_TIME");

					char FrameIDOutput[128] = {};
					snprintf(FrameIDOutput, sizeof(FrameIDOutput), "Frame %d started.\n", FrameID);
					SLogger::Log(FrameIDOutput, LoggerVerbosity_SuperDebug);

					LARGE_INTEGER FrameCpuBeginTime = WinGetWallClock();

					GameInput.MouseDeltaX = GameInput.MouseDeltaY = 0.0f;
                    GameInput.MouseWheelDelta = 0.0f;
					for (uint32_t I = 0; I < ArrayCount(GameInput.Buttons); I++)
					{
						GameInput.Buttons[I].HalfTransitionCount = 0;
					}

					MSG Message;
					while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) 
					{
						switch(Message.message)
						{
							case WM_QUIT:
							{
								bGlobalRunning = false;
							} break;

							default:
							{
								TranslateMessage(&Message);
								DispatchMessageA(&Message);
							} break;
						}
					}
					WinToggleFullscreen(Window);

					if (FrameID == 0)
					{
						GameInput.MouseDeltaX = GameInput.MouseDeltaY = 0.0f;
					}

					if (bGlobalWindowFocused)
					{
						WinProcessKeyboardMessage(GameInput.Buttons[Button_MouseLeft], GetKeyState(VK_LBUTTON) & (1 << 15));
						WinProcessKeyboardMessage(GameInput.Buttons[Button_MouseRight], GetKeyState(VK_RBUTTON) & (1 << 15));

						if (!bGlobalShowMouse)
						{
							RECT Rect;
							GetClientRect(Window, &Rect);

							int Width = Rect.right;
							int Height = Rect.bottom;
							if((GlobalPlatformData.MouseLastX != Width / 2) || (GlobalPlatformData.MouseLastY != Height / 2))
							{
								WinSetCursorPos(&GlobalPlatformData, Width / 2, Height / 2);
							}
						}
					}

					VkSurfaceCapabilitiesKHR SurfaceCaps = {};
					VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCaps));
					if ((SurfaceCaps.currentExtent.width != 0) && (SurfaceCaps.currentExtent.height != 0))
					{
						bool bSwapchainChanged = ResizeSwapchainIfChanged(Swapchain, Device, PhysicalDevice, Surface, SwapchainFormat, SurfaceCaps);
						if (bSwapchainChanged)
						{
							GameInput.MouseDeltaX = GameInput.MouseDeltaY = 0.0f;
						}
						bSwapchainChanged = bSwapchainChanged || ChangeVSyncIfNeeded(Swapchain, Device, PhysicalDevice, Surface, SwapchainFormat, GlobalPresentMode);

						uint32_t FrameInFlight = FrameID % FramesInFlight;

						VkCheck(vkWaitForFences(Device, 1, &FrameFences[FrameInFlight], VK_TRUE, UINT64_MAX));
						VkCheck(vkResetFences(Device, 1, &FrameFences[FrameInFlight]));

						uint32_t ImageIndex = 0;
						VkCheck(vkAcquireNextImageKHR(Device, Swapchain.VkSwapchain, UINT64_MAX, AcquireSemaphores[FrameInFlight], VK_NULL_HANDLE, &ImageIndex));
						SLogger::Log("Next swapchain image acquired.\n", LoggerVerbosity_SuperDebug);

						Vulkan.CommandBuffer = CommandBuffers[FrameInFlight];
						Vulkan.bSwapchainChanged = bSwapchainChanged;
						Vulkan.Width = Swapchain.Width; 
						Vulkan.Height = Swapchain.Height;
						Vulkan.FrameInFlight = FrameInFlight;

						if (!PlatformGetFullscreen())
						{
							Vulkan.InternalWidth = Swapchain.Width;
							Vulkan.InternalHeight = Swapchain.Height;
						}

						BEGIN_PROFILER_BLOCK("GAME_UPDATE_AND_RENDER");

						uint32_t SamplesToWrite = 0;
						SGameSoundBuffer GameSoundBuffer = {};
						if (WinSound.bInitialized)
						{
							if (WinSound.Buffers[WinSound.CurrentBuffer].bWasPrepared)
							{
								if (WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader.dwFlags & WHDR_DONE)
								{
									waveOutUnprepareHeader(WinSound.WaveOut, &WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader, sizeof(WAVEHDR));
									WinSound.Buffers[WinSound.CurrentBuffer].bWasPrepared = false;
								}
							}

							MMTIME SoundPosition = { TIME_SAMPLES };
							waveOutGetPosition(WinSound.WaveOut, &SoundPosition, sizeof(MMTIME));
							uint32_t TargetLatencySamples = uint32_t((WinSound.TargetLatency / 1000.0f) * WinSound.SamplesPerSec);
							int32_t Difference = int32_t(WinSound.SamplesWritten) - int32_t(SoundPosition.u.sample);
							SamplesToWrite = (int32_t(TargetLatencySamples) > Difference) ? TargetLatencySamples - Difference : 0;
							
							if (!WinSound.Buffers[WinSound.CurrentBuffer].bWasPrepared)
							{
								GameSoundBuffer.SamplesPerSec = WinSound.SamplesPerSec;
								GameSoundBuffer.ChannelCount = WinSound.ChannelCount;
								GameSoundBuffer.SampleCount = SamplesToWrite;
								GameSoundBuffer.Samples = (int16_t*) WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader.lpData;
							}
						}
						
	#ifndef ENGINE_RELEASE
						ImGui_ImplVulkan_NewFrame();
						ImGui_ImplWin32_NewFrame();
						ImGui::NewFrame();
	#endif

						GameInput.dt = FrameCpuTime;
						GameInput.FrameID = FrameID;
						GameInput.bShowMouse = bGlobalShowMouse;
						GameInput.PlatformMouseX = (GlobalPlatformData.MouseLastX + 0.5) - 0.5*Swapchain.Width;
						GameInput.PlatformMouseY = 0.5*Swapchain.Height - (GlobalPlatformData.MouseLastY + 0.5);
						VkImage FinalImage = GameUpdateAndRender(Vulkan, &GameMemory, GameInput, GameSoundBuffer);

						if (WinSound.bInitialized)
						{
							if (!WinSound.Buffers[WinSound.CurrentBuffer].bWasPrepared && (SamplesToWrite > 0))
							{
								WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader.dwBufferLength = SamplesToWrite * (WinSound.ChannelCount * (WinSound.BitsPerChannel / 8));

								WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader.dwFlags = 0;
								waveOutPrepareHeader(WinSound.WaveOut, &WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader, sizeof(WAVEHDR));
								waveOutWrite(WinSound.WaveOut, &WinSound.Buffers[WinSound.CurrentBuffer].WaveHeader, sizeof(WAVEHDR));
								WinSound.Buffers[WinSound.CurrentBuffer].bWasPrepared = true;
								
								WinSound.SamplesWritten += SamplesToWrite;
								WinSound.CurrentBuffer = (WinSound.CurrentBuffer + 1) % ArrayCount(WinSound.Buffers);
							}
						}

						SLogger::Log("Game updated and rendered.\n", LoggerVerbosity_SuperDebug);

						END_PROFILER_BLOCK("GAME_UPDATE_AND_RENDER");


						BEGIN_GPU_PROFILER_BLOCK("BLIT_TO_SWAPCHAIN", CommandBuffers[FrameInFlight], FrameInFlight);

						// Copy final image to backbuffer
						VkImageMemoryBarrier RenderBackbufferBeginBarrier = CreateImageMemoryBarrier(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Swapchain.Images[ImageIndex], VK_IMAGE_ASPECT_COLOR_BIT);
						vkCmdPipelineBarrier(CommandBuffers[FrameInFlight], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderBackbufferBeginBarrier);

						VkImageBlit BlitRegion = {};
						BlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						BlitRegion.srcSubresource.baseArrayLayer = 0;
						BlitRegion.srcSubresource.layerCount = 1;
						BlitRegion.srcOffsets[1] = { int(Swapchain.Width), int(Swapchain.Height), 1 };
						BlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						BlitRegion.dstSubresource.baseArrayLayer = 0;
						BlitRegion.dstSubresource.layerCount = 1;
						BlitRegion.dstOffsets[1] = { int(Swapchain.Width), int(Swapchain.Height), 1 };
						vkCmdBlitImage(CommandBuffers[FrameInFlight], FinalImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Swapchain.Images[ImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BlitRegion, VK_FILTER_NEAREST);

						VkImageMemoryBarrier RenderBackbufferEndBarrier = CreateImageMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, Swapchain.Images[ImageIndex], VK_IMAGE_ASPECT_COLOR_BIT);
						vkCmdPipelineBarrier(CommandBuffers[FrameInFlight], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderBackbufferEndBarrier);

						END_GPU_PROFILER_BLOCK("BLIT_TO_SWAPCHAIN", CommandBuffers[FrameInFlight], FrameInFlight);

						VkCheck(vkEndCommandBuffer(CommandBuffers[FrameInFlight]));

						VkPipelineStageFlags SubmitWaitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
						VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
						SubmitInfo.waitSemaphoreCount = 1;
						SubmitInfo.pWaitSemaphores = &AcquireSemaphores[FrameInFlight];
						SubmitInfo.pWaitDstStageMask = &SubmitWaitStage;
						SubmitInfo.commandBufferCount = 1;
						SubmitInfo.pCommandBuffers = &CommandBuffers[FrameInFlight];
						SubmitInfo.signalSemaphoreCount = 1;
						SubmitInfo.pSignalSemaphores = &ReleaseSemaphores[FrameInFlight];
						VkCheck(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, FrameFences[FrameInFlight]));

						VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
						PresentInfo.waitSemaphoreCount = 1;
						PresentInfo.pWaitSemaphores = &ReleaseSemaphores[FrameInFlight];
						PresentInfo.swapchainCount = 1;
						PresentInfo.pSwapchains = &Swapchain.VkSwapchain;
						PresentInfo.pImageIndices = &ImageIndex;
						VkCheck(vkQueuePresentKHR(GraphicsQueue, &PresentInfo));
					}

					END_PROFILER_BLOCK("FRAME_TIME");

					// OUTPUT_PROFILER_INFO();
					CLEAR_PROFILER_INFO();

					if (FrameID > (FramesInFlight - 2))
					{
						// OUTPUT_GPU_PROFILER_INFO(Device, FrameID % FramesInFlight);
					}

					LARGE_INTEGER FrameCpuEndTime = WinGetWallClock();
					float FrameCpuTimeTemp = WinGetSecondsElapsed(FrameCpuBeginTime, FrameCpuEndTime);
					// NOTE(georgii): For breakpoints and other shit
					if (FrameCpuTimeTemp < 1.0f)
					{
						FrameCpuTime = 0.9f * FrameCpuTime + 0.1f * FrameCpuTimeTemp;
					}

					char FrameTimeText[64];
					snprintf(FrameTimeText, sizeof(FrameTimeText), "%.3fms/f %dFPS\n\n", 1000.0f * FrameCpuTime, (uint32_t)(1 / FrameCpuTime));
					// PlatformOutputDebugString(FrameTimeText);

					FrameID++;
				}
			}
			else
			{
				PlatformOutputDebugString("Can't allocate memory for the game.\n");
			}
		}
		else
		{
			PlatformOutputDebugString("Can't create window.\n");
		}
	}
	else
	{
		PlatformOutputDebugString("Can't register window class.\n");
	}

	SLogger::Finish();

	return 0;
}