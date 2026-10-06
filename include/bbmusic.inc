#if defined _bbmusic_included
    #endinput
#endif
#define _bbmusic_included

/*
    ============================================================================
                        BBMusic SA-MP Include (bbmusic.inc)
                Official Audio Stream & Boombox Library for GTA SA-MP
                      Powered by BBMusic Web (bbmusic.biz.id)
    ============================================================================
    Fitur Utama:
    - Realtime Web Sync: Sinkronisasi otomatis dengan lagu terbaru dari web
    - 3D Spatial Audio Stream untuk Boombox (Objek 2226)
    - Dynamic Sync: Pemain yang mendekat otomatis mendengar musik realtime
    - Bebas Emoji: 100% kompatibel dengan font font SA-MP DirectX (Codepage 1252)
    - Input Custom ID / Link Langsung dari bbmusic.biz.id
    - Dialog GUI Interaktif (/boombox)
    - Kompatibel dengan semua Gamemode & Filterscript (ALS Hooked)
    ============================================================================
*/

#include <a_samp>
#include <a_http>

// ================= KONFIGURASI =================
#if !defined BBMUSIC_BASE_STREAM_URL
    #define BBMUSIC_BASE_STREAM_URL "https://bbmusic.biz.id/stream/"
#endif

#if !defined BBMUSIC_API_SYNC_HOST
    #define BBMUSIC_API_SYNC_HOST "bbmusic.biz.id/api/samp/tracks"
#endif

#if !defined BBMUSIC_BOOMBOX_MODEL
    #define BBMUSIC_BOOMBOX_MODEL 2226
#endif

#if !defined BBMUSIC_DEFAULT_DISTANCE
    #define BBMUSIC_DEFAULT_DISTANCE 35.0
#endif

#if !defined MAX_BBMUSIC_WEB_TRACKS
    #define MAX_BBMUSIC_WEB_TRACKS 50
#endif

#if !defined DIALOG_BBMUSIC_BASE
    #define DIALOG_BBMUSIC_BASE 18720
#endif

enum {
    DIALOG_BBMUSIC_MAIN = DIALOG_BBMUSIC_BASE,
    DIALOG_BBMUSIC_WEB_SONGS,
    DIALOG_BBMUSIC_CATEGORIES,
    DIALOG_BBMUSIC_OFFLINE_SONGS,
    DIALOG_BBMUSIC_CUSTOM,
    DIALOG_BBMUSIC_RADIUS
};

// ================= STRUKTUR DATA =================
enum E_BBMUSIC_SONG {
    E_SONG_CATEGORY[24],
    E_SONG_TITLE[48],
    E_SONG_ARTIST[32],
    E_SONG_ID[64]
};

// Koleksi Library Musik Bawaan Offline (Fallback jika server tidak terhubung internet)
new const g_BBMusicOfflineLibrary[][E_BBMUSIC_SONG] = {
    // 1. Pop & Galau Indonesia
    {"Pop Indo", "Bergema Sampai Selamanya", "Nadhif Basalamah", "461800b227"},
    {"Pop Indo", "Komang", "Raim Laode", "komang_samp"},
    {"Pop Indo", "Sial", "Mahalini", "sial_mahalini"},
    {"Pop Indo", "Tak Segampang Itu", "Anggi Marito", "tak_segampang_itu"},
    {"Pop Indo", "Jiwa Yang Bersedih", "Ghea Indrawari", "jiwa_bersedih"},

    // 2. DJ Remix & Jedag-Jedug (Thai Edit)
    {"DJ Remix", "Rasa Ini (Thailand Edit)", "Vierra ft. 0landrys", "9941e16205"},
    {"DJ Remix", "Astaga Bercanda (Thai Edit)", "Akbar Chalay & Mingse", "f47889dec4"},
    {"DJ Remix", "DJ Kuda Yang Mana", "DJ Desa", "dj_kuda_yang_mana"},
    {"DJ Remix", "DJ Santuy Bass Glerr", "DJ Opus", "dj_santuy_bass"},

    // 3. Dangdut & Koplo Jawa
    {"Dangdut Koplo", "Nemen", "Gildcoustic", "nemen_koplo"},
    {"Dangdut Koplo", "Rungkad", "Happy Asmara", "rungkad_koplo"},
    {"Dangdut Koplo", "Dumes", "Woro Widowati", "dumes_koplo"},
    {"Dangdut Koplo", "Sanes", "Guyon Waton ft Denny", "sanes_koplo"},

    // 4. Phonk & Drift Vibes
    {"Phonk & Drift", "Murder In My Mind", "KORDHELL", "murder_in_my_mind"},
    {"Phonk & Drift", "Metamorphosis", "INTERWORLD", "metamorphosis_phonk"},
    {"Phonk & Drift", "Sahara", "Hensonn", "sahara_phonk"},
    {"Phonk & Drift", "Montagem Orquestra", "DJ Tenebroso", "montagem_orquestra"},

    // 5. Pop Global / Hits Barat
    {"Pop Global", "Never Gonna Give You Up", "Rick Astley", "rick_astley"},
    {"Pop Global", "Starboy", "The Weeknd ft. Daft Punk", "starboy_weeknd"},
    {"Pop Global", "Faded", "Alan Walker", "faded_alan_walker"},
    {"Pop Global", "Shape of You", "Ed Sheeran", "shape_of_you"},

    // 6. Chill & Lofi Santai
    {"Chill & Lofi", "Lofi Rain Beats", "Lofi Girl", "lofi_rain_beats"},
    {"Chill & Lofi", "Sunset Drive Vibes", "Chillhop Music", "sunset_drive_lofi"},
    {"Chill & Lofi", "Acoustic Cafe Morning", "Coffee Vibes", "acoustic_cafe"}
};

