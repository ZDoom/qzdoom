#pragma once

#include "win32basevideo.h"
#include "c_cvars.h"
#include "rendering/polyrenderer/backend/poly_framebuffer.h"

EXTERN_CVAR(Bool, vid_fullscreen)

class Win32PolyVideo : public Win32BaseVideo
{
public:
	void Shutdown() override
	{
	}

	DFrameBuffer *CreateFrameBuffer() override
	{
		auto fb = new PolyFrameBuffer(m_hMonitor, vid_fullscreen);
		return fb;
	}
};
