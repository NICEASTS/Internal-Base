#include "Hooks.h"
#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_impl_win32.h"
#include "../../ext/imgui/imgui_impl_dx11.h"
#include "../../ext/minhook/MinHook.h"

#include "../../src/menu/Menu.h"
#include "../../src/feature/visuals/Visuals.h"
#include "../../src/sdk/entity/EntityManager.h"
#include "../sdk/utils/Globals.h"
#include "../sdk/utils/Utils.h"
#include "../sdk/memory/Offsets.h"
#include "../sdk/memory/PatternScan.h"
#include "../sdk/utils/CCSGOInput.h"
#include "../sdk/classes/CViewSetup.h"
#include "../sdk/entity/Classes.h"
#include "../sdk/classes/SceneSystem.h"
#include "../sdk/interfaces/Interfaces.h"
#include "../feature/misc/Misc.h"
#include "../feature/combat/legitbot/Aimbot.h"
#include "../feature/skinchanger/SkinChanger.h"
#include "../feature/skinchanger/SCLogger.h"
#include "../feature/visuals/chams/GPURenderer.h"
#include "../feature/visuals/chamsv2/ChamsV2.h"
#include "../feature/visuals/chamsv4/ChamsV4.h"
#include "../sdk/utils/Raycasting.h"
#include <cmath>
#include <thread>
#include <atomic>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static ID3D11RenderTargetView* g_RTV = nullptr;
ID3D11RenderTargetView* g_ChamsRTV = nullptr; 
static HWND                     g_Window = nullptr;
static bool                     g_Init = false;
static IDXGISwapChain*         g_CurrentSwapChain = nullptr;
static ID3D11RasterizerState*  g_WireframeRS = nullptr;
static ID3D11RasterizerState*  g_SolidRS = nullptr;
static ID3D11DepthStencilState* g_WireframeDepthState = nullptr;
static ID3D11PixelShader*      g_WireframePS = nullptr;
static ID3D11Buffer*           g_WireframeColorCB = nullptr;



float g_BackBufferWidth = 0.f;
float g_BackBufferHeight = 0.f;
float g_WindowWidth = 0.f;
float g_WindowHeight = 0.f;
static bool                    g_IsRenderingUI = false;

struct WireframeColorBuffer {
    float color[4];
};

enum class WireframeTarget {
    None,
    Enemy
};

static bool IsAnyWireframeEnabled()
{
    return Globals::wireframe_enemy_enabled;
}

static bool HasSkinningLikeConstantBuffer(ID3D11DeviceContext* pContext)
{
    if (!pContext)
        return false;

    for (UINT slot = 0; slot < 14; ++slot)
    {
        ID3D11Buffer* vsBuffer = nullptr;
        pContext->VSGetConstantBuffers(slot, 1, &vsBuffer);
        if (!vsBuffer)
            continue;

        D3D11_BUFFER_DESC desc{};
        vsBuffer->GetDesc(&desc);
        vsBuffer->Release();

        if (desc.ByteWidth >= 512 && desc.ByteWidth <= 262144)
            return true;
    }

    return false;
}

static WireframeTarget ClassifyWireframeTarget(ID3D11DeviceContext* pContext, UINT indexCount, UINT instanceCount)
{
    if (!pContext || indexCount == 0 || !IsAnyWireframeEnabled())
        return WireframeTarget::None;

    const UINT safeInstances = instanceCount > 0 ? instanceCount : 1;
    if (safeInstances != 1)
        return WireframeTarget::None;

    ID3D11Buffer* vertexBuffer = nullptr;
    UINT stride = 0;
    UINT offset = 0;
    pContext->IAGetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    if (vertexBuffer) vertexBuffer->Release();
    
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    pContext->IAGetPrimitiveTopology(&topology);

    ID3D11DepthStencilState* depthState = nullptr;
    UINT stencilRef = 0;
    pContext->OMGetDepthStencilState(&depthState, &stencilRef);

    bool depthEnabled = true;
    if (depthState)
    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        depthState->GetDesc(&desc);
        depthEnabled = desc.DepthEnable == TRUE;
        depthState->Release();
    }

    const UINT totalIndexCount = indexCount * safeInstances;
    const bool meshStrideLooksModel = stride >= 28 && stride <= 75;
    const bool topologyLooksModel =
        topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    const bool hasSkinningBuffer = HasSkinningLikeConstantBuffer(pContext);

    const bool enemyCandidate =
        meshStrideLooksModel &&
        topologyLooksModel &&
        hasSkinningBuffer &&
        depthEnabled &&
        totalIndexCount >= 3180 &&
        totalIndexCount <= 32000;

    if (Globals::wireframe_enemy_enabled && enemyCandidate)
        return WireframeTarget::Enemy;

    return WireframeTarget::None;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM
);