// Struktur Data Lagu Realtime dari Web
enum E_WEB_TRACK {
    wt_ID[32],
    wt_Title[48],
    wt_Artist[32],
    wt_Duration[12]
};

static g_WebTracks[MAX_BBMUSIC_WEB_TRACKS][E_WEB_TRACK];
static g_WebTrackCount = 0;

// Data Boombox per Player
enum E_BOOMBOX_DATA {
    bool:bb_Active,
    bool:bb_Playing,
    Float:bb_PosX,
    Float:bb_PosY,
    Float:bb_PosZ,
    Float:bb_Distance,
    bb_ObjectID,
    Text3D:bb_LabelID,
    bb_TrackTitle[64],
    bb_StreamUrl[128],
    bb_SelectedCat[24]
};

static g_PlayerBoombox[MAX_PLAYERS][E_BOOMBOX_DATA];
static bool:g_PlayerHearingBoombox[MAX_PLAYERS][MAX_PLAYERS];
static g_BBMusicSyncTimer = -1;

// ================= FORWARD CALLBACKS =================
forward OnBoomboxPlaced(playerid, objectid, Float:x, Float:y, Float:z);
forward OnBoomboxPlay(playerid, const songTitle[], const streamUrl[], Float:distance);
forward OnBoomboxStop(playerid);
forward OnBoomboxPickup(playerid);
forward BBMusic_SyncListeners();
forward OnBBMusicHttpSync(index, response_code, const data[]);

// ================= FUNGSI HELPER URL =================
stock BBMusic_FormatStreamUrl(const input[], output[], maxlen = sizeof(output)) {
    if (strfind(input, "http://", true) == 0 || strfind(input, "https://", true) == 0) {
        format(output, maxlen, "%s", input);
    } else {
        // Jika input berupa Track ID (contoh: 9941e16205)
        format(output, maxlen, "%s%s.mp3", BBMUSIC_BASE_STREAM_URL, input);
    }
}

// ================= PARSER REALTIME DARI WEBSITE =================
stock BBMusic_ParseWebTracks(const data[]) {
    g_WebTrackCount = 0;
    new len = strlen(data);
    if (len == 0) return 0;

    new lineStart = 0;
    new lineEnd = 0;

    while (lineStart < len && g_WebTrackCount < MAX_BBMUSIC_WEB_TRACKS) {
        lineEnd = lineStart;
        while (lineEnd < len && data[lineEnd] != '\n' && data[lineEnd] != '\r') {
            lineEnd++;
        }

        if (lineEnd > lineStart) {
            new line[160];
            new copyLen = lineEnd - lineStart;
            if (copyLen > 159) copyLen = 159;
            strmid(line, data, lineStart, lineStart + copyLen, sizeof(line));

            // Format: id|title|artist|duration
            new p1 = strfind(line, "|", false);
            if (p1 != -1) {
                new p2 = strfind(line, "|", false, p1 + 1);
                if (p2 != -1) {
                    new p3 = strfind(line, "|", false, p2 + 1);

                    strmid(g_WebTracks[g_WebTrackCount][wt_ID], line, 0, p1, 32);
                    strmid(g_WebTracks[g_WebTrackCount][wt_Title], line, p1 + 1, p2, 48);

                    if (p3 != -1) {
                        strmid(g_WebTracks[g_WebTrackCount][wt_Artist], line, p2 + 1, p3, 32);
                        strmid(g_WebTracks[g_WebTrackCount][wt_Duration], line, p3 + 1, copyLen, 12);
                    } else {
                        strmid(g_WebTracks[g_WebTrackCount][wt_Artist], line, p2 + 1, copyLen, 32);
                        format(g_WebTracks[g_WebTrackCount][wt_Duration], 12, "N/A");
                    }
                    g_WebTrackCount++;
                }
            }
        }

        lineStart = lineEnd + 1;
        while (lineStart < len && (data[lineStart] == '\n' || data[lineStart] == '\r')) {
            lineStart++;
        }
    }
    return g_WebTrackCount;
}

