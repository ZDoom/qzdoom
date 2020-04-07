/*
**  Softpoly backend
**  Copyright (c) 2016-2020 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#include "polyrenderer/backend/poly_renderstate.h"
#include "polyrenderer/backend/poly_framebuffer.h"
#include "polyrenderer/backend/poly_hwtexture.h"
#include "templates.h"
#include "doomstat.h"
#include "r_data/colormaps.h"
#include "hwrenderer/scene/hw_skydome.h"
#include "hwrenderer/scene/hw_viewpointuniforms.h"
#include "hwrenderer/dynlights/hw_lightbuffer.h"
#include "hwrenderer/utility/hw_cvars.h"
#include "hwrenderer/utility/hw_clock.h"
#include "hwrenderer/data/flatvertices.h"
#include "hwrenderer/data/hw_viewpointbuffer.h"
#include "hwrenderer/data/shaderuniforms.h"
#include "swrenderer/r_swcolormaps.h"

static PolyDrawMode dtToDrawMode[] =
{
	PolyDrawMode::Points,
	PolyDrawMode::Lines,
	PolyDrawMode::Triangles,
	PolyDrawMode::TriangleFan,
	PolyDrawMode::TriangleStrip,
};

PolyRenderState::PolyRenderState()
{
	mIdentityMatrix.loadIdentity();
	Reset();
}

void PolyRenderState::ClearScreen()
{
	screen->mViewpoints->Set2D(*this, SCREENWIDTH, SCREENHEIGHT);
	SetColor(0, 0, 0);
	Draw(DT_TriangleStrip, FFlatVertexBuffer::FULLSCREEN_INDEX, 4, true);
}

void PolyRenderState::Draw(int dt, int index, int count, bool apply)
{
	if (apply || mNeedApply)
		Apply();

	mDrawCommands->Draw(index, count, dtToDrawMode[dt]);
}

void PolyRenderState::DrawIndexed(int dt, int index, int count, bool apply)
{
	if (apply || mNeedApply)
		Apply();

	mDrawCommands->DrawIndexed(index, count, dtToDrawMode[dt]);
}

bool PolyRenderState::SetDepthClamp(bool on)
{
	bool lastValue = mDepthClamp;
	mDepthClamp = on;
	mNeedApply = true;
	return lastValue;
}

void PolyRenderState::SetDepthMask(bool on)
{
	mDepthMask = on;
	mNeedApply = true;
}

void PolyRenderState::SetDepthFunc(int func)
{
	mDepthFunc = func;
	mNeedApply = true;
}

void PolyRenderState::SetDepthRange(float min, float max)
{
	mDepthRangeMin = min;
	mDepthRangeMax = max;
	mNeedApply = true;
}

void PolyRenderState::SetColorMask(bool r, bool g, bool b, bool a)
{
	mColorMask[0] = r;
	mColorMask[1] = g;
	mColorMask[2] = b;
	mColorMask[3] = a;
	mNeedApply = true;
}

void PolyRenderState::SetStencil(int offs, int op, int flags)
{
	mStencilValue = screen->stencilValue + offs;
	mStencilOp = op;
	mNeedApply = true;

	if (flags != -1)
	{
		bool cmon = !(flags & SF_ColorMaskOff);
		SetColorMask(cmon, cmon, cmon, cmon); // don't write to the graphics buffer
		SetDepthMask(!(flags & SF_DepthMaskOff));
	}
}

void PolyRenderState::SetCulling(int mode)
{
	mCulling = mode;
	mNeedApply = true;
}

void PolyRenderState::EnableClipDistance(int num, bool state)
{
}

void PolyRenderState::Clear(int targets)
{
	if (mNeedApply)
		Apply();

	//if (targets & CT_Color)
	//	mDrawCommands->ClearColor();
	if (targets & CT_Depth)
		mDrawCommands->ClearDepth(65535.0f);
	if (targets & CT_Stencil)
		mDrawCommands->ClearStencil(0);
}

void PolyRenderState::EnableStencil(bool on)
{
	mStencilEnabled = on;
	mNeedApply = true;
}

void PolyRenderState::SetScissor(int x, int y, int w, int h)
{
	if (w < 0)
	{
		x = 0;
		y = 0;
		w = mRenderTarget.Canvas->GetWidth();
		h = mRenderTarget.Canvas->GetHeight();
	}
	mScissor.x = x;
	mScissor.y = mRenderTarget.Canvas->GetHeight() - y - h;
	mScissor.width = w;
	mScissor.height = h;
	mNeedApply = true;
}

void PolyRenderState::SetViewport(int x, int y, int w, int h)
{
	auto fb = GetPolyFrameBuffer();
	if (w < 0)
	{
		x = 0;
		y = 0;
		w = mRenderTarget.Canvas->GetWidth();
		h = mRenderTarget.Canvas->GetHeight();
	}
	mViewport.x = x;
	mViewport.y = mRenderTarget.Canvas->GetHeight() - y - h;
	mViewport.width = w;
	mViewport.height = h;
	mNeedApply = true;
}

void PolyRenderState::EnableDepthTest(bool on)
{
	mDepthTest = on;
	mNeedApply = true;
}

void PolyRenderState::EnableMultisampling(bool on)
{
}

void PolyRenderState::EnableLineSmooth(bool on)
{
}

void PolyRenderState::EnableDrawBuffers(int count)
{
}

void PolyRenderState::SetColormapShader(bool enable)
{
	mNeedApply = true;
	mColormapShader = enable;
}

void PolyRenderState::EndRenderPass()
{
	mDrawCommands = nullptr;
	mNeedApply = true;
	mFirstMatrixApply = true;
}

void PolyRenderState::Apply()
{
	drawcalls.Clock();

	if (!mDrawCommands)
	{
		mDrawCommands = GetPolyFrameBuffer()->GetDrawCommands();
	}

	if (mNeedApply)
	{
		mDrawCommands->SetViewport(mViewport.x, mViewport.y, mViewport.width, mViewport.height, mRenderTarget.Canvas, mRenderTarget.DepthStencil, mRenderTarget.TopDown);
		mDrawCommands->SetScissor(mScissor.x, mScissor.y, mScissor.width, mScissor.height);
		mDrawCommands->SetViewpointUniforms(mViewpointUniforms);
		mDrawCommands->SetDepthClamp(mDepthClamp);
		mDrawCommands->SetDepthMask(mDepthTest && mDepthMask);
		mDrawCommands->SetDepthFunc(mDepthTest ? mDepthFunc : DF_Always);
		mDrawCommands->SetDepthRange(mDepthRangeMin, mDepthRangeMax);
		mDrawCommands->SetStencil(mStencilValue, mStencilOp);
		mDrawCommands->EnableStencil(mStencilEnabled);
		mDrawCommands->SetCulling(mCulling);
		mDrawCommands->SetColorMask(mColorMask[0], mColorMask[1], mColorMask[2], mColorMask[3]);
		mNeedApply = false;
	}

	int fogset = 0;
	if (mFogEnabled)
	{
		if (mFogEnabled == 2)
		{
			fogset = -3;	// 2D rendering with 'foggy' overlay.
		}
		else if ((GetFogColor() & 0xffffff) == 0)
		{
			fogset = gl_fogmode;
		}
		else
		{
			fogset = -gl_fogmode;
		}
	}

	ApplyMaterial();

	if (mVertexBuffer) mDrawCommands->SetVertexBuffer(mVertexBuffer->Memory());
	if (mIndexBuffer) mDrawCommands->SetIndexBuffer(mIndexBuffer->Memory());
	mDrawCommands->SetInputAssembly(static_cast<PolyVertexBuffer*>(mVertexBuffer)->VertexFormat);
	mDrawCommands->SetRenderStyle(mRenderStyle);

	if (mColormapShader)
	{
		mDrawCommands->SetShader(EFF_NONE, 0, false, true);
	}
	else if (mSpecialEffect > EFF_NONE)
	{
		mDrawCommands->SetShader(mSpecialEffect, 0, false, false);
	}
	else
	{
		int effectState = mMaterial.mOverrideShader >= 0 ? mMaterial.mOverrideShader : (mMaterial.mMaterial ? mMaterial.mMaterial->GetShaderIndex() : 0);
		mDrawCommands->SetShader(EFF_NONE, mTextureEnabled ? effectState : SHADER_NoTexture, mAlphaThreshold >= 0.f, false);
	}

	if (mMaterial.mMaterial && mMaterial.mMaterial->tex)
		mStreamData.timer = static_cast<float>((double)(screen->FrameTime - firstFrame) * (double)mMaterial.mMaterial->tex->shaderspeed / 1000.);
	else
		mStreamData.timer = 0.0f;

	PolyPushConstants constants;
	constants.uFogEnabled = fogset;
	constants.uTextureMode = mTextureMode == TM_NORMAL && mTempTM == TM_OPAQUE ? TM_OPAQUE : mTextureMode;
	constants.uLightDist = mLightParms[0];
	constants.uLightFactor = mLightParms[1];
	constants.uFogDensity = mLightParms[2];
	constants.uLightLevel = mLightParms[3];
	constants.uAlphaThreshold = mAlphaThreshold;
	constants.uClipSplit = { mClipSplit[0], mClipSplit[1] };
	constants.uLightIndex = mLightIndex;

	mDrawCommands->PushStreamData(mStreamData, constants);
	ApplyMatrices();

	if (mBias.mChanged)
	{
		mDrawCommands->SetDepthBias(mBias.mUnits, mBias.mFactor);
		mBias.mChanged = false;
	}

	drawcalls.Unclock();
}

void PolyRenderState::ApplyMaterial()
{
	if (mMaterial.mChanged && mMaterial.mMaterial)
	{
		mTempTM = mMaterial.mMaterial->tex->isHardwareCanvas() ? TM_OPAQUE : TM_NORMAL;

		auto base = static_cast<PolyHardwareTexture*>(mMaterial.mMaterial->GetLayer(0, mMaterial.mTranslation));
		if (base)
		{
			DCanvas *texcanvas = base->GetImage(mMaterial);
			mDrawCommands->SetTexture(0, texcanvas->GetPixels(), texcanvas->GetWidth(), texcanvas->GetHeight(), texcanvas->IsBgra());

			int numLayers = mMaterial.mMaterial->GetLayers();
			for (int i = 1; i < numLayers; i++)
			{
				FTexture* layer;
				auto systex = static_cast<PolyHardwareTexture*>(mMaterial.mMaterial->GetLayer(i, 0, &layer));

				texcanvas = systex->GetImage(layer, 0, mMaterial.mMaterial->isExpanded() ? CTF_Expand : 0);
				mDrawCommands->SetTexture(i, texcanvas->GetPixels(), texcanvas->GetWidth(), texcanvas->GetHeight(), texcanvas->IsBgra());
			}
		}

		mMaterial.mChanged = false;
	}
}

template<typename T>
static void BufferedSet(bool &modified, T &dst, const T &src)
{
	if (dst == src)
		return;
	dst = src;
	modified = true;
}

static void BufferedSet(bool &modified, VSMatrix &dst, const VSMatrix &src)
{
	if (memcmp(dst.get(), src.get(), sizeof(FLOATTYPE) * 16) == 0)
		return;
	dst = src;
	modified = true;
}

void PolyRenderState::ApplyMatrices()
{
	bool modified = mFirstMatrixApply;
	if (mTextureMatrixEnabled)
	{
		BufferedSet(modified, mMatrices.TextureMatrix, mTextureMatrix);
	}
	else
	{
		BufferedSet(modified, mMatrices.TextureMatrix, mIdentityMatrix);
	}

	if (mModelMatrixEnabled)
	{
		BufferedSet(modified, mMatrices.ModelMatrix, mModelMatrix);
		if (modified)
			mMatrices.NormalModelMatrix.computeNormalMatrix(mModelMatrix);
	}
	else
	{
		BufferedSet(modified, mMatrices.ModelMatrix, mIdentityMatrix);
		BufferedSet(modified, mMatrices.NormalModelMatrix, mIdentityMatrix);
	}

	if (modified)
	{
		mFirstMatrixApply = false;
		mDrawCommands->PushMatrices(mMatrices.ModelMatrix, mMatrices.NormalModelMatrix, mMatrices.TextureMatrix);
	}
}

void PolyRenderState::SetRenderTarget(DCanvas *canvas, PolyDepthStencil *depthStencil, bool topdown)
{
	mRenderTarget.Canvas = canvas;
	mRenderTarget.DepthStencil = depthStencil;
	mRenderTarget.TopDown = topdown;
}

void PolyRenderState::Bind(PolyDataBuffer *buffer, uint32_t offset, uint32_t length)
{
	if (buffer->bindingpoint == VIEWPOINT_BINDINGPOINT)
	{
		mViewpointUniforms = reinterpret_cast<HWViewpointUniforms*>(static_cast<uint8_t*>(buffer->Memory()) + offset);
		mNeedApply = true;
	}
}

PolyVertexInputAssembly *PolyRenderState::GetVertexFormat(int numBindingPoints, int numAttributes, size_t stride, const FVertexBufferAttribute *attrs)
{
	for (size_t i = 0; i < mVertexFormats.size(); i++)
	{
		auto f = mVertexFormats[i].get();
		if (f->Attrs.size() == (size_t)numAttributes && f->NumBindingPoints == numBindingPoints && f->Stride == stride)
		{
			bool matches = true;
			for (int j = 0; j < numAttributes; j++)
			{
				if (memcmp(&f->Attrs[j], &attrs[j], sizeof(FVertexBufferAttribute)) != 0)
				{
					matches = false;
					break;
				}
			}

			if (matches)
				return f;
		}
	}

	auto fmt = std::make_unique<PolyVertexInputAssembly>();
	fmt->NumBindingPoints = numBindingPoints;
	fmt->Stride = stride;
	fmt->UseVertexData = 0;
	for (int j = 0; j < numAttributes; j++)
	{
		if (attrs[j].location == VATTR_COLOR)
			fmt->UseVertexData |= 1;
		else if (attrs[j].location == VATTR_NORMAL)
			fmt->UseVertexData |= 2;
		fmt->Attrs.push_back(attrs[j]);
	}

	for (int j = 0; j < numAttributes; j++)
	{
		fmt->mOffsets[attrs[j].location] = attrs[j].offset;
	}
	fmt->mStride = stride;

	mVertexFormats.push_back(std::move(fmt));
	return mVertexFormats.back().get();
}
