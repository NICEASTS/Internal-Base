/** GhostWeave Lua Wiki - i18n Single Page App */

const TRANSLATIONS = {
  en: {
    title: "GhostWeave Lua API",
    subtitle: "Lua Scripting",
    searchPlaceholder: "Search variables...",
    apiReference: "API Reference",
    language: "Language",
    onThisPage: "On this page",
    quickExamples: "Quick Examples",
    stats: "Stats",
    copy: "Copy",
    copied: "Copied!",
    variables: "Variables",
    categories: "Categories",
    examples: "Examples",
    types: "Types",
    gettingStarted: "Getting Started",
    exampleScript: "Example Script",
    syntaxRules: "Syntax Rules",
    configurationExamples: "Configuration Examples",
    apiReferenceDesc: "Complete list of variables recognized by the parser. Click any variable name to jump to its definition.",
    configExamplesDesc: "Below are some ready-to-use configuration examples. Click Copy to copy the script to your clipboard.",
    gettingStartedDesc: "Our scripting system uses a custom line-based parser (not LuaJIT or a full Lua VM). It reads each line of your script, strips comments, and maps variable = value pairs directly to the cheat's internal configuration.",
    noteTitle: "Note",
    noteDesc: "Scripts are stored in C:\\Users\\OEM\\Documents\\GhostWeave\\lua\\ and scanned at runtime. Press Refresh in the LUA tab to detect new files.",
    warningTitle: "Warning",
    warningDesc: "This is not a real Lua interpreter. Do not use loops, functions, conditionals, or tables. Only simple assignments are supported.",
    syntaxList: [
      "Each line must follow the format name = value",
      "Comments start with -- and are ignored by the parser",
      "Booleans: true / false or 1 / 0",
      "Integers: plain numbers like 90",
      "Floats: decimal numbers like 5.0",
      "Strings: only used with print, e.g. print = \"Hello\"",
      "Unknown variable names are silently skipped without errors"
    ],
    exampleBasic: "Basic Config",
    exampleBasicDesc: "Aimbot with ESP for standard gameplay",
    exampleLegit: "Legit Config",
    exampleLegitDesc: "Subtle aimbot with ESP for legitimate gameplay",
    exampleRage: "Rage Config",
    exampleRageDesc: "Maximum performance for HVH (Hack vs Hack)",
    exampleVisuals: "Visuals Only",
    exampleVisualsDesc: "ESP, chams and world visuals without aim",
    exampleSkin: "Skin Changer",
    exampleSkinDesc: "Activate skin changer with default settings",
    defaultLabel: "Default:",
    catAimbot: "Aimbot",
    catTriggerbot: "Triggerbot",
    catESP: "ESP",
    catChams: "Chams",
    catSkinChanger: "Skin Changer",
    catMisc: "Misc",
    catConsole: "Console Output"
  },
  tr: {
    title: "GhostWeave Lua API",
    subtitle: "Lua Scripting",
    searchPlaceholder: "Değişken ara...",
    apiReference: "API Referansı",
    language: "Dil",
    onThisPage: "Sayfa içeriği",
    quickExamples: "Hızlı Örnekler",
    stats: "İstatistikler",
    copy: "Kopyala",
    copied: "Kopyalandı!",
    variables: "Değişken",
    categories: "Kategori",
    examples: "Örnek",
    types: "Tip",
    gettingStarted: "Başlangıç",
    exampleScript: "Örnek Script",
    syntaxRules: "Söz Dizimi Kuralları",
    configurationExamples: "Yapılandırma Örnekleri",
    apiReferenceDesc: "Parser tarafından tanınan tüm değişkenlerin listesi. Herhangi bir değişken adına tıklayarak tanımına atlayabilirsiniz.",
    configExamplesDesc: "Aşağıda kullanıma hazır yapılandırma örnekleri bulunmaktadır. Script'i panoya kopyalamak için Kopyala'ya tıklayın.",
    gettingStartedDesc: "Script sistemimiz özel bir satır tabanlı parser kullanır (LuaJIT veya tam Lua VM değil). Scriptinizin her satırını okur, yorumları temizler ve variable = value çiftlerini doğrudan hile iç yapılandırmasına eşler.",
    noteTitle: "Not",
    noteDesc: "Scriptler C:\\Users\\OEM\\Documents\\GhostWeave\\lua\\ içinde saklanır ve çalışma zamanında taranır. Yeni dosyaları algılamak için LUA sekmesindeki Yenile'ye basın.",
    warningTitle: "Uyarı",
    warningDesc: "Bu gerçek bir Lua yorumlayıcısı değildir. Döngü, fonksiyon, koşul veya tablo kullanmayın. Sadece basit atamalar desteklenir.",
    syntaxList: [
      "Her satır name = value formatında olmalıdır",
      "Yorumlar -- ile başlar ve parser tarafından yok sayılır",
      "Boolean: true / false veya 1 / 0",
      "Integer: düz sayılar, örn. 90",
      "Float: ondalık sayılar, örn. 5.0",
      "String: sadece print ile kullanılır, örn. print = \"Merhaba\"",
      "Bilinmeyen değişken adları sessizce atlanır"
    ],
    exampleBasic: "Temel Ayar",
    exampleBasicDesc: "Standart oynayış için aimbot ve ESP",
    exampleLegit: "Legit Ayar",
    exampleLegitDesc: "Doğal görünümlü oynayış için hafif aimbot ve ESP",
    exampleRage: "Rage Ayar",
    exampleRageDesc: "HVH (Hack vs Hack) için maksimum performans",
    exampleVisuals: "Sadece Görsel",
    exampleVisualsDesc: "Aimbot kapalı, sadece ESP ve chams",
    exampleSkin: "Skin Değiştirici",
    exampleSkinDesc: "Skin değiştiriciyi varsayılan ayarlarla aktifleştir",
    defaultLabel: "Varsayılan:",
    catAimbot: "Aimbot",
    catTriggerbot: "Triggerbot",
    catESP: "ESP",
    catChams: "Chams",
    catSkinChanger: "Skin Değiştirici",
    catMisc: "Diğer",
    catConsole: "Konsol Çıktısı"
  }
};