// Menjalankan permintaan sinkronisasi HTTP ke website
stock BBMusic_SyncWithWeb(playerid = -1) {
    if (playerid != -1 && IsPlayerConnected(playerid)) {
        SendClientMessage(playerid, 0x00BFFFAA, "[BBMusic] Menghubungkan ke web bbmusic.biz.id untuk mengambil lagu terbaru...");
    }
    HTTP(playerid, HTTP_GET, BBMUSIC_API_SYNC_HOST, "", "OnBBMusicHttpSync");
    return 1;
}

// Callback respon HTTP dari website
public OnBBMusicHttpSync(index, response_code, const data[]) {
    if (response_code == 200) {
        new total = BBMusic_ParseWebTracks(data);
        printf("[BBMusic] Berhasil sinkronisasi %d lagu realtime dari bbmusic.biz.id!", total);
        if (index != -1 && IsPlayerConnected(index)) {
            new msg[128];
            format(msg, sizeof(msg), "{00FF7F}[BBMusic] Berhasil sinkronisasi {FFFF00}%d lagu {00FF7F}terbaru dari website!", total);
            SendClientMessage(index, 0x00FF7FAA, msg);
            BBMusic_ShowWebLibrary(index);
        }
    } else {
        printf("[BBMusic] Gagal sinkronisasi lagu dari web (HTTP Code: %d)", response_code);
        if (index != -1 && IsPlayerConnected(index)) {
            SendClientMessage(index, 0xFF6347AA, "[BBMusic] Gagal menghubungi web bbmusic.biz.id. Membuka library offline...");
            BBMusic_ShowCategories(index);
        }
    }
    return 1;
}

// ================= API UTAMA BOOMBOX =================

// Menaruh Boombox di depan pemain
stock Boombox_Place(playerid) {
    if (!IsPlayerConnected(playerid)) return 0;
    if (g_PlayerBoombox[playerid][bb_Active]) {
        SendClientMessage(playerid, 0xFF6347AA, "[BBMusic] Anda sudah memiliki Boombox yang aktif! Ambil dulu sebelum menaruh baru.");
        return 0;
    }

    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);

    // Hitung posisi 1.1 meter di depan pemain
    x += (1.1 * floatsin(-a, degrees));
    y += (1.1 * floatcos(-a, degrees));

    // Animasi meletakkan benda
    ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.0, 0, 0, 0, 0, 0, 1);

    new objId = CreateObject(BBMUSIC_BOOMBOX_MODEL, x, y, z - 0.75, 0.0, 0.0, a);
    
    new labelText[256], pName[MAX_PLAYER_NAME];
    GetPlayerName(playerid, pName, sizeof(pName));
    format(labelText, sizeof(labelText), 
        "{00BFFF}[ BOOMBOX BBMUSIC ]\n\
        {FFFFFF}Pemilik: {FFD700}%s {FFFFFF}(ID: %d)\n\
        {AAAAAA}Status: {FFA500}Musik Belum Diputar\n\
        {77DD77}Gunakan /boombox untuk memutar lagu", 
        pName, playerid
    );

    new Text3D:lblId = Create3DTextLabel(labelText, 0xFFFFFFFF, x, y, z - 0.3, 25.0, GetPlayerVirtualWorld(playerid), 1);

    g_PlayerBoombox[playerid][bb_Active] = true;
    g_PlayerBoombox[playerid][bb_Playing] = false;
    g_PlayerBoombox[playerid][bb_PosX] = x;
    g_PlayerBoombox[playerid][bb_PosY] = y;
    g_PlayerBoombox[playerid][bb_PosZ] = z;
    g_PlayerBoombox[playerid][bb_Distance] = BBMUSIC_DEFAULT_DISTANCE;
    g_PlayerBoombox[playerid][bb_ObjectID] = objId;
    g_PlayerBoombox[playerid][bb_LabelID] = lblId;
    g_PlayerBoombox[playerid][bb_TrackTitle][0] = '\0';
    g_PlayerBoombox[playerid][bb_StreamUrl][0] = '\0';

    SendClientMessage(playerid, 0x00FF7FAA, "[BBMusic] Boombox berhasil diletakkan di tanah! Ketik {FFFF00}/boombox {00FF7F}untuk memilih lagu.");

    CallLocalFunction("OnBoomboxPlaced", "idfff", playerid, objId, x, y, z);
    return 1;
}

