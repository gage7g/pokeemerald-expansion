// Shining Helix title screen.
// Starry sky + meteorite (BG3), gold logo (BG1), twisting DNA helix (BG2, 16 tilemap frames),
// flashing PRESS START (BG0), and a 2x Apex Deoxys sprite that floats, pulses its core,
// swaps poses and flashes into a new form every few seconds.
#include "global.h"
#include "berry_fix_program.h"
#include "bg.h"
#include "clear_save_data_menu.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "intro.h"
#include "m4a.h"
#include "main.h"
#include "main_menu.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "quickstart.h"
#include "reset_rtc_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "title_screen.h"
#include "trig.h"
#include "util.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"

#if IS_FRLG

// ---- Layout (must match graphics/title_screen_helix/layout.txt) ----
#define LOGO_TILE_COUNT     112 // press start tiles are loaded right after the logo tiles
#define HELIX_X             1   // in tiles
#define HELIX_Y             10
#define HELIX_W             12
#define HELIX_H             3
#define HELIX_FRAME_COUNT   16

#define PAL_SLOT_SKY        0
#define PAL_SLOT_LOGO       1
#define PAL_SLOT_HELIX      2
#define PAL_SLOT_PRESS      3

// Deoxys palette indices of the purple core (dark, mid, light)
#define GEM_DARK            12
#define GEM_MID             11
#define GEM_LIGHT           6

// ---- Timing (in frames) ----
#define ANIM_STEP           6   // helix advances one frame every 6 frames
#define POSE_TIME           24  // pose swap + PRESS START blink
#define FORM_TIME           96  // Deoxys changes form
#define PULSE_TIME          48  // core glow cycle
#define ANIM_LOOP           384 // everything lines back up after this many frames
#define FLASH_START         14  // strength of the purple flash when the form changes (out of 16)
#define FLASH_COLOR         RGB(25, 15, 31)
#define IDLE_RESTART_TIME   2700

#define DEOXYS_X            172 // center of the 128x128 sprite
#define DEOXYS_Y            90

#define TAG_APEX_DEOXYS     0x4D58

static const u16 sSky_Pal[]           = INCGFX_U16("graphics/title_screen_helix/sky.png", ".gbapal");
static const u8  sSky_Tiles[]         = INCGFX_U8("graphics/title_screen_helix/sky.png", ".4bpp.smol");
static const u8  sSky_Map[]           = INCBIN_U8("graphics/title_screen_helix/sky.bin.smolTM");
static const u16 sLogo_Pal[]          = INCGFX_U16("graphics/title_screen_helix/logo.png", ".gbapal");
static const u8  sLogo_Tiles[]        = INCGFX_U8("graphics/title_screen_helix/logo.png", ".4bpp.smol");
static const u8  sLogo_Map[]          = INCBIN_U8("graphics/title_screen_helix/logo.bin.smolTM");
static const u16 sPressStart_Pal[]    = INCGFX_U16("graphics/title_screen_helix/press_start.png", ".gbapal");
static const u8  sPressStart_Tiles[]  = INCGFX_U8("graphics/title_screen_helix/press_start.png", ".4bpp.smol");
static const u8  sPressStart_Map[]    = INCBIN_U8("graphics/title_screen_helix/press_start.bin.smolTM");
static const u16 sHelix_Pal[]         = INCGFX_U16("graphics/title_screen_helix/helix.png", ".gbapal");
static const u8  sHelix_Tiles[]       = INCGFX_U8("graphics/title_screen_helix/helix.png", ".4bpp.smol");
static const u16 sHelix_Frames[]      = INCBIN_U16("graphics/title_screen_helix/helix_frames.bin");
static const u16 sApexDeoxys_Pal[]    = INCGFX_U16("graphics/title_screen_helix/apex_deoxys.png", ".gbapal");
static const u32 sApexDeoxys_Gfx[]    = INCGFX_U32("graphics/title_screen_helix/apex_deoxys.png", ".4bpp.smol");

static EWRAM_DATA u8 sHelixFrame = 0;
static EWRAM_DATA bool8 sHelixMapDirty = FALSE;

static void CB2_TitleScreenHelixRun(void);
static void VBlankCB_TitleScreenHelix(void);
static void Task_TitleScreenHelixMain(u8 taskId);
static void Task_TitleScreenHelixAnim(u8 taskId);