const DOCS_DATA = {
  "categories": [
    {
      "name": "Aimbot",
      "icon": "crosshair",
      "variables": [
        {"name": "aim_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for the aimbot feature.", "desc_tr": "Aimbot özelliği için ana anahtar."},
        {"name": "aim_fov", "type": "float", "default": "7.0", "desc_en": "Aimbot field of view radius.", "desc_tr": "Aimbot görüş alanı (FOV) yarıçapı."},
        {"name": "aim_smoothing", "type": "float", "default": "0.4", "desc_en": "Aim smoothing factor (0.0 = instant, 1.0 = very slow).", "desc_tr": "Aim yumuşatma faktörü (0.0 = anında, 1.0 = çok yavaş)."},
        {"name": "aim_key", "type": "int", "default": "0", "desc_en": "Virtual key code for aimbot activation.", "desc_tr": "Aimbot aktifleştirme sanal tuş kodu."},
        {"name": "aim_hitbox_head", "type": "bool", "default": "true", "desc_en": "Allow targeting the head hitbox.", "desc_tr": "Kafa vuruş bölgesini hedeflemeye izin ver."},
        {"name": "aim_hitbox_neck", "type": "bool", "default": "false", "desc_en": "Allow targeting the neck hitbox.", "desc_tr": "Boyun vuruş bölgesini hedeflemeye izin ver."},
        {"name": "aim_hitbox_chest", "type": "bool", "default": "false", "desc_en": "Allow targeting the chest hitbox.", "desc_tr": "Göğüs vuruş bölgesini hedeflemeye izin ver."},
        {"name": "aim_hitbox_stomach", "type": "bool", "default": "false", "desc_en": "Allow targeting the stomach hitbox.", "desc_tr": "Karın vuruş bölgesini hedeflemeye izin ver."},
        {"name": "aim_hitbox_pelvis", "type": "bool", "default": "false", "desc_en": "Allow targeting the pelvis hitbox.", "desc_tr": "Kalça vuruş bölgesini hedeflemeye izin ver."},
        {"name": "aim_ignore_flash", "type": "bool", "default": "false", "desc_en": "Ignore flashbang blindness when aiming.", "desc_tr": "Aim yaparken flashbang körlüğünü yoksay."},
        {"name": "aim_scope_only", "type": "bool", "default": "false", "desc_en": "Only activate aimbot when scoped (snipers).", "desc_tr": "Sadece scope açıkken aimbot'u aktifleştir (snipers)."},
        {"name": "aim_draw_fov", "type": "bool", "default": "false", "desc_en": "Draw the aimbot FOV circle on screen.", "desc_tr": "Ekranda aimbot FOV çemberini göster."},
        {"name": "aim_vis_check", "type": "bool", "default": "false", "desc_en": "Visibility check - only target enemies you can directly see (blocks wallbang). Requires raycasting map files (.tri) to function properly. If raycasting is not loaded, falls back to engine visibility which may not block all wallbangs.", "desc_tr": "Görünürlük kontrolü - sadece doğrudan görebildiğin düşmanları hedefle (duvar arkası engeli). Düzgün çalışması için raycasting map dosyaları (.tri) gerekir. Raycasting yüklenmemişse engine görünürlük kontrolüne düşer ki bu tüm duvar arkası engelini kaldıramayabilir."},
        {"name": "aim_humanize", "type": "bool", "default": "false", "desc_en": "Add humanization curves to aim movement.", "desc_tr": "Aim hareketine insanileştirme eğrileri ekle."},
        {"name": "aim_sticky_target", "type": "bool", "default": "true", "desc_en": "Keep aiming at current target even if a better target appears. Prevents rapid target switching.", "desc_tr": "Daha iyi bir hedef çıksa bile mevcut hedefte kal. Hızlı hedef değiştirmeyi engeller."},
        {"name": "aim_switch_delay_ms", "type": "int", "default": "100", "desc_en": "Minimum delay in milliseconds before switching to a new target. Only works with Sticky Target enabled.", "desc_tr": "Yeni hedefe geçmeden önceki minimum gecikme (milisaniye). Sadece Sticky Target aktifken çalışır."},
        {"name": "aim_dynamic_smoothing", "type": "bool", "default": "true", "desc_en": "Use distance-based smoothing. Faster when close, smoother when far.", "desc_tr": "Mesafeye bağlı yumuşatma kullan. Yakında daha hızlı, uzakta daha yumuşak."},
        {"name": "aim_smoothing_near", "type": "float", "default": "0.16", "desc_en": "Smoothing value used when target is very close (0 = instant, 1 = very slow).", "desc_tr": "Hedef çok yakında kullanılan yumuşatma değeri (0 = anında, 1 = çok yavaş)."},
        {"name": "aim_smoothing_far", "type": "float", "default": "0.30", "desc_en": "Smoothing value used when target is far away (0 = instant, 1 = very slow).", "desc_tr": "Hedef uzakta kullanılan yumuşatma değeri (0 = anında, 1 = çok yavaş)."}
      ]
    },
    {
      "name": "Triggerbot",
      "icon": "zap",
      "variables": [
        {"name": "trigger_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for the triggerbot feature.", "desc_tr": "Triggerbot özelliği için ana anahtar."},
        {"name": "trigger_delay", "type": "float", "default": "0.0", "desc_en": "Delay in seconds before firing.", "desc_tr": "Ateşlemeden önceki gecikme (saniye)."},
        {"name": "trigger_fov", "type": "float", "default": "7.0", "desc_en": "Triggerbot activation FOV.", "desc_tr": "Triggerbot aktifleşme FOV'u."},
        {"name": "trigger_hitbox_head", "type": "bool", "default": "true", "desc_en": "Trigger on head hitbox.", "desc_tr": "Kafa vuruş bölgesinde trigger'la."},
        {"name": "trigger_hitbox_neck", "type": "bool", "default": "false", "desc_en": "Trigger on neck hitbox.", "desc_tr": "Boyun vuruş bölgesinde trigger'la."},
        {"name": "trigger_hitbox_chest", "type": "bool", "default": "false", "desc_en": "Trigger on chest hitbox.", "desc_tr": "Göğüs vuruş bölgesinde trigger'la."},
        {"name": "trigger_hitbox_stomach", "type": "bool", "default": "false", "desc_en": "Trigger on stomach hitbox.", "desc_tr": "Karın vuruş bölgesinde trigger'la."},
        {"name": "trigger_hitbox_pelvis", "type": "bool", "default": "false", "desc_en": "Trigger on pelvis hitbox.", "desc_tr": "Kalça vuruş bölgesinde trigger'la."},
        {"name": "trigger_ignore_flash", "type": "bool", "default": "false", "desc_en": "Ignore flashbang blindness for triggerbot.", "desc_tr": "Triggerbot için flashbang körlüğünü yoksay."},
        {"name": "trigger_scope_only", "type": "bool", "default": "false", "desc_en": "Only trigger when scoped (for snipers). Prevents accidental shots when not scoped.", "desc_tr": "Sadece scope açıkken trigger'la (snipers için). Scope kapalıyken yanlışlıkla ateş etmeyi engeller."},
        {"name": "trigger_random_delay", "type": "bool", "default": "false", "desc_en": "Add random delay on top of base delay. More legit looking reaction times.", "desc_tr": "Baz gecikmenin üzerine rastgele gecikme ekle. Daha doğal görünen reaksiyon süreleri."},
        {"name": "trigger_min_delay", "type": "int", "default": "0", "desc_en": "Minimum random delay in milliseconds (only works with Random Delay enabled).", "desc_tr": "Minimum rastgele gecikme (milisaniye). Sadece Random Delay aktifken çalışır."},
        {"name": "trigger_max_delay", "type": "int", "default": "15", "desc_en": "Maximum random delay in milliseconds (only works with Random Delay enabled).", "desc_tr": "Maksimum rastgele gecikme (milisaniye). Sadece Random Delay aktifken çalışır."},
        {"name": "trigger_velocity_check", "type": "bool", "default": "false", "desc_en": "Don't trigger while moving. More legit for holding angles.", "desc_tr": "Hareket halinde trigger atma. Açı tutarken daha legit görünür."},
        {"name": "trigger_draw_fov", "type": "bool", "default": "false", "desc_en": "Draw the triggerbot FOV circle.", "desc_tr": "Triggerbot FOV çemberini göster."}
      ]
    },
    {
      "name": "ESP",
      "icon": "eye",
      "variables": [
        {"name": "esp_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for enemy ESP.", "desc_tr": "Düşman ESP'si için ana anahtar."},
        {"name": "esp_box", "type": "bool", "default": "false", "desc_en": "Draw 2D bounding boxes around enemies.", "desc_tr": "Düşmanların etrafında 2D kutu çiz."},
        {"name": "esp_skeleton", "type": "bool", "default": "false", "desc_en": "Draw player skeleton lines.", "desc_tr": "Oyuncu iskelet çizgilerini çiz."},
        {"name": "esp_health", "type": "bool", "default": "false", "desc_en": "Show health bar or health text.", "desc_tr": "Can barı veya can metni göster."},
        {"name": "esp_name", "type": "bool", "default": "false", "desc_en": "Show player name tags.", "desc_tr": "Oyuncu isim etiketlerini göster."},
        {"name": "esp_weapon", "type": "bool", "default": "false", "desc_en": "Show active weapon name.", "desc_tr": "Aktif silah adını göster."},
        {"name": "esp_distance", "type": "bool", "default": "false", "desc_en": "Show distance to target in meters.", "desc_tr": "Hedefe olan mesafeyi metre olarak göster."},
        {"name": "esp_head", "type": "bool", "default": "false", "desc_en": "Draw head dot or head circle.", "desc_tr": "Kafa noktası veya kafa çemberi çiz."},
        {"name": "esp_glow", "type": "bool", "default": "false", "desc_en": "Apply glow outline effect to enemies.", "desc_tr": "Düşmanlara parlama kontur efekti uygula."}
      ]
    },
    {
      "name": "Chams",
      "icon": "layers",
      "variables": [
        {"name": "chams_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for standard chams.", "desc_tr": "Standart chams için ana anahtar."},
        {"name": "chamsv4_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for ChamsV4 advanced material system.", "desc_tr": "ChamsV4 gelişmiş materyal sistemi için ana anahtar."},
        {"name": "chamsv4_wallhack", "type": "bool", "default": "true", "desc_en": "Show hidden enemies through walls with chams.", "desc_tr": "Duvarların arkasındaki düşmanları chams ile göster."},
        {"name": "chamsv4_players", "type": "bool", "default": "true", "desc_en": "Apply chams to player models.", "desc_tr": "Oyuncu modellerine chams uygula."},
        {"name": "chamsv4_hands", "type": "bool", "default": "false", "desc_en": "Apply chams to viewmodel hands.", "desc_tr": "Viewmodel ellere chams uygula."},
        {"name": "chamsv4_gloves", "type": "bool", "default": "false", "desc_en": "Apply chams to gloves.", "desc_tr": "Eldivenlere chams uygula."},
        {"name": "chamsv4_weapons", "type": "bool", "default": "false", "desc_en": "Apply chams to dropped weapons.", "desc_tr": "Yere düşmüş silahlara chams uygula."},
        {"name": "chamsv4_team", "type": "bool", "default": "false", "desc_en": "Apply chams to teammates.", "desc_tr": "Takım arkadaşlarına chams uygula."}
      ]
    },
    {
      "name": "Skin Changer",
      "icon": "palette",
      "variables": [
        {"name": "sc_enabled", "type": "bool", "default": "false", "desc_en": "Master toggle for skin changer.", "desc_tr": "Skin değiştirici için ana anahtar."},
        {"name": "sc_selected_knife", "type": "int", "default": "0", "desc_en": "Selected knife skin index.", "desc_tr": "Seçili bıçak skin indeksi."},
        {"name": "sc_selected_glove_type", "type": "int", "default": "-1", "desc_en": "Selected glove type index.", "desc_tr": "Seçili eldiven tipi indeksi."},
        {"name": "sc_selected_glove_skin", "type": "int", "default": "-1", "desc_en": "Selected glove skin index.", "desc_tr": "Seçili eldiven skin indeksi."},
        {"name": "sc_selected_music_kit", "type": "int", "default": "-1", "desc_en": "Selected music kit index.", "desc_tr": "Seçili müzik kiti indeksi."},
        {"name": "sc_selected_agent_ct", "type": "int", "default": "-1", "desc_en": "Selected CT agent index.", "desc_tr": "Seçili CT ajanı indeksi."},
        {"name": "sc_selected_agent_t", "type": "int", "default": "-1", "desc_en": "Selected T agent index.", "desc_tr": "Seçili T ajanı indeksi."}
      ]
    },
    {
      "name": "Misc",
      "icon": "sliders",
      "variables": [
        {"name": "bunnyhop_enabled", "type": "bool", "default": "false", "desc_en": "Automatic bunny hop when holding space.", "desc_tr": "Boşluk tuşuna basılı tutarken otomatik bunny hop."},
        {"name": "hitsound_enabled", "type": "bool", "default": "false", "desc_en": "Play custom sound on hit.", "desc_tr": "Vuruşta özel ses çal."},
        {"name": "hitmarker_enabled", "type": "bool", "default": "false", "desc_en": "Draw screen hitmarker on hit.", "desc_tr": "Vuruşta ekranda hitmarker göster."},
        {"name": "thirdperson_enabled", "type": "bool", "default": "false", "desc_en": "Enable third-person camera view.", "desc_tr": "Üçüncü şahıs kamera görünümünü aktifleştir."},
        {"name": "antiflash_enabled", "type": "bool", "default": "false", "desc_en": "Reduce or remove flashbang blindness.", "desc_tr": "Flashbang körlüğünü azalt veya kaldır."},
        {"name": "visuals_fov_enabled", "type": "bool", "default": "false", "desc_en": "Override field of view.", "desc_tr": "Görüş alanını (FOV) geçersiz kıl."},
        {"name": "visuals_fov", "type": "float", "default": "90.0", "desc_en": "Desired FOV value.", "desc_tr": "İstenen FOV değeri."},
        {"name": "chatspam_enabled", "type": "bool", "default": "false", "desc_en": "Enable automatic chat spam.", "desc_tr": "Otomatik chat spam'ini aktifleştir."},
        {"name": "autoaccept_enabled", "type": "bool", "default": "false", "desc_en": "Auto-accept matchmaking matches.", "desc_tr": "Eşleştirme maçlarını otomatik kabul et."},
        {"name": "rcs_enabled", "type": "bool", "default": "false", "desc_en": "Enable recoil control system.", "desc_tr": "Geri tepme kontrol sistemini aktifleştir."},
        {"name": "rcs_amount", "type": "float", "default": "0.5", "desc_en": "RCS compensation strength.", "desc_tr": "RCS telafi gücü."}
      ]
    },
    {
      "name": "Console Output",
      "icon": "terminal",
      "variables": [
        {"name": "print", "type": "string", "default": "\"\"", "desc_en": "Print a message to the Lua console output.", "desc_tr": "Lua konsol çıktısına mesaj yazdır. Kullanım: print = \"Merhaba\""}
      ]
    }
  ]
};