// Memutar musik pada boombox pemain
stock Boombox_Play(playerid, const songIdOrUrl[], const songTitle[], Float:distance = BBMUSIC_DEFAULT_DISTANCE) {
    if (!IsPlayerConnected(playerid) || !g_PlayerBoombox[playerid][bb_Active]) {
        SendClientMessage(playerid, 0xFF6347AA, "[BBMusic] Anda belum meletakkan Boombox! Letakkan terlebih dahulu.");
        return 0;
    }

    new finalUrl[128];
    BBMusic_FormatStreamUrl(songIdOrUrl, finalUrl, sizeof(finalUrl));

    g_PlayerBoombox[playerid][bb_Playing] = true;
    g_PlayerBoombox[playerid][bb_Distance] = (distance > 5.0) ? distance : BBMUSIC_DEFAULT_DISTANCE;
    format(g_PlayerBoombox[playerid][bb_TrackTitle], 64, "%s", songTitle);
    format(g_PlayerBoombox[playerid][bb_StreamUrl], 128, "%s", finalUrl);

    // Update 3D Text Label (Tanpa emoji)
    new labelText[256], pName[MAX_PLAYER_NAME];
    GetPlayerName(playerid, pName, sizeof(pName));
    format(labelText, sizeof(labelText), 
        "{00BFFF}[ BOOMBOX BBMUSIC ]\n\
        {00FF7F}* Lagu: %s\n\
        {FFFFFF}Pemilik: {FFD700}%s\n\
        {AAAAAA}Radius Suara: {00FFFF}%.0fm\n\
        {77DD77}Ketik /boombox untuk kontrol", 
        songTitle, pName, g_PlayerBoombox[playerid][bb_Distance]
    );
    Update3DTextLabelText(g_PlayerBoombox[playerid][bb_LabelID], 0xFFFFFFFF, labelText);

    // Putar audio untuk semua pemain di sekitar boombox
    new Float:bx = g_PlayerBoombox[playerid][bb_PosX];
    new Float:by = g_PlayerBoombox[playerid][bb_PosY];
    new Float:bz = g_PlayerBoombox[playerid][bb_PosZ];
    new Float:dist = g_PlayerBoombox[playerid][bb_Distance];
    new pWorld = GetPlayerVirtualWorld(playerid);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i)) continue;
        if (GetPlayerVirtualWorld(i) != pWorld) continue;

        if (IsPlayerInRangeOfPoint(i, dist, bx, by, bz)) {
            PlayAudioStreamForPlayer(i, finalUrl, bx, by, bz, dist, 1);
            g_PlayerHearingBoombox[i][playerid] = true;
        }
    }

    new msg[144];
    format(msg, sizeof(msg), "[BBMusic] Sedang memutar: {FFFF00}%s {00FF7F}(Radius: %.0fm)", songTitle, dist);
    SendClientMessage(playerid, 0x00FF7FAA, msg);

    CallLocalFunction("OnBoomboxPlay", "issf", playerid, songTitle, finalUrl, dist);
    return 1;
}

// Menghentikan musik pada boombox
stock Boombox_Stop(playerid) {
    if (!IsPlayerConnected(playerid) || !g_PlayerBoombox[playerid][bb_Active]) return 0;
    if (!g_PlayerBoombox[playerid][bb_Playing]) {
        SendClientMessage(playerid, 0xFFA500AA, "[BBMusic] Musik pada Boombox saat ini memang tidak sedang berputar.");
        return 0;
    }

    g_PlayerBoombox[playerid][bb_Playing] = false;
    g_PlayerBoombox[playerid][bb_StreamUrl][0] = '\0';

    // Matikan audio untuk semua pendengar
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (g_PlayerHearingBoombox[i][playerid]) {
            StopAudioStreamForPlayer(i);
            g_PlayerHearingBoombox[i][playerid] = false;
        }
    }

    // Update 3D Text Label
    new labelText[256], pName[MAX_PLAYER_NAME];
    GetPlayerName(playerid, pName, sizeof(pName));
    format(labelText, sizeof(labelText), 
        "{00BFFF}[ BOOMBOX BBMUSIC ]\n\
        {FFFFFF}Pemilik: {FFD700}%s {FFFFFF}(ID: %d)\n\
        {FFA500}* Musik Dihentikan *\n\
        {77DD77}Gunakan /boombox untuk memutar kembali", 
        pName, playerid
    );
    Update3DTextLabelText(g_PlayerBoombox[playerid][bb_LabelID], 0xFFFFFFFF, labelText);

    SendClientMessage(playerid, 0xFFA500AA, "[BBMusic] Musik berhasil dihentikan.");

    CallLocalFunction("OnBoomboxStop", "i", playerid);
    return 1;
}

