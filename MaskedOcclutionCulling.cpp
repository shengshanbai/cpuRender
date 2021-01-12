#include "MaskedOcclutionCulling.h"
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cassert>

using namespace std;

// Size of guard band in pixels. Clipping doesn't seem to be very expensive so we use a small guard band
// to improve rasterization performance. It's not recommended to set the guard band to zero, as this may
// cause leakage along the screen border due to precision/rounding.
#define GUARD_BAND_PIXEL_SIZE   1.0f

// Sub-tiles (used for updating the masked HiZ buffer) are 8x4 tiles, so there are 4x2 sub-tiles in a tile
#define SUB_TILE_WIDTH          8
#define SUB_TILE_HEIGHT         4

// Tile dimensions are 32xN pixels. These values are not tweakable and the code must also be modified
// to support different tile sizes as it is tightly coupled with the SSE/AVX register size
#define TILE_WIDTH_SHIFT       5
#define TILE_HEIGHT_SHIFT      3
#define TILE_WIDTH             (1 << TILE_WIDTH_SHIFT)
#define TILE_HEIGHT            (1 << TILE_HEIGHT_SHIFT)

MaskedOcclusionCulling *MaskedOcclusionCulling::Create(){
    auto implement=MaskedOcclusionCulling::DetectCPUFeatures();
    if (implement<MaskedOcclusionCulling::AVX2){
        printf("the cpu not support avx2!\n");
        return nullptr;
    }
    MaskedOcclusionCulling* moc=static_cast<MaskedOcclusionCulling*>(aligned_alloc(64,sizeof(MaskedOcclusionCulling)));
    new (moc) MaskedOcclusionCulling();
    return moc;
}

void MaskedOcclusionCulling::Destroy(MaskedOcclusionCulling *moc){
    moc->~MaskedOcclusionCulling();
	free(moc);
}


MaskedOcclusionCulling::Implementation MaskedOcclusionCulling::DetectCPUFeatures(){
    struct CpuInfo { int regs[4]; };

	// Get regular CPUID values
	int regs[4];
	__cpuidex(regs, 0, 0);

    //  MOCVectorAllocator<CpuInfo> mocalloc( alignedAlloc, alignedFree );
    //  std::vector<CpuInfo, MOCVectorAllocator<CpuInfo>> cpuId( mocalloc ), cpuIdEx( mocalloc );
    //  cpuId.resize( regs[0] );
    size_t cpuIdCount = regs[0];
    CpuInfo * cpuId = static_cast<CpuInfo*>(std::aligned_alloc( 64, sizeof(CpuInfo) * cpuIdCount));
    
	for (size_t i = 0; i < cpuIdCount; ++i)
		__cpuidex(cpuId[i].regs, (int)i, 0);

	// Get extended CPUID values
	__cpuidex(regs, 0x80000000, 0);

    //cpuIdEx.resize(regs[0] - 0x80000000);
    size_t cpuIdExCount = regs[0] - 0x80000000;
    CpuInfo * cpuIdEx = static_cast<CpuInfo*>(std::aligned_alloc( 64, sizeof( CpuInfo ) * cpuIdExCount));

    for (size_t i = 0; i < cpuIdExCount; ++i)
		__cpuidex(cpuIdEx[i].regs, 0x80000000 + (int)i, 0);

	#define TEST_BITS(A, B)            (((A) & (B)) == (B))
	#define TEST_FMA_MOVE_OXSAVE       (cpuIdCount >= 1 && TEST_BITS(cpuId[1].regs[2], (1 << 12) | (1 << 22) | (1 << 27)))
	#define TEST_LZCNT                 (cpuIdExCount >= 1 && TEST_BITS(cpuIdEx[1].regs[2], 0x20))
	#define TEST_SSE41                 (cpuIdCount >= 1 && TEST_BITS(cpuId[1].regs[2], (1 << 19)))
	#define TEST_XMM_YMM               (cpuIdCount >= 1 && TEST_BITS(_xgetbv(0), (1 << 2) | (1 << 1)))
	#define TEST_OPMASK_ZMM            (cpuIdCount >= 1 && TEST_BITS(_xgetbv(0), (1 << 7) | (1 << 6) | (1 << 5)))
	#define TEST_BMI1_BMI2_AVX2        (cpuIdCount >= 7 && TEST_BITS(cpuId[7].regs[1], (1 << 3) | (1 << 5) | (1 << 8)))
	#define TEST_AVX512_F_BW_DQ        (cpuIdCount >= 7 && TEST_BITS(cpuId[7].regs[1], (1 << 16) | (1 << 17) | (1 << 30)))

    MaskedOcclusionCulling::Implementation retVal = MaskedOcclusionCulling::SSE2;
	if (TEST_FMA_MOVE_OXSAVE && TEST_LZCNT && TEST_SSE41)
	{
		if (TEST_XMM_YMM && TEST_OPMASK_ZMM && TEST_BMI1_BMI2_AVX2 && TEST_AVX512_F_BW_DQ)
			retVal = MaskedOcclusionCulling::AVX512;
		else if (TEST_XMM_YMM && TEST_BMI1_BMI2_AVX2)
			retVal = MaskedOcclusionCulling::AVX2;
	} 
    else if (TEST_SSE41)
		retVal = MaskedOcclusionCulling::SSE41;
    std::free(cpuId);
    std::free(cpuIdEx);
    return retVal;
}

