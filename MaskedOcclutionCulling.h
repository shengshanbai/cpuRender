#pragma once

class MaskedOcclusionCulling{
    public:
        enum Implementation 
        {
            SSE2   = 0,
            SSE41  = 1,
            AVX2   = 2,
            AVX512 = 3
        };
        static MaskedOcclusionCulling *Create();
        static void Destroy(MaskedOcclusionCulling *moc);
        static Implementation DetectCPUFeatures();
    private:
        MaskedOcclusionCulling();
        ~MaskedOcclusionCulling();
};