// Mengambil / menyimpan kembali boombox
stock Boombox_Pickup(playerid) {
    if (!IsPlayerConnected(playerid) || !g_PlayerBoombox[playerid][bb_Active]) {
        SendClientMessage(playerid, 0xFF6347AA, "[BBMusic] Anda tidak memiliki Boombox di tanah.");
        return 0;
    }

    if (g_PlayerBoombox[playerid][bb_Playing]) {
        Boombox_Stop(playerid);
    }

    DestroyObject(g_PlayerBoombox[playerid][bb_ObjectID]);
    Delete3DTextLabel(g_PlayerBoombox[playerid][bb_LabelID]);

    g_PlayerBoombox[playerid][bb_Active] = false;
    g_PlayerBoombox[playerid][bb_Playing] = false;
    g_PlayerBoombox[playerid][bb_ObjectID] = INVALID_OBJECT_ID;

    // Animasi mengambil barang
    ApplyAnimation(playerid, "CARRY", "crry_prtial", 4.0, 0, 0, 0, 0, 0, 1);

    SendClientMessage(playerid, 0x00FF7FAA, "[BBMusic] Boombox berhasil Anda ambil dan disimpan kembali.");

    CallLocalFunction("OnBoomboxPickup", "i", playerid);
    return 1;
}

// Cek apakah pemain memiliki boombox aktif
stock Boombox_IsActive(playerid) {
    if (!IsPlayerConnected(playerid)) return 0;
    return g_PlayerBoombox[playerid][bb_Active];
}

// ================= DYNAMIC LISTENER SYNC TIMER =================
public BBMusic_SyncListeners() {
    for (new owner = 0; owner < MAX_PLAYERS; owner++) {
        if (!g_PlayerBoombox[owner][bb_Active] || !g_PlayerBoombox[owner][bb_Playing]) continue;

        new Float:bx = g_PlayerBoombox[owner][bb_PosX];
        new Float:by = g_PlayerBoombox[owner][bb_PosY];
        new Float:bz = g_PlayerBoombox[owner][bb_PosZ];
        new Float:dist = g_PlayerBoombox[owner][bb_Distance];
        new oWorld = GetPlayerVirtualWorld(owner);

        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (!IsPlayerConnected(i)) continue;
            new bool:inRange = (GetPlayerVirtualWorld(i) == oWorld && IsPlayerInRangeOfPoint(i, dist, bx, by, bz));

            if (inRange && !g_PlayerHearingBoombox[i][owner]) {
                // Pemain baru memasuki radius suara
                PlayAudioStreamForPlayer(i, g_PlayerBoombox[owner][bb_StreamUrl], bx, by, bz, dist, 1);
                g_PlayerHearingBoombox[i][owner] = true;
            } else if (!inRange && g_PlayerHearingBoombox[i][owner]) {
                // Pemain keluar dari radius suara
                StopAudioStreamForPlayer(i);
                g_PlayerHearingBoombox[i][owner] = false;
            }
        }
    }
    return 1;
}

// ================= DIALOG GUI INTERAKTIF (BEBAS EMOJI) =================

