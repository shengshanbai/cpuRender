#include "MaskedOcclutionCulling.h"
#include <cstdlib>
#include "CompilerSpecific.h"

MaskedOcclusionCulling *MaskedOcclusionCulling::Create(){
    auto implement=MaskedOcclusionCulling::DetectCPUFeatures();
    std::cout<<implement<<endl;
    return nullptr;
}

void MaskedOcclusionCulling::Destroy(MaskedOcclusionCulling *moc){

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
    std::
    CpuInfo * cpuId = static_cast<CpuInfo*>std::aligned_alloc( 64, sizeof(CpuInfo) * cpuIdCount);
    
	for (size_t i = 0; i < cpuIdCount; ++i)
		__cpuidex(cpuId[i].regs, (int)i, 0);

	// Get extended CPUID values
	__cpuidex(regs, 0x80000000, 0);

    //cpuIdEx.resize(regs[0] - 0x80000000);
    size_t cpuIdExCount = regs[0] - 0x80000000;
    CpuInfo * cpuIdEx = static_cast<CpuInfo*>std::aligned_alloc( 64, sizeof( CpuInfo ) * cpuIdExCount );

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