static std::thread g_FOVThread;
static std::atomic<bool> g_UnloadHooks{ false };

namespace Hooks {


inline std::atomic<uintptr_t> g_CachedPawn{ 0 };
inline std::atomic<uintptr_t> g_CachedController{ 0 };

static bool IsPawnValid(C_CSPlayerPawn* pawn)
{
    uintptr_t pPawn = reinterpret_cast<uintptr_t>(pawn);
    
    if (!pawn || !Utils::IsValidPtr(pPawn) || pPawn < 0x1000000 || pPawn > 0x00007FFFFFFFFFFF) 
        return false;

    __try {
        int health = Utils::SafeRead<int>(pPawn + Offsets::m_iHealth);
        if (health <= 0 || health > 100) return false;

        uint8_t lifeState = Utils::SafeRead<uint8_t>(pPawn + Offsets::m_lifeState);
        if (lifeState != 0) return false;

        uint32_t team = Utils::SafeRead<uint32_t>(pPawn + Offsets::m_iTeamNum);
        if (team < 2 || team > 3) return false;

        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void UpdateFOV(C_CSPlayerPawn* localPawn, C_CSPlayerController* localController)
{
    if (!IsPawnValid(localPawn)) return;

    __try {
        uintptr_t pPawn = reinterpret_cast<uintptr_t>(localPawn);
        uintptr_t pCtrl = reinterpret_cast<uintptr_t>(localController);
        
        
        if (pPawn < 0x1000000 || pPawn > 0x00007FFFFFFFFFFF || pCtrl < 0x1000000 || pCtrl > 0x00007FFFFFFFFFFF) 
            return;

        
        if (Utils::SafeRead<bool>(pPawn + Offsets::m_bIsScoped) || Utils::SafeRead<bool>(pPawn + Offsets::m_bIsBuyMenuOpen))
            return;

        float targetFov = static_cast<float>(static_cast<int>(Globals::visuals_fov));
        if (targetFov < 1.0f || targetFov > 170.0f) targetFov = 90.0f;

        
        if (Utils::IsValidPtr(pCtrl))
        {
            uint32_t currentDesired = Utils::SafeRead<uint32_t>(pCtrl + Offsets::m_iDesiredFOV);
            if (currentDesired > 0 && currentDesired < 180 && currentDesired != static_cast<uint32_t>(targetFov))
                Utils::SafeWrite<uint32_t>(pCtrl + Offsets::m_iDesiredFOV, static_cast<uint32_t>(targetFov));
        }

        
        uintptr_t cameraServices = Utils::SafeRead<uintptr_t>(pPawn + Offsets::m_pCameraServices);
        if (Utils::IsValidPtr(cameraServices) && cameraServices > 0x1000000 && cameraServices < 0x00007FFFFFFFFFFF)
        {
            
            Utils::SafeWrite<uint32_t>(cameraServices + Offsets::m_iFOV, static_cast<uint32_t>(targetFov));
            Utils::SafeWrite<uint32_t>(cameraServices + Offsets::m_iFOVStart, static_cast<uint32_t>(targetFov));
        }

        
        float currentVmFov = Utils::SafeRead<float>(pPawn + Offsets::m_flViewmodelFOV);
        if (currentVmFov > 0.0f && currentVmFov < 180.0f && currentVmFov != targetFov)
            Utils::SafeWrite<float>(pPawn + Offsets::m_flViewmodelFOV, targetFov);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FOVPhantomThread()
{
    while (!g_UnloadHooks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        const uintptr_t cachedPawn = g_CachedPawn.load(std::memory_order_relaxed);
        if (!Globals::visuals_fov_enabled || !cachedPawn)
            continue;

        __try {
            const uintptr_t cachedController = g_CachedController.load(std::memory_order_relaxed);
            auto localPawn = reinterpret_cast<C_CSPlayerPawn*>(cachedPawn);
            auto localCtrl = reinterpret_cast<C_CSPlayerController*>(cachedController);

            
            if (cachedPawn >= 0x1000000 && cachedPawn < 0x00007FFFFFFFFFFF)
                UpdateFOV(localPawn, localCtrl);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}

static bool hkWndProcInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Menu::IsOpen)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        
        switch (msg)
        {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
        case WM_CHAR:
            return true;
        }
    }
    return false;
}

LRESULT __stdcall hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (hkWndProcInternal(hWnd, msg, wParam, lParam))
        return true;

    if (!oWndProc)
        return CallWindowProcW(DefWindowProcW, hWnd, msg, wParam, lParam);

    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

static void InitChamsV2(ID3D11Device* device, ID3D11DeviceContext* context)
{
    constexpr const char* ctModelPath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\ascania\\models\\ctm_sas.glb";
    constexpr const char* tModelPath  = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\ascania\\models\\tm_phoenix.glb";
    ChamsV2::g_MeshRenderer.Initialize(device, context, ctModelPath, tModelPath);
}

void PresentInternal(IDXGISwapChain* swapChain, UINT sync, UINT flags)
{
    __try {
        if (!g_Init || g_CurrentSwapChain != swapChain)
        {
            if (g_Init)
            {
                ImGui_ImplDX11_Shutdown();
                ImGui_ImplWin32_Shutdown();
                if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
                if (g_WireframeDepthState) { g_WireframeDepthState->Release(); g_WireframeDepthState = nullptr; }
                if (g_WireframePS) { g_WireframePS->Release(); g_WireframePS = nullptr; }
                if (g_WireframeColorCB) { g_WireframeColorCB->Release(); g_WireframeColorCB = nullptr; }
                if (g_WireframeRS) { g_WireframeRS->Release(); g_WireframeRS = nullptr; }
                if (g_SolidRS) { g_SolidRS->Release(); g_SolidRS = nullptr; }
                if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }
                if (g_Context) { g_Context->Release(); g_Context = nullptr; }
                if (g_Device) { g_Device->Release(); g_Device = nullptr; }
                g_Init = false;
            }

            if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device)))
                return;

            g_Device->GetImmediateContext(&g_Context);
            DXGI_SWAP_CHAIN_DESC sd{};
            swapChain->GetDesc(&sd);
            g_Window = sd.OutputWindow;
            ID3D11Texture2D* backBuffer = nullptr;
            if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)))
            {
                g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RTV);
                backBuffer->Release();
            }

            if (!oWndProc) {
                oWndProc = (WNDPROC)SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            }

            ImGui::CreateContext();
            ImGui_ImplWin32_Init(g_Window);
            ImGui_ImplDX11_Init(g_Device, g_Context);

            D3D11_RASTERIZER_DESC rsDesc{};
            rsDesc.FillMode = D3D11_FILL_WIREFRAME;
            rsDesc.CullMode = D3D11_CULL_NONE;
            rsDesc.DepthClipEnable = TRUE;
            g_Device->CreateRasterizerState(&rsDesc, &g_WireframeRS);

            rsDesc.FillMode = D3D11_FILL_SOLID;
            g_Device->CreateRasterizerState(&rsDesc, &g_SolidRS);

            D3D11_DEPTH_STENCIL_DESC dsDesc{};
            dsDesc.DepthEnable = FALSE;
            dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            dsDesc.StencilEnable = FALSE;
            g_Device->CreateDepthStencilState(&dsDesc, &g_WireframeDepthState);

            const char* psSource =
                "cbuffer WireframeColorBuffer : register(b0) { float4 wireColor; };"
                "float4 main() : SV_Target { return wireColor; }";
            ID3DBlob* psBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            if (SUCCEEDED(D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob)))
            {
                g_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_WireframePS);
                psBlob->Release();
            }
            if (errorBlob) errorBlob->Release();

            D3D11_BUFFER_DESC cbDesc{};
            cbDesc.Usage = D3D11_USAGE_DYNAMIC;
            cbDesc.ByteWidth = sizeof(WireframeColorBuffer);
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            g_Device->CreateBuffer(&cbDesc, nullptr, &g_WireframeColorCB);
            
            Menu::Initialize(g_Device);
            GPUChamsRenderer::Get().Initialize(g_Device, g_Context);
            InitChamsV2(g_Device, g_Context);

            g_CurrentSwapChain = swapChain;
            g_Init = true;
        }

        
        
        {
            ID3D11Texture2D* backBuf = nullptr;
            if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf)) && backBuf)
            {
                D3D11_TEXTURE2D_DESC bbDesc;
                backBuf->GetDesc(&bbDesc);
                g_BackBufferWidth = (float)bbDesc.Width;
                g_BackBufferHeight = (float)bbDesc.Height;
                backBuf->Release();
            }

            RECT clientRect;
            if (GetClientRect(g_Window, &clientRect)) {
                g_WindowWidth = (float)(clientRect.right - clientRect.left);
                g_WindowHeight = (float)(clientRect.bottom - clientRect.top);
            }
        }

        
        
        {
            D3D11_VIEWPORT gameVP = {};
            UINT numVP = 1;
            g_Context->RSGetViewports(&numVP, &gameVP);

            if (numVP > 0 && gameVP.Width > 0 && gameVP.Height > 0) {
                Globals::GameViewportX = gameVP.TopLeftX;
                Globals::GameViewportY = gameVP.TopLeftY;
                Globals::GameViewportWidth = gameVP.Width;
                Globals::GameViewportHeight = gameVP.Height;
            } else {
                
                Globals::GameViewportX = 0.f;
                Globals::GameViewportY = 0.f;
                Globals::GameViewportWidth = g_BackBufferWidth;
                Globals::GameViewportHeight = g_BackBufferHeight;
            }
        }

        
        
        Globals::ScreenWidth = (int)Globals::GameViewportWidth;
        Globals::ScreenHeight = (int)Globals::GameViewportHeight;

        if (!g_RTV || !g_Context || !g_Init) return;

        uintptr_t client = Memory::GetModuleBase("client.dll");
        if (client)
        {
            for (int i = 0; i < 16; i++) {
                Globals::ViewMatrix[i] = Utils::SafeRead<float>(client + Offsets::dwViewMatrix + (i * sizeof(float)));
            }

            uintptr_t localPawnAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
            uintptr_t localCtrlAddr = Utils::SafeRead<uintptr_t>(client + Offsets::dwLocalPlayerController);

            auto localPawn = reinterpret_cast<C_CSPlayerPawn*>(localPawnAddr);
            auto localCtrl = reinterpret_cast<C_CSPlayerController*>(localCtrlAddr);

            if (IsPawnValid(localPawn))
            {
                
                g_CachedPawn.store(localPawnAddr, std::memory_order_relaxed);
                g_CachedController.store(localCtrlAddr, std::memory_order_relaxed);
            }
            else
            {
                
                g_CachedPawn.store(0, std::memory_order_relaxed);
                g_CachedController.store(0, std::memory_order_relaxed);
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        
        
        
        
        if (g_BackBufferWidth > 0 && g_BackBufferHeight > 0) {
            ImGui::GetIO().DisplaySize = ImVec2(g_BackBufferWidth, g_BackBufferHeight);
        }

        ImGui::NewFrame();

        if (GetAsyncKeyState(Globals::menu_key) & 1)
            Menu::IsOpen = !Menu::IsOpen;

        Menu::Render();

        
        Raycasting::Get().UpdateMap();

        
        g_ChamsRTV = g_RTV;
        Visuals::Render(); 
        Misc::Render();
        Misc::Run();

        ImGui::Render();
        g_Context->OMSetRenderTargets(1, &g_RTV, nullptr);
        g_IsRenderingUI = true;
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_IsRenderingUI = false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!oPresent) return pSwapChain->Present(SyncInterval, Flags);
    PresentInternal(pSwapChain, SyncInterval, Flags);
    return oPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    
    if (g_RTV) { g_RTV->Release(); g_RTV = nullptr; }

    
    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    
    if (SUCCEEDED(hr) && g_Device)
    {
        ID3D11Texture2D* backBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)))
        {
            g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RTV);
            backBuffer->Release();
        }
    }

    return hr;
}