let currentLang = localStorage.getItem('gw_wiki_lang') || 'en';
let flatVars = [];
let headings = [];

function t(key) {
  return TRANSLATIONS[currentLang]?.[key] || TRANSLATIONS['en'][key] || key;
}

function getVarDesc(v) {
  return v[`desc_${currentLang}`] || v.desc_en || '';
}

function getCatName(catName) {
  const key = 'cat' + catName.replace(/\s+/g, '');
  return t(key) || catName;
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/[&<>"']/g, m => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;','\'':'&#39;'}[m]));
}

function highlightSearch(text, query) {
  if (!query) return escapeHtml(text);
  const q = escapeHtml(query);
  const regex = new RegExp(`(${q.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
  return escapeHtml(text).replace(regex, '<span class="search-highlight">$1</span>');
}

function syntaxHighlight(code) {
  code = escapeHtml(code);
  code = code.replace(/\b(false|true)\b/g, '<span class="hljs-boolean">$1</span>');
  code = code.replace(/\b(print|aim_|esp_|trigger_|chams_|sc_|bunnyhop_|hitsound_|hitmarker_|thirdperson_|antiflash_|visuals_|chatspam_|autoaccept_|rcs_)\w*\b/g, '<span class="hljs-function">$1</span>');
  code = code.replace(/(--.*$)/gm, '<span class="hljs-comment">$1</span>');
  code = code.replace(/\b(\d+\.?\d*)\b/g, '<span class="hljs-number">$1</span>');
  code = code.replace(/(=)/g, '<span class="hljs-operator">$1</span>');
  return code;
}

function copyCode(btn, code) {
  navigator.clipboard.writeText(code).then(() => {
    const original = btn.innerHTML;
    btn.innerHTML = t('copied');
    setTimeout(() => btn.innerHTML = original, 1500);
  });
}

function getCategoryIcon(icon) {
  const icons = {
    crosshair: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="22" y1="12" x2="18" y2="12"/><line x1="6" y1="12" x2="2" y2="12"/><line x1="12" y1="6" x2="12" y2="2"/><line x1="12" y1="22" x2="12" y2="18"/></svg>',
    zap: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>',
    eye: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>',
    layers: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 2 7 12 12 22 7 12 2"/><polyline points="2 17 12 22 22 17"/><polyline points="2 12 12 17 22 12"/></svg>',
    palette: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="13.5" cy="6.5" r=".5"/><circle cx="17.5" cy="10.5" r=".5"/><circle cx="8.5" cy="7.5" r=".5"/><circle cx="6.5" cy="12.5" r=".5"/><path d="M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.01 17.461 2 12 2z"/></svg>',
    sliders: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><line x1="1" y1="14" x2="7" y2="14"/><line x1="9" y1="8" x2="15" y2="8"/><line x1="17" y1="16" x2="23" y2="16"/></svg>',
    terminal: '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="4 17 10 11 4 5"/><line x1="12" y1="19" x2="20" y2="19"/></svg>'
  };
  return icons[icon] || '';
}

function renderNav(filter = '') {
  const nav = document.getElementById('navTree');
  const f = filter.toLowerCase().trim();
  nav.innerHTML = '';

  DOCS_DATA.categories.forEach(cat => {
    const matches = cat.variables.filter(v => 
      v.name.toLowerCase().includes(f) || 
      getVarDesc(v).toLowerCase().includes(f)
    );
    if (f && !matches.length) return;

    const group = document.createElement('div');
    group.className = 'nav-group' + (f ? '' : ' collapsed');
    group.dataset.cat = cat.name.toLowerCase().replace(/\s+/g, '-');

    const header = document.createElement('div');
    header.className = 'nav-group-header';
    const icon = getCategoryIcon(cat.icon);
    const chevronDown = `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" class="chevron-svg"><path d="m6 9 6 6 6-6"/></svg>`;
    const chevronUp = `<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" class="chevron-svg"><path d="m18 15-6-6-6 6"/></svg>`;
    
    const displayName = getCatName(cat.name);
    header.innerHTML = `<div style="display:flex;align-items:center;gap:8px">${icon}<span>${escapeHtml(displayName)}</span></div><span class="chevron-icon">${f ? chevronUp : chevronDown}</span>`;
    header.onclick = () => {
      group.classList.toggle('collapsed');
      const chevronSpan = header.querySelector('.chevron-icon');
      if (chevronSpan) {
        chevronSpan.innerHTML = group.classList.contains('collapsed') ? chevronDown : chevronUp;
      }
    };
    group.appendChild(header);

    const items = document.createElement('div');
    items.className = 'nav-items';
    (f ? matches : cat.variables).forEach(v => {
      const item = document.createElement('div');
      item.className = 'nav-item';
      item.innerHTML = highlightSearch(v.name, f);
      item.onclick = () => {
        document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));
        item.classList.add('active');
        location.hash = '#' + v.name;
        scrollToVar(v.name);
        closeMobileMenu();
      };
      items.appendChild(item);
    });
    group.appendChild(items);
    nav.appendChild(group);
  });
}

