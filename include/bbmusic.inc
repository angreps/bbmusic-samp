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
    - Auto-Download Offline Library: Jika file scriptfiles/bbmusic_offline.txt
      tidak ada di hosting, server otomatis mendownload 10 lagu dari web
      bbmusic.biz.id dan menyimpannya di hosting sebagai library offline.
    - Realtime Web Sync: Sinkronisasi otomatis dengan lagu terbaru dari web.
    - 3D Spatial Audio Stream untuk Boombox (Objek 2226).
    - Dynamic Sync: Pemain yang mendekat otomatis mendengar musik realtime.
    - Bebas Emoji: 100% kompatibel dengan font font SA-MP DirectX (Codepage 1252).
    - Input Custom ID / Link Langsung dari bbmusic.biz.id.
    - Dialog GUI Interaktif (/boombox).
    - Kompatibel dengan semua Gamemode & Filterscript (ALS Hooked).
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

#if !defined BBMUSIC_API_OFFLINE_HOST
    #define BBMUSIC_API_OFFLINE_HOST "bbmusic.biz.id/api/samp/tracks?limit=10"
#endif

#if !defined BBMUSIC_OFFLINE_FILE
    #define BBMUSIC_OFFLINE_FILE "bbmusic_offline.txt"
#endif

#if !defined MAX_BBMUSIC_OFFLINE_TRACKS
    #define MAX_BBMUSIC_OFFLINE_TRACKS 10
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
    DIALOG_BBMUSIC_OFFLINE_SONGS,
    DIALOG_BBMUSIC_CUSTOM,
    DIALOG_BBMUSIC_RADIUS
};

// ================= STRUKTUR DATA =================
enum E_BBMUSIC_TRACK {
    bm_ID[32],
    bm_Title[48],
    bm_Artist[32],
    bm_Duration[12]
};

// Struktur Data Lagu Realtime dari Web
static g_WebTracks[MAX_BBMUSIC_WEB_TRACKS][E_BBMUSIC_TRACK];
static g_WebTrackCount = 0;

// Struktur Data Lagu Offline yang Disimpan di Hosting (scriptfiles/bbmusic_offline.txt)
static g_OfflineTracks[MAX_BBMUSIC_OFFLINE_TRACKS][E_BBMUSIC_TRACK];
static g_OfflineTrackCount = 0;

// 10 Lagu Default berkualitas jika web offline saat download perdana
new const g_DefaultOfflineTracks[10][E_BBMUSIC_TRACK] = {
    {"461800b227", "Bergema Sampai Selamanya", "Nadhif Basalamah", "3:45"},
    {"9941e16205", "Rasa Ini (Thai Edit)", "Vierra ft 0landrys", "3:12"},
    {"f47889dec4", "Astaga Bercanda (Thai Edit)", "Akbar Chalay & Mingse", "2:55"},
    {"50941873dd", "Gone, Gone, Gone", "Phillip Phillips", "3:28"},
    {"3cde51f0e9", "What If I Call", "Alex Crichton", "2:40"},
    {"rick_astley", "Never Gonna Give You Up", "Rick Astley", "3:32"},
    {"starboy_weeknd", "Starboy", "The Weeknd ft Daft Punk", "3:50"},
    {"nemen_koplo", "Nemen", "Gildcoustic", "4:10"},
    {"rungkad_koplo", "Rungkad", "Happy Asmara", "3:58"},
    {"lofi_rain_beats", "Lofi Rain Beats", "Lofi Girl", "2:30"}
};

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
    bb_StreamUrl[128]
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
forward OnBBMusicDownloadOffline(index, response_code, const data[]);

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

                    strmid(g_WebTracks[g_WebTrackCount][bm_ID], line, 0, p1, 32);
                    strmid(g_WebTracks[g_WebTrackCount][bm_Title], line, p1 + 1, p2, 48);

                    if (p3 != -1) {
                        strmid(g_WebTracks[g_WebTrackCount][bm_Artist], line, p2 + 1, p3, 32);
                        strmid(g_WebTracks[g_WebTrackCount][bm_Duration], line, p3 + 1, copyLen, 12);
                    } else {
                        strmid(g_WebTracks[g_WebTrackCount][bm_Artist], line, p2 + 1, copyLen, 32);
                        format(g_WebTracks[g_WebTrackCount][bm_Duration], 12, "N/A");
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
            SendClientMessage(index, 0xFF6347AA, "[BBMusic] Gagal menghubungi web bbmusic.biz.id. Membuka library offline hosting...");
            BBMusic_ShowOfflineLibrary(index);
        }
    }
    return 1;
}