MaskedOcclusionCulling::MaskedOcclusionCulling():mFullscreenScissor(0,0,0,0){
    mMaskedHiZBuffer=nullptr;
    SetNearClipPlane(0.0f);

    mCSFrustumPlanes[0] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[1] = _mm_setr_ps(1.0f, 0.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[2] = _mm_setr_ps(-1.0f, 0.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[3] = _mm_setr_ps(0.0f, 1.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[4] = _mm_setr_ps(0.0f, -1.0f, 1.0f, 0.0f);

    SetResolution(0, 0);
}

MaskedOcclusionCulling::~MaskedOcclusionCulling(){
    if (mMaskedHiZBuffer != nullptr)
			free(mMaskedHiZBuffer);
	mMaskedHiZBuffer = nullptr;
}

void MaskedOcclusionCulling::SetNearClipPlane(float nearDist){
    mNearDist=nearDist;
    mCSFrustumPlanes[0]=_mm_setr_ps(0.0f, 0.0f, 1.0f, -nearDist);
}

void MaskedOcclusionCulling::SetResolution(unsigned int width, unsigned int height){
    // Resolution must be a multiple of the subtile size
	assert(width % SUB_TILE_WIDTH == 0 && height % SUB_TILE_HEIGHT == 0);
    // Delete current masked hierarchical Z buffer
    if (mMaskedHiZBuffer!=nullptr){
        free(mMaskedHiZBuffer);
        mMaskedHiZBuffer=nullptr;
    }
    // Setup various resolution dependent constant values
	mWidth = (int)width;
	mHeight = (int)height;

    mTilesWidth = (int)(width + TILE_WIDTH - 1) >> TILE_WIDTH_SHIFT; //贴片的个数
	mTilesHeight = (int)(height + TILE_HEIGHT - 1) >> TILE_HEIGHT_SHIFT;
    mCenterX=_mm256_set1_ps(static_cast<float>(mWidth*0.5));
    mCenterY=_mm256_set1_ps(static_cast<float>(mHeight*0.5));
    mICenter = _mm_setr_ps((float)mWidth * 0.5f, (float)mWidth * 0.5f, (float)mHeight * 0.5f, (float)mHeight * 0.5f);
    mHalfWidth=_mm256_set1_ps((float)mWidth  * 0.5f);
    mHalfHeight = _mm256_set1_ps((float)-mHeight * 0.5f);
	mIHalfSize = _mm_setr_ps((float)mWidth * 0.5f, (float)mWidth * 0.5f, (float)-mHeight * 0.5f, (float)-mHeight * 0.5f);
    mIScreenSize = _mm_setr_epi32(mWidth - 1, mWidth - 1, mHeight - 1, mHeight - 1);

    // Setup a full screen scissor rectangle
	mFullscreenScissor.mMinX = 0;
	mFullscreenScissor.mMinY = 0;
	mFullscreenScissor.mMaxX = mTilesWidth << TILE_WIDTH_SHIFT;
	mFullscreenScissor.mMaxY = mTilesHeight << TILE_HEIGHT_SHIFT;

    // Adjust clip planes to include a small guard band to avoid clipping leaks
	float guardBandWidth = (2.0f / (float)mWidth) * GUARD_BAND_PIXEL_SIZE;
	float guardBandHeight = (2.0f / (float)mHeight) * GUARD_BAND_PIXEL_SIZE;
	mCSFrustumPlanes[1] = _mm_setr_ps(1.0f - guardBandWidth, 0.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[2] = _mm_setr_ps(-1.0f + guardBandWidth, 0.0f, 1.0f, 0.0f);
	mCSFrustumPlanes[3] = _mm_setr_ps(0.0f, 1.0f - guardBandHeight, 1.0f, 0.0f);
	mCSFrustumPlanes[4] = _mm_setr_ps(0.0f, -1.0f + guardBandHeight, 1.0f, 0.0f);

    // Allocate masked hierarchical Z buffer (if zero size leave at nullptr)
	if(mTilesWidth * mTilesHeight > 0)
		mMaskedHiZBuffer = (ZTile *)aligned_alloc(64, sizeof(ZTile) * mTilesWidth * mTilesHeight);
}

void MaskedOcclusionCulling::ClearBuffer(){
    assert(mMaskedHiZBuffer != nullptr);
    for (int i = 0; i < mTilesWidth * mTilesHeight; i++){
        mMaskedHiZBuffer[i].mMask = _mm256_setzero_si256();
        // Clear z0 to beyond infinity to ensure we never merge with clear data
        mMaskedHiZBuffer[i].mZMin[0]=_mm256_set1_ps(-1.0f);
        // Clear z1 to nearest depth value as it is pushed back on each update
        mMaskedHiZBuffer[i].mZMin[1] = _mm256_set1_ps(FLT_MAX);
    }
}