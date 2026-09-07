// SPDX-License-Identifier: GPL-3.0-only
#include <windows.h>
#include "ffgl/FFGL.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main(int argc, char** argv) {
    assert(argc == 2);
    HMODULE module = LoadLibraryA(argv[1]);
    if (!module) {
        std::cerr << "LoadLibrary failed: " << GetLastError() << '\n';
        return 1;
    }
    using Entry = FFMixed (__stdcall *)(FFUInt32, FFMixed, FFInstanceID);
    auto entry = reinterpret_cast<Entry>(GetProcAddress(module, "plugMain"));
    assert(entry);
    FFMixed input{};
    auto* info = static_cast<PluginInfoStruct*>(entry(FF_GET_INFO, input, nullptr).PointerValue);
    assert(info && std::memcmp(info->PluginUniqueID, "FSAR", 4) == 0);
    assert(info->PluginType == FF_SOURCE && info->APIMajorVersion == 2);
    assert(entry(FF_INITIALISE_V2, input, nullptr).UIntValue == FF_SUCCESS);
    assert(entry(FF_GET_NUM_PARAMETERS, input, nullptr).UIntValue == 5);
    assert(entry(FF_DEINITIALISE, input, nullptr).UIntValue == FF_SUCCESS);
    assert(FreeLibrary(module));
    std::cout << "PASS: Windows DLL load, FFGL source ID and parameters\n";
}
