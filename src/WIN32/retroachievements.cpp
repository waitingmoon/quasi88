#if USE_RETROACHIEVEMENTS
#include "retroachievements.h"

extern "C"
{
    #include "device.h"
    #include "initval.h"
    #include "memory.h"
    #include "quasi88.h"
    #include "screen.h"
}

/****************************************************************************
 * 実績処理に使用するメモリの読み書き関数
 *****************************************************************************/
unsigned char ByteReader(byte *buf, size_t nOffs)
{
    return *(buf + nOffs);
}

void ByteWriter(byte *buf, size_t nOffs, unsigned char nVal)
{
    *(buf + nOffs) = nVal;
}

unsigned char MainRAMReader(size_t nOffs)
{
    return ByteReader(main_ram, nOffs);
}

void MainRAMWriter(size_t nOffs, unsigned char nVal)
{
    ByteWriter(main_ram, nOffs, nVal);
}

unsigned char MainHighRAMReader(size_t nOffs)
{
    return ByteReader(main_high_ram, nOffs);
}

void MainHighRAMWriter(size_t nOffs, unsigned char nVal)
{
    ByteWriter(main_high_ram, nOffs, nVal);
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

char loaded_title[_MAX_FNAME];
void GetEstimatedGameTitle(char* sNameOut)
{
    if (loaded_title)
    {
        sNameOut = (char*)loaded_title;
    }
}

void ResetEmulation()
{
    quasi88_reset(NULL);
}

void LoadROM(const char* sFullPath)
{
    quasi88_disk_insert_all(sFullPath, TRUE);
}

void RA_InitShared()
{
    RA_InstallSharedFunctions(&GameIsActive, &CauseUnpause, &CausePause, &RebuildMenu, &GetEstimatedGameTitle, &ResetEmulation, &LoadROM);
}

void RA_InitUI()
{
    RA_Init(g_hWnd, /* RA_Quasi88 */ 9, Q_VERSION);
    RA_InitShared();
    RebuildMenu();
    RA_AttemptLogin(true);
    RebuildMenu();
}

void RA_InitMemory()
{
    RA_ClearMemoryBanks();
    RA_InstallMemoryBank(0, MainRAMReader, MainRAMWriter, 0x10000);
    RA_InstallMemoryBank(1, MainHighRAMReader, MainHighRAMWriter, 0x1000);
}

void RA_ClearMemory()
{
    RA_ClearMemoryBanks();
    loaded_title[0] = '\0';
}

void RA_SetGameTitle(char *title)
{
    strcpy(loaded_title, title);
    RA_UpdateAppTitle(title);
}

int RA_HandleMenuEvent(int id)
{
    if (LOWORD(id) >= IDM_RA_MENUSTART &&
        LOWORD(id) < IDM_RA_MENUEND)
    {
        RA_InvokeDialog(LOWORD(id));
        return TRUE;
    }

    return FALSE;
}

void RA_RenderOverlayFrame()
{
    HDC hdc = GetDC(g_hWnd);
    RECT window_size = { 0, 0, SCREEN_W, SCREEN_H };
    ControllerInput input;

    RA_UpdateRenderOverlay(hdc, &input, frameskip_rate / DEFAULT_VSYNC_FREQ_HZ, &window_size, use_fullscreen, (bool)quasi88_is_pause());
}

#endif
