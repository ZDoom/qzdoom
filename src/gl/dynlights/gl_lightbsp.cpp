// 
//---------------------------------------------------------------------------
// Doom BSP tree on the GPU
// Copyright(C) 2017 Magnus Norddahl
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include "gl/system/gl_system.h"
#include "gl/shaders/gl_shader.h"
#include "gl/dynlights/gl_lightbsp.h"
#include "gl/system/gl_interface.h"
#include "r_state.h"

int FLightBSP::GetNodesBuffer()
{
	UpdateBuffers();
	return NodesBuffer;
}

int FLightBSP::GetSegsBuffer()
{
	UpdateBuffers();
	return SegsBuffer;
}

void FLightBSP::UpdateBuffers()
{
	if (numnodes != NumNodes || numsegs != NumSegs) // To do: there is probably a better way to detect a map change than this..
		Clear();

	if (NodesBuffer == 0)
		GenerateBuffers();
}

void FLightBSP::GenerateBuffers()
{
	UploadNodes();
	UploadSegs();
}

void FLightBSP::UploadNodes()
{
	TArray<GPUNode> gpunodes;
	gpunodes.Resize(numnodes);
	for (int i = 0; i < numnodes; i++)
	{
		const auto &node = nodes[i];
		auto &gpunode = gpunodes[i];

		gpunode.x = FIXED2FLOAT(node.x);
		gpunode.y = FIXED2FLOAT(node.y);
		gpunode.dx = FIXED2FLOAT(node.dx);
		gpunode.dy = FIXED2FLOAT(node.dy);

		for (int j = 0; j < 2; j++)
		{
			bool isNode = (!((size_t)node.children[j] & 1));
			if (isNode)
			{
				node_t *bsp = (node_t *)node.children[j];
				gpunode.children[j] = (int)(ptrdiff_t)(bsp - nodes);
				gpunode.linecount[j] = -1;
			}
			else
			{
				subsector_t *sub = (subsector_t *)((BYTE *)node.children[j] - 1);
				gpunode.children[j] = (sub->numlines > 0) ? (int)(ptrdiff_t)(sub->firstline - segs) : 0;
				gpunode.linecount[j] = sub->numlines;
			}
		}
	}

	int oldBinding = 0;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &oldBinding);

	glGenBuffers(1, (GLuint*)&NodesBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, NodesBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUNode) * gpunodes.Size(), &gpunodes[0], GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, oldBinding);

	NumNodes = numnodes;
}

void FLightBSP::UploadSegs()
{
	TArray<GPUSeg> gpusegs;
	gpusegs.Resize(numsegs);
	for (int i = 0; i < numsegs; i++)
	{
		const auto &seg = segs[i];
		auto &gpuseg = gpusegs[i];

		gpuseg.x = (float)seg.v1->fX();
		gpuseg.y = (float)seg.v2->fY();
		gpuseg.nx = (float)-(seg.v2->fY() - seg.v1->fY());
		gpuseg.ny = (float)(seg.v2->fX() - seg.v1->fX());
		float length = sqrt(gpuseg.nx * gpuseg.nx + gpuseg.ny * gpuseg.ny);
		gpuseg.nx /= length;
		gpuseg.ny /= length;
		gpuseg.bSolid = (seg.backsector == nullptr) ? 1.0f : 0.0f;
		gpuseg.padding1 = 0.0f;
		gpuseg.padding2 = 0.0f;
		gpuseg.padding3 = 0.0f;
	}

	int oldBinding = 0;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &oldBinding);

	glGenBuffers(1, (GLuint*)&SegsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SegsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUSeg) * gpusegs.Size(), &gpusegs[0], GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, oldBinding);

	NumSegs = numsegs;
}

void FLightBSP::Clear()
{
	if (NodesBuffer == 0)
	{
		glDeleteBuffers(1, (GLuint*)&NodesBuffer);
		NodesBuffer = 0;
	}
	if (SegsBuffer == 0)
	{
		glDeleteBuffers(1, (GLuint*)&SegsBuffer);
		SegsBuffer = 0;
	}
}
