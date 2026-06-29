#ifndef NOMINMAX
#define NOMINMAX
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include "Menu.h"
#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"
#include "../../ext/imgui/custom.h"
#include "../../ext/imgui/globals.h"
#include "../../ext/imgui/bytearray.h"
#include "../sdk/utils/Globals.h"
#include "../sdk/utils/Config.h"
#include "../feature/skinchanger/SkinData.h"
#include "../feature/skinchanger/SkinDB.h"
#include "../feature/skinchanger/SkinChanger.h"
#include "../feature/skinchanger/SCLogger.h"
#include "../feature/misc/Misc.h"
#include "../sdk/utils/Raycasting.h"
#include "../../esp.h"

extern SCLogger g_Logger;
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <winhttp.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "winhttp.lib")

using Microsoft::WRL::ComPtr;

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern ImFont* poppins;
extern ImFont* font_icon;
extern ID3D11ShaderResourceView* logo;
extern ID3D11ShaderResourceView* logotwo;
extern ID3D11ShaderResourceView* foto_user;
extern char discord_username[64];
extern char expiry_date[64];
extern ID3D11ShaderResourceView* logo_png;

ImFont* esp_font = nullptr;
ImFont* weapon_icon_font = nullptr;

#include "../feature/visuals/esp/WeaponIcon.h"

static ID3D11Device* g_MenuDevice = nullptr;
static ID3D11ShaderResourceView* g_EspPreviewSrv = nullptr;
static int g_EspPreviewW = 0;
static int g_EspPreviewH = 0;
static bool g_EspPreviewTried = false;




#define FAT_BG         ImVec4(0.068f, 0.070f, 0.073f, 1.0f)   
#define FAT_BLOCK      ImVec4(0.105f, 0.108f, 0.113f, 1.0f)   
#define FAT_OUTLINE    ImVec4(0.245f, 0.255f, 0.270f, 0.68f)   
#define FAT_TEXT       ImVec4(0.800f, 0.810f, 0.820f, 1.0f)   
#define FAT_TEXT_LIGHT ImVec4(0.960f, 0.960f, 0.960f, 1.0f)   
#define FAT_TEXT_DIM   ImVec4(0.392f, 0.392f, 0.392f, 1.0f)   
#define FAT_ODD        ImVec4(0.048f, 0.050f, 0.054f, 1.0f)   
#define FAT_EVEN       ImVec4(0.070f, 0.073f, 0.078f, 1.0f)   

static ImVec4 GetAccent() {
    return ImVec4(Globals::menu_accent_color[0], Globals::menu_accent_color[1], Globals::menu_accent_color[2], 1.0f);
}




static bool LoadTextureFromMemory(ID3D11Device* device, unsigned char* data, size_t size, ID3D11ShaderResourceView** out_srv)
{
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return false;
    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(&stream))) return false;
    if (FAILED(stream->InitializeFromMemory(data, (DWORD)size))) return false;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) return false;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) return false;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter))) return false;
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) return false;
    UINT width, height;
    converter->GetSize(&width, &height);
    std::vector<unsigned char> pixels(width * height * 4);
    if (FAILED(converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data()))) return false;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA subData{};
    subData.pSysMem = pixels.data(); subData.SysMemPitch = width * 4;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&desc, &subData, &texture))) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format; srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    return SUCCEEDED(device->CreateShaderResourceView(texture.Get(), &srvDesc, out_srv));
}

static void EnsureEspPreviewTexture()
{
    if (g_EspPreviewSrv || g_EspPreviewTried || !g_MenuDevice) return;
    g_EspPreviewTried = true;

    if (!LoadTextureFromMemory(g_MenuDevice, espImage, sizeof(espImage), &g_EspPreviewSrv)) return;

    ComPtr<ID3D11Resource> resource;
    g_EspPreviewSrv->GetResource(&resource);
    ComPtr<ID3D11Texture2D> tex;
    if (SUCCEEDED(resource.As(&tex))) {
        D3D11_TEXTURE2D_DESC d{};
        tex->GetDesc(&d);
        g_EspPreviewW = (int)d.Width;
        g_EspPreviewH = (int)d.Height;
    }
}

static void DrawCornerBox(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 col, float th)
{
    float w = max.x - min.x;
    float h = max.y - min.y;
    float lx = w * 0.25f;
    float ly = h * 0.18f;

    dl->AddLine(ImVec2(min.x, min.y), ImVec2(min.x + lx, min.y), col, th);
    dl->AddLine(ImVec2(min.x, min.y), ImVec2(min.x, min.y + ly), col, th);
    dl->AddLine(ImVec2(max.x - lx, min.y), ImVec2(max.x, min.y), col, th);
    dl->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + ly), col, th);

    dl->AddLine(ImVec2(min.x, max.y - ly), ImVec2(min.x, max.y), col, th);
    dl->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + lx, max.y), col, th);
    dl->AddLine(ImVec2(max.x - lx, max.y), ImVec2(max.x, max.y), col, th);
    dl->AddLine(ImVec2(max.x, max.y - ly), ImVec2(max.x, max.y), col, th);
}

static void DrawEspPreviewOverlay(ImVec2 tl, ImVec2 br, bool boxOn, bool skelOn, bool headOn, bool nameOn, bool distOn, bool wepOn, bool hpOn,
    int boxType, const float* boxCol, const float* skelCol, const float* headCol, const float* nameCol, const float* distCol, const float* wepCol, const float* hpCol)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = br.x - tl.x;
    float h = br.y - tl.y;
    auto P = [&](float nx, float ny) -> ImVec2 {
        return ImVec2(tl.x + w * nx, tl.y + h * ny);
    };

    ImVec2 pTop = P(0.48f, 0.093478f);
    ImVec2 pNeck = P(0.493333f, 0.213043f);
    ImVec2 pChest = P(0.496667f, 0.308696f);
    ImVec2 pPelvis = P(0.51f, 0.478261f);
    ImVec2 pLShoulder = P(0.376667f, 0.243478f);
    ImVec2 pRShoulder = P(0.636667f, 0.230435f);
    ImVec2 pLElbow = P(0.353333f, 0.302174f);
    ImVec2 pRElbow = P(0.726667f, 0.308696f);
    ImVec2 pLHand = P(0.263333f, 0.319565f);
    ImVec2 pRHand = P(0.62f, 0.278261f);
    ImVec2 pLHip = P(0.426667f, 0.480435f);
    ImVec2 pRHip = P(0.58f, 0.484783f);
    ImVec2 pLKnee = P(0.376667f, 0.628261f);
    ImVec2 pRKnee = P(0.67f, 0.682609f);
    ImVec2 pLAnkle = P(0.46f, 0.802174f);
    ImVec2 pRAnkle = P(0.72f, 0.869565f);
    ImVec2 pLFoot = P(0.336667f, 0.823913f);
    ImVec2 pRFoot = P(0.683333f, 0.932609f);

    ImU32 cBox = ImGui::ColorConvertFloat4ToU32(ImVec4(boxCol[0], boxCol[1], boxCol[2], boxCol[3]));
    ImU32 cSk = ImGui::ColorConvertFloat4ToU32(ImVec4(skelCol[0], skelCol[1], skelCol[2], skelCol[3]));
    ImU32 cHd = ImGui::ColorConvertFloat4ToU32(ImVec4(headCol[0], headCol[1], headCol[2], headCol[3]));
    ImU32 cNm = ImGui::ColorConvertFloat4ToU32(ImVec4(nameCol[0], nameCol[1], nameCol[2], nameCol[3]));
    ImU32 cDs = ImGui::ColorConvertFloat4ToU32(ImVec4(distCol[0], distCol[1], distCol[2], distCol[3]));
    ImU32 cWp = ImGui::ColorConvertFloat4ToU32(ImVec4(wepCol[0], wepCol[1], wepCol[2], wepCol[3]));
    ImU32 cHp = ImGui::ColorConvertFloat4ToU32(ImVec4(hpCol[0], hpCol[1], hpCol[2], hpCol[3]));

    ImVec2 bMin = P(0.245f, 0.07f);
    ImVec2 bMax = P(0.74f, 0.945f);

    if (boxOn) {
        if (boxType == 1) DrawCornerBox(dl, bMin, bMax, cBox, 2.0f);
        else dl->AddRect(bMin, bMax, cBox, 0.0f, 0, 2.0f);
    }
    if (hpOn) {
        dl->AddRectFilled(ImVec2(bMin.x - 8.0f, bMin.y), ImVec2(bMin.x - 4.0f, bMax.y), IM_COL32(30, 30, 30, 200));
        dl->AddRectFilled(ImVec2(bMin.x - 8.0f, bMin.y + (bMax.y - bMin.y) * 0.20f), ImVec2(bMin.x - 4.0f, bMax.y), cHp);
    }
    if (skelOn) {
        dl->AddLine(pTop, pNeck, cSk, 1.8f);
        dl->AddLine(pNeck, pChest, cSk, 1.8f);
        dl->AddLine(pChest, pPelvis, cSk, 1.8f);
        dl->AddLine(pLShoulder, pRShoulder, cSk, 1.8f);
        dl->AddLine(pChest, pLShoulder, cSk, 1.8f);
        dl->AddLine(pChest, pRShoulder, cSk, 1.8f);
        dl->AddLine(pLShoulder, pLElbow, cSk, 1.8f);
        dl->AddLine(pRShoulder, pRElbow, cSk, 1.8f);
        dl->AddLine(pLElbow, pLHand, cSk, 1.8f);
        dl->AddLine(pRElbow, pRHand, cSk, 1.8f);
        dl->AddLine(pLHip, pRHip, cSk, 1.8f);
        dl->AddLine(pPelvis, pLHip, cSk, 1.8f);
        dl->AddLine(pPelvis, pRHip, cSk, 1.8f);
        dl->AddLine(pLHip, pLKnee, cSk, 1.8f);
        dl->AddLine(pLKnee, pLAnkle, cSk, 1.8f);
        dl->AddLine(pLAnkle, pLFoot, cSk, 1.8f);
        dl->AddLine(pRHip, pRKnee, cSk, 1.8f);
        dl->AddLine(pRKnee, pRAnkle, cSk, 1.8f);
        dl->AddLine(pRAnkle, pRFoot, cSk, 1.8f);
    }
    if (headOn) dl->AddCircle(pTop, w * 0.083333f, cHd, 24, 2.0f);
    if (nameOn) {
        float ny = bMin.y - 22.0f;
        if (ny < tl.y + 6.0f) ny = tl.y + 6.0f;
        ImVec2 np(bMin.x, ny);
        dl->AddText(ImVec2(np.x + 1.0f, np.y + 1.0f), IM_COL32(0, 0, 0, 255), "Enemy Preview");
        dl->AddText(np, cNm, "Enemy Preview");
    }
    if (wepOn) dl->AddText(ImVec2(bMin.x, bMax.y + 4.0f), cWp, "AK-47");
    if (distOn) dl->AddText(ImVec2(bMax.x + 8.0f, pPelvis.y), cDs, "18m");
}

static void DrawEspFloatingPreview(ImVec2 menuPos, float menuW, bool enabled,
    bool boxOn, bool skelOn, bool headOn, bool nameOn, bool distOn, bool wepOn, bool hpOn,
    int boxType,
    const float* boxCol, const float* skelCol, const float* nameCol, const float* distCol, const float* wepCol, const float* hpCol)
{
    if (!enabled) return;
    EnsureEspPreviewTexture();

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0549f, 0.0549f, 0.0549f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0549f, 0.0549f, 0.0549f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.0549f, 0.0549f, 0.0549f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.227f, 0.227f, 0.227f, 1.0f));

    ImGui::SetNextWindowBgAlpha(0.98f);
    ImGui::SetNextWindowPos(ImVec2(menuPos.x + menuW + 14.0f, menuPos.y + 58.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 460.0f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
    if (ImGui::Begin("ESP Preview", nullptr, flags)) {
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 wpos = ImGui::GetWindowPos();
        float ww = ImGui::GetWindowSize().x;
        float titleH = ImGui::GetFrameHeight();
        wdl->AddLine(ImVec2(wpos.x, wpos.y + titleH), ImVec2(wpos.x + ww, wpos.y + titleH), IM_COL32(151, 45, 99, 255), 1.0f);

        ImVec2 tl = ImGui::GetCursorScreenPos();
        ImVec2 area = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##esp_floating_area", area);
        ImVec2 br = ImVec2(tl.x + area.x, tl.y + area.y);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(tl, br, IM_COL32(20, 20, 20, 255), 4.0f);

        if (g_EspPreviewSrv) {
            float iw = (float)g_EspPreviewW;
            float ih = (float)g_EspPreviewH;
            float scale = (ih > 0.0f) ? ((area.y / ih) * 0.88f) : 1.0f;
            float drawW = iw * scale;
            float drawH = ih * scale;
            if (drawW > area.x) {
                float s2 = area.x / drawW;
                drawW *= s2;
                drawH *= s2;
            }
            ImVec2 imgTl(tl.x + (area.x - drawW) * 0.5f, tl.y + (area.y - drawH) * 0.5f);
            ImVec2 imgBr(imgTl.x + drawW, imgTl.y + drawH);
            dl->AddImage((ImTextureID)g_EspPreviewSrv, imgTl, imgBr);
            DrawEspPreviewOverlay(imgTl, imgBr, boxOn, skelOn, headOn, nameOn, distOn, wepOn, hpOn, boxType, boxCol, skelCol, skelCol, nameCol, distCol, wepCol, hpCol);
        } else {
            dl->AddText(ImVec2(tl.x + 10.0f, tl.y + 10.0f), IM_COL32(180, 180, 180, 255), "ESP image not loaded");
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(4);
}




struct SkinImageEntry {
    ID3D11ShaderResourceView* texture = nullptr;
    bool attempted = false;
    bool downloading = false;
    bool uploadPending = false;
    DWORD lastAttemptTick = 0;
    int failCount = 0;
    std::vector<unsigned char> pendingData;
};
static std::unordered_map<std::string, SkinImageEntry> g_SkinImages;
static std::mutex g_SkinImageMutex;

static bool DownloadImageData(const std::string& url, std::vector<unsigned char>& outData)
{
    std::wstring wurl(url.begin(), url.end());
    URL_COMPONENTS uc{}; uc.dwStructSize = sizeof(uc);
    wchar_t hostBuf[256]{}, pathBuf[2048]{};
    uc.lpszHostName = hostBuf; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = pathBuf; uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return false;
    HINTERNET hSession = WinHttpOpen(L"SkinImg/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, hostBuf, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pathBuf, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    WinHttpSetTimeouts(hRequest, 3000, 3000, 7000, 7000);
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    DWORD dwSize = 0, dwDownloaded = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<unsigned char> buf(dwSize);
        if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwDownloaded)) break;
        if (dwDownloaded == 0) break;
        outData.insert(outData.end(), buf.begin(), buf.begin() + dwDownloaded);
    } while (dwSize > 0);
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    return !outData.empty();
}

static void RequestSkinImage(const std::string& imageUrl) {
    if (!g_MenuDevice || imageUrl.empty()) return;
    { std::lock_guard<std::mutex> lock(g_SkinImageMutex);
      auto it = g_SkinImages.find(imageUrl);
      if (it != g_SkinImages.end() && it->second.downloading) return;
      SkinImageEntry& entry = g_SkinImages[imageUrl];
      entry.downloading = true;
      entry.uploadPending = false;
      entry.pendingData.clear();
      entry.lastAttemptTick = GetTickCount(); }

    struct ImageThreadParam { std::string url; };
    auto* param = new ImageThreadParam{ imageUrl };
    CreateThread(nullptr, 0, [](LPVOID p) -> DWORD {
        auto* param = (ImageThreadParam*)p;
        std::string url = param->url;
        delete param;

        std::vector<unsigned char> imgData;
        bool ok = DownloadImageData(url, imgData);

        {
            std::lock_guard<std::mutex> lock(g_SkinImageMutex);
            SkinImageEntry& e = g_SkinImages[url];
            e.attempted = true;
            e.downloading = false;
            if (ok && !imgData.empty()) {
                e.pendingData.swap(imgData);
                e.uploadPending = true;
            } else {
                e.uploadPending = false;
                e.pendingData.clear();
                e.failCount++;
            }
        }

        g_Logger.Log("Image download: url=%s, ok=%d, bytes=%d", url.c_str(), ok, (int)imgData.size());
        return 0;
    }, (LPVOID)param, 0, nullptr);
}

static ID3D11ShaderResourceView* GetSkinImage(const std::string& imageUrl) {
    if (imageUrl.empty()) return nullptr;
    bool shouldRequest = false;
    ID3D11ShaderResourceView* result = nullptr;
    bool shouldUpload = false;
    std::vector<unsigned char> uploadData;
    {
        std::lock_guard<std::mutex> lock(g_SkinImageMutex);
        auto it = g_SkinImages.find(imageUrl);
        if (it != g_SkinImages.end()) {
            result = it->second.texture;
            if (!result && it->second.uploadPending && !it->second.pendingData.empty()) {
                uploadData = it->second.pendingData;
                it->second.pendingData.clear();
                it->second.uploadPending = false;
                shouldUpload = true;
            }
            if (!it->second.texture && !it->second.downloading) {
                DWORD now = GetTickCount();
                DWORD retryDelay = (it->second.failCount <= 2) ? 2000 : 6000;
                if (now - it->second.lastAttemptTick >= retryDelay) {
                    it->second.attempted = false;
                    shouldRequest = true;
                }
            }
        } else {
            shouldRequest = true;
        }
    }

    if (shouldUpload && !uploadData.empty() && g_MenuDevice) {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        ID3D11ShaderResourceView* tex = nullptr;
        bool loaded = LoadTextureFromMemory(g_MenuDevice, uploadData.data(), uploadData.size(), &tex);
        CoUninitialize();

        std::lock_guard<std::mutex> lock(g_SkinImageMutex);
        SkinImageEntry& e = g_SkinImages[imageUrl];
        if (loaded) {
            e.texture = tex;
            e.failCount = 0;
            result = tex;
        } else {
            e.failCount++;
        }
        g_Logger.Log("Image upload: url=%s, loaded=%d, bytes=%d", imageUrl.c_str(), loaded, (int)uploadData.size());
    }

    if (shouldRequest) RequestSkinImage(imageUrl);
    return result;
}




static ImVec4 GetRarityColor(int rarity) {
    switch (rarity) {
        case 7: return ImVec4(0.89f, 0.68f, 0.22f, 1.0f);
        case 6: return ImVec4(0.92f, 0.29f, 0.29f, 1.0f);
        case 5: return ImVec4(0.84f, 0.33f, 0.84f, 1.0f);
        case 4: return ImVec4(0.55f, 0.31f, 0.88f, 1.0f);
        case 3: return ImVec4(0.31f, 0.46f, 0.88f, 1.0f);
        case 2: return ImVec4(0.42f, 0.69f, 0.87f, 1.0f);
        default: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    }
}

static std::string GetKeyNameStr(int vk) {
    if (vk == VK_LMENU) return "ALT"; if (vk == VK_LBUTTON) return "M1"; if (vk == VK_RBUTTON) return "M2";
    if (vk == VK_MBUTTON) return "M3"; if (vk == VK_XBUTTON1) return "M4"; if (vk == VK_XBUTTON2) return "M5";
    if (vk == VK_SHIFT || vk == VK_LSHIFT) return "SHIFT";
    if (vk == VK_CONTROL || vk == VK_LCONTROL) return "CTRL";
    if (vk == VK_INSERT) return "INS"; if (vk == VK_HOME) return "HOME";
    if (vk == VK_F1) return "F1"; if (vk == 0) return "AUTO";
    char name[64]; if (GetKeyNameTextA(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC) << 16, name, 64)) return name;
    return "Key " + std::to_string(vk);
}

