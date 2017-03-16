
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include "doomtype.h"
#include "c_dispatch.h"
#include "templates.h"
#include "i_system.h"
#include "i_video.h"
#include "i_input.h"
#include "v_video.h"
#include "v_pfx.h"
#include "stats.h"
#include "doomerrors.h"
#include "r_data/r_translate.h"
#include "f_wipe.h"
#include "sbar.h"
#include "win32iface.h"
#include "win32swiface.h"
#include "doomstat.h"
#include "v_palette.h"
#include "w_wad.h"
#include "textures.h"
#include "r_data/colormaps.h"
#include "SkylineBinPack.h"
#include "swrenderer/scene/r_light.h"
#include "fb_d3d11.h"

extern HWND Window;
extern IVideo *Video;
extern BOOL AppActive;
extern int SessionState;
extern bool VidResizing;

HMODULE D3D11_dll;
FuncD3D11CreateDeviceAndSwapChain D3D11_createdeviceandswapchain;

IMPLEMENT_CLASS(D3D11FB, false, false)

D3D11FB::D3D11FB(int width, int height, bool bgra, bool fullscreen) : BaseWinFB(width, height, bgra)
{
	memcpy(mPalette, GPalette.BaseColors, sizeof(PalEntry) * 256);

	std::vector<D3D_FEATURE_LEVEL> requestLevels =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;// DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_B8G8R8A8_UNORM (except 10.x on Windows Vista)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = Window;
	swapChainDesc.Windowed = TRUE; // Seems the MSDN documentation wants us to call IDXGISwapChain::SetFullscreenState afterwards
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT result = D3D11_createdeviceandswapchain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, requestLevels.data(), (UINT)requestLevels.size(), D3D11_SDK_VERSION, &swapChainDesc, mSwapChain.OutputVariable(), mDevice.OutputVariable(), &mFeatureLevel, mDeviceContext.OutputVariable());
	if (FAILED(result))
		I_FatalError("D3D11CreateDeviceAndSwapChain failed");

	CreateFBTexture();
	CreateFBStagingTexture();
	SetInitialWindowLocation();

	if (!Windowed)
		mSwapChain->SetFullscreenState(TRUE, nullptr);
}

D3D11FB::~D3D11FB()
{
}

bool D3D11FB::Lock(bool buffered)
{
	if (LockCount++ > 0) return false;

	HRESULT result = mDeviceContext->Map(mFBStaging, 0, D3D11_MAP_READ_WRITE, 0, &mMappedFBStaging);
	if (FAILED(result))
		I_FatalError("Map failed");

	int pixelsize = IsBgra() ? 4 : 1;
	Buffer = (uint8_t*)mMappedFBStaging.pData;
	Pitch = mMappedFBStaging.RowPitch / pixelsize;
	return true;
}

void D3D11FB::Unlock()
{
	if (LockCount == 0) return;
	if (--LockCount == 0)
	{
		Buffer = nullptr;
		mDeviceContext->Unmap(mFBStaging, 0);
	}
}

bool D3D11FB::Begin2D(bool copy3d)
{
	// To do: call Update if Lock was called
	return false;
}

void D3D11FB::Update()
{
	DrawRateStuff();

	bool isLocked = LockCount > 0;
	if (isLocked)
	{
		Buffer = nullptr;
		mDeviceContext->Unmap(mFBStaging, 0);
	}

	int pixelsize = IsBgra() ? 4 : 1;
	//mDeviceContext->UpdateSubresource(mFBTexture.Get(), 0, nullptr, MemBuffer, Pitch * pixelsize, 0);
	mDeviceContext->CopyResource(mFBTexture, mFBStaging);

	ComPtr<ID3D11Texture2D> backbuffer;
	HRESULT result = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuffer.OutputVariable());
	if (SUCCEEDED(result))
		mDeviceContext->CopyResource(backbuffer, mFBTexture);
	backbuffer.Clear();

	// Limiting the frame rate is as simple as waiting for the timer to signal this event.
	I_FPSLimit();
	mSwapChain->Present(1, 0);

	if (Windowed)
	{
		RECT box;
		GetClientRect(Window, &box);
		if (box.right > 0 && box.right > 0 && (Width != box.right || Height != box.bottom))
		{
			Resize(box.right, box.bottom);

			result = mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
			if (FAILED(result))
				I_FatalError("ResizeBuffers failed");
			CreateFBTexture();
			CreateFBStagingTexture();

			V_OutputResized(Width, Height);
		}
	}

	if (isLocked)
	{
		result = mDeviceContext->Map(mFBStaging, 0, D3D11_MAP_READ_WRITE, 0, &mMappedFBStaging);
		if (FAILED(result))
			I_FatalError("Map failed");
		Buffer = (uint8_t*)mMappedFBStaging.pData;
		Pitch = mMappedFBStaging.RowPitch / pixelsize;
	}
}