void __stdcall hkDrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    if (!oDrawIndexed) return;

    if (g_WireframeRS && g_WireframeDepthState && g_WireframePS && g_WireframeColorCB && !g_IsRenderingUI) {
        WireframeTarget target = ClassifyWireframeTarget(pContext, IndexCount, 1);
        if (target != WireframeTarget::None) {
            ID3D11RasterizerState* pOldRS = nullptr;
            ID3D11DepthStencilState* pOldDepthState = nullptr;
            ID3D11PixelShader* pOldPS = nullptr;
            ID3D11Buffer* pOldCB = nullptr;
            UINT oldStencilRef = 0;
            pContext->RSGetState(&pOldRS);
            pContext->OMGetDepthStencilState(&pOldDepthState, &oldStencilRef);
            pContext->PSGetShader(&pOldPS, nullptr, nullptr);
            pContext->PSGetConstantBuffers(0, 1, &pOldCB);

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(pContext->Map(g_WireframeColorCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                std::memcpy(mapped.pData, Globals::wireframe_color, sizeof(float) * 4);
                pContext->Unmap(g_WireframeColorCB, 0);
            }

            pContext->RSSetState(g_WireframeRS);
            pContext->OMSetDepthStencilState(g_WireframeDepthState, 0);
            pContext->PSSetConstantBuffers(0, 1, &g_WireframeColorCB);
            pContext->PSSetShader(g_WireframePS, nullptr, 0);
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

            pContext->PSSetShader(pOldPS, nullptr, 0);
            pContext->PSSetConstantBuffers(0, 1, &pOldCB);
            pContext->OMSetDepthStencilState(pOldDepthState, oldStencilRef);
            pContext->RSSetState(pOldRS ? pOldRS : g_SolidRS);

            if (pOldCB) pOldCB->Release();
            if (pOldPS) pOldPS->Release();
            if (pOldDepthState) pOldDepthState->Release();
            if (pOldRS) pOldRS->Release();
            return;
        }
    }

    oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

void __stdcall hkDrawIndexedInstanced(ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
    if (!oDrawIndexedInstanced) return;

    if (g_WireframeRS && g_WireframeDepthState && g_WireframePS && g_WireframeColorCB && !g_IsRenderingUI) {
        WireframeTarget target = ClassifyWireframeTarget(pContext, IndexCountPerInstance, InstanceCount);
        if (target != WireframeTarget::None) {
            ID3D11RasterizerState* pOldRS = nullptr;
            ID3D11DepthStencilState* pOldDepthState = nullptr;
            ID3D11PixelShader* pOldPS = nullptr;
            ID3D11Buffer* pOldCB = nullptr;
            UINT oldStencilRef = 0;
            pContext->RSGetState(&pOldRS);
            pContext->OMGetDepthStencilState(&pOldDepthState, &oldStencilRef);
            pContext->PSGetShader(&pOldPS, nullptr, nullptr);
            pContext->PSGetConstantBuffers(0, 1, &pOldCB);

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(pContext->Map(g_WireframeColorCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                std::memcpy(mapped.pData, Globals::wireframe_color, sizeof(float) * 4);
                pContext->Unmap(g_WireframeColorCB, 0);
            }

            pContext->RSSetState(g_WireframeRS);
            pContext->OMSetDepthStencilState(g_WireframeDepthState, 0);
            pContext->PSSetConstantBuffers(0, 1, &g_WireframeColorCB);
            pContext->PSSetShader(g_WireframePS, nullptr, 0);
            oDrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

            pContext->PSSetShader(pOldPS, nullptr, 0);
            pContext->PSSetConstantBuffers(0, 1, &pOldCB);
            pContext->OMSetDepthStencilState(pOldDepthState, oldStencilRef);
            pContext->RSSetState(pOldRS ? pOldRS : g_SolidRS);

            if (pOldCB) pOldCB->Release();
            if (pOldPS) pOldPS->Release();
            if (pOldDepthState) pOldDepthState->Release();
            if (pOldRS) pOldRS->Release();
            return;
        }
    }

    oDrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

bool __fastcall hkCreateMove(void* pInput, int nSlot, uint8_t a3)
{
    if (!oCreateMove) return false;
    bool result = oCreateMove(pInput, nSlot, a3);

    if (!pInput)
        return result;

    uintptr_t ctrlAddr = g_CachedController.load(std::memory_order_relaxed);
    if (!ctrlAddr)
        return result;

    auto* input = reinterpret_cast<i_csgo_input*>(pInput);
    c_user_cmd* cmd = input->get_user_cmd(reinterpret_cast<void*>(ctrlAddr));
    if (!cmd)
        return result;

    uintptr_t pawnAddr = g_CachedPawn.load(std::memory_order_relaxed);
    if (pawnAddr)
    {
        auto* localPawn = reinterpret_cast<C_CSPlayerPawn*>(pawnAddr);
        if (localPawn->IsAlive())
        {
            Misc::run_bunnyhop(cmd);
            Misc::run_auto_strafe(cmd);
        }
    }

    return result;
}

void __fastcall hkFrameStageNotify(void* _this, int curStage)
{
    
    if (oFrameStageNotify)
        oFrameStageNotify(_this, curStage);

    
    
    
    
    {
        static void* g_pPVS = nullptr;
        static bool g_pvs_init = false;
        if (!g_pvs_init) {
            g_pvs_init = true;
            __try {
                uintptr_t pvsScan = Memory::PatternScan("engine2.dll", "48 8D 0D ? ? ? ? 33 D2 FF 50");
                if (pvsScan) {
                    g_pPVS = reinterpret_cast<void*>(Utils::ResolveRelativeAddress(pvsScan, 3, 7));
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                g_pPVS = nullptr;
            }
        }
        if (g_pPVS) {
            __try {
                
                using fn_pvs_set = void(__fastcall*)(void*, bool);
                void** vtable = *reinterpret_cast<void***>(g_pPVS);
                if (vtable && vtable[6]) {
                    reinterpret_cast<fn_pvs_set>(vtable[6])(g_pPVS, false);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }
    
    
    
    
    if (curStage == 6 || curStage == 7) {
        __try {
            SkinChanger::RunVisuals(curStage);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}


static bool s_wallsWereActive = false;
static int s_wallRestoreCount = 0;

void __fastcall hkDrawAggregateSceneObject(void* a1, void* a2, c_aggregate_object_array* a3)
{
    if (!oDrawAggregateSceneObject) return;

    
    if (Globals::world_walls_enabled && a3 && a3->object) {
        __try {
            uintptr_t sceneObj = reinterpret_cast<uintptr_t>(a3->object);
            int count = *reinterpret_cast<int*>(sceneObj + 0x18C);
            uintptr_t dataBase = *reinterpret_cast<uintptr_t*>(sceneObj + 0x160);

            if (dataBase && count > 0 && count < 100000) {
                uint8_t wr = static_cast<uint8_t>(Globals::world_walls_color[0] * 255.0f);
                uint8_t wg = static_cast<uint8_t>(Globals::world_walls_color[1] * 255.0f);
                uint8_t wb = static_cast<uint8_t>(Globals::world_walls_color[2] * 255.0f);
                for (int i = 0; i < count; i++) {
                    uintptr_t colorAddr = dataBase + static_cast<uintptr_t>(i) * 0x58 + 0x21;
                    *reinterpret_cast<uint8_t*>(colorAddr)     = wr;
                    *reinterpret_cast<uint8_t*>(colorAddr + 1) = wg;
                    *reinterpret_cast<uint8_t*>(colorAddr + 2) = wb;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        s_wallsWereActive = true;
        s_wallRestoreCount = 0;
    }
    else if (s_wallsWereActive && !Globals::world_walls_enabled) {
        
        s_wallRestoreCount = 500;
        s_wallsWereActive = false;
    }

    
    if (s_wallRestoreCount > 0 && a3 && a3->object) {
        __try {
            uintptr_t sceneObj = reinterpret_cast<uintptr_t>(a3->object);
            int count = *reinterpret_cast<int*>(sceneObj + 0x18C);
            uintptr_t dataBase = *reinterpret_cast<uintptr_t*>(sceneObj + 0x160);

            if (dataBase && count > 0 && count < 100000) {
                for (int i = 0; i < count; i++) {
                    uintptr_t colorAddr = dataBase + static_cast<uintptr_t>(i) * 0x58 + 0x21;
                    *reinterpret_cast<uint8_t*>(colorAddr)     = 0xFF;
                    *reinterpret_cast<uint8_t*>(colorAddr + 1) = 0xFF;
                    *reinterpret_cast<uint8_t*>(colorAddr + 2) = 0xFF;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        s_wallRestoreCount--;
    }

    oDrawAggregateSceneObject(a1, a2, a3);
}

void __fastcall hkGeneratePrimitives(void* __this, void* object, void* a3, c_mesh_primitive_output_buffer* render_buf)
{
    if (!oGeneratePrimitives) return;
    oGeneratePrimitives(__this, object, a3, render_buf);

    
}

void __fastcall hkSkyBoxObjectDrawArray(std::int64_t this_ptr, std::int64_t render, std::int64_t primitive, int nCount, int renderFlags, std::int64_t viewInfo, std::int64_t renderStats)
{
    if (Globals::skybox_changer && nCount > 0)
    {
        __try
        {
            
            
            const uintptr_t skyboxDataAddr = static_cast<uintptr_t>(0x68 * nCount + primitive - 0x50);
            const uintptr_t pSkyDesc = *reinterpret_cast<uintptr_t*>(skyboxDataAddr);

            if (pSkyDesc)
            {
                
                float* skyColor = reinterpret_cast<float*>(pSkyDesc + 0xE8);
                float intensity = Globals::skybox_intensity;
                skyColor[0] = Globals::skybox_color[0] * intensity;
                skyColor[1] = Globals::skybox_color[1] * intensity;
                skyColor[2] = Globals::skybox_color[2] * intensity;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    if (oSkyBoxObjectDrawArray)
        oSkyBoxObjectDrawArray(this_ptr, render, primitive, nCount, renderFlags, viewInfo, renderStats);
}


static bool s_lightWasActive = false;
static float s_origLightRGBA[4] = { 0 };

void __fastcall hkLightScene(__int64 a1, __int64 a2)
{
    if (oLightScene)
        oLightScene(a1, a2);

    if (Globals::world_light_enabled) {
        __try {
            float* lightPtr = reinterpret_cast<float*>(a1 + 4);
            
            if (!s_lightWasActive) {
                s_origLightRGBA[0] = lightPtr[0];
                s_origLightRGBA[1] = lightPtr[1];
                s_origLightRGBA[2] = lightPtr[2];
                s_origLightRGBA[3] = lightPtr[3];
                s_lightWasActive = true;
            }
            float intensity = Globals::world_light_intensity;
            lightPtr[0] = Globals::world_light_color[0] * intensity;
            lightPtr[1] = Globals::world_light_color[1] * intensity;
            lightPtr[2] = Globals::world_light_color[2] * intensity;
            lightPtr[3] = Globals::world_light_color[3] * intensity;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    else if (s_lightWasActive) {
        
        __try {
            float* lightPtr = reinterpret_cast<float*>(a1 + 4);
            lightPtr[0] = s_origLightRGBA[0];
            lightPtr[1] = s_origLightRGBA[1];
            lightPtr[2] = s_origLightRGBA[2];
            lightPtr[3] = s_origLightRGBA[3];
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        s_lightWasActive = false;
    }
}

static void FlashGameWindow()
{
    if (!g_Window)
        return;

    FLASHWINFO fi{};
    fi.cbSize = sizeof(fi);
    fi.hwnd = g_Window;
    fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
    FlashWindowEx(&fi);
}

void __fastcall hkMatchFoundHandler()
{
    if (Globals::autoaccept_enabled)
    {
        if (SetPlayerReady)
            SetPlayerReady(nullptr, "accept");
        FlashGameWindow();
    }

    if (oMatchFoundHandler)
        oMatchFoundHandler();
}

__int64 __fastcall hkPanoramaEvent(void* pUnk, const char* szEventName, void* pUnk1, float flUnk)
{
    if (szEventName && std::strcmp(szEventName, "popup_accept_match_found") == 0 && Globals::autoaccept_enabled)
    {
        FlashGameWindow();
        std::thread([]()
        {
            
            if (Hooks::SetPlayerReady)
                Hooks::SetPlayerReady(nullptr, "accept");
        }).detach();
    }

    if (oPanoramaEvent)
        return oPanoramaEvent(pUnk, szEventName, pUnk1, flUnk);

    return 0;
}

void Setup()
{
    if (MH_Initialize() != MH_OK)
        return;

    
    SkinChanger::InstallHooks();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"DummyDX";

    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl;

    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx)))
    {
        void** vtable = *reinterpret_cast<void***>(sc);
        MH_CreateHook(vtable[8], &hkPresent, reinterpret_cast<void**>(&oPresent));
        MH_EnableHook(vtable[8]);

        
        MH_CreateHook(vtable[13], &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers));
        MH_EnableHook(vtable[13]);

        void** contextVTable = *reinterpret_cast<void***>(ctx);
        MH_CreateHook(contextVTable[12], &hkDrawIndexed, reinterpret_cast<void**>(&oDrawIndexed));
        MH_EnableHook(contextVTable[12]);
        MH_CreateHook(contextVTable[20], &hkDrawIndexedInstanced, reinterpret_cast<void**>(&oDrawIndexedInstanced));
        MH_EnableHook(contextVTable[20]);

        sc->Release();
        dev->Release();
        ctx->Release();
    }

    Interfaces::Setup();

    uintptr_t drawAggregateAddr = Memory::PatternScan("scenesystem.dll", "48 8B C4 48 89 50 10 48 89 48 08 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8");
    if (drawAggregateAddr)
    {
        MH_CreateHook((void*)drawAggregateAddr, &hkDrawAggregateSceneObject, reinterpret_cast<void**>(&oDrawAggregateSceneObject));
        MH_EnableHook((void*)drawAggregateAddr);
    }

    uintptr_t generatePrimitivesAddr = Memory::PatternScan("client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B 01 48 8B F1");
    if (generatePrimitivesAddr)
    {
        MH_CreateHook((void*)generatePrimitivesAddr, &hkGeneratePrimitives, reinterpret_cast<void**>(&oGeneratePrimitives));
        MH_EnableHook((void*)generatePrimitivesAddr);
    }

    uintptr_t skyBoxObjectDrawArrayAddr = Memory::PatternScan("scenesystem.dll", "45 85 C9 0F 8E ? ? ? ? 4C 8B DC");
    if (skyBoxObjectDrawArrayAddr)
    {
        MH_CreateHook((void*)skyBoxObjectDrawArrayAddr, &hkSkyBoxObjectDrawArray, reinterpret_cast<void**>(&oSkyBoxObjectDrawArray));
        MH_EnableHook((void*)skyBoxObjectDrawArrayAddr);
    }

    
    uintptr_t lightSceneAddr = Memory::PatternScan("scenesystem.dll", "8B 02 89 01 F2 0F 10 42 ? F2 0F 11 41 ? 8B 42 ? 89 41 ? F2 0F 10 42");
    if (lightSceneAddr)
    {
        MH_CreateHook((void*)lightSceneAddr, &hkLightScene, reinterpret_cast<void**>(&oLightScene));
        MH_EnableHook((void*)lightSceneAddr);
    }

    uintptr_t setPlayerReadyAddr = Memory::PatternScan("client.dll", "40 53 48 83 EC 20 48 8B DA 48 8D 15 ? ? ? ? 48 8B CB FF 15 ? ? ? ? 85 C0 75 14 BA 02 00");
    if (setPlayerReadyAddr)
        SetPlayerReady = reinterpret_cast<decltype(SetPlayerReady)>(setPlayerReadyAddr);

    uintptr_t matchFoundHandlerAddr = Memory::PatternScan("client.dll", "48 83 EC 28 48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 48 89 7C 24 20 FF 50 78");
    if (matchFoundHandlerAddr)
    {
        MH_CreateHook((void*)matchFoundHandlerAddr, &hkMatchFoundHandler, reinterpret_cast<void**>(&oMatchFoundHandler));
        MH_EnableHook((void*)matchFoundHandlerAddr);
    }

    uintptr_t panoramaEventAddr = Memory::PatternScan("client.dll", "40 56 57 41 57 48 83 EC 40 48 8B 3D ? ? ? ? 4D 85 C0 48 8B 35 ? ? ? ? 4C 8B FA 49 0F 45 F8");
    if (panoramaEventAddr)
    {
        MH_CreateHook((void*)panoramaEventAddr, &hkPanoramaEvent, reinterpret_cast<void**>(&oPanoramaEvent));
        MH_EnableHook((void*)panoramaEventAddr);
    }

    uintptr_t createMoveAddr = Memory::PatternScan("client.dll", BhopV2Patterns::create_move);
    if (createMoveAddr)
    {
        MH_CreateHook((void*)createMoveAddr, &hkCreateMove, reinterpret_cast<void**>(&oCreateMove));
        MH_EnableHook((void*)createMoveAddr);
        g_Logger.Log("[BhopV2] CreateMove hooked at 0x%llX", createMoveAddr);
    }
    else
    {
        g_Logger.Log("[BhopV2] WARNING: CreateMove pattern not found!");
    }

    MH_EnableHook(MH_ALL_HOOKS);

    
    ChamsV4::Initialize();
    if (ChamsV4::g_DrawObjectAddr) {
        MH_CreateHook((void*)ChamsV4::g_DrawObjectAddr, &ChamsV4::hkDrawObject_Trampoline, reinterpret_cast<void**>(&ChamsV4::oDrawObject));
        MH_EnableHook((void*)ChamsV4::g_DrawObjectAddr);
    }

    
    
    if (Interfaces::m_pIClient) {
        g_Logger.Log("Interfaces::m_pIClient is valid: 0x%llX", Interfaces::m_pIClient);
        void** vtable = *reinterpret_cast<void***>(Interfaces::m_pIClient);
        if (vtable) {
            g_Logger.Log("IClient VTable is valid: 0x%llX", vtable);
            void* frameStageFn = vtable[36];
            g_Logger.Log("FrameStageNotify vtable[36] function ptr: 0x%llX", frameStageFn);
            
            MH_STATUS s1 = MH_CreateHook(frameStageFn, &hkFrameStageNotify, reinterpret_cast<void**>(&oFrameStageNotify));
            g_Logger.Log("MH_CreateHook(FrameStageNotify) result: %d", s1);
            
            MH_STATUS s2 = MH_EnableHook(frameStageFn);
            g_Logger.Log("MH_EnableHook(FrameStageNotify) result: %d", s2);
        } else {
            g_Logger.Log("IClient VTable is NULL!");
        }
    } else {
        g_Logger.Log("Interfaces::m_pIClient is NULL! FrameStageNotify hook skipped.");
    }

    g_UnloadHooks = false;
    g_FOVThread = std::thread(FOVPhantomThread);

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

void Destroy()
{
    g_UnloadHooks = true;
    if (g_FOVThread.joinable())
        g_FOVThread.join();

    MH_DisableHook(MH_ALL_HOOKS);
    ChamsV4::Shutdown();
    MH_Uninitialize();

    if (g_Window && oWndProc)
        SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)oWndProc);

    if (!g_Init) return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_WireframeDepthState) g_WireframeDepthState->Release();
    if (g_WireframePS) g_WireframePS->Release();
    if (g_WireframeColorCB) g_WireframeColorCB->Release();
    if (g_WireframeRS) g_WireframeRS->Release();
    if (g_SolidRS) g_SolidRS->Release();
    if (g_RTV)     g_RTV->Release();
    if (g_Context) g_Context->Release();
    if (g_Device)  g_Device->Release();

    GPUChamsRenderer::Get().Shutdown();
    ChamsV2::g_MeshRenderer.Shutdown();

    g_Init = false;
}

} 