// ================= SISTEM LIBRARY OFFLINE HOSTING =================

// Menyimpan data string lagu ke file scriptfiles/bbmusic_offline.txt
stock BBMusic_SaveOfflineTracks(const data[]) {
    new File:handle = fopen(BBMUSIC_OFFLINE_FILE, io_write);
    if (!handle) {
        printf("[BBMusic Error] Gagal membuat file scriptfiles/%s! Pastikan folder scriptfiles ada.", BBMUSIC_OFFLINE_FILE);
        return 0;
    }

    new len = strlen(data);
    new lineStart = 0;
    new lineEnd = 0;
    new count = 0;

    while (lineStart < len && count < MAX_BBMUSIC_OFFLINE_TRACKS) {
        lineEnd = lineStart;
        while (lineEnd < len && data[lineEnd] != '\n' && data[lineEnd] != '\r') {
            lineEnd++;
        }

        if (lineEnd > lineStart) {
            new line[160];
            new copyLen = lineEnd - lineStart;
            if (copyLen > 150) copyLen = 150;
            strmid(line, data, lineStart, lineStart + copyLen, sizeof(line));

            // Pastikan baris memiliki pemisah pipa |
            if (strfind(line, "|", false) != -1) {
                format(line, sizeof(line), "%s\r\n", line);
                fwrite(handle, line);
                count++;
            }
        }

        lineStart = lineEnd + 1;
        while (lineStart < len && (data[lineStart] == '\n' || data[lineStart] == '\r')) {
            lineStart++;
        }
    }

    // Jika lagu dari web kurang dari 10, lengkapi dari default agar pas 10 lagu
    if (count < MAX_BBMUSIC_OFFLINE_TRACKS) {
        for (new i = 0; i < sizeof(g_DefaultOfflineTracks) && count < MAX_BBMUSIC_OFFLINE_TRACKS; i++) {
            new line[160];
            format(line, sizeof(line), "%s|%s|%s|%s\r\n",
                g_DefaultOfflineTracks[i][bm_ID],
                g_DefaultOfflineTracks[i][bm_Title],
                g_DefaultOfflineTracks[i][bm_Artist],
                g_DefaultOfflineTracks[i][bm_Duration]
            );
            fwrite(handle, line);
            count++;
        }
    }

    fclose(handle);
    return count;
}

// Menulis 10 lagu default ke hosting jika server offline saat download awal
stock BBMusic_WriteDefaultOffline() {
    new File:handle = fopen(BBMUSIC_OFFLINE_FILE, io_write);
    if (!handle) {
        printf("[BBMusic Error] Gagal menulis file default scriptfiles/%s!", BBMUSIC_OFFLINE_FILE);
        return 0;
    }

    for (new i = 0; i < sizeof(g_DefaultOfflineTracks); i++) {
        new line[160];
        format(line, sizeof(line), "%s|%s|%s|%s\r\n",
            g_DefaultOfflineTracks[i][bm_ID],
            g_DefaultOfflineTracks[i][bm_Title],
            g_DefaultOfflineTracks[i][bm_Artist],
            g_DefaultOfflineTracks[i][bm_Duration]
        );
        fwrite(handle, line);
    }

    fclose(handle);
    return sizeof(g_DefaultOfflineTracks);
}

