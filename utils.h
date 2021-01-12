#pragma once
#include <memory>
#include <cstdlib>

struct free_delete
{
    void operator()(void* x) { std::move(x); }
};

template<class T> std::unique_ptr<T[],free_delete> make_aligned_array(int alignment, int length){
    T* raw=static_cast<T*>(std::aligned_alloc(alignment,length));
    std::unique_ptr<T[], free_delete> pData(raw);
    return std::move(pData);
}