// Menampilkan dialog utama /boombox
stock Boombox_ShowMenu(playerid) {
    if (!IsPlayerConnected(playerid)) return 0;

    new mainDialog[512];
    if (!g_PlayerBoombox[playerid][bb_Active]) {
        format(mainDialog, sizeof(mainDialog), 
            "{00FF7F}1. [PASANG] Letakkan Boombox di Tanah\n\
            {AAAAAA}2. [WEB] Lagu Terbaru dari Website (%d Lagu Tersinkron)\n\
            {AAAAAA}3. [OFFLINE] Library Lagu Bawaan Server\n\
            {AAAAAA}4. [CUSTOM] Input Track ID / Link Sendiri\n\
            {00BFFF}5. [SYNC] Sinkronisasi Ulang dari Web bbmusic.biz.id\n\
            {AAAAAA}6. [TUTUP] Tutup Menu",
            g_WebTrackCount
        );
        ShowPlayerDialog(playerid, DIALOG_BBMUSIC_MAIN, DIALOG_STYLE_LIST, "{00BFFF}BBMusic {FFFFFF}- Menu Boombox", mainDialog, "Pilih", "Batal");
    } else {
        new statusStr[64];
        if (g_PlayerBoombox[playerid][bb_Playing]) {
            format(statusStr, sizeof(statusStr), "{00FF7F}Memutar: %s", g_PlayerBoombox[playerid][bb_TrackTitle]);
        } else {
            format(statusStr, sizeof(statusStr), "{FFA500}Berhenti");
        }

        format(mainDialog, sizeof(mainDialog), 
            "{FF6347}1. [AMBIL] Ambil Kembali Boombox (Pickup)\n\
            {00FF7F}2. [WEB] Lagu Terbaru dari Website ({FFFF00}%d Lagu{00FF7F})\n\
            {00BFFF}3. [OFFLINE] Library Lagu Bawaan Server\n\
            {00FFFF}4. [CUSTOM] Input Track ID / Link Custom\n\
            {FFD700}5. [SYNC] Sinkronisasi / Tarik Lagu Terbaru dari Web\n\
            {FFFF00}6. [RADIUS] Atur Jarak Suara (Saat ini: %.0fm)\n\
            {FF4500}7. [STOP] Hentikan Musik (%s)\n\
            {FFFFFF}8. [TUTUP] Tutup Menu",
            g_WebTrackCount, g_PlayerBoombox[playerid][bb_Distance], statusStr
        );
        ShowPlayerDialog(playerid, DIALOG_BBMUSIC_MAIN, DIALOG_STYLE_LIST, "{00BFFF}BBMusic {FFFFFF}- Menu Kontrol Boombox", mainDialog, "Pilih", "Batal");
    }
    return 1;
}

// Dialog daftar lagu hasil sync dari website (Realtime)
stock BBMusic_ShowWebLibrary(playerid) {
    if (g_WebTrackCount == 0) {
        SendClientMessage(playerid, 0xFFA500AA, "[BBMusic] Belum ada lagu yang tersinkronisasi. Menghubungkan ke web...");
        BBMusic_SyncWithWeb(playerid);
        return 1;
    }

    new songList[2048];
    for (new i = 0; i < g_WebTrackCount; i++) {
        if (i > 0) strcat(songList, "\n");
        new line[128];
        format(line, sizeof(line), "{FFFFFF}%d. {00FF7F}%s {AAAAAA}- %s {00FFFF}[%s]", i + 1, g_WebTracks[i][wt_Title], g_WebTracks[i][wt_Artist], g_WebTracks[i][wt_Duration]);
        strcat(songList, line);
    }

    new header[64];
    format(header, sizeof(header), "{00BFFF}BBMusic Web: {FFFFFF}%d Lagu Tersedia", g_WebTrackCount);
    ShowPlayerDialog(playerid, DIALOG_BBMUSIC_WEB_SONGS, DIALOG_STYLE_LIST, header, songList, "Putar", "Kembali");
    return 1;
}

// Dialog kategori offline library
stock BBMusic_ShowCategories(playerid) {
    new catList[512];
    format(catList, sizeof(catList), 
        "1. [POP] Pop & Galau Indonesia\n\
        2. [REMIX] DJ Remix & Thai Edit (Jedag-Jedug)\n\
        3. [KOPLO] Dangdut & Koplo Jawa\n\
        4. [PHONK] Phonk & Drift Vibes\n\
        5. [WESTERN] Pop Global / Hits Barat\n\
        6. [CHILL] Chill, Lofi & Acoustic"
    );
    ShowPlayerDialog(playerid, DIALOG_BBMUSIC_CATEGORIES, DIALOG_STYLE_LIST, "{00BFFF}BBMusic {FFFFFF}- Kategori Offline", catList, "Buka", "Kembali");
    return 1;
}

