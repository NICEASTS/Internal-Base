#include <imgui.h>
#include <d3d11.h>


ImFont* poppins = nullptr;
ImFont* font_icon = nullptr;


ID3D11ShaderResourceView* logo = nullptr;
ID3D11ShaderResourceView* logotwo = nullptr;
ID3D11ShaderResourceView* foto_user = nullptr;


char discord_username[64] = "bouy";
char expiry_date[64] = "25.09.2023";


float accent_colour[4] = { 1.f, 1.f, 1.f, 1.f };
bool particles = true;
int old_tab = 0;
float content_animation = 0.0f;
float dpi_scale = 1.0f;
const char* games_list[4] = { "Counter-Strike: GO", "Dying Light", "Apex Legends", "Warface" };
ID3D11ShaderResourceView* logo_png = nullptr;