// Membaca lagu dari scriptfiles/bbmusic_offline.txt ke memori
stock BBMusic_LoadOfflineTracks() {
    g_OfflineTrackCount = 0;
    if (!fexist(BBMUSIC_OFFLINE_FILE)) {
        return 0;
    }

    new File:handle = fopen(BBMUSIC_OFFLINE_FILE, io_read);
    if (!handle) return 0;

    new line[160];
    while (g_OfflineTrackCount < MAX_BBMUSIC_OFFLINE_TRACKS && fread(handle, line)) {
        new lineLen = strlen(line);
        while (lineLen > 0 && (line[lineLen - 1] == '\r' || line[lineLen - 1] == '\n' || line[lineLen - 1] == ' ')) {
            line[--lineLen] = '\0';
        }
        if (lineLen < 3) continue;

        new p1 = strfind(line, "|", false);
        if (p1 != -1) {
            new p2 = strfind(line, "|", false, p1 + 1);
            if (p2 != -1) {
                new p3 = strfind(line, "|", false, p2 + 1);

                strmid(g_OfflineTracks[g_OfflineTrackCount][bm_ID], line, 0, p1, 32);
                strmid(g_OfflineTracks[g_OfflineTrackCount][bm_Title], line, p1 + 1, p2, 48);

                if (p3 != -1) {
                    strmid(g_OfflineTracks[g_OfflineTrackCount][bm_Artist], line, p2 + 1, p3, 32);
                    strmid(g_OfflineTracks[g_OfflineTrackCount][bm_Duration], line, p3 + 1, lineLen, 12);
                } else {
                    strmid(g_OfflineTracks[g_OfflineTrackCount][bm_Artist], line, p2 + 1, lineLen, 32);
                    format(g_OfflineTracks[g_OfflineTrackCount][bm_Duration], 12, "N/A");
                }
                g_OfflineTrackCount++;
            }
        }
    }

    fclose(handle);
    return g_OfflineTrackCount;
}

// Inisialisasi: Cek file di hosting. Jika tidak ada -> Otomatis download 10 lagu dari web
stock BBMusic_InitOfflineLibrary(playerid = -1) {
    if (fexist(BBMUSIC_OFFLINE_FILE)) {
        new count = BBMusic_LoadOfflineTracks();
        printf("[BBMusic] File %s ditemukan di hosting. Memuat %d lagu library offline.", BBMUSIC_OFFLINE_FILE, count);
        if (playerid != -1 && IsPlayerConnected(playerid)) {
            new msg[128];
            format(msg, sizeof(msg), "{00FF7F}[BBMusic] Berhasil memuat {FFFF00}%d lagu offline {00FF7F}dari hosting!", count);
            SendClientMessage(playerid, 0x00FF7FAA, msg);
        }
        return count;
    }

    // File tidak ditemukan di hosting -> Download otomatis 10 lagu dari web bbmusic.biz.id
    printf("[BBMusic] File %s belum ada di hosting! Otomatis mendownload 10 lagu dari web bbmusic.biz.id...", BBMUSIC_OFFLINE_FILE);
    if (playerid != -1 && IsPlayerConnected(playerid)) {
        SendClientMessage(playerid, 0x00BFFFAA, "[BBMusic] File library offline belum ada di hosting. Mengunduh 10 lagu dari web...");
    }

    HTTP(playerid, HTTP_GET, BBMUSIC_API_OFFLINE_HOST, "", "OnBBMusicDownloadOffline");
    return 1;
}

// Download ulang / paksa perbarui 10 lagu offline dari web
stock BBMusic_DownloadOffline(playerid = -1) {
    printf("[BBMusic] Mengunduh ulang 10 lagu dari bbmusic.biz.id ke hosting (scriptfiles/%s)...", BBMUSIC_OFFLINE_FILE);
    if (playerid != -1 && IsPlayerConnected(playerid)) {
        SendClientMessage(playerid, 0x00BFFFAA, "[BBMusic] Mengunduh 10 lagu offline dari web untuk disimpan di hosting...");
    }
    HTTP(playerid, HTTP_GET, BBMUSIC_API_OFFLINE_HOST, "", "OnBBMusicDownloadOffline");
    return 1;
}