static bool KeyBinder(const char* label, int* key, float w = 80.0f) {
    static const char* activeId = nullptr; bool changed = false;
    ImGui::Text("%s", label); ImGui::SameLine(200);
    char buf[64]; bool active = (activeId && strcmp(activeId, label) == 0);
    sprintf(buf, "%s##%s", active ? "..." : GetKeyNameStr(*key).c_str(), label);
    if (ImGui::Button(buf, ImVec2(w, 22))) activeId = label;
    if (active) { for (int i = 1; i < 0xFE; i++) { if (GetAsyncKeyState(i) & 0x8000) { *key = i; activeId = nullptr; changed = true; break; } } }
    return changed;
}

static bool SliderFloatInput(const char* label, float* value, float minValue, float maxValue, const char* sliderFormat, const char* inputFormat = nullptr)
{
    bool changed = false;
    ImGui::PushID(label);

    float labelW = ImGui::CalcTextSize(label).x + 8.0f;
    float inputW = 64.0f;
    float availW = ImGui::GetContentRegionAvail().x;
    float sliderW = availW - inputW - labelW - ImGui::GetStyle().ItemSpacing.x * 2.0f;
    if (sliderW < 110.0f)
        sliderW = 110.0f;

    ImGui::SetNextItemWidth(sliderW);
    changed |= ImGui::SliderFloat("##slider", value, minValue, maxValue, sliderFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputW);
    changed |= ImGui::InputFloat("##input", value, 0.0f, 0.0f, inputFormat ? inputFormat : sliderFormat);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    if (*value < minValue) { *value = minValue; changed = true; }
    if (*value > maxValue) { *value = maxValue; changed = true; }

    ImGui::PopID();
    return changed;
}

static bool SliderIntInput(const char* label, int* value, int minValue, int maxValue, const char* sliderFormat)
{
    bool changed = false;
    ImGui::PushID(label);

    float labelW = ImGui::CalcTextSize(label).x + 8.0f;
    float inputW = 58.0f;
    float availW = ImGui::GetContentRegionAvail().x;
    float sliderW = availW - inputW - labelW - ImGui::GetStyle().ItemSpacing.x * 2.0f;
    if (sliderW < 110.0f)
        sliderW = 110.0f;

    ImGui::SetNextItemWidth(sliderW);
    changed |= ImGui::SliderInt("##slider", value, minValue, maxValue, sliderFormat);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(inputW);
    changed |= ImGui::InputInt("##input", value, 0, 0);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    if (*value < minValue) { *value = minValue; changed = true; }
    if (*value > maxValue) { *value = maxValue; changed = true; }

    ImGui::PopID();
    return changed;
}

static void SectionHeader(const char* title)
{
    ImVec4 accent = GetAccent();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    ImU32 accentCol = ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, 0.85f));
    ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.23f, 0.24f, 0.25f, 0.85f));

    dl->AddRectFilled(ImVec2(pos.x, pos.y + 2.0f), ImVec2(pos.x + 3.0f, pos.y + 15.0f), accentCol, 1.0f);
    dl->AddLine(ImVec2(pos.x + 4.0f, pos.y + 15.0f), ImVec2(pos.x + width, pos.y + 15.0f), lineCol, 1.0f);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 9.0f, pos.y));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(accent.x, accent.y, accent.z, 0.95f));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 23.0f));
}


static void GroupBoxBegin(const char* title, ImVec2 size) {
    ImVec4 acc = GetAccent();
    const float titlePadX = 10.0f;
    const float titlePadY = 2.0f;

    ImVec2 textSize = ImGui::CalcTextSize(title);
    float titleH = textSize.y + titlePadY * 2.0f;
    float boxTopInset = titleH * 0.5f;

    // Current cursor (screen space)
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Draw box background + border
    ImVec2 boxMin(pos.x, pos.y + boxTopInset);
    ImVec2 boxMax(pos.x + size.x, pos.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(boxMin, boxMax, ImGui::ColorConvertFloat4ToU32(FAT_BLOCK), 4.0f);
    dl->AddRect(boxMin, boxMax, ImGui::ColorConvertFloat4ToU32(FAT_OUTLINE), 4.0f, 0, 1.0f);

    // Draw title (half outside)
    float titleX = pos.x + 14.0f;
    float titleY = pos.y;
    dl->AddRectFilled(ImVec2(titleX, titleY), ImVec2(titleX + textSize.x + titlePadX * 2.0f, titleY + titleH),
                      ImGui::ColorConvertFloat4ToU32(FAT_BLOCK), 3.0f);
    dl->AddText(ImVec2(titleX + titlePadX, titleY + titlePadY),
                ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.9f)), title);

    // Advance layout cursor past entire groupbox
    ImGui::Dummy(size);

    // Start content child inside the box (below title area)
    ImVec2 childPos(pos.x + 14.0f, boxMin.y + 13.0f);
    ImVec2 childSize(size.x - 28.0f, size.y - boxTopInset - 21.0f);
    if (childSize.x < 1.0f) childSize.x = 1.0f;
    if (childSize.y < 1.0f) childSize.y = 1.0f;

    ImGui::SetCursorScreenPos(childPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild(title, childSize, false);
}

static void GroupBoxEnd() {
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}


static std::string ToLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static int GetSkinGridColumns(float width) {
    if (width >= 760.0f) return 4;
    if (width >= 560.0f) return 3;
    return 2;
}

static void ApplyFeaturePreset(int preset)
{
    const bool proOrFull = preset >= 1;
    const bool full = preset >= 2;

    Globals::aim_enabled = true;
    Globals::aim_key_mode = 0;
    Globals::aim_vis_check = false;
    Globals::aim_ignore_flash = true;
    Globals::aim_scope_only = false;
    Globals::aim_draw_fov = true;
    Globals::aim_fov = proOrFull ? 8.0f : 6.0f;
    Globals::aim_smoothing = proOrFull ? 0.45f : 0.60f;
    Globals::aim_persistent = proOrFull;
    Globals::aim_soft_visible = proOrFull;
    Globals::aim_soft_visible_grace_ms = proOrFull ? 250 : 150;
    Globals::aim_soft_visible_min_bones = 1;
    Globals::aim_sticky_target = true;
    Globals::aim_switch_delay_ms = proOrFull ? 120 : 80;
    Globals::aim_dynamic_smoothing = true;
    Globals::aim_smoothing_near = proOrFull ? 0.16f : 0.22f;
    Globals::aim_smoothing_far = proOrFull ? 0.30f : 0.36f;
    Globals::aim_hitbox_head = true;
    Globals::aim_hitbox_neck = proOrFull;
    Globals::aim_hitbox_chest = proOrFull;
    Globals::aim_hitbox_stomach = false;
    Globals::aim_hitbox_pelvis = false;
    Globals::aim_humanize = proOrFull;

    for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) {
        auto& group = Globals::aim_weapon_groups[i];
        group.enabled = true;
        group.fov = Globals::aim_fov;
        group.smoothing = Globals::aim_smoothing;
        group.dynamic_smoothing = Globals::aim_dynamic_smoothing;
        group.smoothing_near = Globals::aim_smoothing_near;
        group.smoothing_far = Globals::aim_smoothing_far;
        group.scope_only = false;
        group.hitbox_head = true;
        group.hitbox_neck = proOrFull;
        group.hitbox_chest = proOrFull;
        group.hitbox_stomach = false;
        group.hitbox_pelvis = false;
    }

    auto& sniperAim = Globals::aim_weapon_groups[Globals::AIM_GROUP_SNIPER];
    sniperAim.fov = proOrFull ? 4.0f : 3.5f;
    sniperAim.smoothing = proOrFull ? 0.34f : 0.45f;
    sniperAim.smoothing_near = proOrFull ? 0.12f : 0.18f;
    sniperAim.smoothing_far = proOrFull ? 0.24f : 0.32f;
    sniperAim.scope_only = true;
    sniperAim.hitbox_neck = false;
    sniperAim.hitbox_chest = false;

    Globals::trigger_enabled = true;
    Globals::trigger_key_mode = 0;
    Globals::trigger_ignore_flash = true;
    Globals::trigger_delay = proOrFull ? 70.0f : 90.0f;
    Globals::trigger_fov = proOrFull ? 4.5f : 3.0f;
    Globals::trigger_draw_fov = false;
    Globals::trigger_hitbox_head = true;
    Globals::trigger_hitbox_neck = proOrFull;
    Globals::trigger_hitbox_chest = proOrFull;
    Globals::trigger_hitbox_stomach = false;
    Globals::trigger_hitbox_pelvis = false;

    Globals::rcs_enabled = true;
    Globals::rcs_mode = 0;
    Globals::rcs_horizontal = proOrFull ? 38.0f : 34.0f;
    Globals::rcs_vertical = proOrFull ? 52.0f : 44.0f;
    Globals::rcs_smooth = proOrFull ? 0.55f : 0.45f;
    for (int i = 0; i < Globals::AIM_GROUP_COUNT; ++i) {
        auto& group = Globals::rcs_weapon_groups[i];
        group.enabled = true;
        group.mode = Globals::rcs_mode;
        group.horizontal = Globals::rcs_horizontal;
        group.vertical = Globals::rcs_vertical;
    }

    Globals::esp_enabled = true;
    Globals::esp_box = true;
    Globals::esp_skeleton = proOrFull;
    Globals::esp_head = proOrFull;
    Globals::esp_name = true;
    Globals::esp_distance = true;
    Globals::esp_weapon = proOrFull;
    Globals::esp_health = true;
    Globals::esp_sound_enabled = proOrFull;
    Globals::esp_glow = full;

    Globals::chams_enabled = proOrFull;
    Globals::chamsv4_enabled = full;

    Globals::bunnyhop_enabled = true;
    Globals::antiflash_enabled = true;
    Globals::autoaccept_enabled = true;
    Globals::bombtimer_enabled = proOrFull;
    Globals::speclist_enabled = proOrFull;
}

struct AimConfigSnapshot {
    bool aim_enabled, aim_vis_check, aim_ignore_flash, aim_scope_only, aim_soft_visible, aim_persistent, aim_draw_fov;
    int aim_soft_visible_grace_ms, aim_soft_visible_min_bones;
    float aim_fov, aim_smoothing, aim_fov_color[4];
    int aim_key, aim_key_mode;
    bool aim_hitbox_head, aim_hitbox_neck, aim_hitbox_chest, aim_hitbox_stomach, aim_hitbox_pelvis;
    bool aim_humanize;
    float aim_humanize_strength, aim_humanize_jitter, aim_humanize_curve;
    bool aim_sticky_target, aim_dynamic_smoothing;
    int aim_switch_delay_ms;
    float aim_smoothing_near, aim_smoothing_far;
    int aim_selected_weapon_group;
    Globals::AimGroupSettings aim_weapon_groups[Globals::AIM_GROUP_COUNT];
};

struct TriggerConfigSnapshot {
    bool trigger_enabled, trigger_draw_fov, trigger_ignore_flash;
    int trigger_key, trigger_key_mode;
    float trigger_delay, trigger_fov, trigger_fov_color[4];
    bool trigger_hitbox_head, trigger_hitbox_neck, trigger_hitbox_chest, trigger_hitbox_stomach, trigger_hitbox_pelvis;
    bool rcs_enabled;
    int rcs_mode;
    float rcs_horizontal, rcs_vertical, rcs_smooth, rcs_calibration;
    Globals::RcsGroupSettings rcs_weapon_groups[Globals::AIM_GROUP_COUNT];
};