// Dialog daftar lagu offline per kategori
stock BBMusic_ShowOfflineSongs(playerid, const category[]) {
    format(g_PlayerBoombox[playerid][bb_SelectedCat], 24, "%s", category);
    new songList[1024];
    new count = 0;

    for (new i = 0; i < sizeof(g_BBMusicOfflineLibrary); i++) {
        if (strcmp(g_BBMusicOfflineLibrary[i][E_SONG_CATEGORY], category, true) == 0) {
            if (count > 0) strcat(songList, "\n");
            new line[128];
            format(line, sizeof(line), "{FFFFFF}%s {AAAAAA}- %s", g_BBMusicOfflineLibrary[i][E_SONG_TITLE], g_BBMusicOfflineLibrary[i][E_SONG_ARTIST]);
            strcat(songList, line);
            count++;
        }
    }

    if (count == 0) {
        SendClientMessage(playerid, 0xFFA500AA, "[BBMusic] Belum ada lagu pada kategori ini.");
        return BBMusic_ShowCategories(playerid);
    }

    new header[64];
    format(header, sizeof(header), "{00BFFF}Offline: {FFFFFF}%s", category);
    ShowPlayerDialog(playerid, DIALOG_BBMUSIC_OFFLINE_SONGS, DIALOG_STYLE_LIST, header, songList, "Putar", "Kembali");
    return 1;
}

// ================= ALS CALLBACK HOOKS =================

// Hook Init
public OnGameModeInit() {
    if (g_BBMusicSyncTimer == -1) {
        g_BBMusicSyncTimer = SetTimer("BBMusic_SyncListeners", 2000, true);
    }
    // Auto-sync awal dengan web saat server pertama kali start
    BBMusic_SyncWithWeb(-1);

    #if defined BBMusic_OnGameModeInit
        return BBMusic_OnGameModeInit();
    #else
        return 1;
    #endif
}
#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif
#define OnGameModeInit BBMusic_OnGameModeInit
#if defined BBMusic_OnGameModeInit
    forward BBMusic_OnGameModeInit();
#endif

