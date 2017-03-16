
#pragma once

#include <d3d11.h>

template <typename Type>
class ComPtr
{
public:
	ComPtr() : ptr(0) { }
	explicit ComPtr(Type *ptr) : ptr(ptr) { }
	ComPtr(const ComPtr &copy) : ptr(copy.ptr) { if (ptr) ptr->AddRef(); }
	~ComPtr() { Clear(); }
	ComPtr &operator =(const ComPtr &copy)
	{
		if (this == &copy)
			return *this;
		if (copy.ptr)
			copy.ptr->AddRef();
		if (ptr)
			ptr->Release();
		ptr = copy.ptr;
		return *this;
	}

	template<typename That>
	explicit ComPtr(const ComPtr<That> &that)
		: ptr(static_cast<Type*>(that.ptr))
	{
		if (ptr)
			ptr->AddRef();
	}

	bool operator ==(const ComPtr &other) const { return ptr == other.ptr; }
	bool operator !=(const ComPtr &other) const { return ptr != other.ptr; }
	bool operator <(const ComPtr &other) const { return ptr < other.ptr; }
	bool operator <=(const ComPtr &other) const { return ptr <= other.ptr; }
	bool operator >(const ComPtr &other) const { return ptr > other.ptr; }
	bool operator >=(const ComPtr &other) const { return ptr >= other.ptr; }

	// const does not exist in COM, so we drop the const qualifier on returned objects to avoid needing mutable variables elsewhere

	Type * const operator ->() const { return const_cast<Type*>(ptr); }
	Type *operator ->() { return ptr; }
	operator Type *() const { return const_cast<Type*>(ptr); }
	operator bool() const { return ptr != 0; }

	bool IsNull() const { return ptr == 0; }
	void Clear() { if (ptr) ptr->Release(); ptr = 0; }
	Type *Get() const { return const_cast<Type*>(ptr); }
	Type **OutputVariable() { Clear(); return &ptr; }

private:
	Type *ptr;
};

class D3D11FB : public BaseWinFB
{
	DECLARE_CLASS(D3D11FB, BaseWinFB)
	D3D11FB() { }
public:
	D3D11FB(int width, int height, bool bgra, bool fullscreen);
	~D3D11FB();

	// BaseWinFB interface (legacy junk we don't need to support):
	void Blank() override { }
	bool PaintToWindow() override { return true; }
	long GetHR() override { return 0; }
	bool CreateResources() override { return true; }
	void ReleaseResources() override { }
	void PaletteChanged() override { }
	int QueryNewPalette() override { return 0; }
	bool Is8BitMode() override { return false; }
	bool IsValid() override { return true; }

	// DFrameBuffer interface:
	bool Lock(bool buffered) override;
	void Unlock() override;
	bool Begin2D(bool copy3d) override;
	void Update() override;
	PalEntry *GetPalette() override;
	void GetFlashedPalette(PalEntry palette[256]) override;
	void UpdatePalette() override;
	bool SetGamma(float gamma) override;
	bool SetFlash(PalEntry rgb, int amount) override;
	void GetFlash(PalEntry &rgb, int &amount) override;
	int GetPageCount() override { return 2; }
	// void SetVSync(bool vsync) override;
	// void NewRefreshRate() override;
	// void SetBlendingRect(int x1, int y1, int x2, int y2) override;
	// void DrawBlendingRect() override;
	// FNativeTexture *CreateTexture(FTexture *gametex, bool wrapping) override;
	// FNativePalette *CreatePalette(FRemapTable *remap) override;
	// void GameRestart() override;
	// bool WipeStartScreen(int type) override;
	// void WipeEndScreen() override;
	// bool WipeDo(int ticks) override;
	// void WipeCleanup() override;

private:
	void SetInitialWindowLocation();
	void CreateFBTexture();
	void CreateFBStagingTexture();

	PalEntry mPalette[256];
	PalEntry mFlashColor = 0;
	int mFlashAmount = 0;

	float mGamma = 1.0f;
	float mLastGamma = 0.0f;

	D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL();
	ComPtr<ID3D11Device> mDevice;
	ComPtr<ID3D11DeviceContext> mDeviceContext;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D11Texture2D> mFBTexture;
	ComPtr<ID3D11Texture2D> mFBStaging;
	D3D11_MAPPED_SUBRESOURCE mMappedFBStaging = D3D11_MAPPED_SUBRESOURCE();
};

typedef HRESULT(WINAPI *FuncD3D11CreateDeviceAndSwapChain)(
	__in_opt IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	__in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	__in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	__out_opt IDXGISwapChain** ppSwapChain,
	__out_opt ID3D11Device** ppDevice,
	__out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
	__out_opt ID3D11DeviceContext** ppImmediateContext);

extern HMODULE D3D11_dll;
extern FuncD3D11CreateDeviceAndSwapChain D3D11_createdeviceandswapchain;
