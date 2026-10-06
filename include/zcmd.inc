#if defined _zcmd_included
    #endinput
#endif
#define _zcmd_included

/*
 *  ZCMD - Fast Command Processor by Zeex
 */

#define CMD:%1(%2) \
    forward cmd_%1(%2); \
    public cmd_%1(%2)

#define COMMAND:%1(%2) \
    CMD:%1(%2)

#define CMD_SUCCESS 1
#define CMD_FAILURE 0

public OnPlayerCommandText(playerid, cmdtext[])
{
    static
        funcname[32] = "cmd_",
        pos;

    if ((pos = strfind(cmdtext, " ", true, 1)) != -1)
    {
        funcname[4] = '\0';
        strcat(funcname, cmdtext[1], pos + 4);
        while (cmdtext[pos] == ' ') pos++;

        if (cmdtext[pos])
        {
            return CallLocalFunction(funcname, "is", playerid, cmdtext[pos]);
        }
        return CallLocalFunction(funcname, "is", playerid, "\1");
    }
    funcname[4] = '\0';
    strcat(funcname, cmdtext[1], sizeof (funcname));
    return CallLocalFunction(funcname, "is", playerid, "\1");
}
