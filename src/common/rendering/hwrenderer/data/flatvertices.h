
#ifndef _HW__VERTEXBUFFER_H
#define _HW__VERTEXBUFFER_H

#include "tarray.h"
#include "hwrenderer/data/buffers.h"
#include <atomic>
#include <mutex>

class FRenderState;
struct secplane_t;

struct FFlatVertex
{
	float x, z, y;	// world position
	float u, v;		// texture coordinates
	float lu, lv;	// lightmap texture coordinates
	float lindex;	// lightmap texture index

	void Set(float xx, float zz, float yy, float uu, float vv)
	{
		x = xx;
		z = zz;
		y = yy;
		u = uu;
		v = vv;
		lindex = -1.0f;
	}

	void Set(float xx, float zz, float yy, float uu, float vv, float llu, float llv, float llindex)
	{
		x = xx;
		z = zz;
		y = yy;
		u = uu;
		v = vv;
		lu = llu;
		lv = llv;
		lindex = llindex;
	}

	void SetVertex(float _x, float _y, float _z = 0)
	{
		x = _x;
		z = _y;
		y = _z;
	}

	void SetTexCoord(float _u = 0, float _v = 0)
	{
		u = _u;
		v = _v;
	}

};

class FFlatVertexBuffer
{
public:
	TArray<FFlatVertex> vbo_shadowdata;
	TArray<uint32_t> ibo_data;

	int mPipelineNbr;
	int mPipelinePos = 0;

	IVertexBuffer* mVertexBuffer;
	IVertexBuffer *mVertexBufferPipeline[HW_MAX_PIPELINE_BUFFERS];
	IIndexBuffer *mIndexBuffer;



	unsigned int mIndex;
	std::atomic<unsigned int> mCurIndex;
	unsigned int mNumReserved;

	unsigned int mMapStart;

	static const unsigned int BUFFER_SIZE = 2000000;
	static const unsigned int BUFFER_SIZE_TO_USE = BUFFER_SIZE-500;

public:
	enum
	{
		QUAD_INDEX = 0,
		FULLSCREEN_INDEX = 4,
		PRESENT_INDEX = 8,
		STENCILTOP_INDEX = 12,
		STENCILBOTTOM_INDEX = 16,

		NUM_RESERVED = 20
	};

	FFlatVertexBuffer(int width, int height, int pipelineNbr = 1);
	~FFlatVertexBuffer();

	void OutputResized(int width, int height);
	std::pair<IVertexBuffer *, IIndexBuffer *> GetBufferObjects() const 
	{
		return std::make_pair(mVertexBuffer, mIndexBuffer);
	}

	void Copy(int start, int count);

	FFlatVertex *GetBuffer(int index) const
	{
		FFlatVertex *ff = (FFlatVertex*)mVertexBuffer->Memory();
		return &ff[index];
	}

	FFlatVertex *GetBuffer() const
	{
		return GetBuffer(mCurIndex);
	}

	std::pair<FFlatVertex *, unsigned int> AllocVertices(unsigned int count);

	void Reset()
	{
		mCurIndex = mIndex;
	}

	void NextPipelineBuffer()
	{
		mPipelinePos++;
		mPipelinePos %= mPipelineNbr;

		mVertexBuffer = mVertexBufferPipeline[mPipelinePos];
	}

	void Map()
	{
		mMapStart = mCurIndex;
		mVertexBuffer->Map();
	}

	void Unmap()
	{
		mVertexBuffer->Unmap();
		mVertexBuffer->Upload(mMapStart * sizeof(FFlatVertex), (mCurIndex - mMapStart) * sizeof(FFlatVertex));
	}

	void DropSync()
	{
		mVertexBuffer->GPUDropSync();
	}

	void WaitSync()
	{
		mVertexBuffer->GPUWaitSync();
	}

	int GetPipelinePos() 
	{ 
		return mPipelinePos; 
	}
};

#endif