static AimConfigSnapshot CaptureAimConfig() {
    AimConfigSnapshot s{};
    s.aim_enabled = Globals::aim_enabled;
    s.aim_vis_check = Globals::aim_vis_check;
    s.aim_ignore_flash = Globals::aim_ignore_flash;
    s.aim_scope_only = Globals::aim_scope_only;
    s.aim_soft_visible = Globals::aim_soft_visible;
    s.aim_soft_visible_grace_ms = Globals::aim_soft_visible_grace_ms;
    s.aim_soft_visible_min_bones = Globals::aim_soft_visible_min_bones;
    s.aim_persistent = Globals::aim_persistent;
    s.aim_draw_fov = Globals::aim_draw_fov;
    s.aim_fov = Globals::aim_fov;
    s.aim_smoothing = Globals::aim_smoothing;
    memcpy(s.aim_fov_color, Globals::aim_fov_color, sizeof(s.aim_fov_color));
    s.aim_key = Globals::aim_key;
    s.aim_key_mode = Globals::aim_key_mode;
    s.aim_hitbox_head = Globals::aim_hitbox_head;
    s.aim_hitbox_neck = Globals::aim_hitbox_neck;
    s.aim_hitbox_chest = Globals::aim_hitbox_chest;
    s.aim_hitbox_stomach = Globals::aim_hitbox_stomach;
    s.aim_hitbox_pelvis = Globals::aim_hitbox_pelvis;
    s.aim_humanize = Globals::aim_humanize;
    s.aim_humanize_strength = Globals::aim_humanize_strength;
    s.aim_humanize_jitter = Globals::aim_humanize_jitter;
    s.aim_humanize_curve = Globals::aim_humanize_curve;
    s.aim_sticky_target = Globals::aim_sticky_target;
    s.aim_switch_delay_ms = Globals::aim_switch_delay_ms;
    s.aim_dynamic_smoothing = Globals::aim_dynamic_smoothing;
    s.aim_smoothing_near = Globals::aim_smoothing_near;
    s.aim_smoothing_far = Globals::aim_smoothing_far;
    s.aim_selected_weapon_group = Globals::aim_selected_weapon_group;
    memcpy(s.aim_weapon_groups, Globals::aim_weapon_groups, sizeof(s.aim_weapon_groups));
    return s;
}

static TriggerConfigSnapshot CaptureTriggerConfig() {
    TriggerConfigSnapshot s{};
    s.trigger_enabled = Globals::trigger_enabled;
    s.trigger_draw_fov = Globals::trigger_draw_fov;
    s.trigger_ignore_flash = Globals::trigger_ignore_flash;
    s.trigger_key = Globals::trigger_key;
    s.trigger_key_mode = Globals::trigger_key_mode;
    s.trigger_delay = Globals::trigger_delay;
    s.trigger_fov = Globals::trigger_fov;
    memcpy(s.trigger_fov_color, Globals::trigger_fov_color, sizeof(s.trigger_fov_color));
    s.trigger_hitbox_head = Globals::trigger_hitbox_head;
    s.trigger_hitbox_neck = Globals::trigger_hitbox_neck;
    s.trigger_hitbox_chest = Globals::trigger_hitbox_chest;
    s.trigger_hitbox_stomach = Globals::trigger_hitbox_stomach;
    s.trigger_hitbox_pelvis = Globals::trigger_hitbox_pelvis;
    s.rcs_enabled = Globals::rcs_enabled;
    s.rcs_mode = Globals::rcs_mode;
    s.rcs_horizontal = Globals::rcs_horizontal;
    s.rcs_vertical = Globals::rcs_vertical;
    s.rcs_smooth = Globals::rcs_smooth;
    s.rcs_calibration = Globals::rcs_calibration;
    memcpy(s.rcs_weapon_groups, Globals::rcs_weapon_groups, sizeof(s.rcs_weapon_groups));
    return s;
}

static void RestoreAimConfig(const AimConfigSnapshot& s) {
    Globals::aim_enabled = s.aim_enabled;
    Globals::aim_vis_check = s.aim_vis_check;
    Globals::aim_ignore_flash = s.aim_ignore_flash;
    Globals::aim_scope_only = s.aim_scope_only;
    Globals::aim_soft_visible = s.aim_soft_visible;
    Globals::aim_soft_visible_grace_ms = s.aim_soft_visible_grace_ms;
    Globals::aim_soft_visible_min_bones = s.aim_soft_visible_min_bones;
    Globals::aim_persistent = s.aim_persistent;
    Globals::aim_draw_fov = s.aim_draw_fov;
    Globals::aim_fov = s.aim_fov;
    Globals::aim_smoothing = s.aim_smoothing;
    memcpy(Globals::aim_fov_color, s.aim_fov_color, sizeof(s.aim_fov_color));
    Globals::aim_key = s.aim_key;
    Globals::aim_key_mode = s.aim_key_mode;
    Globals::aim_hitbox_head = s.aim_hitbox_head;
    Globals::aim_hitbox_neck = s.aim_hitbox_neck;
    Globals::aim_hitbox_chest = s.aim_hitbox_chest;
    Globals::aim_hitbox_stomach = s.aim_hitbox_stomach;
    Globals::aim_hitbox_pelvis = s.aim_hitbox_pelvis;
    Globals::aim_humanize = s.aim_humanize;
    Globals::aim_humanize_strength = s.aim_humanize_strength;
    Globals::aim_humanize_jitter = s.aim_humanize_jitter;
    Globals::aim_humanize_curve = s.aim_humanize_curve;
    Globals::aim_sticky_target = s.aim_sticky_target;
    Globals::aim_switch_delay_ms = s.aim_switch_delay_ms;
    Globals::aim_dynamic_smoothing = s.aim_dynamic_smoothing;
    Globals::aim_smoothing_near = s.aim_smoothing_near;
    Globals::aim_smoothing_far = s.aim_smoothing_far;
    Globals::aim_selected_weapon_group = s.aim_selected_weapon_group;
    memcpy(Globals::aim_weapon_groups, s.aim_weapon_groups, sizeof(s.aim_weapon_groups));
}

static void RestoreTriggerConfig(const TriggerConfigSnapshot& s) {
    Globals::trigger_enabled = s.trigger_enabled;
    Globals::trigger_draw_fov = s.trigger_draw_fov;
    Globals::trigger_ignore_flash = s.trigger_ignore_flash;
    Globals::trigger_key = s.trigger_key;
    Globals::trigger_key_mode = s.trigger_key_mode;
    Globals::trigger_delay = s.trigger_delay;
    Globals::trigger_fov = s.trigger_fov;
    memcpy(Globals::trigger_fov_color, s.trigger_fov_color, sizeof(s.trigger_fov_color));
    Globals::trigger_hitbox_head = s.trigger_hitbox_head;
    Globals::trigger_hitbox_neck = s.trigger_hitbox_neck;
    Globals::trigger_hitbox_chest = s.trigger_hitbox_chest;
    Globals::trigger_hitbox_stomach = s.trigger_hitbox_stomach;
    Globals::trigger_hitbox_pelvis = s.trigger_hitbox_pelvis;
    Globals::rcs_enabled = s.rcs_enabled;
    Globals::rcs_mode = s.rcs_mode;
    Globals::rcs_horizontal = s.rcs_horizontal;
    Globals::rcs_vertical = s.rcs_vertical;
    Globals::rcs_smooth = s.rcs_smooth;
    Globals::rcs_calibration = s.rcs_calibration;
    memcpy(Globals::rcs_weapon_groups, s.rcs_weapon_groups, sizeof(s.rcs_weapon_groups));
}

struct WeaponPick_t {
    WeaponsEnum id;
    const char* name;
};

static const WeaponPick_t kWeaponPicks[] = {
    { WEP_Ak47, "AK-47" }, { WEP_Aug, "AUG" }, { WEP_Awp, "AWP" },
    { WEP_Bizon, "PP-Bizon" }, { WEP_Cz75A, "CZ75-Auto" }, { WEP_Deagle, "Desert Eagle" },
    { WEP_Elite, "Dual Berettas" }, { WEP_Famas, "FAMAS" }, { WEP_FiveSeven, "Five-SeveN" },
    { WEP_G3Sg1, "G3SG1" }, { WEP_Galil, "Galil AR" }, { WEP_Glock, "Glock-18" },
    { WEP_M249, "M249" }, { WEP_M4A1S, "M4A1-S" }, { WEP_M4A4, "M4A4" },
    { WEP_Mac10, "MAC-10" }, { WEP_Mag7, "MAG-7" }, { WEP_Mp5SD, "MP5-SD" },
    { WEP_Mp7, "MP7" }, { WEP_Mp9, "MP9" }, { WEP_Negev, "Negev" },
    { WEP_Nova, "Nova" }, { WEP_Xm1014, "XM1014" }, { WEP_P2000, "P2000" },
    { WEP_P250, "P250" }, { WEP_P90, "P90" }, { WEP_Revolver, "R8 Revolver" },
    { WEP_Sawedoff, "Sawed-Off" }, { WEP_Scar20, "SCAR-20" }, { WEP_Sg556, "SG 553" },
    { WEP_Ssg08, "SSG 08" }, { WEP_Tec9, "Tec-9" }, { WEP_Ump45, "UMP-45" },
    { WEP_UspS, "USP-S" }, { WEP_Zeus, "Zeus x27" }
};

static bool RenderSkinCard(const SkinInfo_t& skin, bool isSelected, float cardW, float cardH = 128.0f) {
    ImVec4 rarityCol = GetRarityColor(skin.rarity);
    ImGui::PushID(skin.paintKit * 10000 + (int)skin.weaponType);

    ImVec4 baseCol = isSelected
        ? ImVec4(rarityCol.x * 0.22f, rarityCol.y * 0.22f, rarityCol.z * 0.22f, 1.0f)
        : ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    ImVec4 hovCol = isSelected
        ? ImVec4(rarityCol.x * 0.30f, rarityCol.y * 0.30f, rarityCol.z * 0.30f, 1.0f)
        : ImVec4(0.14f, 0.14f, 0.14f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, baseCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, hovCol);

    bool clicked = ImGui::Button("##skin_card", ImVec2(cardW, cardH));
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    ID3D11ShaderResourceView* img = nullptr;
    if (!skin.image_url.empty()) img = GetSkinImage(skin.image_url);

    const float pad = 6.0f;
    const float imageH = cardH - 34.0f;
    ImVec2 imgMin(min.x + pad, min.y + pad);
    ImVec2 imgMax(max.x - pad, min.y + imageH);

    if (img) {
        draw->AddImage((ImTextureID)img, imgMin, imgMax);
    } else {
        ImU32 ph = ImGui::ColorConvertFloat4ToU32(ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
        draw->AddRectFilled(imgMin, imgMax, ph, 4.0f);
        draw->AddText(ImVec2(imgMin.x + 8.0f, imgMin.y + 8.0f), ImGui::ColorConvertFloat4ToU32(FAT_TEXT_DIM), "No Image");
    }

    if (isSelected) {
        ImU32 sel = ImGui::ColorConvertFloat4ToU32(ImVec4(rarityCol.x, rarityCol.y, rarityCol.z, 1.0f));
        draw->AddRect(min, max, sel, 5.0f, 0, 2.0f);
        draw->AddText(ImVec2(min.x + 8.0f, min.y + 8.0f), sel, "SELECTED");
    }

    ImGui::PushClipRect(ImVec2(min.x + 8.0f, max.y - 24.0f), ImVec2(max.x - 8.0f, max.y - 4.0f), true);
    draw->AddText(ImVec2(min.x + 8.0f, max.y - 22.0f), ImGui::ColorConvertFloat4ToU32(rarityCol), skin.name.c_str());
    ImGui::PopClipRect();

    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return clicked;
}




void Menu::Initialize(ID3D11Device* device)
{
    static bool initialized = false;
    if (initialized) return;
    CoInitialize(nullptr);
    g_MenuDevice = device;
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fc; fc.PixelSnapH = true; fc.OversampleH = 1; fc.OversampleV = 1;
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0 };
    io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 16, &fc, ranges);
    font_icon = io.Fonts->AddFontFromMemoryTTF(icon_font, sizeof(icon_font), 25.0f, &fc, ranges);
    poppins = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 25.0f, &fc, ranges);

    ImFontConfig tahoma_fc;
    tahoma_fc.FontBuilderFlags = 0; 
    esp_font = io.Fonts->AddFontFromFileTTF("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\ascania\\fonts\\ARCADECLASSIC.TTF", 11.0f, &tahoma_fc, io.Fonts->GetGlyphRangesDefault());
    
    weapon_icon_font = io.Fonts->AddFontFromMemoryTTF((void*)cs_icon, sizeof(cs_icon), 14.0f);

    LoadTextureFromMemory(device, logo_one, sizeof(logo_one), &logo);
    LoadTextureFromMemory(device, logo_two, sizeof(logo_two), &logotwo);
    DWORD size = sizeof(discord_username); GetUserNameA(discord_username, &size);
    time_t now = time(0); struct tm ts; localtime_s(&ts, &now);
    strftime(expiry_date, sizeof(expiry_date), "%Y-%m-%d", &ts);
    IsOpen = true; initialized = true;
}




static void MenuRenderInternal();