function scrollToVar(name) {
  const el = document.getElementById('var-' + name);
  if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

function renderToc() {
  const tocNav = document.getElementById('tocNav');
  tocNav.innerHTML = '';
  
  headings = [];
  document.querySelectorAll('.content [id]').forEach(el => {
    const level = el.tagName === 'H2' ? 2 : el.tagName === 'H3' ? 3 : 0;
    if (!level) return;
    const text = el.dataset.tocTitle || el.textContent;
    headings.push({ id: el.id, level, text, el });
    
    const a = document.createElement('a');
    a.className = `toc-item h${level}`;
    a.href = '#' + el.id;
    a.textContent = text;
    a.onclick = (e) => {
      e.preventDefault();
      el.scrollIntoView({ behavior: 'smooth', block: 'start' });
      history.pushState(null, '', '#' + el.id);
    };
    tocNav.appendChild(a);
  });
}

function renderTocExamples() {
  const container = document.getElementById('tocExamples');
  if (!container) return;
  container.innerHTML = '';
  
  const examples = [
    'exampleBasic',
    'exampleLegit',
    'exampleRage',
    'exampleVisuals',
    'exampleSkin'
  ];
  
  examples.forEach(key => {
    const div = document.createElement('div');
    div.className = 'toc-example-item';
    div.textContent = t(key);
    div.onclick = () => {
      const el = document.getElementById(key);
      if (el) {
        el.scrollIntoView({ behavior: 'smooth', block: 'start' });
        history.pushState(null, '', '#' + key);
      }
      closeMobileMenu();
    };
    container.appendChild(div);
  });
}

function renderTocStats() {
  const container = document.getElementById('tocStats');
  if (!container) return;
  
  const totalVars = DOCS_DATA.categories.reduce((acc, c) => acc + c.variables.length, 0);
  const totalCats = DOCS_DATA.categories.length;
  const totalExamples = 5;
  
  container.innerHTML = `
    <div class="toc-stat-item">
      <div class="toc-stat-value">${totalVars}</div>
      <div class="toc-stat-label">${t('variables')}</div>
    </div>
    <div class="toc-stat-item">
      <div class="toc-stat-value">${totalCats}</div>
      <div class="toc-stat-label">${t('categories')}</div>
    </div>
    <div class="toc-stat-item">
      <div class="toc-stat-value">${totalExamples}</div>
      <div class="toc-stat-label">${t('examples')}</div>
    </div>
    <div class="toc-stat-item">
      <div class="toc-stat-value">3</div>
      <div class="toc-stat-label">${t('types')}</div>
    </div>
  `;
}

function updateTocActive() {
  const scrollPos = window.scrollY + 120;
  let active = null;
  
  for (const h of headings) {
    if (h.el.offsetTop <= scrollPos) {
      active = h.id;
    }
  }
  
  document.querySelectorAll('.toc-item').forEach(item => {
    item.classList.toggle('active', item.getAttribute('href') === '#' + active);
  });
}

function renderContent() {
  const content = document.getElementById('content');
  let html = '';

  html += `<div class="content-header">
    <h1>${t('title')}</h1>
    <p>${t('gettingStartedDesc')}</p>
  </div>`;

  html += `<section class="intro-section">`;
  
  html += `<h2 id="getting-started" data-toc-title="${t('gettingStarted')}">${t('gettingStarted')}</h2>`;
  
  html += `<p>${t('gettingStartedDesc')}</p>`;

  html += `<div class="callout callout-info">
    <svg class="callout-icon" viewBox="0 0 24 24" fill="none" stroke="#2563ef" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>
    <div class="callout-content">
      <div class="callout-title">${t('noteTitle')}</div>
      <p>${t('noteDesc')}</p>
    </div>
  </div>`;

  html += `<h3 id="example-script" data-toc-title="${t('exampleScript')}">${t('exampleScript')}</h3>`;

  const exampleCode = `-- ${t('exampleBasic')}
aim_enabled = true
aim_fov = 5.0
aim_smoothing = 0.15

-- ESP
esp_enabled = true
esp_box = true
esp_skeleton = true

-- ${t('catConsole')}
print = "GhostWeave Lua script loaded!"`;

  html += `<div class="code-block-wrapper">
    <div class="code-block-header">
      <span class="code-block-lang">Lua</span>
      <button class="code-block-copy" onclick="copyCode(this, \`${escapeHtml(exampleCode)}\`)">${t('copy')}</button>
    </div>
    <pre class="code-block"><code>${syntaxHighlight(exampleCode)}</code></pre>
  </div>`;

  html += `<h3 id="syntax-rules" data-toc-title="${t('syntaxRules')}">${t('syntaxRules')}</h3>`;

  const syntaxItems = t('syntaxList');
  html += `<ul>`;
  syntaxItems.forEach(item => {
    html += `<li>${item}</li>`;
  });
  html += `</ul>`;

  html += `<div class="callout callout-warn">
    <svg class="callout-icon" viewBox="0 0 24 24" fill="none" stroke="#d4a017" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path><line x1="12" y1="9" x2="12" y2="13"></line><line x1="12" y1="17" x2="12.01" y2="17"></line></svg>
    <div class="callout-content">
      <div class="callout-title">${t('warningTitle')}</div>
      <p>${t('warningDesc')}</p>
    </div>
  </div>`;

  html += `</section>`;

  // Examples
  html += `<h2 id="examples" data-toc-title="${t('configurationExamples')}">${t('configurationExamples')}</h2>`;
  html += `<p>${t('configExamplesDesc')}</p>`;

  const examples = [
    {
      key: 'exampleBasic',
      code: `-- ${t('exampleBasic')}
aim_enabled = true
aim_fov = 5.0
aim_smoothing = 0.15
aim_hitbox_head = true

esp_enabled = true
esp_box = true
esp_health = true

print = "${t('exampleBasic')}!"`
    },
    {
      key: 'exampleLegit',
      code: `-- ${t('exampleLegit')}
aim_enabled = true
aim_fov = 3.5
aim_smoothing = 0.35
aim_hitbox_head = true
aim_hitbox_chest = true
aim_draw_fov = true

esp_enabled = true
esp_box = false
esp_skeleton = false
esp_health = true
esp_name = true
esp_distance = true

print = "${t('exampleLegit')}!"`
    },
    {
      key: 'exampleRage',
      code: `-- ${t('exampleRage')}
aim_enabled = true
aim_fov = 15.0
aim_smoothing = 0.05
aim_hitbox_head = true
aim_hitbox_neck = true
aim_hitbox_chest = true
aim_ignore_flash = true

esp_enabled = true
esp_box = true
esp_skeleton = true
esp_health = true
esp_glow = true

chamsv4_enabled = true
chamsv4_wallhack = true
chamsv4_players = true

print = "${t('exampleRage')}!"`
    },
    {
      key: 'exampleVisuals',
      code: `-- ${t('exampleVisuals')}
aim_enabled = false
trigger_enabled = false

esp_enabled = true
esp_box = true
esp_skeleton = true
esp_health = true
esp_name = true
esp_weapon = true
esp_distance = true
esp_head = true
esp_glow = true

chamsv4_enabled = true
chamsv4_wallhack = true
chamsv4_players = true
chamsv4_hands = true
chamsv4_weapons = true

print = "${t('exampleVisuals')}!"`
    },
    {
      key: 'exampleSkin',
      code: `-- ${t('exampleSkin')}
sc_enabled = true
sc_selected_knife = 1
sc_selected_glove_type = 0
sc_selected_glove_skin = 100
sc_selected_music_kit = 3
sc_selected_agent_ct = 5
sc_selected_agent_t = 7

print = "${t('exampleSkin')}!"`
    }
  ];

  examples.forEach(ex => {
    const exId = ex.key;
    html += `<div class="example-card" id="${exId}">
      <div class="example-header">
        <div>
          <div class="example-title">${t(ex.key)}</div>
          <div class="example-desc">${t(ex.key + 'Desc')}</div>
        </div>
        <button class="code-block-copy" onclick="copyCode(this, \`${escapeHtml(ex.code)}\`)">${t('copy')}</button>
      </div>
      <div class="example-content">
        <pre class="code-block"><code>${syntaxHighlight(ex.code)}</code></pre>
      </div>
    </div>`;
  });

  // API Reference
  html += `<h2 id="api-reference" data-toc-title="${t('apiReference')}">${t('apiReference')}</h2>`;
  html += `<p>${t('apiReferenceDesc')}</p>`;

  DOCS_DATA.categories.forEach(cat => {
    if (cat.name === 'Console Output') return;
    const catId = cat.name.toLowerCase().replace(/\s+/g, '-');
    const displayName = getCatName(cat.name);
    
    html += `<div class="variables-section">`;
    html += `<h3 id="cat-${catId}" data-toc-title="${escapeHtml(displayName)}">${escapeHtml(displayName)} <span class="var-count">${cat.variables.length}</span></h3>`;
    
    cat.variables.forEach(v => {
      const desc = getVarDesc(v);
      html += `<div class="variable-card" id="var-${v.name}">
        <a href="#${v.name}" class="variable-anchor" aria-label="Anchor">#</a>
        <div class="variable-header">
          <span class="variable-name">${escapeHtml(v.name)}</span>
          <span class="variable-type">${escapeHtml(v.type)}</span>
        </div>
        <div class="variable-default"><span class="label">${t('defaultLabel')}</span><span class="value">${escapeHtml(v.default)}</span></div>
        <div class="variable-desc">${escapeHtml(desc)}</div>
      </div>`;
    });
    
    html += `</div>`;
  });

  const consoleCat = DOCS_DATA.categories.find(c => c.name === 'Console Output');
  if (consoleCat) {
    html += `<div class="variables-section">`;
    const displayName = getCatName(consoleCat.name);
    html += `<h3 id="cat-console-output" data-toc-title="${escapeHtml(displayName)}">${escapeHtml(displayName)} <span class="var-count">${consoleCat.variables.length}</span></h3>`;
    
    consoleCat.variables.forEach(v => {
      const desc = getVarDesc(v);
      html += `<div class="variable-card" id="var-${v.name}">
        <a href="#${v.name}" class="variable-anchor" aria-label="Anchor">#</a>
        <div class="variable-header">
          <span class="variable-name">${escapeHtml(v.name)}</span>
          <span class="variable-type">${escapeHtml(v.type)}</span>
        </div>
        <div class="variable-default"><span class="label">${t('defaultLabel')}</span><span class="value">${escapeHtml(v.default)}</span></div>
        <div class="variable-desc">${escapeHtml(desc)}</div>
      </div>`;
    });
    
    html += `</div>`;
  }

  content.innerHTML = html;
}

function updateStaticLabels() {
  const sidebarNavTitle = document.getElementById('sidebarNavTitle');
  if (sidebarNavTitle) sidebarNavTitle.textContent = t('apiReference');
  
  const searchInput = document.getElementById('searchInput');
  if (searchInput) searchInput.placeholder = t('searchPlaceholder');
  
  const tocTitlePage = document.getElementById('tocTitlePage');
  if (tocTitlePage) tocTitlePage.textContent = t('onThisPage');
  
  const tocTitleExamples = document.getElementById('tocTitleExamples');
  if (tocTitleExamples) tocTitleExamples.textContent = t('quickExamples');
  
  const tocTitleStats = document.getElementById('tocTitleStats');
  if (tocTitleStats) tocTitleStats.textContent = t('stats');
  
  document.title = t('title') + ' - ' + t('subtitle');
}

function handleHash() {
  const h = location.hash.slice(1);
  if (!h) return;
  
  const el = document.getElementById('var-' + h) || document.getElementById(h);
  if (el) {
    setTimeout(() => el.scrollIntoView({ behavior: 'smooth', block: 'start' }), 100);
    document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
    const navItem = Array.from(document.querySelectorAll('.nav-item')).find(n => n.textContent.trim() === h);
    if (navItem) {
      navItem.classList.add('active');
      navItem.closest('.nav-group').classList.remove('collapsed');
    }
  }
}

function initSearch() {
  const input = document.getElementById('searchInput');
  let debounce;
  
  input.addEventListener('input', e => {
    clearTimeout(debounce);
    debounce = setTimeout(() => {
      const val = e.target.value;
      renderNav(val);
      
      if (!val) {
        renderContent();
        renderToc();
        renderTocExamples();
        handleHash();
      } else {
        document.querySelectorAll('.variables-section').forEach(sec => {
          let hasVisible = false;
          sec.querySelectorAll('.variable-card').forEach(card => {
            const name = card.querySelector('.variable-name').textContent.toLowerCase();
            const desc = card.querySelector('.variable-desc').textContent.toLowerCase();
            const visible = name.includes(val.toLowerCase()) || desc.includes(val.toLowerCase());
            card.style.display = visible ? '' : 'none';
            if (visible) hasVisible = true;
          });
          sec.style.display = hasVisible ? '' : 'none';
        });
        
        document.querySelectorAll('.example-card').forEach(card => {
          const text = card.textContent.toLowerCase();
          card.style.display = text.includes(val.toLowerCase()) ? '' : 'none';
        });
        
        document.querySelector('.intro-section')?.style.setProperty('display', val ? 'none' : '');
        document.getElementById('examples')?.style.setProperty('display', val ? 'none' : '');
        document.getElementById('api-reference')?.style.setProperty('display', val ? 'none' : '');
        renderToc();
      }
    }, 150);
  });
}

function initMobileMenu() {
  const toggle = document.getElementById('menuToggle');
  const sidebar = document.getElementById('sidebar');
  const overlay = document.getElementById('sidebarOverlay');
  
  function open() {
    sidebar.classList.add('open');
    overlay.classList.add('active');
    document.body.style.overflow = 'hidden';
  }
  
  function close() {
    sidebar.classList.remove('open');
    overlay.classList.remove('active');
    document.body.style.overflow = '';
  }
  
  toggle.addEventListener('click', () => sidebar.classList.contains('open') ? close() : open());
  overlay.addEventListener('click', close);
  
  window.closeMobileMenu = close;
}

function initScrollSpy() {
  let ticking = false;
  window.addEventListener('scroll', () => {
    if (!ticking) {
      requestAnimationFrame(() => {
        updateTocActive();
        ticking = false;
      });
      ticking = true;
    }
  });
}

function initLangSelector() {
  const dropdown = document.getElementById('langDropdown');
  const trigger = document.getElementById('langDropdownTrigger');
  const currentLabel = document.getElementById('langDropdownCurrent');
  const menu = document.getElementById('langDropdownMenu');
  
  if (!dropdown || !trigger || !menu) return;
  
  const items = menu.querySelectorAll('.lang-dropdown-item');
  
  function setActiveLang(lang) {
    currentLang = lang;
    localStorage.setItem('gw_wiki_lang', lang);
    
    items.forEach(item => {
      const isSelected = item.dataset.value === lang;
      item.classList.toggle('selected', isSelected);
    });
    
    const activeItem = menu.querySelector(`[data-value="${lang}"]`);
    if (activeItem && currentLabel) {
      const nameEl = activeItem.querySelector('.lang-dropdown-name');
      const flagEl = activeItem.querySelector('.lang-dropdown-flag');
      if (nameEl) currentLabel.textContent = nameEl.textContent;
      const triggerFlag = trigger.querySelector('.lang-dropdown-flag');
      if (triggerFlag && flagEl) triggerFlag.textContent = flagEl.textContent;
    }
    
    updateStaticLabels();
    renderContent();
    renderNav();
    renderToc();
    renderTocExamples();
    renderTocStats();
  }
  
  // Set initial state
  setActiveLang(currentLang);
  
  // Toggle dropdown
  trigger.addEventListener('click', (e) => {
    e.preventDefault();
    e.stopPropagation();
    dropdown.classList.toggle('open');
    trigger.classList.toggle('active', dropdown.classList.contains('open'));
  });
  
  // Select item
  items.forEach(item => {
    item.addEventListener('click', (e) => {
      e.stopPropagation();
      const lang = item.dataset.value;
      setActiveLang(lang);
      dropdown.classList.remove('open');
      trigger.classList.remove('active');
    });
  });
  
  // Close on outside click
  document.addEventListener('click', () => {
    dropdown.classList.remove('open');
    trigger.classList.remove('active');
  });
}

function init() {
  flatVars = [];
  DOCS_DATA.categories.forEach(cat => {
    cat.variables.forEach(v => flatVars.push({ ...v, category: cat.name }));
  });
  
  initLangSelector();
  updateStaticLabels();
  renderContent();
  renderNav();
  renderToc();
  renderTocExamples();
  renderTocStats();
  initSearch();
  initMobileMenu();
  initScrollSpy();
  
  handleHash();
  window.addEventListener('hashchange', handleHash);
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