// Callback respon HTTP saat mengunduh 10 lagu offline
public OnBBMusicDownloadOffline(index, response_code, const data[]) {
    if (response_code == 200 && strlen(data) > 5) {
        new count = BBMusic_SaveOfflineTracks(data);
        BBMusic_LoadOfflineTracks();
        printf("[BBMusic] SUKSES! %d lagu dari web berhasil disimpan di hosting (scriptfiles/%s) sebagai library offline.", count, BBMUSIC_OFFLINE_FILE);
        if (index != -1 && IsPlayerConnected(index)) {
            new msg[144];
            format(msg, sizeof(msg), "{00FF7F}[BBMusic] Sukses mendownload & menyimpan {FFFF00}%d lagu {00FF7F}ke hosting untuk library offline!", count);
            SendClientMessage(index, 0x00FF7FAA, msg);
            BBMusic_ShowOfflineLibrary(index);
        }
    } else {
        printf("[BBMusic] Gagal mengunduh dari web (HTTP %d). Menulis 10 lagu default ke hosting...", response_code);
        new count = BBMusic_WriteDefaultOffline();
        BBMusic_LoadOfflineTracks();
        printf("[BBMusic] %d lagu default berhasil disimpan di hosting (scriptfiles/%s).", count, BBMUSIC_OFFLINE_FILE);
        if (index != -1 && IsPlayerConnected(index)) {
            SendClientMessage(index, 0xFFA500AA, "[BBMusic] Gagal download dari web, menggunakan 10 lagu offline default di hosting.");
            BBMusic_ShowOfflineLibrary(index);
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
            {AAAAAA}3. [OFFLINE] Library Lagu Offline Hosting (%d Lagu Tersimpan)\n\
            {AAAAAA}4. [CUSTOM] Input Track ID / Link Sendiri\n\
            {00BFFF}5. [SYNC] Sinkronisasi Ulang dari Web bbmusic.biz.id\n\
            {AAAAAA}6. [TUTUP] Tutup Menu",
            g_WebTrackCount, g_OfflineTrackCount
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
            {00BFFF}3. [OFFLINE] Library Lagu Offline Hosting ({FFFF00}%d Lagu{00BFFF})\n\
            {00FFFF}4. [CUSTOM] Input Track ID / Link Custom\n\
            {FFD700}5. [SYNC] Sinkronisasi / Tarik Lagu Terbaru dari Web\n\
            {FFFF00}6. [RADIUS] Atur Jarak Suara (Saat ini: %.0fm)\n\
            {FF4500}7. [STOP] Hentikan Musik (%s)\n\
            {FFFFFF}8. [TUTUP] Tutup Menu",
            g_WebTrackCount, g_OfflineTrackCount, g_PlayerBoombox[playerid][bb_Distance], statusStr
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
        format(line, sizeof(line), "{FFFFFF}%d. {00FF7F}%s {AAAAAA}- %s {00FFFF}[%s]", i + 1, g_WebTracks[i][bm_Title], g_WebTracks[i][bm_Artist], g_WebTracks[i][bm_Duration]);
        strcat(songList, line);
    }

    new header[64];
    format(header, sizeof(header), "{00BFFF}BBMusic Web: {FFFFFF}%d Lagu Tersedia", g_WebTrackCount);
    ShowPlayerDialog(playerid, DIALOG_BBMUSIC_WEB_SONGS, DIALOG_STYLE_LIST, header, songList, "Putar", "Kembali");
    return 1;
}

// Dialog daftar lagu offline yang tersimpan di hosting (scriptfiles/bbmusic_offline.txt)
stock BBMusic_ShowOfflineLibrary(playerid) {
    if (!IsPlayerConnected(playerid)) return 0;

    if (g_OfflineTrackCount == 0) {
        if (!fexist(BBMUSIC_OFFLINE_FILE)) {
            BBMusic_InitOfflineLibrary(playerid);
            return 1;
        } else {
            BBMusic_LoadOfflineTracks();
        }
    }

    new songList[2048];
    for (new i = 0; i < g_OfflineTrackCount; i++) {
        if (i > 0) strcat(songList, "\n");
        new line[128];
        format(line, sizeof(line), "{FFFFFF}%d. {00FF7F}%s {AAAAAA}- %s {00FFFF}[%s]", 
            i + 1, g_OfflineTracks[i][bm_Title], g_OfflineTracks[i][bm_Artist], g_OfflineTracks[i][bm_Duration]
        );
        strcat(songList, line);
    }

    new header[64];
    format(header, sizeof(header), "{00BFFF}BBMusic Offline: {FFFFFF}%d Lagu di Hosting", g_OfflineTrackCount);
    ShowPlayerDialog(playerid, DIALOG_BBMUSIC_OFFLINE_SONGS, DIALOG_STYLE_LIST, header, songList, "Putar", "Kembali");
    return 1;
}

// ================= ALS CALLBACK HOOKS =================

// Hook Init untuk Filterscript & Gamemode
#if defined FILTERSCRIPT
public OnFilterScriptInit() {
    if (g_BBMusicSyncTimer == -1) {
        g_BBMusicSyncTimer = SetTimer("BBMusic_SyncListeners", 2000, true);
    }
    // Cek file offline di hosting, jika belum ada otomatis download 10 lagu dari web
    BBMusic_InitOfflineLibrary();
    // Auto-sync awal dengan web
    BBMusic_SyncWithWeb(-1);

    #if defined BBMusic_OnFilterScriptInit
        return BBMusic_OnFilterScriptInit();
    #else
        return 1;
    #endif
}
#if defined _ALS_OnFilterScriptInit
    #undef OnFilterScriptInit
#else
    #define _ALS_OnFilterScriptInit
#endif
#define OnFilterScriptInit BBMusic_OnFilterScriptInit
#if defined BBMusic_OnFilterScriptInit
    forward BBMusic_OnFilterScriptInit();
#endif
#else
public OnGameModeInit() {
    if (g_BBMusicSyncTimer == -1) {
        g_BBMusicSyncTimer = SetTimer("BBMusic_SyncListeners", 2000, true);
    }
    // Cek file offline di hosting, jika belum ada otomatis download 10 lagu dari web
    BBMusic_InitOfflineLibrary();
    // Auto-sync awal dengan web
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
                    case 2: BBMusic_ShowOfflineLibrary(playerid);
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
                format(fullTitle, sizeof(fullTitle), "%s - %s", g_WebTracks[listitem][bm_Artist], g_WebTracks[listitem][bm_Title]);
                Boombox_Play(playerid, g_WebTracks[listitem][bm_ID], fullTitle, g_PlayerBoombox[playerid][bb_Distance]);
            }
            return 1;
        }

        case DIALOG_BBMUSIC_OFFLINE_SONGS: {
            if (!response) return Boombox_ShowMenu(playerid);
            if (listitem >= 0 && listitem < g_OfflineTrackCount) {
                new fullTitle[64];
                format(fullTitle, sizeof(fullTitle), "%s - %s", g_OfflineTracks[listitem][bm_Artist], g_OfflineTracks[listitem][bm_Title]);
                Boombox_Play(playerid, g_OfflineTracks[listitem][bm_ID], fullTitle, g_PlayerBoombox[playerid][bb_Distance]);
                new msg[128];
                format(msg, sizeof(msg), "{00FF7F}[BBMusic] Memutar lagu dari offline library hosting: {FFFF00}%s", fullTitle);
                SendClientMessage(playerid, 0x00FF7FAA, msg);
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