// Hook Disconnect (Auto Pickup)
public OnPlayerDisconnect(playerid, reason) {
    if (g_PlayerBoombox[playerid][bb_Active]) {
        Boombox_Pickup(playerid);
    }
    for (new owner = 0; owner < MAX_PLAYERS; owner++) {
        g_PlayerHearingBoombox[playerid][owner] = false;
    }

    #if defined BBMusic_OnPlayerDisconnect
        return BBMusic_OnPlayerDisconnect(playerid, reason);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerDisconnect
    #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect BBMusic_OnPlayerDisconnect
#if defined BBMusic_OnPlayerDisconnect
    forward BBMusic_OnPlayerDisconnect(playerid, reason);
#endif

// Hook Dialog Response
public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]) {
    switch (dialogid) {
        case DIALOG_BBMUSIC_MAIN: {
            if (!response) return 1;

            if (!g_PlayerBoombox[playerid][bb_Active]) {
                switch (listitem) {
                    case 0: Boombox_Place(playerid);
                    case 4: BBMusic_SyncWithWeb(playerid);
                    case 5: return 1;
                    default: {
                        SendClientMessage(playerid, 0xFFA500AA, "[BBMusic] Letakkan boombox di tanah terlebih dahulu (Menu 1).");
                        Boombox_ShowMenu(playerid);
                    }
                }
            } else {
                switch (listitem) {
                    case 0: Boombox_Pickup(playerid);
                    case 1: BBMusic_ShowWebLibrary(playerid);
                    case 2: BBMusic_ShowCategories(playerid);
                    case 3: ShowPlayerDialog(playerid, DIALOG_BBMUSIC_CUSTOM, DIALOG_STYLE_INPUT, "{00BFFF}BBMusic {FFFFFF}- Input ID / Link Musik", "{FFFFFF}Masukkan {00FF7F}Track ID {FFFFFF}dari web (contoh: {FFFF00}9941e16205{FFFFFF})\natau URL direct stream MP3:\n\n{AAAAAA}Tips: Dapatkan ID lagu gratis di https://bbmusic.biz.id", "Putar", "Kembali");
                    case 4: BBMusic_SyncWithWeb(playerid);
                    case 5: ShowPlayerDialog(playerid, DIALOG_BBMUSIC_RADIUS, DIALOG_STYLE_INPUT, "{00BFFF}BBMusic {FFFFFF}- Atur Radius Suara", "{FFFFFF}Masukkan radius jangkauan suara (meter):\n{AAAAAA}Nilai yang diperbolehkan: 10 - 75 meter\nDefault: 35 meter", "Simpan", "Kembali");
                    case 6: Boombox_Stop(playerid);
                    case 7: return 1;
                }
            }
            return 1;
        }

        case DIALOG_BBMUSIC_WEB_SONGS: {
            if (!response) return Boombox_ShowMenu(playerid);
            if (listitem >= 0 && listitem < g_WebTrackCount) {
                new fullTitle[64];
                format(fullTitle, sizeof(fullTitle), "%s - %s", g_WebTracks[listitem][wt_Artist], g_WebTracks[listitem][wt_Title]);
                Boombox_Play(playerid, g_WebTracks[listitem][wt_ID], fullTitle, g_PlayerBoombox[playerid][bb_Distance]);
            }
            return 1;
        }

        case DIALOG_BBMUSIC_CATEGORIES: {
            if (!response) return Boombox_ShowMenu(playerid);

            switch (listitem) {
                case 0: BBMusic_ShowOfflineSongs(playerid, "Pop Indo");
                case 1: BBMusic_ShowOfflineSongs(playerid, "DJ Remix");
                case 2: BBMusic_ShowOfflineSongs(playerid, "Dangdut Koplo");
                case 3: BBMusic_ShowOfflineSongs(playerid, "Phonk & Drift");
                case 4: BBMusic_ShowOfflineSongs(playerid, "Pop Global");
                case 5: BBMusic_ShowOfflineSongs(playerid, "Chill & Lofi");
            }
            return 1;
        }

        case DIALOG_BBMUSIC_OFFLINE_SONGS: {
            if (!response) return BBMusic_ShowCategories(playerid);

            new selectedIndex = -1;
            new matchCounter = 0;

            for (new i = 0; i < sizeof(g_BBMusicOfflineLibrary); i++) {
                if (strcmp(g_BBMusicOfflineLibrary[i][E_SONG_CATEGORY], g_PlayerBoombox[playerid][bb_SelectedCat], true) == 0) {
                    if (matchCounter == listitem) {
                        selectedIndex = i;
                        break;
                    }
                    matchCounter++;
                }
            }

            if (selectedIndex != -1) {
                new fullTitle[64];
                format(fullTitle, sizeof(fullTitle), "%s - %s", g_BBMusicOfflineLibrary[selectedIndex][E_SONG_ARTIST], g_BBMusicOfflineLibrary[selectedIndex][E_SONG_TITLE]);
                Boombox_Play(playerid, g_BBMusicOfflineLibrary[selectedIndex][E_SONG_ID], fullTitle, g_PlayerBoombox[playerid][bb_Distance]);
            }
            return 1;
        }

        case DIALOG_BBMUSIC_CUSTOM: {
            if (!response) return Boombox_ShowMenu(playerid);

            if (strlen(inputtext) < 2) {
                SendClientMessage(playerid, 0xFF6347AA, "[BBMusic] ID lagu atau URL tidak boleh kosong!");
                return Boombox_ShowMenu(playerid);
            }

            new title[48];
            format(title, sizeof(title), "Custom Track (%s)", inputtext);
            Boombox_Play(playerid, inputtext, title, g_PlayerBoombox[playerid][bb_Distance]);
            return 1;
        }

        case DIALOG_BBMUSIC_RADIUS: {
            if (!response) return Boombox_ShowMenu(playerid);

            new Float:rad = float(strval(inputtext));
            if (rad < 10.0 || rad > 75.0) {
                SendClientMessage(playerid, 0xFF6347AA, "[BBMusic] Radius suara harus antara 10 sampai 75 meter!");
                return Boombox_ShowMenu(playerid);
            }

            g_PlayerBoombox[playerid][bb_Distance] = rad;
            if (g_PlayerBoombox[playerid][bb_Playing]) {
                Boombox_Play(playerid, g_PlayerBoombox[playerid][bb_StreamUrl], g_PlayerBoombox[playerid][bb_TrackTitle], rad);
            } else {
                new msg[128];
                format(msg, sizeof(msg), "[BBMusic] Radius suara Boombox diubah menjadi {00FFFF}%.0f meter{00FF7F}.", rad);
                SendClientMessage(playerid, 0x00FF7FAA, msg);
            }
            return 1;
        }
    }

    #if defined BBMusic_OnDialogResponse
        return BBMusic_OnDialogResponse(playerid, dialogid, response, listitem, inputtext);
    #else
        return 0;
    #endif
}
#if defined _ALS_OnDialogResponse
    #undef OnDialogResponse
#else
    #define _ALS_OnDialogResponse
#endif
#define OnDialogResponse BBMusic_OnDialogResponse
#if defined BBMusic_OnDialogResponse
    forward BBMusic_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]);
#endif
