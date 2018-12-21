#if USE_RETROACHIEVEMENTS
#include "retroachievements.h"
#include "RA_Interface.h"

extern "C"
{
    #include "device.h"
    #include "quasi88.h"
}

int GetMenuItemIndex(HMENU hMenu, const char* ItemName)
{
    int index = 0;
    char buf[256];

    while (index < GetMenuItemCount(hMenu))
    {
        if (GetMenuString(hMenu, index, buf, sizeof(buf) - 1, MF_BYPOSITION))
        {
            if (!strcmp(ItemName, buf))
                return index;
        }
        index++;
    }

    // not found
    return -1;
}

bool GameIsActive()
{
    return true;
}

void CauseUnpause()
{
    quasi88_exec();
}

void CausePause()
{
    quasi88_pause();
}

void RebuildMenu()
{
    // get main menu handle
    HMENU hMainMenu = g_hMenu;
    if (!hMainMenu) return;

    // get file menu index
    int index = GetMenuItemIndex(hMainMenu, "&RetroAchievements");
    if (index >= 0)
        DeleteMenu(hMainMenu, index, MF_BYPOSITION);

    index = GetMenuItemIndex(hMainMenu, "&Debug");
    if (index >= 0)
        DeleteMenu(hMainMenu, index, MF_BYPOSITION);

    // embed RA
    AppendMenu(hMainMenu, MF_POPUP | MF_STRING, (UINT_PTR)RA_CreatePopupMenu(), TEXT("&RetroAchievements"));

    DrawMenuBar(g_hWnd);
}

void GetEstimatedGameTitle(char* sNameOut)
{
}

void ResetEmulation()
{
    quasi88_reset(NULL);
}

void LoadROM(const char* sFullPath)
{
    quasi88_load_tape_insert(sFullPath);
}

void RA_InitShared()
{
    RA_InstallSharedFunctions(&GameIsActive, &CauseUnpause, &CausePause, &RebuildMenu, &GetEstimatedGameTitle, &ResetEmulation, &LoadROM);
}

#endif
