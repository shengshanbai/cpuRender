#pragma once
#include "CompilerSpecific.h"
#include "ObjModel.h"

class MaskedOcclusionCulling{
    public:
        enum Implementation 
        {
            SSE2   = 0,
            SSE41  = 1,
            AVX2   = 2,
            AVX512 = 3
        };

            /*!
        * Used to control scissoring during rasterization. Note that we only provide coarse scissor support. 
        * The scissor box x coordinates must be a multiple of 32, and the y coordinates a multiple of 8. 
        * Scissoring is mainly meant as a means of enabling binning (sort middle) rasterizers in case
        * application developers want to use that approach for multithreading.
        */
        struct ScissorRect
        {
            ScissorRect() {}
            ScissorRect(int minX, int minY, int maxX, int maxY) :
                mMinX(minX), mMinY(minY), mMaxX(maxX), mMaxY(maxY) {}

            int mMinX; //!< Screen space X coordinate for left side of scissor rect, inclusive and must be a multiple of 32
            int mMinY; //!< Screen space Y coordinate for bottom side of scissor rect, inclusive and must be a multiple of 8
            int mMaxX; //!< Screen space X coordinate for right side of scissor rect, <B>non</B> inclusive and must be a multiple of 32
            int mMaxY; //!< Screen space Y coordinate for top side of scissor rect, <B>non</B> inclusive and must be a multiple of 8
        };

        struct ZTile{
            __m256 mZMin[2];
            __m256i mMask;
        };

        __m128          mCSFrustumPlanes[5];

        ZTile       *mMaskedHiZBuffer;
        ScissorRect  mFullscreenScissor;
        float           mNearDist;
        int             mWidth;
	    int             mHeight;
        int             mTilesWidth;
	    int             mTilesHeight;
        __m256          mCenterX;
	    __m256          mCenterY;
        __m128          mICenter;
        __m256          mHalfWidth;
        __m256          mHalfHeight;
        __m128          mIHalfSize;
        __m128i         mIScreenSize;

        static MaskedOcclusionCulling *Create();
        static void Destroy(MaskedOcclusionCulling *moc);
        static Implementation DetectCPUFeatures();
        void SetResolution(unsigned int width, unsigned int height);
        void ClearBuffer();
        void renderTriangles(const float* inVtx,const unsigned);
    private:
        MaskedOcclusionCulling();
        ~MaskedOcclusionCulling();
        void SetNearClipPlane(float nearDist);
};