PalEntry *D3D11FB::GetPalette()
{
	return mPalette;
}

void D3D11FB::GetFlashedPalette(PalEntry palette[256])
{
	memcpy(palette, mPalette, 256 * sizeof(PalEntry));
	if (mFlashAmount)
	{
		DoBlending(palette, palette, 256, mFlashColor.r, mFlashColor.g, mFlashColor.b, mFlashAmount);
	}
}

void D3D11FB::UpdatePalette()
{
	// NeedPalUpdate = true;
}

bool D3D11FB::SetGamma(float gamma)
{
	mGamma = gamma;
	return true;
}

bool D3D11FB::SetFlash(PalEntry rgb, int amount)
{
	mFlashColor = rgb;
	mFlashAmount = amount;

	// Fill in the constants for the pixel shader to do linear interpolation between the palette and the flash:
	//float r = rgb.r / 255.f, g = rgb.g / 255.f, b = rgb.b / 255.f, a = amount / 256.f;
	//FlashColor0 = D3DCOLOR_COLORVALUE(r * a, g * a, b * a, 0);
	//a = 1 - a;
	//FlashColor1 = D3DCOLOR_COLORVALUE(a, a, a, 1);
	return true;
}

void D3D11FB::GetFlash(PalEntry &rgb, int &amount)
{
	rgb = mFlashColor;
	amount = mFlashAmount;
}

void D3D11FB::CreateFBTexture()
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = Width;
	desc.Height = Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = IsBgra() ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;

	HRESULT result = mDevice->CreateTexture2D(&desc, 0, (ID3D11Texture2D **)mFBTexture.OutputVariable());
	if (FAILED(result))
		I_FatalError("Could not create frame buffer texture: CreateTexture2D failed");
}

void D3D11FB::CreateFBStagingTexture()
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = Width;
	desc.Height = Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = IsBgra() ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	HRESULT result = mDevice->CreateTexture2D(&desc, 0, (ID3D11Texture2D **)mFBStaging.OutputVariable());
	if (FAILED(result))
		I_FatalError("Could not create frame buffer texture: CreateTexture2D failed");
}

void D3D11FB::SetInitialWindowLocation()
{
	RECT r;
	LONG style, exStyle;

	ShowWindow(Window, SW_SHOW);
	GetWindowRect(Window, &r);
	style = WS_VISIBLE | WS_CLIPSIBLINGS;
	exStyle = 0;

	if (!Windowed)
	{
		style |= WS_POPUP;
	}
	else
	{
		style |= WS_OVERLAPPEDWINDOW;
		exStyle |= WS_EX_WINDOWEDGE;
	}

	SetWindowLong(Window, GWL_STYLE, style);
	SetWindowLong(Window, GWL_EXSTYLE, exStyle);
	SetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	if (!Windowed)
	{
		int monX = 0;
		int monY = 0;
		MoveWindow(Window, monX, monY, Width, GetTrueHeight(), FALSE);
	}
	else
	{
		RECT windowRect;
		windowRect.left = r.left;
		windowRect.top = r.top;
		windowRect.right = windowRect.left + Width;
		windowRect.bottom = windowRect.top + Height;
		AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);
		MoveWindow(Window, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);

		I_RestoreWindowedPos();
	}
}