void Menu::Render()
{
    if (!IsOpen) return;

    __try {
        MenuRenderInternal();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        g_Logger.Log("MENU CRASH! Resetting ImGui state.");
        
        if (GImGui) {
            __try {
                ImGuiContext& g = *GImGui;
                while (g.ColorStack.Size > 0) { __try { ImGui::PopStyleColor(); } __except(EXCEPTION_EXECUTE_HANDLER) { break; } }
                while (g.StyleVarStack.Size > 0) { __try { ImGui::PopStyleVar(); } __except(EXCEPTION_EXECUTE_HANDLER) { break; } }
                while (g.CurrentWindowStack.Size > 1) { __try { ImGui::End(); } __except(EXCEPTION_EXECUTE_HANDLER) { break; } }
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
    }
}

static void MenuRenderInternal()
{
    
    static bool pendingReapply = false;
    static float reapplyTimer = 0.0f;
    static WeaponsEnum pendingWeapon = WEP_NONE;
    static int pendingPaintKit = 0;
    static bool pendingIsKnife = false;
    static bool pendingIsGlove = false;
    static uint16_t pendingGloveDefIndex = 0;
    static std::string pendingGloveName = "";
    
    
    if (pendingReapply) {
        reapplyTimer += ImGui::GetIO().DeltaTime;
        if (reapplyTimer >= 0.1f) {
            
            if (g_SkinManager) {
                if (pendingIsGlove) {
                    
                    g_SkinManager->Gloves.defIndex = pendingGloveDefIndex;
                    g_SkinManager->Gloves.paintKit = pendingPaintKit;
                    g_SkinManager->Gloves.name = pendingGloveName;
                    Globals::sc_force_update = 3;
                    g_Logger.Log("Auto-reapplied glove: defIndex=%d paintKit=%d", pendingGloveDefIndex, pendingPaintKit);
                } else if (pendingIsKnife && pendingWeapon == WEP_CtKnife) {
                    
                    SkinInfo_t s;
                    s.paintKit = pendingPaintKit;
                    s.weaponType = WEP_CtKnife;
                    s.wear = 0.001f;
                    s.seed = 0;
                    s.statTrak = -1;
                    g_SkinManager->AddSkin(s);
                    s.weaponType = WEP_TKnife;
                    g_SkinManager->AddSkin(s);
                    Globals::sc_force_update = 3;
                    g_Logger.Log("Auto-reapplied knife skin: paintKit=%d", pendingPaintKit);
                } else if (pendingWeapon != WEP_NONE) {
                    
                    SkinInfo_t s;
                    s.paintKit = pendingPaintKit;
                    s.weaponType = pendingWeapon;
                    s.wear = 0.001f;
                    s.seed = 0;
                    s.statTrak = -1;
                    g_SkinManager->AddSkin(s);
                    Globals::sc_force_update = 3;
                    g_Logger.Log("Auto-reapplied skin: weapon=%d paintKit=%d", (int)pendingWeapon, pendingPaintKit);
                }
            }
            pendingReapply = false;
            reapplyTimer = 0.0f;
            pendingIsKnife = false;
            pendingIsGlove = false;
        }
    }
    
    static float open_alpha = 0.0f;
    open_alpha = ImLerp(open_alpha, Menu::IsOpen ? 1.0f : 0.0f, 0.04f * (1.0f - ImGui::GetIO().DeltaTime));
    if (open_alpha > 0.98f) open_alpha = 1.0f;
    if (open_alpha < 0.01f) return;

    for (int i = 0; i < 4; i++) accent_colour[i] = Globals::menu_accent_color[i];
    ImVec4 accent = GetAccent();

    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_CheckMark] = accent;
    style.Colors[ImGuiCol_SliderGrab] = accent;
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(accent.x*0.8f, accent.y*0.8f, accent.z*0.8f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.15f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(accent.x*0.4f, accent.y*0.4f, accent.z*0.4f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = accent;
    style.Colors[ImGuiCol_Border] = ImVec4(0.227f, 0.227f, 0.227f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078f, 0.078f, 0.078f, 0.98f);
    style.Colors[ImGuiCol_Text] = FAT_TEXT;
    style.FrameBorderSize = 1.0f;
    style.FramePadding = ImVec2(6, 2);
    style.FrameRounding = 3;
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.WindowBorderSize = 2.0f;
    style.WindowRounding = 6.0f;
    style.ScrollbarRounding = 3;
    style.ScrollbarSize = 5;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, open_alpha);

    
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float menuW = 850.0f;
    float menuH = 620.0f;
    
    if (menuW > displaySize.x * 0.90f) menuW = displaySize.x * 0.90f;
    if (menuH > displaySize.y * 0.90f) menuH = displaySize.y * 0.90f;
    
    if (menuW < 500.0f) menuW = 500.0f;
    if (menuH < 400.0f) menuH = 400.0f;

    ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);
    ImGui::Begin("##fatality_main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        
        ImVec2 p = ImGui::GetWindowPos();
        float clampedX = p.x;
        float clampedY = p.y;
        if (clampedX + menuW > displaySize.x) clampedX = displaySize.x - menuW;
        if (clampedY + menuH > displaySize.y) clampedY = displaySize.y - menuH;
        if (clampedX < 0.0f) clampedX = 0.0f;
        if (clampedY < 0.0f) clampedY = 0.0f;
        if (clampedX != p.x || clampedY != p.y) {
            ImGui::SetWindowPos(ImVec2(clampedX, clampedY));
            p = ImVec2(clampedX, clampedY);
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        static int m_tab = 0;

        
        ImU32 bg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.078f, 0.078f, 0.078f, 0.98f * open_alpha));
        draw->AddRectFilled(p, ImVec2(p.x + menuW, p.y + menuH), bg, 6.0f);

        
        ImU32 topBarBg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.046f, 0.049f, 0.053f, open_alpha));
        draw->AddRectFilled(p, ImVec2(p.x + menuW, p.y + 48), topBarBg, 6.0f, ImDrawFlags_RoundCornersTop);
        ImU32 topBarLine = ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, 0.5f * open_alpha));
        draw->AddRectFilled(ImVec2(p.x, p.y + 47), ImVec2(p.x + menuW, p.y + 48), topBarLine);

        
        if (poppins) {
            ImGui::PushFont(poppins);
            ImU32 vgkCol = ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, open_alpha));
            const char* brandText = "NICE WARE";
            ImVec2 brandSize = ImGui::CalcTextSize(brandText);
            float brandX = p.x + menuW - brandSize.x - 28.0f;
            draw->AddText(poppins, poppins->FontSize, ImVec2(brandX, p.y + 8), vgkCol, brandText);
            ImGui::PopFont();
        }
        
        float dateX = p.x + menuW - 93.0f;
        draw->AddText(ImVec2(dateX, p.y + 26), ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, 0.7f * open_alpha)), expiry_date);

        
        const float tabMarginX = 5.0f;
        const float tabMarginTop = 6.0f;
        float contentW = menuW - (tabMarginX * 2.0f);
        const char* tabNames[] = { "LEGIT", "VISUALS", "SKINS", "MISC", "LUA", "CONFIGS" };
        const int tabCount = 6;
        const int tabIds[] = { 0, 3, 9, 6, 7, 8 };
        const float tabH = 30.0f;

        ImGui::SetCursorPos(ImVec2(tabMarginX + 3.0f, 58.0f + tabMarginTop));
        ImGui::BeginGroup();
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(accent.x * 0.25f, accent.y * 0.25f, accent.z * 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(accent.x * 0.20f, accent.y * 0.20f, accent.z * 0.20f, 1.0f));
            
            if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None))
            {
                for (int i = 0; i < tabCount; ++i)
                {
                    bool sel = (m_tab == tabIds[i]);
                    ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(accent.x, accent.y, accent.z, 1.0f) : FAT_TEXT);
                    
                    if (ImGui::BeginTabItem(tabNames[i]))
                    {
                        m_tab = tabIds[i];
                        if (m_tab == 8) Config::Refresh();
                        ImGui::EndTabItem();
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::EndTabBar();
            }
            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar();
        }
        ImGui::EndGroup();

        float contentH = menuH - 128.0f;
        float halfW = (contentW - 12.0f) * 0.5f;

        {
            const char* watermarkText = "NICE WARE";
            ImFont* wmFont = poppins; 
            if (wmFont) {
                ImGui::PushFont(wmFont);
                float wmScale = 3.8f; 
                ImVec2 wmTextSize = ImGui::CalcTextSize(watermarkText);
                wmTextSize.x *= wmScale;
                wmTextSize.y *= wmScale;
                float wmX = p.x + 15.0f + (contentW - wmTextSize.x) * 0.5f;
                float wmY = p.y + 58.0f + (contentH - wmTextSize.y) * 0.5f;
                ImU32 wmColor = ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, 0.028f * open_alpha));
                wmFont->Scale = wmScale;
                draw->AddText(wmFont, wmFont->FontSize * wmScale, ImVec2(wmX, wmY), wmColor, watermarkText);
                wmFont->Scale = 1.0f;
                ImGui::PopFont();
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::BeginChild("##TabScrollArea", ImVec2(contentW, contentH), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
        {
            switch (m_tab)
            {
            case 0:
            {
                ImVec2 legitRow = ImGui::GetCursorPos();
                const float legitPanelH = std::max(360.0f, contentH - 10.0f);
                GroupBoxBegin("AIMBOT", ImVec2(halfW, legitPanelH));
                    ImGui::Checkbox("Enable Aimbot", &Globals::aim_enabled);
                    KeyBinder("Aim Key", &Globals::aim_key);
                    const char* aimModes[] = { "Hold", "Toggle" };
                    ImGui::SetNextItemWidth(120);
                    ImGui::Combo("Aim Key Mode", &Globals::aim_key_mode, aimModes, IM_ARRAYSIZE(aimModes));
                    if (Globals::aim_key_mode == 0) {
                        bool keyDown = (Globals::aim_key != 0) && ((GetAsyncKeyState(Globals::aim_key) & 0x8000) != 0);
                        ImGui::TextDisabled("Aim Key State: %s", keyDown ? "DOWN" : "UP");
                    } else {
                        ImGui::TextDisabled("Aim Toggle State: %s", Globals::aim_toggle_state ? "ON" : "OFF");
                    }
                    ImGui::Checkbox("Aimlock", &Globals::aim_persistent);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.50f);
                    ImGui::Checkbox("Soft Visible", &Globals::aim_soft_visible);
                    if (Globals::aim_soft_visible) {
                        SliderIntInput("Visible Grace", &Globals::aim_soft_visible_grace_ms, 0, 1000, "%d ms");
                        SliderIntInput("Min Visible Bones", &Globals::aim_soft_visible_min_bones, 1, 5, "%d");
                    }
                    ImGui::Checkbox("Ignore Flash", &Globals::aim_ignore_flash);
                    ImGui::Checkbox("Draw FOV", &Globals::aim_draw_fov);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 32.0f);
                    ImGui::ColorEdit4("##fovC", Globals::aim_fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Visibility Check", &Globals::aim_vis_check);
                    if (Globals::aim_vis_check) {
                        bool rayLoaded = Raycasting::Get().IsLoaded();
                        if (!rayLoaded) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
                            ImGui::Text("(!) Using engine fallback (may rarely see through walls)");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                            ImGui::Text("Raycasting loaded - visibility filtering active");
                            ImGui::PopStyleColor();
                        }
                    }

                    ImGui::Spacing();
                    SectionHeader("WEAPON GROUP");
                    const char* aimGroupNames[] = { "Pistol", "SMG", "Rifle", "Sniper", "Heavy", "Shotgun", "Other" };
                    if (Globals::aim_selected_weapon_group < 0 || Globals::aim_selected_weapon_group >= Globals::AIM_GROUP_COUNT)
                        Globals::aim_selected_weapon_group = Globals::AIM_GROUP_RIFLE;
                    ImGui::SetNextItemWidth(200);
                    ImGui::Combo("Group", &Globals::aim_selected_weapon_group, aimGroupNames, IM_ARRAYSIZE(aimGroupNames));
                    auto& group = Globals::aim_weapon_groups[Globals::aim_selected_weapon_group];
                    ImGui::Checkbox("Enable This Group", &group.enabled);
                    if (group.enabled) {
                        SliderFloatInput("FOV", &group.fov, 0.f, 180.f, "%.1f");
                        SliderFloatInput("Smoothing", &group.smoothing, 0.01f, 1.f, "%.2f");
                        ImGui::Checkbox("Dynamic Smoothing", &group.dynamic_smoothing);
                        if (group.dynamic_smoothing) {
                            SliderFloatInput("Near Smooth", &group.smoothing_near, 0.01f, 1.f, "%.2f");
                            SliderFloatInput("Far Smooth", &group.smoothing_far, 0.01f, 1.f, "%.2f");
                        }
                        ImGui::Checkbox("Scope Only", &group.scope_only);

                        std::string groupAimPreview = "";
                        if (group.hitbox_head) groupAimPreview += "Head, ";
                        if (group.hitbox_neck) groupAimPreview += "Neck, ";
                        if (group.hitbox_chest) groupAimPreview += "Chest, ";
                        if (group.hitbox_stomach) groupAimPreview += "Stomach, ";
                        if (group.hitbox_pelvis) groupAimPreview += "Pelvis, ";
                        if (groupAimPreview.empty()) groupAimPreview = "None";
                        else groupAimPreview.erase(groupAimPreview.length() - 2);

                        ImGui::SetNextItemWidth(200);
                        if (ImGui::BeginCombo("Target Hitboxes", groupAimPreview.c_str())) {
                            ImGui::Selectable("Head", &group.hitbox_head, ImGuiSelectableFlags_DontClosePopups);
                            ImGui::Selectable("Neck", &group.hitbox_neck, ImGuiSelectableFlags_DontClosePopups);
                            ImGui::Selectable("Chest", &group.hitbox_chest, ImGuiSelectableFlags_DontClosePopups);
                            ImGui::Selectable("Stomach", &group.hitbox_stomach, ImGuiSelectableFlags_DontClosePopups);
                            ImGui::Selectable("Pelvis", &group.hitbox_pelvis, ImGuiSelectableFlags_DontClosePopups);
                            ImGui::EndCombo();
                        }
                    }
                    ImGui::Checkbox("Sticky Target", &Globals::aim_sticky_target);
                    if (Globals::aim_sticky_target) {
                        SliderIntInput("Switch Delay", &Globals::aim_switch_delay_ms, 0, 250, "%d ms");
                    }
                    ImGui::Spacing();
                    SectionHeader("EXTRAS");
                    ImGui::Checkbox("Enable Humanize", &Globals::aim_humanize);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.50f);
                    ImGui::Checkbox("Target Teammates", &Globals::target_teammates);
                    if (Globals::aim_humanize) {
                        SliderFloatInput("Strength", &Globals::aim_humanize_strength, 0.f, 2.f, "%.2f");
                        SliderFloatInput("Jitter", &Globals::aim_humanize_jitter, 0.f, 1.f, "%.2f");
                        SliderFloatInput("Curve", &Globals::aim_humanize_curve, 0.f, 1.f, "%.2f");
                    }
                    ImGui::Spacing();
                GroupBoxEnd();

                ImGui::SetCursorPos(ImVec2(legitRow.x + halfW + 6, legitRow.y));

                GroupBoxBegin("TRIGGER / RCS", ImVec2(halfW, legitPanelH));
                    ImGui::Checkbox("Enable Triggerbot", &Globals::trigger_enabled);
                    KeyBinder("Trigger Key", &Globals::trigger_key);
                    const char* trigModes[] = { "Hold", "Toggle" };
                    ImGui::SetNextItemWidth(120);
                    ImGui::Combo("Trigger Key Mode", &Globals::trigger_key_mode, trigModes, IM_ARRAYSIZE(trigModes));
                    if (Globals::trigger_key_mode == 0) {
                        bool keyDown = (Globals::trigger_key != 0) && ((GetAsyncKeyState(Globals::trigger_key) & 0x8000) != 0);
                        ImGui::TextDisabled("Trigger Key State: %s", keyDown ? "DOWN" : "UP");
                    } else {
                        ImGui::TextDisabled("Trigger Toggle State: %s", Globals::trigger_toggle_state ? "ON" : "OFF");
                    }
                    ImGui::Checkbox("Ignore Flash##trig", &Globals::trigger_ignore_flash);
                    ImGui::Checkbox("Scope Only", &Globals::trigger_scope_only);
                    ImGui::Checkbox("Velocity Check", &Globals::trigger_velocity_check);
                    SliderFloatInput("Trigger FOV", &Globals::trigger_fov, 0.f, 180.f, "%.1f");
                    SliderFloatInput("Shot Delay", &Globals::trigger_delay, 0.f, 500.f, "%.0f ms", "%.0f");
                    ImGui::Checkbox("Random Delay", &Globals::trigger_random_delay);
                    if (Globals::trigger_random_delay) {
                        SliderIntInput("Min Delay", &Globals::trigger_min_delay, 0, 50, "%d ms");
                        SliderIntInput("Max Delay", &Globals::trigger_max_delay, 0, 50, "%d ms");
                        if (Globals::trigger_min_delay > Globals::trigger_max_delay)
                            Globals::trigger_min_delay = Globals::trigger_max_delay;
                    }
                    ImGui::Checkbox("Draw FOV##trig", &Globals::trigger_draw_fov);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 32.0f);
                    ImGui::ColorEdit4("##trfC", Globals::trigger_fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Spacing();

                    std::string trig_preview = "";
                    if (Globals::trigger_hitbox_head) trig_preview += "Head, ";
                    if (Globals::trigger_hitbox_neck) trig_preview += "Neck, ";
                    if (Globals::trigger_hitbox_chest) trig_preview += "Chest, ";
                    if (Globals::trigger_hitbox_stomach) trig_preview += "Stomach, ";
                    if (Globals::trigger_hitbox_pelvis) trig_preview += "Pelvis, ";
                    if (trig_preview.empty()) trig_preview = "None";
                    else trig_preview.erase(trig_preview.length() - 2);

                    ImGui::SetNextItemWidth(200);
                    if (ImGui::BeginCombo("Trigger Hitboxes", trig_preview.c_str())) {
                        ImGui::Selectable("Head", &Globals::trigger_hitbox_head, ImGuiSelectableFlags_DontClosePopups);
                        ImGui::Selectable("Neck", &Globals::trigger_hitbox_neck, ImGuiSelectableFlags_DontClosePopups);
                        ImGui::Selectable("Chest", &Globals::trigger_hitbox_chest, ImGuiSelectableFlags_DontClosePopups);
                        ImGui::Selectable("Stomach", &Globals::trigger_hitbox_stomach, ImGuiSelectableFlags_DontClosePopups);
                        ImGui::Selectable("Pelvis", &Globals::trigger_hitbox_pelvis, ImGuiSelectableFlags_DontClosePopups);
                        ImGui::EndCombo();
                    }
                    ImGui::Spacing();
                    SectionHeader("RECOIL CONTROL");
                    ImGui::Checkbox("Enable RCS", &Globals::rcs_enabled);
                    if (Globals::aim_selected_weapon_group < 0 || Globals::aim_selected_weapon_group >= Globals::AIM_GROUP_COUNT)
                        Globals::aim_selected_weapon_group = Globals::AIM_GROUP_RIFLE;
                    ImGui::SetNextItemWidth(200);
                    ImGui::Combo("RCS Group", &Globals::aim_selected_weapon_group, aimGroupNames, IM_ARRAYSIZE(aimGroupNames));
                    auto& rcsGroup = Globals::rcs_weapon_groups[Globals::aim_selected_weapon_group];
                    ImGui::Checkbox("Enable This RCS Group", &rcsGroup.enabled);
                    const char* rcsModes[] = { "Standalone", "Aiming" };
                    if (rcsGroup.mode < 0 || rcsGroup.mode > 1)
                        rcsGroup.mode = 0;
                    ImGui::SetNextItemWidth(200);
                    ImGui::Combo("Mode", &rcsGroup.mode, rcsModes, IM_ARRAYSIZE(rcsModes));
                    SliderFloatInput("Horizontal", &rcsGroup.horizontal, 0.0f, 150.0f, "%.1f");
                    SliderFloatInput("Vertical", &rcsGroup.vertical, 0.0f, 150.0f, "%.1f");
                GroupBoxEnd();
                break;
            }
            case 3: 
            {
                static int visuals_target = 0;
                if (visuals_target < 0 || visuals_target > 1)
                    visuals_target = 0;
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
                    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(accent.x * 0.25f, accent.y * 0.25f, accent.z * 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(accent.x * 0.20f, accent.y * 0.20f, accent.z * 0.20f, 1.0f));
                    if (ImGui::BeginTabBar("##VisualsTabs", ImGuiTabBarFlags_None))
                    {
                        const char* vtabs[] = { "ENEMY", "TEAM" };
                        for (int i = 0; i < 2; i++)
                        {
                            bool sel = (visuals_target == i);
                            ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(accent.x, accent.y, accent.z, 1.0f) : FAT_TEXT_DIM);
                            if (ImGui::BeginTabItem(vtabs[i]))
                            {
                                visuals_target = i;
                                ImGui::EndTabItem();
                            }
                            ImGui::PopStyleColor();
                        }
                        ImGui::EndTabBar();
                    }
                    ImGui::PopStyleColor(5);
                    ImGui::PopStyleVar();
                }
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
                const bool enemySelected = visuals_target == 0;
                bool* esp_enabled = enemySelected ? &Globals::esp_enabled : &Globals::esp_enabled_team;
                bool* esp_box = enemySelected ? &Globals::esp_box : &Globals::esp_box_team;
                bool* esp_skeleton = enemySelected ? &Globals::esp_skeleton : &Globals::esp_skeleton_team;
                bool* esp_head = enemySelected ? &Globals::esp_head : &Globals::esp_head_team;
                bool* esp_name = enemySelected ? &Globals::esp_name : &Globals::esp_name_team;
                bool* esp_distance = enemySelected ? &Globals::esp_distance : &Globals::esp_distance_team;
                bool* esp_weapon = enemySelected ? &Globals::esp_weapon : &Globals::esp_weapon_team;
                bool* esp_health = enemySelected ? &Globals::esp_health : &Globals::esp_health_team;
                bool* esp_sound_enabled = enemySelected ? &Globals::esp_sound_enabled : &Globals::esp_sound_enabled_team;
                bool* esp_glow = enemySelected ? &Globals::esp_glow : &Globals::esp_glow_team;
                bool* chams_enabled = enemySelected ? &Globals::chams_enabled : &Globals::chams_enabled_team;
                bool* chams_wireframe = enemySelected ? &Globals::chams_wireframe : &Globals::chams_wireframe_team;
                bool* chams_filled = enemySelected ? &Globals::chams_filled : &Globals::chams_filled_team;
                bool* chamsv2_enabled = enemySelected ? &Globals::chamsv2_enabled : &Globals::chamsv2_enabled_team;
                int* chamsv2_material_type = enemySelected ? &Globals::chamsv2_material_type : &Globals::chamsv2_material_type_team;
                int* esp_box_type = enemySelected ? &Globals::esp_box_type : &Globals::esp_box_type_team;
                int* esp_skeleton_type = enemySelected ? &Globals::esp_skeleton_type : &Globals::esp_skeleton_type_team;
                float* esp_box_color = enemySelected ? Globals::esp_box_color : Globals::esp_box_color_team;
                float* esp_box_color_vis = enemySelected ? Globals::esp_box_color_vis : Globals::esp_box_color_vis_team;
                float* esp_skeleton_color = enemySelected ? Globals::esp_skeleton_color : Globals::esp_skeleton_color_team;
                float* esp_skeleton_color_vis = enemySelected ? Globals::esp_skeleton_color_vis : Globals::esp_skeleton_color_vis_team;
                float* esp_name_color = enemySelected ? Globals::esp_name_color : Globals::esp_name_color_team;
                float* esp_name_color_vis = enemySelected ? Globals::esp_name_color_vis : Globals::esp_name_color_vis_team;
                float* esp_distance_color = enemySelected ? Globals::esp_distance_color : Globals::esp_distance_color_team;
                float* esp_distance_color_vis = enemySelected ? Globals::esp_distance_color_vis : Globals::esp_distance_color_vis_team;
                float* esp_weapon_color = enemySelected ? Globals::esp_weapon_color : Globals::esp_weapon_color_team;
                float* esp_weapon_color_vis = enemySelected ? Globals::esp_weapon_color_vis : Globals::esp_weapon_color_vis_team;
                float* esp_health_color = enemySelected ? Globals::esp_health_color : Globals::esp_health_color_team;
                float* esp_health_color_vis = enemySelected ? Globals::esp_health_color_vis : Globals::esp_health_color_vis_team;
                float* esp_sound_color = enemySelected ? Globals::esp_sound_color : Globals::esp_sound_color_team;
                float* esp_glow_color = enemySelected ? Globals::esp_glow_color : Globals::esp_glow_color_team;
                float* esp_glow_color_vis = enemySelected ? Globals::esp_glow_color_vis : Globals::esp_glow_color_vis_team;
                float* chams_wire_color = enemySelected ? Globals::chams_wire_color : Globals::chams_wire_color_team;
                float* chams_wire_color_vis = enemySelected ? Globals::chams_wire_color_vis : Globals::chams_wire_color_vis_team;
                float* chams_fill_color = enemySelected ? Globals::chams_fill_color : Globals::chams_fill_color_team;
                float* chams_fill_color_vis = enemySelected ? Globals::chams_fill_color_vis : Globals::chams_fill_color_vis_team;
                float* chamsv2_fill_color = enemySelected ? Globals::chamsv2_fill_color : Globals::chamsv2_fill_color_team;
                float* chamsv2_fill_color_vis = enemySelected ? Globals::chamsv2_fill_color_vis : Globals::chamsv2_fill_color_vis_team;
                float* chamsv2_wire_color = enemySelected ? Globals::chamsv2_wire_color : Globals::chamsv2_wire_color_team;
                float* chamsv2_wire_color_vis = enemySelected ? Globals::chamsv2_wire_color_vis : Globals::chamsv2_wire_color_vis_team;

                ImVec2 visualsRow = ImGui::GetCursorPos();
                GroupBoxBegin("PLAYERS ESP", ImVec2(halfW, 330.0f));
                    ImGui::Checkbox("Enable ESP", esp_enabled);
                    ImGui::Separator();
                    ImGui::Checkbox("Box", esp_box);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##bxC", esp_box_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##bxCV", esp_box_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    if (*esp_box) {
                        const char* boxModes[] = { "Default", "Corner" };
                        ImGui::SameLine(100);
                        ImGui::SetNextItemWidth(100);
                        ImGui::Combo("##esp_box_type", esp_box_type, boxModes, IM_ARRAYSIZE(boxModes));
                    }
                    ImGui::Checkbox("Skeleton", esp_skeleton);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##skC", esp_skeleton_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##skCV", esp_skeleton_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    if (*esp_skeleton) {
                        const char* skelModes[] = { "Default", "Rounded" };
                        ImGui::SameLine(100);
                        ImGui::SetNextItemWidth(100);
                        ImGui::Combo("##esp_skel_type", esp_skeleton_type, skelModes, IM_ARRAYSIZE(skelModes));
                    }
                    ImGui::Checkbox("Head ESP", esp_head);
                    ImGui::Checkbox("Name", esp_name);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##nmC", esp_name_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##nmCV", esp_name_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Distance", esp_distance);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##dsC", esp_distance_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##dsCV", esp_distance_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Weapon", esp_weapon);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##wpC", esp_weapon_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##wpCV", esp_weapon_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Health Bar", esp_health);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##hpC", esp_health_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##hpCV", esp_health_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    const bool glowLockedByWireframe = enemySelected && Globals::wireframe_enemy_enabled;
                    if (glowLockedByWireframe) {
                        Globals::esp_glow = true;
                    }
                    ImGui::BeginDisabled(glowLockedByWireframe);
                    ImGui::Checkbox("Glow ESP", esp_glow);
                    ImGui::EndDisabled();
                    ImGui::SameLine(230); ImGui::ColorEdit4("##glC", esp_glow_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##glCV", esp_glow_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    if (*esp_glow) {
                        ImGui::SameLine(100);
                        ImGui::SetNextItemWidth(100);
                        ImGui::SliderFloat("##glow_thick", &Globals::esp_glow_thickness, 1.0f, 8.0f, "%.1f");
                    }
                    if (enemySelected) {
                        ImGui::Checkbox("Glow Dead", &Globals::esp_glow_dead);
                    }
                    ImGui::Checkbox("Sound ESP", esp_sound_enabled);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##seC", esp_sound_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                    if (enemySelected) {
                        ImGui::Checkbox("Grenade Prediction", &Globals::grenade_prediction_enabled);
                        ImGui::SameLine(230); ImGui::ColorEdit4("##gpPathColor", Globals::grenade_prediction_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SameLine(260); ImGui::ColorEdit4("##gpHitColor", Globals::grenade_prediction_hit_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                    }
                GroupBoxEnd();

                ImGui::SetCursorPos(ImVec2(visualsRow.x + halfW + 6, visualsRow.y));

                if (enemySelected) {
                    GroupBoxBegin("CHAMS", ImVec2(halfW, 267.0f));
                        ImGui::Checkbox("Enable##cv4", &Globals::chamsv4_enabled);
                        ImGui::Checkbox("Wallhack##cv4wh", &Globals::chamsv4_wallhack);

                        ImGui::Separator();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(accent.x, accent.y, accent.z, 0.9f));
                        ImGui::Text("TARGETS"); ImGui::PopStyleColor();

                        const char* cv4MatAll[] = { "White", "Latex", "Glow", "Glass", "Generic", "Unlit", "Solid", "Wireframe", "Bloom", "Bloom 2", "Illuminate", "Gost", "Crystal", "Gost 2", "Metallic", "Flow", "Dark Matter", "Data" };
                        const char* cv4MatPlayer[] = { "White", "Latex", "Glow", "Bloom", "Bloom 2", "Illuminate", "Gost", "Crystal", "Gost 2", "Metallic", "Flow", "Dark Matter", "Data" };

                        
                        ImGui::Checkbox("Players##cv4p", &Globals::chamsv4_players);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Chams for ENEMY players only");
                        ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                        ImGui::Combo("##cv4matP", &Globals::chamsv4_mat_players, cv4MatPlayer, IM_ARRAYSIZE(cv4MatPlayer));

                        
                        
                        
                        ImGui::Checkbox("Team##cv4tm", &Globals::chamsv4_team);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Chams for TEAMMATES (filtered by team index).\nDisabled = teammates never rendered.");
                        ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                        ImGui::BeginDisabled(!Globals::chamsv4_team);
                        ImGui::Combo("##cv4matTM", &Globals::chamsv4_mat_team, cv4MatPlayer, IM_ARRAYSIZE(cv4MatPlayer));
                        ImGui::EndDisabled();

                        
                        
                        
                        ImGui::Checkbox("Hands##cv4h", &Globals::chamsv4_hands);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Viewmodel hands/arms.\nAuto-hidden when Aimbot targeting is active.");
                        ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                        ImGui::Combo("##cv4matH", &Globals::chamsv4_mat_hands, cv4MatAll, IM_ARRAYSIZE(cv4MatAll));

                        ImGui::Checkbox("Gloves##cv4gl", &Globals::chamsv4_gloves);
                        ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                        ImGui::Combo("##cv4matGL", &Globals::chamsv4_mat_gloves, cv4MatAll, IM_ARRAYSIZE(cv4MatAll));

                        ImGui::Checkbox("Weapons##cv4wp", &Globals::chamsv4_weapons);
                        if (Globals::chamsv4_weapons) {
                            ImGui::Indent(20.0f);
                            ImGui::Checkbox("Guns##cv4guns", &Globals::chamsv4_weapons_guns);
                            ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                            ImGui::Combo("##cv4matGN", &Globals::chamsv4_mat_guns, cv4MatAll, IM_ARRAYSIZE(cv4MatAll));
                            ImGui::Checkbox("Knives##cv4knf", &Globals::chamsv4_weapons_knives);
                            ImGui::SameLine(140); ImGui::SetNextItemWidth(110);
                            ImGui::Combo("##cv4matKN", &Globals::chamsv4_mat_knives, cv4MatAll, IM_ARRAYSIZE(cv4MatAll));
                            ImGui::Unindent(20.0f);
                        }

                        ImGui::Separator();
                        ImGui::Text("Hidden Color");
                        ImGui::SameLine(230); ImGui::ColorEdit4("##cv4hC", Globals::chamsv4_hidden_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                        ImGui::Text("Visible Color");
                        ImGui::SameLine(230); ImGui::ColorEdit4("##cv4vC", Globals::chamsv4_visible_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                    GroupBoxEnd();
                } else {
                    GroupBoxBegin("3D CUBE ESP", ImVec2(halfW, 98.0f));
                    ImGui::Checkbox("Enable", chams_enabled);
                    ImGui::Separator();
                    ImGui::Checkbox("Wireframe", chams_wireframe);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##cwC", chams_wire_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##cwCV", chams_wire_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::Checkbox("Filled", chams_filled);
                    ImGui::SameLine(230); ImGui::ColorEdit4("##cfC", chams_fill_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::SameLine(260); ImGui::ColorEdit4("##cfCV", chams_fill_color_vis, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    GroupBoxEnd();
                }

                DrawEspFloatingPreview(p, menuW, *esp_enabled,
                    *esp_box, *esp_skeleton, *esp_head, *esp_name, *esp_distance, *esp_weapon, *esp_health,
                    *esp_box_type,
                    esp_box_color, esp_skeleton_color, esp_name_color, esp_distance_color, esp_weapon_color, esp_health_color);

                break;
            }
            case 6: 
            {
                const float miscGap = 6.0f;
                const float generalH = 155.0f;
                const float worldH = 100.0f;
                const float effectsH = 130.0f;
                const float cameraH = 105.0f;
                const float combatH = 100.0f;
                const float soundsH = 70.0f;

                ImGui::BeginGroup();
                GroupBoxBegin("GENERAL", ImVec2(halfW, 120.0f));
                    ImGui::Checkbox("Third Person", &Globals::thirdperson_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Spectator List", &Globals::speclist_enabled);
                    ImGui::Checkbox("Bomb Timer", &Globals::bombtimer_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Anti-Flash", &Globals::antiflash_enabled);
                    ImGui::Checkbox("Bunny Hop V2", &Globals::bunnyhop_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Auto Accept", &Globals::autoaccept_enabled);
                    ImGui::Checkbox("Chat Spam", &Globals::chatspam_enabled);
                    if (Globals::chatspam_enabled) {
                        ImGui::SetNextItemWidth(250);
                        ImGui::InputTextWithHint("##chatspam_msg", "Message...", Globals::chatspam_message, sizeof(Globals::chatspam_message));
                        ImGui::SetNextItemWidth(200);
                        ImGui::SliderFloat("Interval##chatspam", &Globals::chatspam_interval, 1.0f, 60.0f, "%.1f sec");
                    }
                GroupBoxEnd();
                ImGui::Dummy(ImVec2(0.0f, miscGap));

                GroupBoxBegin("EFFECTS", ImVec2(halfW, 97.0f));
                    ImGui::Checkbox("Bullet Tracers", &Globals::bullettracer_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Kill Particles", &Globals::killparticle_enabled);
                    if (Globals::bullettracer_enabled) {
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##TracerColor", Globals::bullettracer_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SliderFloat("Trail Life", &Globals::bullettracer_traillife, 0.5f, 5.0f, "%.1fs");
                        ImGui::SliderFloat("Thickness", &Globals::bullettracer_thickness, 1.0f, 6.0f, "%.1f");
                    }
                    if (Globals::killparticle_enabled) {
                        const char* killParticleTypes[] = { "Ember Burst", "Nova Ring", "Soul Spiral", "Shock Bloom" };
                        ImGui::SetNextItemWidth(180);
                        ImGui::Combo("Particle Type", &Globals::killparticle_type, killParticleTypes, IM_ARRAYSIZE(killParticleTypes));
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##KillParticleColor", Globals::killparticle_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    ImGui::Checkbox("Hit Particle Effect", &Globals::hitparticle_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Color Override", &Globals::hitparticle_color_override);
                    if (Globals::hitparticle_enabled) {
                        const char* hitParticleStyles[] = { "Blood Splash", "Heavy Gore Burst", "Energy Hit", "Shockwave Ring", "Spark Burst" };
                        ImGui::SetNextItemWidth(180);
                        ImGui::Combo("Hit Style", &Globals::hitparticle_style, hitParticleStyles, IM_ARRAYSIZE(hitParticleStyles));
                        if (Globals::hitparticle_color_override) {
                            ImGui::SameLine(0, 10);
                            ImGui::ColorEdit4("##HitParticleColor", Globals::hitparticle_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        }
                    }
                    ImGui::Checkbox("Chams Kill Effect", &Globals::chams_kill_effect);
                GroupBoxEnd();
                ImGui::Dummy(ImVec2(0.0f, miscGap));

                GroupBoxBegin("CAMERA", ImVec2(halfW, 72.0f));
                    ImGui::Checkbox("Override FOV", &Globals::visuals_fov_enabled);
                    ImGui::SliderFloat("Field of View", &Globals::visuals_fov, 60.f, 140.f, "%.0f");
                GroupBoxEnd();

                ImGui::EndGroup();

                ImGui::SameLine(0, 10);
                ImGui::BeginGroup();
                GroupBoxBegin("WORLD MODULATION", ImVec2(halfW, 72.0f));
                    ImGui::Checkbox("Skybox Changer", &Globals::skybox_changer);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Light Changer", &Globals::world_light_enabled);
                    if (Globals::skybox_changer) {
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##SkyboxColor", Globals::skybox_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SliderFloat("Skybox Intensity", &Globals::skybox_intensity, 0.0f, 10.0f, "%.2f");
                    }
                    if (Globals::world_light_enabled) {
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##WorldLightColor", Globals::world_light_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SliderFloat("Light Intensity", &Globals::world_light_intensity, 0.1f, 10.0f, "%.1f");
                    }
                    ImGui::Checkbox("Wall Changer", &Globals::world_walls_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Smoke Color Changer", &Globals::smoke_color_enabled);
                    if (Globals::world_walls_enabled) {
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##WorldWallColor", Globals::world_walls_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    }
                    if (Globals::smoke_color_enabled) {
                        ImGui::SameLine(0, 10);
                        ImGui::ColorEdit4("##SmokeColor", Globals::smoke_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
                    }
                GroupBoxEnd();
                ImGui::Dummy(ImVec2(0.0f, miscGap));

                GroupBoxBegin("SOUNDS", ImVec2(halfW, 47.0f));
                    Misc::InitHitsound();
                    ImGui::Checkbox("Enable Hitsound", &Globals::hitsound_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Enable Death Sound", &Globals::deathsound_enabled);
                    if (Globals::hitsound_enabled) {
                        if (!Globals::hitsound_files.empty()) {
                            ImGui::SetNextItemWidth(300);
                            const char* previewName = (Globals::hitsound_selected >= 0 && Globals::hitsound_selected < (int)Globals::hitsound_files.size())
                                ? Globals::hitsound_files[Globals::hitsound_selected].c_str() : "Select a sound...";
                            if (ImGui::BeginCombo("##hitsound_wav", previewName)) {
                                for (int i = 0; i < (int)Globals::hitsound_files.size(); i++) {
                                    bool isSel = (Globals::hitsound_selected == i);
                                    if (ImGui::Selectable(Globals::hitsound_files[i].c_str(), isSel)) Globals::hitsound_selected = i;
                                    if (isSel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }
                    Misc::InitDeathSound();
                    if (Globals::deathsound_enabled && !Globals::deathsound_files.empty()) {
                        ImGui::SetNextItemWidth(300);
                        const char* previewName = (Globals::deathsound_selected >= 0 && Globals::deathsound_selected < (int)Globals::deathsound_files.size())
                            ? Globals::deathsound_files[Globals::deathsound_selected].c_str() : "Select a sound...";
                        if (ImGui::BeginCombo("##deathsound_file", previewName)) {
                            for (int i = 0; i < (int)Globals::deathsound_files.size(); i++) {
                                bool isSel = (Globals::deathsound_selected == i);
                                if (ImGui::Selectable(Globals::deathsound_files[i].c_str(), isSel)) Globals::deathsound_selected = i;
                                if (isSel) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SliderFloat("Death Sound Volume", &Globals::deathsound_volume, 0.0f, 1.0f, "%.2f");
                    }
                GroupBoxEnd();
                ImGui::Dummy(ImVec2(0.0f, miscGap));

                GroupBoxBegin("COMBAT FEEDBACK", ImVec2(halfW, 72.0f));
                    ImGui::Checkbox("Enable Hitmarker", &Globals::hitmarker_enabled);
                    ImGui::SameLine(200.0f);
                    ImGui::Checkbox("Enable Hit Log", &Globals::hitlog_enabled);
                    ImGui::SameLine(0, 10);
                    ImGui::ColorEdit4("##HitmarkerColor", Globals::hitmarker_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    ImGui::Checkbox("Draw Damage in World", &Globals::hitlog_draw_damage);
                GroupBoxEnd();
                ImGui::EndGroup();
                break;
            }
            case 7:
            {
                static std::vector<std::string> s_lua_list;
                static int s_lua_sel = -1;
                static bool s_lua_dirty = true;
                static std::string s_lua_status = "Ready.";
                static std::string s_lua_preview = "-- Select a script to preview --";
                static const char* k_lua_dir = "C:\\Users\\OEM\\Documents\\GhostWeave\\lua\\";
                
                auto LuaEnsureDir = []() {
                    CreateDirectoryA("C:\\Users\\OEM\\Documents\\GhostWeave", nullptr);
                    CreateDirectoryA(k_lua_dir, nullptr);
                };
                
                auto LuaScan = [&]() {
                    LuaEnsureDir();
                    s_lua_list.clear();
                    WIN32_FIND_DATAA fd;
                    std::string pattern = std::string(k_lua_dir) + "*.lua";
                    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
                    if (h != INVALID_HANDLE_VALUE) {
                        do {
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                            s_lua_list.push_back(fd.cFileName);
                        } while (FindNextFileA(h, &fd));
                        FindClose(h);
                    }
                    s_lua_dirty = false;
                    if (s_lua_sel >= (int)s_lua_list.size()) s_lua_sel = -1;
                };
                
                auto LuaReadFile = [&](int idx) -> std::string {
                    if (idx < 0 || idx >= (int)s_lua_list.size()) return "";
                    std::string path = std::string(k_lua_dir) + s_lua_list[idx];
                    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (h == INVALID_HANDLE_VALUE) return "";
                    DWORD size = GetFileSize(h, nullptr);
                    if (size == 0 || size > 8192) { CloseHandle(h); return ""; }
                    std::string content(size, '\0');
                    DWORD read;
                    ReadFile(h, &content[0], size, &read, nullptr);
                    CloseHandle(h);
                    return content;
                };
                
                auto LuaExecuteScript = [&](const std::string& content) {
                    if (content.empty()) {
                        s_lua_status = "[Error] Script is empty.";
                        return;
                    }
                    
                    s_lua_status = "[Lua] Executing script...\n";
                    int changes = 0;
                    
                    std::stringstream ss(content);
                    std::string rawLine;
                    while (std::getline(ss, rawLine)) {
                        size_t commentPos = rawLine.find("--");
                        std::string line = (commentPos != std::string::npos) ? rawLine.substr(0, commentPos) : rawLine;
                        
                        size_t start = line.find_first_not_of(" \t\r\n");
                        if (start == std::string::npos) continue;
                        size_t end = line.find_last_not_of(" \t\r\n");
                        line = line.substr(start, end - start + 1);
                        
                        if (line.empty()) continue;
                        
                        size_t eqPos = line.find('=');
                        if (eqPos == std::string::npos || eqPos == 0) continue;
                        
                        std::string name = line.substr(0, eqPos);
                        std::string val = line.substr(eqPos + 1);
                        
                        start = name.find_first_not_of(" \t");
                        end = name.find_last_not_of(" \t");
                        name = name.substr(start, end - start + 1);
                        
                        start = val.find_first_not_of(" \t");
                        end = val.find_last_not_of(" \t");
                        if (start == std::string::npos) continue;
                        val = val.substr(start, end - start + 1);
                        
                        auto applyBool = [&](bool& ref) {
                            if (val == "true" || val == "1") { ref = true; changes++; s_lua_status += "- " + name + " = true\n"; }
                            else if (val == "false" || val == "0") { ref = false; changes++; s_lua_status += "- " + name + " = false\n"; }
                        };
                        auto applyInt = [&](int& ref) {
                            try { ref = std::stoi(val); changes++; s_lua_status += "- " + name + " = " + val + "\n"; } catch (...) {}
                        };
                        auto applyFloat = [&](float& ref) {
                            try { ref = std::stof(val); changes++; s_lua_status += "- " + name + " = " + val + "\n"; } catch (...) {}
                        };
                        
                        // AIMBOT
                        if (name == "aim_enabled") applyBool(Globals::aim_enabled);
                        else if (name == "aim_fov") applyFloat(Globals::aim_fov);
                        else if (name == "aim_smoothing") applyFloat(Globals::aim_smoothing);
                        else if (name == "aim_key") applyInt(Globals::aim_key);
                        else if (name == "aim_hitbox_head") applyBool(Globals::aim_hitbox_head);
                        else if (name == "aim_hitbox_neck") applyBool(Globals::aim_hitbox_neck);
                        else if (name == "aim_hitbox_chest") applyBool(Globals::aim_hitbox_chest);
                        else if (name == "aim_hitbox_stomach") applyBool(Globals::aim_hitbox_stomach);
                        else if (name == "aim_hitbox_pelvis") applyBool(Globals::aim_hitbox_pelvis);
                        else if (name == "aim_ignore_flash") applyBool(Globals::aim_ignore_flash);
                        else if (name == "aim_scope_only") applyBool(Globals::aim_scope_only);
                        else if (name == "aim_draw_fov") applyBool(Globals::aim_draw_fov);
                        else if (name == "aim_humanize") applyBool(Globals::aim_humanize);
                        
                        // TRIGGERBOT
                        else if (name == "trigger_enabled") applyBool(Globals::trigger_enabled);
                        else if (name == "trigger_delay") applyFloat(Globals::trigger_delay);
                        else if (name == "trigger_fov") applyFloat(Globals::trigger_fov);
                        else if (name == "trigger_hitbox_head") applyBool(Globals::trigger_hitbox_head);
                        else if (name == "trigger_hitbox_neck") applyBool(Globals::trigger_hitbox_neck);
                        else if (name == "trigger_hitbox_chest") applyBool(Globals::trigger_hitbox_chest);
                        else if (name == "trigger_hitbox_stomach") applyBool(Globals::trigger_hitbox_stomach);
                        else if (name == "trigger_hitbox_pelvis") applyBool(Globals::trigger_hitbox_pelvis);
                        else if (name == "trigger_ignore_flash") applyBool(Globals::trigger_ignore_flash);
                        else if (name == "trigger_draw_fov") applyBool(Globals::trigger_draw_fov);
                        
                        // ESP
                        else if (name == "esp_enabled") applyBool(Globals::esp_enabled);
                        else if (name == "esp_box") applyBool(Globals::esp_box);
                        else if (name == "esp_skeleton") applyBool(Globals::esp_skeleton);
                        else if (name == "esp_health") applyBool(Globals::esp_health);
                        else if (name == "esp_name") applyBool(Globals::esp_name);
                        else if (name == "esp_weapon") applyBool(Globals::esp_weapon);
                        else if (name == "esp_distance") applyBool(Globals::esp_distance);
                        else if (name == "esp_head") applyBool(Globals::esp_head);
                        else if (name == "esp_glow") applyBool(Globals::esp_glow);
                        
                        // CHAMS
                        else if (name == "chams_enabled") applyBool(Globals::chams_enabled);
                        else if (name == "chamsv4_enabled") applyBool(Globals::chamsv4_enabled);
                        else if (name == "chamsv4_wallhack") applyBool(Globals::chamsv4_wallhack);
                        else if (name == "chamsv4_players") applyBool(Globals::chamsv4_players);
                        else if (name == "chamsv4_hands") applyBool(Globals::chamsv4_hands);
                        else if (name == "chamsv4_gloves") applyBool(Globals::chamsv4_gloves);
                        else if (name == "chamsv4_weapons") applyBool(Globals::chamsv4_weapons);
                        else if (name == "chamsv4_team") applyBool(Globals::chamsv4_team);
                        
                        // SKIN CHANGER
                        else if (name == "sc_enabled") applyBool(Globals::sc_enabled);
                        else if (name == "sc_selected_knife") applyInt(Globals::sc_selected_knife);
                        else if (name == "sc_selected_glove_type") applyInt(Globals::sc_selected_glove_type);
                        else if (name == "sc_selected_glove_skin") applyInt(Globals::sc_selected_glove_skin);
                        else if (name == "sc_selected_music_kit") applyInt(Globals::sc_selected_music_kit);
                        else if (name == "sc_selected_agent_ct") applyInt(Globals::sc_selected_agent_ct);
                        else if (name == "sc_selected_agent_t") applyInt(Globals::sc_selected_agent_t);
                        
                        // MISC
                        else if (name == "bunnyhop_enabled") applyBool(Globals::bunnyhop_enabled);
                        else if (name == "hitsound_enabled") applyBool(Globals::hitsound_enabled);
                        else if (name == "hitmarker_enabled") applyBool(Globals::hitmarker_enabled);
                        else if (name == "thirdperson_enabled") applyBool(Globals::thirdperson_enabled);
                        else if (name == "antiflash_enabled") applyBool(Globals::antiflash_enabled);
                        else if (name == "visuals_fov_enabled") applyBool(Globals::visuals_fov_enabled);
                        else if (name == "visuals_fov") applyFloat(Globals::visuals_fov);
                        else if (name == "chatspam_enabled") applyBool(Globals::chatspam_enabled);
                        else if (name == "autoaccept_enabled") applyBool(Globals::autoaccept_enabled);
                        else if (name == "rcs_enabled") applyBool(Globals::rcs_enabled);
                        else if (name == "rcs_mode") applyInt(Globals::rcs_mode);
                        else if (name == "rcs_amount") applyFloat(Globals::rcs_amount);
                        else if (name == "rcs_horizontal") applyFloat(Globals::rcs_horizontal);
                        else if (name == "rcs_vertical") applyFloat(Globals::rcs_vertical);
                        else if (name == "rcs_smooth") applyFloat(Globals::rcs_smooth);
                        else if (name == "rcs_calibration") applyFloat(Globals::rcs_calibration);
                        
                        // PRINT
                        else if (name == "print") {
                            s_lua_status += "- Output: " + val + "\n";
                        }
                    }
                    
                    s_lua_status += "\nApplied " + std::to_string(changes) + " settings.";
                };
                
                auto LuaOpenDir = [&]() {
                    LuaEnsureDir();
                    ShellExecuteA(nullptr, "open", k_lua_dir, nullptr, nullptr, SW_SHOW);
                };
                
                if (s_lua_dirty) LuaScan();
                
                ImVec2 luaRow = ImGui::GetCursorPos();
                
                GroupBoxBegin("LUA SCRIPTS", ImVec2(halfW, 370.0f));
                {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float list_w = ImGui::GetContentRegionAvail().x;
                    float list_h = 220.0f;
                    ImVec2 list_pos = ImGui::GetCursorScreenPos();
                    ImVec2 list_end(list_pos.x + list_w, list_pos.y + list_h);
                    dl->AddRectFilled(list_pos, list_end, ImGui::ColorConvertFloat4ToU32(ImVec4(0.024f, 0.024f, 0.024f, 1.0f)), 3.0f);
                    dl->AddRect(list_pos, list_end, ImGui::ColorConvertFloat4ToU32(FAT_OUTLINE), 3.0f, 0, 1.0f);
                    
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    int hov_idx = -1;
                    const float item_h = 24.0f;
                    int max_vis = (int)(list_h / item_h);
                    for (int i = 0; i < (int)s_lua_list.size() && i < max_vis; i++) {
                        ImVec2 ip0(list_pos.x + 1, list_pos.y + 1 + i * item_h);
                        ImVec2 ip1(list_end.x - 1, ip0.y + item_h);
                        bool hov_i = (mouse.x >= ip0.x && mouse.x <= ip1.x && mouse.y >= ip0.y && mouse.y <= ip1.y);
                        bool sel_i = (s_lua_sel == i);
                        if (hov_i) hov_idx = i;
                        if (sel_i) dl->AddRectFilled(ip0, ip1, ImGui::ColorConvertFloat4ToU32(ImVec4(0.14f, 0.14f, 0.14f, 1.0f)), 2.0f);
                        else if (hov_i) dl->AddRectFilled(ip0, ip1, ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.10f, 1.0f)), 2.0f);
                        
                        float tx = ip0.x + 12;
                        float ty = ip0.y + (item_h - ImGui::GetFontSize()) * 0.5f;
                        ImU32 tc = sel_i ? ImGui::ColorConvertFloat4ToU32(GetAccent()) : (hov_i ? ImGui::ColorConvertFloat4ToU32(FAT_TEXT_LIGHT) : ImGui::ColorConvertFloat4ToU32(FAT_TEXT));
                        dl->AddText(ImVec2(tx, ty), tc, s_lua_list[i].c_str());
                    }
                    
                    ImGui::SetCursorScreenPos(list_pos);
                    ImGui::InvisibleButton("##lua_list", ImVec2(list_w, list_h));
                    if (ImGui::IsItemClicked() && hov_idx >= 0)
                        s_lua_sel = hov_idx;
                    
                    float by = list_end.y + 8;
                    float btn_w = (list_w - 8) / 2.0f;
                    ImGui::SetCursorScreenPos(ImVec2(list_pos.x, by));
                    if (ImGui::Button("Refresh", ImVec2(btn_w, 26))) {
                        s_lua_dirty = true;
                        s_lua_status = "Scanning lua folder...";
                    }
                    ImGui::SameLine(0, 8);
                    if (ImGui::Button("Open Folder", ImVec2(btn_w, 26))) {
                        LuaOpenDir();
                    }
                    by += 30;
                    ImGui::SetCursorScreenPos(ImVec2(list_pos.x, by));
                    if (ImGui::Button("Execute Selected", ImVec2(list_w, 30))) {
                        if (s_lua_sel >= 0 && s_lua_sel < (int)s_lua_list.size()) {
                            std::string content = LuaReadFile(s_lua_sel);
                            if (!content.empty()) {
                                LuaExecuteScript(content);
                            } else {
                                s_lua_status = "Failed to read script.";
                            }
                        } else {
                            s_lua_status = "No script selected.";
                        }
                    }
                    by += 34;
                    ImGui::SetCursorScreenPos(ImVec2(list_pos.x, by));
                    if (ImGui::Button("Load", ImVec2(list_w, 30))) {
                        s_lua_status = "Load button pressed.";
                    }
                }
                GroupBoxEnd();
                
                ImGui::SetCursorPos(ImVec2(luaRow.x + halfW + 6, luaRow.y));
                
                GroupBoxBegin("CONSOLE", ImVec2(halfW, 350.0f));
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    ImGui::TextUnformatted(s_lua_status.c_str());
                    ImGui::PopStyleColor();
                }
                GroupBoxEnd();
                break;
            }
            case 8: 
            {
                static char cfg_name[64] = "";
                static int preset_idx = 1;
                static int load_mode = 0;
                static int sel_cfg = -1;
                const char* presets[] = { "Minimal", "Pro", "Full" };

                const float presetsH = 180.0f;
                ImVec2 configRow = ImGui::GetCursorPos();
                GroupBoxBegin("PRESETS", ImVec2(halfW, presetsH));
                {
                    ImGui::Text("Preset");
                    ImGui::SameLine(80);
                    ImGui::SetNextItemWidth(140);
                    ImGui::Combo("##preset_combo", &preset_idx, presets, IM_ARRAYSIZE(presets));
                    if (ImGui::Button("Apply Preset", ImVec2(300, 24))) {
                        AimConfigSnapshot aimBefore = CaptureAimConfig();
                        TriggerConfigSnapshot triggerBefore = CaptureTriggerConfig();
                        ApplyFeaturePreset(preset_idx);
                        if (load_mode == 0) {
                            RestoreTriggerConfig(triggerBefore);
                        } else {
                            RestoreAimConfig(aimBefore);
                        }
                    }
                    ImGui::Separator();
                    ImGui::Text("Load Scope");
                    ImGui::RadioButton("Aim", &load_mode, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Trigger", &load_mode, 1);
                    ImGui::TextDisabled("Preset secili scope'a (Aim/Trigger) gore uygulanir.");
                }
                GroupBoxEnd();

                ImGui::SetCursorPos(ImVec2(configRow.x + halfW + 6, configRow.y));

                GroupBoxBegin("CONFIG SAVE/LOAD", ImVec2(halfW, 418.0f));
                {
                    ImGui::InputTextWithHint("##cfgN", "Config Name", cfg_name, 64);
                    if (ImGui::Button("Create / Save", ImVec2(300, 26))) Config::Save(cfg_name);
                    const char* favoriteLabel = Config::favorite_config.empty() ? "Favorite: none" : Config::favorite_config.c_str();
                    ImGui::TextDisabled("%s", favoriteLabel);
                    ImGui::Separator();

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, FAT_ODD);
                    ImGui::PushStyleColor(ImGuiCol_Border, FAT_OUTLINE);
                    ImGui::BeginChild("##cfgL", ImVec2(300, 184), true);
                    for (int i = 0; i < (int)Config::configs.size(); i++) {
                        const bool isFavorite = Config::configs[i] == Config::favorite_config;
                        std::string label = (isFavorite ? "[*] " : "    ") + Config::configs[i];
                        label += "##cfg_item_" + std::to_string(i);
                        if (ImGui::Selectable(label.c_str(), sel_cfg == i)) sel_cfg = i;
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2); ImGui::PopStyleVar();

                    if (sel_cfg >= 0 && sel_cfg < (int)Config::configs.size()) {
                        if (ImGui::Button("Load", ImVec2(145, 26))) {
                            AimConfigSnapshot aimBefore = CaptureAimConfig();
                            TriggerConfigSnapshot triggerBefore = CaptureTriggerConfig();
                            Config::Load(Config::configs[sel_cfg]);
                            if (load_mode == 0) {
                                RestoreTriggerConfig(triggerBefore);
                            } else {
                                RestoreAimConfig(aimBefore);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Save##2", ImVec2(145, 26))) Config::Save(Config::configs[sel_cfg]);
                        if (ImGui::Button("Set Favorite", ImVec2(145, 26))) Config::SetFavorite(Config::configs[sel_cfg]);
                        ImGui::SameLine();
                        if (ImGui::Button("Clear Favorite", ImVec2(145, 26))) Config::ClearFavorite();
                        if (ImGui::Button("Delete", ImVec2(300, 26))) { Config::Delete(Config::configs[sel_cfg]); sel_cfg = -1; }
                    }

                    if (ImGui::Button("Refresh", ImVec2(300, 26))) Config::Refresh();
                }
                GroupBoxEnd();
                break;
            }
            case 9:
            {
                static int sc_sub = 0;
                static char searchBuf[128] = "";

                if (sc_sub < 0 || sc_sub > 3)
                    sc_sub = 0;

                // Tabs inside scroll area (will stay at top of scroll)
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
                ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(accent.x * 0.25f, accent.y * 0.25f, accent.z * 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(accent.x * 0.20f, accent.y * 0.20f, accent.z * 0.20f, 1.0f));
                if (ImGui::BeginTabBar("##SkinsTabs", ImGuiTabBarFlags_None))
                {
                    const char* stabs[] = {"WEAPONS", "KNIVES", "GLOVES", "AGENTS"};
                    for (int i = 0; i < 4; i++)
                    {
                        bool sel = (sc_sub == i);
                        ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(accent.x, accent.y, accent.z, 1.0f) : FAT_TEXT_DIM);
                        if (ImGui::BeginTabItem(stabs[i]))
                        {
                            sc_sub = i;
                            searchBuf[0] = '\0';
                            ImGui::EndTabItem();
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::PopStyleColor(5);
                ImGui::PopStyleVar();
                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                // Scrollable skins content area
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
                ImGui::BeginChild("##SkinsContent", ImVec2(765.0f, 415.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

                if (ImGui::Checkbox("Enable SkinChanger", &Globals::sc_enabled)) {
                    if (Globals::sc_enabled) {
                        constexpr const char* kAutoAgentModel = "agents/models/ctm_st6/ctm_st6_variante.vmdl";
                        for (int i = 0; i < (int)AgentsCT.size(); i++) {
                            if (AgentsCT[i].model == kAutoAgentModel) {
                                Globals::sc_selected_agent_ct = i;
                                Globals::sc_force_update = 3;
                                break;
                            }
                        }
                    }
                }
                ImGui::SameLine(contentW > 500.0f ? 450.0f : contentW * 0.65f);
                if (ImGui::Button("Force Update", ImVec2(110, 22))) Globals::sc_force_update = 3;

                if (!g_SkinDB || !g_SkinDB->IsDumped()) {
                    const char* skinDbStatus = g_SkinDB ? g_SkinDB->GetStatus().c_str() : "Initializing";
                    ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "Skin database: %s", skinDbStatus);
                }
                ImGui::Separator();

                switch (sc_sub)
                {
                case 0: 
                {
                    static int selectedWeaponIdx = -1;
                    int weaponCount = (int)(sizeof(kWeaponPicks) / sizeof(kWeaponPicks[0]));
                    if (selectedWeaponIdx < -1 || selectedWeaponIdx >= weaponCount)
                        selectedWeaponIdx = -1;
                    const char* curWeaponLabel = "Active Weapon";
                    if (selectedWeaponIdx >= 0 && selectedWeaponIdx < weaponCount)
                        curWeaponLabel = kWeaponPicks[selectedWeaponIdx].name;

                    ImGui::Text("Weapon");
                    ImGui::SameLine(110);
                    ImGui::SetNextItemWidth(260);
                    if (ImGui::BeginCombo("##ws", curWeaponLabel)) {
                        if (ImGui::Selectable("Active Weapon", selectedWeaponIdx == -1)) selectedWeaponIdx = -1;
                        for (int i = 0; i < weaponCount; i++) {
                            bool sel = (selectedWeaponIdx == i);
                            if (ImGui::Selectable(kWeaponPicks[i].name, sel)) selectedWeaponIdx = i;
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    WeaponsEnum curWep = (selectedWeaponIdx == -1)
                        ? Globals::sc_current_weapon_def
                        : kWeaponPicks[selectedWeaponIdx].id;

                    const char* wn = WeaponIdToName((uint16_t)curWep);
                    if (selectedWeaponIdx == -1) {
                        if (curWep != WEP_NONE && !IsKnife((uint16_t)curWep))
                            ImGui::TextColored(ImVec4(accent.x, accent.y, accent.z, 1.0f), "Active: %s", wn);
                        else
                            ImGui::TextDisabled("Hold a weapon to see skins");
                    } else {
                        ImGui::TextColored(ImVec4(accent.x, accent.y, accent.z, 1.0f), "Selected: %s", wn);
                    }

                    ImGui::SetNextItemWidth(300);
                    ImGui::InputTextWithHint("##sw", "Search...", searchBuf, 128);

                    
                    if (curWep != WEP_NONE && !IsKnife((uint16_t)curWep)) {
                        bool isFixed = Globals::sc_fix_skin_weapons.count((int)curWep) > 0;
                        
                        if (isFixed) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(accent.x * 0.3f, accent.y * 0.3f, accent.z * 0.3f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x * 0.5f, accent.y * 0.5f, accent.z * 0.5f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x * 0.6f, accent.y * 0.6f, accent.z * 0.6f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(accent.x, accent.y, accent.z, 1.0f));
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, FAT_TEXT);
                        }
                        
                        if (ImGui::Button(isFixed ? "Fix Skin [ON]" : "Fix Skin", ImVec2(130, 26))) {
                            if (isFixed)
                                Globals::sc_fix_skin_weapons.erase((int)curWep);
                            else
                                Globals::sc_fix_skin_weapons.insert((int)curWep);
                            Globals::sc_force_update = 3;
                        }
                        ImGui::PopStyleColor(4);
                        
                        ImGui::SameLine();
                        ImGui::TextDisabled("Skininiz garip gorunuyorsa bu butona basin");
                    }

                    if (curWep != WEP_NONE && !IsKnife((uint16_t)curWep) && g_SkinDB && g_SkinDB->IsDumped() && g_SkinManager) {
                        auto skins = g_SkinDB->GetWeaponSkins(curWep);
                        std::string q = searchBuf; std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                        SkinInfo_t curS = g_SkinManager->GetSkin(curWep);

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, FAT_ODD);
                        ImGui::PushStyleColor(ImGuiCol_Border, FAT_OUTLINE);
                        ImGui::BeginChild("##wl", ImVec2(755.0f, 276.0f), true);
                        {
                            std::vector<int> filtered;
                            filtered.reserve(skins.size());
                            int selectedIdx = -1;
                            for (int i = 0; i < (int)skins.size(); i++) {
                                if (skins[i].paintKit == 0) continue;
                                if (!q.empty()) {
                                    std::string n = ToLowerCopy(skins[i].name);
                                    if (n.find(q) == std::string::npos) continue;
                                }
                                filtered.push_back(i);
                                if (skins[i].paintKit == curS.paintKit) selectedIdx = i;
                            }

                            int cols = 3;
                            float cardW = 230.0f;

                            int gridCount = 0;
                            if (selectedIdx >= 0) {
                                for (int i = 0; i < (int)filtered.size(); i++) {
                                    if (filtered[i] == selectedIdx) {
                                        int tmp = filtered[0];
                                        filtered[0] = filtered[i];
                                        filtered[i] = tmp;
                                        break;
                                    }
                                }
                            }

                            for (int i = 0; i < (int)filtered.size(); i++) {
                                int idx = filtered[i];
                                bool isSel = (idx == selectedIdx);
                                if (RenderSkinCard(skins[idx], isSel, cardW, 132.0f)) {
                                    if (skins[idx].paintKit == 0) {
                                        g_SkinManager->RemoveSkin(curWep);
                                        pendingReapply = false;
                                        pendingIsKnife = false;
                                        pendingIsGlove = false;
                                    } else {
                                        SkinInfo_t s = skins[idx];
                                        s.weaponType = curWep;
                                        s.wear = 0.001f;
                                        g_SkinManager->AddSkin(s);
                                        pendingReapply = true;
                                        reapplyTimer = 0.0f;
                                        pendingWeapon = curWep;
                                        pendingPaintKit = skins[idx].paintKit;
                                        pendingIsKnife = false;
                                        pendingIsGlove = false;
                                    }
                                    Globals::sc_force_update = 3;
                                }
                                gridCount++;
                                if (gridCount % cols != 0) ImGui::SameLine();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor(2); ImGui::PopStyleVar();
                    }
                    break;
                }
                case 1: 
                {
                    ImGui::Text("Knife Model"); ImGui::SameLine(110); ImGui::SetNextItemWidth(250);
                    int knifeCount = (int)Knives.size();
                    if (Globals::sc_selected_knife < 0 || Globals::sc_selected_knife > knifeCount)
                        Globals::sc_selected_knife = 0;
                    const char* curKN = (Globals::sc_selected_knife > 0 && Globals::sc_selected_knife <= knifeCount) ? Knives[Globals::sc_selected_knife - 1].name.c_str() : "Default";
                    if (ImGui::BeginCombo("##ks", curKN)) {
                        if (ImGui::Selectable("Default", Globals::sc_selected_knife == 0)) { Globals::sc_selected_knife = 0; Globals::sc_force_update = 3; }
                        for (int i = 0; i < knifeCount; i++) {
                            bool sel = (Globals::sc_selected_knife == i + 1);
                            if (ImGui::Selectable(Knives[i].name.c_str(), sel)) { Globals::sc_selected_knife = i + 1; Globals::sc_force_update = 3; }
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Spacing();

                    if (g_SkinDB && g_SkinDB->IsDumped() && g_SkinManager && Globals::sc_selected_knife > 0 && Globals::sc_selected_knife <= knifeCount) {
                        std::string fn = Knives[Globals::sc_selected_knife - 1].name;
                        auto& aks = g_SkinDB->GetKnifeSkins();
                        ImGui::Text("Skins: %s", fn.c_str());
                        ImGui::SetNextItemWidth(300);
                        ImGui::InputTextWithHint("##sk", "Search...", searchBuf, 128);
                        std::string q = searchBuf; std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                        SkinInfo_t ckS = g_SkinManager->GetSkin(WEP_CtKnife);

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, FAT_ODD);
                        ImGui::PushStyleColor(ImGuiCol_Border, FAT_OUTLINE);
                        ImGui::BeginChild("##ksl", ImVec2(740.0f, 274.0f), true);
                        {
                            std::vector<SkinInfo_t> viewSkins;
                            viewSkins.reserve(aks.size());

                            for (int i = 0; i < (int)aks.size(); i++) {
                                const auto& skin = aks[i];
                                if (skin.name.find(fn) == std::string::npos) continue;
                                if (!q.empty()) {
                                    std::string n = ToLowerCopy(skin.name);
                                    if (n.find(q) == std::string::npos) continue;
                                }
                                viewSkins.push_back(skin);
                            }

                            int cols = 3;
                            float cardW = 230.0f;

                            int selectedIdx = -1;
                            for (int i = 0; i < (int)viewSkins.size(); i++) {
                                if (viewSkins[i].paintKit == ckS.paintKit) { selectedIdx = i; break; }
                            }
                            if (selectedIdx == -1) selectedIdx = -1;

                            int gridCount = 0;
                            if (selectedIdx >= 0 && selectedIdx < (int)viewSkins.size()) {
                                SkinInfo_t tmp = viewSkins[0];
                                viewSkins[0] = viewSkins[selectedIdx];
                                viewSkins[selectedIdx] = tmp;
                                selectedIdx = 0;
                            }

                            for (int i = 0; i < (int)viewSkins.size(); i++) {
                                const auto& skin = viewSkins[i];
                                bool isSel = (i == selectedIdx);
                                if (RenderSkinCard(skin, isSel, cardW, 132.0f)) {
                                    if (skin.paintKit == 0) {
                                        g_SkinManager->RemoveSkin(WEP_CtKnife);
                                        g_SkinManager->RemoveSkin(WEP_TKnife);
                                        pendingReapply = false;
                                        pendingIsKnife = false;
                                        pendingIsGlove = false;
                                    } else {
                                        SkinInfo_t s = skin;
                                        s.weaponType = WEP_CtKnife; g_SkinManager->AddSkin(s);
                                        s.weaponType = WEP_TKnife; g_SkinManager->AddSkin(s);
                                        pendingReapply = true;
                                        reapplyTimer = 0.0f;
                                        pendingWeapon = WEP_CtKnife;
                                        pendingPaintKit = s.paintKit;
                                        pendingIsKnife = true;
                                        pendingIsGlove = false;
                                    }
                                    Globals::sc_force_update = 3;
                                }
                                gridCount++;
                                if (gridCount % cols != 0) ImGui::SameLine();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor(2); ImGui::PopStyleVar();
                    } else if (Globals::sc_selected_knife == 0) {
                        ImGui::TextDisabled("Select a knife model first");
                    }
                    break;
                }
                case 2: 
                {
                    ImGui::Text("Glove Model"); ImGui::SameLine(110); ImGui::SetNextItemWidth(250);
                    int gloveCount = (int)GloveTypes.size();
                    if (Globals::sc_selected_glove_type < -1 || Globals::sc_selected_glove_type >= gloveCount)
                        Globals::sc_selected_glove_type = -1;
                    const char* curGN = (Globals::sc_selected_glove_type >= 0 && Globals::sc_selected_glove_type < gloveCount) ? GloveTypes[Globals::sc_selected_glove_type].name.c_str() : "Default";
                    if (ImGui::BeginCombo("##gs", curGN)) {
                        if (ImGui::Selectable("Default", Globals::sc_selected_glove_type == -1)) {
                            Globals::sc_selected_glove_type = -1;
                            if (g_SkinManager) { g_SkinManager->Gloves = Glove_t(); }
                            Globals::sc_force_update = 3;
                            pendingReapply = false; 
                        }
                        for (int i = 0; i < gloveCount; i++) {
                            bool sel = (Globals::sc_selected_glove_type == i);
                            if (ImGui::Selectable(GloveTypes[i].name.c_str(), sel)) {
                                Globals::sc_selected_glove_type = i;
                                if (g_SkinManager) {
                                    g_SkinManager->Gloves.defIndex = GloveTypes[i].defIndex;
                                    g_SkinManager->Gloves.name = GloveTypes[i].name;
                                }
                                Globals::sc_force_update = 3;
                            }
                            if (sel) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Spacing();

                    if (g_SkinDB && g_SkinDB->IsDumped() && g_SkinManager && Globals::sc_selected_glove_type >= 0 && Globals::sc_selected_glove_type < gloveCount) {
                        std::string gloveName = GloveTypes[Globals::sc_selected_glove_type].name;
                        auto gloveSkins = g_SkinDB->GetGloveSkins(gloveName);
                        ImGui::Text("Skins: %s", gloveName.c_str());
                        ImGui::SetNextItemWidth(300);
                        ImGui::InputTextWithHint("##sg", "Search...", searchBuf, 128);
                        std::string q = searchBuf; std::transform(q.begin(), q.end(), q.begin(), ::tolower);

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, FAT_ODD);
                        ImGui::PushStyleColor(ImGuiCol_Border, FAT_OUTLINE);
                        ImGui::BeginChild("##gsl", ImVec2(740.0f, 274.0f), true);
                        {
                            std::vector<int> filtered;
                            filtered.reserve(gloveSkins.size());
                            int selectedIdx = -1;
                            for (int i = 0; i < (int)gloveSkins.size(); i++) {
                                if (!q.empty()) {
                                    std::string n = ToLowerCopy(gloveSkins[i].name);
                                    if (n.find(q) == std::string::npos) continue;
                                }
                                filtered.push_back(i);
                                if (g_SkinManager->Gloves.paintKit == gloveSkins[i].paintKit && gloveSkins[i].paintKit != 0) selectedIdx = i;
                            }

                            int cols = 3;
                            float cardW = 230.0f;

                            int gridCount = 0;
                            if (selectedIdx >= 0) {
                                for (int i = 0; i < (int)filtered.size(); i++) {
                                    if (filtered[i] == selectedIdx) {
                                        int tmp = filtered[0];
                                        filtered[0] = filtered[i];
                                        filtered[i] = tmp;
                                        break;
                                    }
                                }
                            }

                            for (int i = 0; i < (int)filtered.size(); i++) {
                                int idx = filtered[i];
                                bool isSel = (idx == selectedIdx);
                                if (RenderSkinCard(gloveSkins[idx], isSel, cardW, 132.0f)) {
                                    g_SkinManager->Gloves.defIndex = GloveTypes[Globals::sc_selected_glove_type].defIndex;
                                    g_SkinManager->Gloves.paintKit = gloveSkins[idx].paintKit;
                                    g_SkinManager->Gloves.name = GloveTypes[Globals::sc_selected_glove_type].name;
                                    Globals::sc_force_update = 3;
                                    pendingReapply = true;
                                    reapplyTimer = 0.0f;
                                    pendingIsGlove = true;
                                    pendingIsKnife = false;
                                    pendingGloveDefIndex = GloveTypes[Globals::sc_selected_glove_type].defIndex;
                                    pendingPaintKit = gloveSkins[idx].paintKit;
                                    pendingGloveName = GloveTypes[Globals::sc_selected_glove_type].name;
                                }
                                gridCount++;
                                if (gridCount % cols != 0) ImGui::SameLine();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor(2); ImGui::PopStyleVar();
                    } else if (Globals::sc_selected_glove_type == -1) {
                        ImGui::TextDisabled("Select a glove model first");
                    }
                    break;
                }
                case 3: 
                {
                    static int agent_team_tab = 0; 
                    if (agent_team_tab < 0 || agent_team_tab > 1)
                        agent_team_tab = 0;
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.0f);
                        ImGui::BeginChild("##AgentTabsWrap", ImVec2(ImGui::GetContentRegionAvail().x - 5.0f, 30.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
                        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(accent.x * 0.25f, accent.y * 0.25f, accent.z * 0.25f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(accent.x * 0.20f, accent.y * 0.20f, accent.z * 0.20f, 1.0f));
                        if (ImGui::BeginTabBar("##AgentTabs", ImGuiTabBarFlags_None))
                        {
                            const char* teamTabs[] = { "CT AGENTS", "T AGENTS" };
                            for (int i = 0; i < 2; i++)
                            {
                                bool sel = (agent_team_tab == i);
                                ImGui::PushStyleColor(ImGuiCol_Text, sel ? ImVec4(accent.x, accent.y, accent.z, 1.0f) : FAT_TEXT_DIM);
                                if (ImGui::BeginTabItem(teamTabs[i]))
                                {
                                    agent_team_tab = i;
                                    ImGui::EndTabItem();
                                }
                                ImGui::PopStyleColor();
                            }
                            ImGui::EndTabBar();
                        }
                        ImGui::PopStyleColor(5);
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                    }
                    ImGui::Dummy(ImVec2(0.0f, 4.0f));
                    ImGui::SetNextItemWidth(300);
                    ImGui::InputTextWithHint("##sa", "Search...", searchBuf, 128);
                    std::string q = searchBuf; std::transform(q.begin(), q.end(), q.begin(), ::tolower);

                    auto& agentList = (agent_team_tab == 0) ? AgentsCT : AgentsT;
                    int* selectedAgent = (agent_team_tab == 0) ? &Globals::sc_selected_agent_ct : &Globals::sc_selected_agent_t;
                    if (*selectedAgent < -1 || *selectedAgent >= (int)agentList.size())
                        *selectedAgent = -1;

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, FAT_ODD);
                    ImGui::PushStyleColor(ImGuiCol_Border, FAT_OUTLINE);
                    ImGui::BeginChild("##al", ImVec2(745.0f, 300.0f), true);
                    {
                        
                        bool isDefault = (*selectedAgent == -1);
                        if (ImGui::Selectable("Default", isDefault, 0, ImVec2(0, 22))) {
                            *selectedAgent = -1;
                            Globals::sc_force_update = 3;
                        }

                        for (int i = 0; i < (int)agentList.size(); i++) {
                            if (!q.empty()) {
                                std::string n = agentList[i].name;
                                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                                if (n.find(q) == std::string::npos) continue;
                            }
                            bool isSel = (*selectedAgent == i);
                            ImGui::PushID(i);
                            ImVec4 agentColor = (agent_team_tab == 0)
                                ? ImVec4(0.31f, 0.46f, 0.88f, 1.0f)  
                                : ImVec4(0.92f, 0.68f, 0.22f, 1.0f); 
                            ImGui::PushStyleColor(ImGuiCol_Text, agentColor);
                            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(agentColor.x * 0.15f, agentColor.y * 0.15f, agentColor.z * 0.15f, 0.8f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(agentColor.x * 0.25f, agentColor.y * 0.25f, agentColor.z * 0.25f, 0.8f));
                            if (ImGui::Selectable(agentList[i].name.c_str(), isSel, 0, ImVec2(0, 22))) {
                                *selectedAgent = i;
                                Globals::sc_force_update = 3;
                            }
                            ImGui::PopStyleColor(3);
                            ImGui::PopID();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor(2); ImGui::PopStyleVar();

                    
                    if (*selectedAgent >= 0 && *selectedAgent < (int)agentList.size()) {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(accent.x, accent.y, accent.z, 1.0f), "Selected: %s", agentList[*selectedAgent].name.c_str());
                        ImGui::TextDisabled("Model: %s", agentList[*selectedAgent].model.c_str());
                    } else {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Using default agent model");
                    }
                    break;
                }
                } 
                ImGui::EndChild();
                ImGui::PopStyleVar();
                break;
            } 
            default: break;
            } 
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
