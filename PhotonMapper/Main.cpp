#include "DxrPhotonMapper.h"

#define APP_INVOKER_IMPLEMENTATION
#include "AppInvoker.h"

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR /*cmdline*/,
    _In_ int /*nCmdShow*/)
{
    DxrPhotonMapper photonMapper(1280, 720);
    return AppInvoker::Execute(&photonMapper, hInstance);
}