static const struct BgTemplate sBgTemplates[] = {
    { // PRESS START
        .bg = 0,
        .charBaseIndex = 3,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }, { // logo
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }, { // DNA helix
        .bg = 2,
        .charBaseIndex = 1,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }, { // sky + meteorite
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct OamData sOam_ApexDeoxys = {
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64),
    .priority = 1,
};

// 4 forms x 2 poses, 64 tiles each: normal, attack, defense, speed
static const union AnimCmd sAnim_Deoxys0[] = { ANIMCMD_FRAME(0 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys1[] = { ANIMCMD_FRAME(1 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys2[] = { ANIMCMD_FRAME(2 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys3[] = { ANIMCMD_FRAME(3 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys4[] = { ANIMCMD_FRAME(4 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys5[] = { ANIMCMD_FRAME(5 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys6[] = { ANIMCMD_FRAME(6 * 64, 1), ANIMCMD_END };
static const union AnimCmd sAnim_Deoxys7[] = { ANIMCMD_FRAME(7 * 64, 1), ANIMCMD_END };

static const union AnimCmd *const sAnims_ApexDeoxys[] = {
    sAnim_Deoxys0, sAnim_Deoxys1, sAnim_Deoxys2, sAnim_Deoxys3,
    sAnim_Deoxys4, sAnim_Deoxys5, sAnim_Deoxys6, sAnim_Deoxys7,
};

// 0x200 = drawn at 2x size
static const union AffineAnimCmd sAffineAnim_Double[] = {
    AFFINEANIMCMD_FRAME(0x200, 0x200, 0, 0),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sAffineAnims_ApexDeoxys[] = {
    sAffineAnim_Double
};

static const struct SpriteTemplate sSpriteTemplate_ApexDeoxys = {
    .tileTag = TAG_APEX_DEOXYS,
    .paletteTag = TAG_APEX_DEOXYS,
    .oam = &sOam_ApexDeoxys,
    .anims = sAnims_ApexDeoxys,
    .images = NULL,
    .affineAnims = sAffineAnims_ApexDeoxys,
    .callback = SpriteCallbackDummy
};

static const struct CompressedSpriteSheet sSpriteSheet_ApexDeoxys = {sApexDeoxys_Gfx, 64 * 64 * 8 / 2, TAG_APEX_DEOXYS};
static const struct SpritePalette sSpritePal_ApexDeoxys = {sApexDeoxys_Pal, TAG_APEX_DEOXYS};

static void ResetGpuRegs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT,  0);
    SetGpuReg(REG_OFFSET_BLDCNT,   0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY,     0);
    SetGpuReg(REG_OFFSET_BG0HOFS,  0);
    SetGpuReg(REG_OFFSET_BG0VOFS,  0);
    SetGpuReg(REG_OFFSET_BG1HOFS,  0);
    SetGpuReg(REG_OFFSET_BG1VOFS,  0);
    SetGpuReg(REG_OFFSET_BG2HOFS,  0);
    SetGpuReg(REG_OFFSET_BG2VOFS,  0);
    SetGpuReg(REG_OFFSET_BG3HOFS,  0);
    SetGpuReg(REG_OFFSET_BG3VOFS,  0);
}

// task data for Task_TitleScreenHelixAnim
#define tAnimTimer      data[0]
#define tSpriteId       data[1]
#define tBlinkEnabled   data[2]

void CB2_InitTitleScreenHelix(void)
{
    switch (gMain.state)
    {
    default:
        gMain.state = 0;
        // fallthrough
    case 0:
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        StartTimer1();
        InitHeap(gHeap, HEAP_SIZE);
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetGpuRegs();
        DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
        DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
        DmaFill16(3, 0, (void *)PLTT, PLTT_SIZE);
        ResetBgsAndClearDma3BusyFlags(FALSE);
        InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        sHelixFrame = 0;
        sHelixMapDirty = TRUE;
        break;
    case 1:
        LoadPalette(sSky_Pal,        BG_PLTT_ID(PAL_SLOT_SKY),   PLTT_SIZE_4BPP);
        LoadPalette(sLogo_Pal,       BG_PLTT_ID(PAL_SLOT_LOGO),  PLTT_SIZE_4BPP);
        LoadPalette(sHelix_Pal,      BG_PLTT_ID(PAL_SLOT_HELIX), PLTT_SIZE_4BPP);
        LoadPalette(sPressStart_Pal, BG_PLTT_ID(PAL_SLOT_PRESS), PLTT_SIZE_4BPP);
        DecompressAndCopyTileDataToVram(3, sSky_Tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(3, sSky_Map, 0, 0, 1);
        DecompressAndCopyTileDataToVram(1, sLogo_Tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(1, sLogo_Map, 0, 0, 1);
        DecompressAndCopyTileDataToVram(0, sPressStart_Tiles, 0, LOGO_TILE_COUNT, 0);
        DecompressAndCopyTileDataToVram(0, sPressStart_Map, 0, 0, 1);
        DecompressAndCopyTileDataToVram(2, sHelix_Tiles, 0, 0, 0);
        LoadCompressedSpriteSheet(&sSpriteSheet_ApexDeoxys);
        LoadSpritePalette(&sSpritePal_ApexDeoxys);
        break;
    case 2:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            u8 taskId = CreateTask(Task_TitleScreenHelixAnim, 3);
            gTasks[taskId].tSpriteId = CreateSprite(&sSpriteTemplate_ApexDeoxys, DEOXYS_X, DEOXYS_Y, 0);
            gTasks[taskId].tBlinkEnabled = TRUE;
            CreateTask(Task_TitleScreenHelixMain, 4);

            ShowBg(0);
            ShowBg(1);
            ShowBg(2);
            ShowBg(3);
            BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
            BeginNormalPaletteFade(PALETTES_ALL, 1, 16, 0, RGB_BLACK);
            SetVBlankCallback(VBlankCB_TitleScreenHelix);
            SetMainCallback2(CB2_TitleScreenHelixRun);
            m4aSongNumStart(MUS_TITLE);
        }
        return;
    }
    gMain.state++;
}

static void CB2_TitleScreenHelixRun(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_TitleScreenHelix(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();

    // Swap in this frame of the twisting helix
    if (sHelixMapDirty)
    {
        u32 row;
        const u16 *src = &sHelix_Frames[sHelixFrame * HELIX_W * HELIX_H];
        u16 *dst = (u16 *)BG_SCREEN_ADDR(sBgTemplates[2].mapBaseIndex) + HELIX_Y * 32 + HELIX_X;
        for (row = 0; row < HELIX_H; row++)
            CpuCopy16(&src[row * HELIX_W], &dst[row * 32], HELIX_W * 2);
        sHelixMapDirty = FALSE;
    }
}

// Raises a color toward white by up to 45% (amount is 0-16)
static u16 BrightenColor(u16 color, u32 amount)
{
    s32 r = GET_R(color);
    s32 g = GET_G(color);
    s32 b = GET_B(color);
    r += ((31 - r) * (s32)amount * 45) / (16 * 100);
    g += ((31 - g) * (s32)amount * 45) / (16 * 100);
    b += ((31 - b) * (s32)amount * 45) / (16 * 100);
    return RGB(r, g, b);
}

static void Task_TitleScreenHelixAnim(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    struct Sprite *sprite = &gSprites[tSpriteId];
    u32 t = tAnimTimer;
    u32 form = (t / FORM_TIME) % 4;
    u32 pose = (t / POSE_TIME) % 2;
    u32 helixFrame = (t / ANIM_STEP) % HELIX_FRAME_COUNT;
    u32 palOffset = OBJ_PLTT_ID(IndexOfSpritePaletteTag(TAG_APEX_DEOXYS));
    u32 pulse = (Sin((t * 256 / PULSE_TIME) & 0xFF, 8) + 8);   // 0-16
    u32 sinceFormChange = t % FORM_TIME;
    u32 flash = (sinceFormChange < FLASH_START) ? FLASH_START - sinceFormChange : 0;

    // Helix twist
    if (helixFrame != sHelixFrame)
    {
        sHelixFrame = helixFrame;
        sHelixMapDirty = TRUE;
    }

    // Deoxys: float up and down, swap poses, change forms
    sprite->y2 = Sin((t * 256 / FORM_TIME) & 0xFF, 2);
    if (sprite->animNum != form * 2 + pose)
        StartSpriteAnim(sprite, form * 2 + pose);

    // Core glow (and a purple flash right after a form change)
    gPlttBufferUnfaded[palOffset + GEM_DARK]  = BrightenColor(sApexDeoxys_Pal[GEM_DARK], pulse);
    gPlttBufferUnfaded[palOffset + GEM_MID]   = BrightenColor(sApexDeoxys_Pal[GEM_MID], pulse);
    gPlttBufferUnfaded[palOffset + GEM_LIGHT] = BrightenColor(sApexDeoxys_Pal[GEM_LIGHT], pulse);
    if (!gPaletteFade.active)
        BlendPalette(palOffset, 16, flash, FLASH_COLOR);

    // PRESS START blinks
    if (tBlinkEnabled)
    {
        if (pose == 0)
            ShowBg(0);
        else
            HideBg(0);
    }

    if (++tAnimTimer >= ANIM_LOOP)
        tAnimTimer = 0;
}

#undef tAnimTimer
#undef tSpriteId
#undef tBlinkEnabled

static void StopPressStartBlink(void)
{
    u8 taskId = FindTaskIdByFunc(Task_TitleScreenHelixAnim);
    if (taskId != TASK_NONE)
        gTasks[taskId].data[2] = FALSE;
    ShowBg(0);
}

static void LeaveTitleScreen(MainCallback callback)
{
    u8 taskId = FindTaskIdByFunc(Task_TitleScreenHelixAnim);
    if (taskId != TASK_NONE)
        DestroyTask(taskId);
    taskId = FindTaskIdByFunc(Task_TitleScreenHelixMain);
    if (taskId != TASK_NONE)
        DestroyTask(taskId);
    SetVBlankCallback(NULL);
    SetMainCallback2(callback);
}

#define KEYSTROKE_DELSAVE   (B_BUTTON | SELECT_BUTTON | DPAD_UP)
#define KEYSTROKE_RESET_RTC (B_BUTTON | SELECT_BUTTON | DPAD_LEFT)
#define KEYSTROKE_BERRY_FIX (B_BUTTON | SELECT_BUTTON)

enum {
    STATE_WAIT_FADE_IN,
    STATE_RUN,
    STATE_CRY,
    STATE_FADE_TO_MENU,
    STATE_RESTART,
    STATE_WAIT_RESTART,
};

// task data for Task_TitleScreenHelixMain
#define tState      data[0]
#define tTimer      data[1]
#define tIdleTimer  data[2]

static void Task_TitleScreenHelixMain(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case STATE_WAIT_FADE_IN:
        if (!gPaletteFade.active)
        {
            if (QUICKSTART && QUICKSTART_HUD)
                CreateQuickstartHud();
            tState = STATE_RUN;
        }
        break;
    case STATE_RUN:
        if (JOY_HELD(KEYSTROKE_DELSAVE) == KEYSTROKE_DELSAVE)
        {
            LeaveTitleScreen(CB2_InitClearSaveDataScreen);
        }
        else if (JOY_HELD(KEYSTROKE_RESET_RTC) == KEYSTROKE_RESET_RTC && CanResetRTC() == TRUE)
        {
            m4aMPlayAllStop();
            LeaveTitleScreen(CB2_InitResetRtcScreen);
        }
        else if (JOY_HELD(KEYSTROKE_BERRY_FIX) == KEYSTROKE_BERRY_FIX)
        {
            m4aMPlayAllStop();
            LeaveTitleScreen(CB2_InitBerryFixProgram);
        }
        else if (JOY_NEW(A_BUTTON | START_BUTTON))
        {
            StopPressStartBlink();
            PlayCry_Normal(SPECIES_DEOXYS, 0);
            tTimer = 0;
            tState = STATE_CRY;
        }
        else if (QUICKSTART && JOY_NEW(SELECT_BUTTON))
        {
            Quickstart();
        }
        else if (++tIdleTimer >= IDLE_RESTART_TIME)
        {
            tState = STATE_RESTART;
        }
        break;
    case STATE_CRY:
        if (++tTimer >= 60)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITE);
            FadeOutBGM(4);
            tState = STATE_FADE_TO_MENU;
        }
        break;
    case STATE_FADE_TO_MENU:
        if (!gPaletteFade.active)
            LeaveTitleScreen(CB2_InitMainMenu);
        break;
    case STATE_RESTART:
        FadeOutMapMusic(10);
        BeginNormalPaletteFade(PALETTES_ALL, 3, 0, 16, RGB_BLACK);
        tState = STATE_WAIT_RESTART;
        break;
    case STATE_WAIT_RESTART:
        if (IsNotWaitingForBGMStop() && !gPaletteFade.active)
            LeaveTitleScreen(CB2_InitCopyrightScreenAfterTitleScreen);
        break;
    }
}

#undef tState
#undef tTimer
#undef tIdleTimer

#endif // IS_FRLG
