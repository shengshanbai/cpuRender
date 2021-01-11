#include "MaskedOcclutionCulling.h"
#include <iostream>
#include <chrono>

using namespace std;

int main() {
    MaskedOcclusionCulling *moc=MaskedOcclusionCulling::Create();
    MaskedOcclusionCulling::Destroy(moc);
    return 0;
}