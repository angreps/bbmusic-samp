#define FILTERSCRIPT

#include <a_samp>
#include <zcmd>
#include <bbmusic>

public OnFilterScriptInit() {
    print("\n========================================================");
    print("       BBMusic 3D Boombox & Audio Stream System         ");
    print("       Powered by bbmusic.biz.id | Status: ACTIVE       ");
    print("========================================================\n");
    return 1;
}

public OnFilterScriptExit() {
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && Boombox_IsActive(i)) {
            Boombox_Pickup(i);
        }
    }
    print("BBMusic Boombox Filterscript Unloaded.");
    return 1;
}

// Perintah menggunakan ZCMD include
CMD:boombox(playerid, params[]) {
    Boombox_ShowMenu(playerid);
    return 1;
}

CMD:bbmusic(playerid, params[]) {
    Boombox_ShowMenu(playerid);
    return 1;
}

CMD:music(playerid, params[]) {
    Boombox_ShowMenu(playerid);
    return 1;
}
