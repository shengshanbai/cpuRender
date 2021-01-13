#pragma once
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <opencv2/opencv.hpp>

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(__clang__)
	#define __MICROSOFT_COMPILER
#endif

#if defined(_WIN32)	&& (defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__clang__)) // Windows: MSVC / Intel compiler / clang
	#include <intrin.h>
	#include <new.h>

	#define FORCE_INLINE __forceinline

	FORCE_INLINE unsigned long find_clear_lsb(unsigned int *mask)
	{
		unsigned long idx;
		_BitScanForward(&idx, *mask);
		*mask &= *mask - 1;
		return idx;
	}

	FORCE_INLINE void *aligned_alloc(size_t alignment, size_t size)
	{
		return _aligned_malloc(size, alignment);
	}

	FORCE_INLINE void aligned_free(void *ptr)
	{
		_aligned_free(ptr);
	}

#elif defined(__GNUG__)	|| defined(__clang__) // G++ or clang
	#include <cpuid.h>
#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
	#include <malloc/malloc.h> // memalign
#else
	#include <malloc.h> // memalign
#endif
	#include <mm_malloc.h>
	#include <immintrin.h>
	#include <new>

	#define FORCE_INLINE inline

	FORCE_INLINE unsigned long find_clear_lsb(unsigned int *mask)
	{
		unsigned long idx;
		idx = __builtin_ctzl(*mask);
		*mask &= *mask - 1;
		return idx;
	}

	FORCE_INLINE void *aligned_alloc(size_t alignment, size_t size)
	{
		return memalign(alignment, size);
	}

	FORCE_INLINE void aligned_free(void *ptr)
	{
		free(ptr);
	}

	FORCE_INLINE void __cpuidex(int* cpuinfo, int function, int subfunction)
	{
		__cpuid_count(function, subfunction, cpuinfo[0], cpuinfo[1], cpuinfo[2], cpuinfo[3]);
	}

	FORCE_INLINE unsigned long long _xgetbv(unsigned int index)
	{
		unsigned int eax, edx;
		__asm__ __volatile__(
			"xgetbv;"
			: "=a" (eax), "=d"(edx)
			: "c" (index)
		);
		return ((unsigned long long)edx << 32) | eax;
	}

#else
	#error Unsupported compiler
#endif

#define DEFAULT_ALIGN 32

struct free_delete
{
    void operator()(void* x) { aligned_free(x); }
};

template<class T> std::unique_ptr<T[],free_delete> make_aligned_array(int alignment, int length){
    T* raw=static_cast<T*>(aligned_alloc(alignment,length));
    std::unique_ptr<T[], free_delete> pData(raw);
    return std::move(pData);
}