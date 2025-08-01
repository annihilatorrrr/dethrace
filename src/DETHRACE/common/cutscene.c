#include "cutscene.h"
#include "drmem.h"
#include "errors.h"
#include "flicplay.h"
#include "globvars.h"
#include "globvrpb.h"
#include "graphics.h"
#include "harness/config.h"
#include "harness/hooks.h"
#include "harness/os.h"
#include "harness/trace.h"
#include "input.h"
#include "loading.h"
#include "pd/sys.h"
#include "smackw32/smackw32.h"
#include "sound.h"
#include "utility.h"
#include <stdlib.h>
#include <time.h>

tS32 gLast_demo_end_anim = -90000;

// IDA: void __usercall ShowCutScene(int pIndex@<EAX>, int pWait_end@<EDX>, int pSound_ID@<EBX>, br_scalar pDelay)
// FUNCTION: CARM95 0x004a58c0
void ShowCutScene(int pIndex, int pWait_end, int pSound_ID, br_scalar pDelay) {

    gProgram_state.cut_scene = 1;
    if (pSound_ID >= 0) {
        DRS3LoadSound(pSound_ID);
        SetFlicSound(pSound_ID, PDGetTotalTime() + 1000.f * pDelay);
    }
    SetNonFatalAllocationErrors();
    RunFlic(pIndex);
    ResetNonFatalAllocationErrors();
    if (pWait_end) {
        WaitForAKey();
    } else {
        WaitForNoKeys();
    }
    FadePaletteDown();
    ClearEntireScreen();
    if (pSound_ID >= 0) {
        DRS3ReleaseSound(pSound_ID);
        SetFlicSound(0, 0);
    }
    gProgram_state.cut_scene = 0;
}

// IDA: void __cdecl DoSCILogo()
// FUNCTION: CARM95 0x004a5974
void DoSCILogo(void) {
}

// IDA: void __cdecl DoStainlessLogo()
// FUNCTION: CARM95 0x004a597f
void DoStainlessLogo(void) {
}

// IDA: void __usercall PlaySmackerFile(char *pSmack_name@<EAX>)
// FUNCTION: CARM95 0x004a598a
void PlaySmackerFile(char* pSmack_name) {
    tPath_name the_path;
    br_colour* br_colours_ptr;
    tU8* smack_colours_ptr;
    Smack* smk;
    int i;
    int j;
    int len;
    int fuck_off;

    if (!gSound_override && !gCut_scene_override) {
        StopMusic();
        FadePaletteDown();
        ClearEntireScreen();
        SmackSoundUseDirectSound(NULL);
        br_colours_ptr = gCurrent_palette->pixels;
        PathCat(the_path, gApplication_path, "CUTSCENE");
        PathCat(the_path, the_path, pSmack_name);
        dr_dprintf("Trying to open smack file '%s'", the_path);
        smk = SmackOpen(the_path, SMACKTRACKS, SMACKAUTOEXTRA);
        if (smk == NULL) {
            dr_dprintf("Unable to open smack file - attempt to load smack from CD...");
            if (GetCDPathFromPathsTxtFile(the_path)) {
                strcat(the_path, gDir_separator);
                strcat(the_path, "DATA");
                PathCat(the_path, the_path, "CUTSCENE");
                PathCat(the_path, the_path, pSmack_name);
                if (PDCheckDriveExists(the_path)) {
                    smk = SmackOpen(the_path, SMACKTRACKS, SMACKAUTOEXTRA);
                }
            } else {
                dr_dprintf("Can't get CD directory name");
            }
        }
        if (smk != NULL) {
            dr_dprintf("Smack file opened OK");
            for (i = 1; i <= smk->Frames; i++) {
                SmackToBuffer(smk, 0, 0, gBack_screen->row_bytes, gBack_screen->height, gBack_screen->pixels, 0);

                if (smk->NewPalette) {
                    smack_colours_ptr = smk->Palette;
                    for (j = 0; j < 256; j++) {
                        br_colours_ptr[j] = (smack_colours_ptr[j * 3] << 16) | smack_colours_ptr[j * 3 + 2] | (smack_colours_ptr[j * 3 + 1] << 8);
                    }

                    // TOOD: remove the commented-out line below when smk->NewPalette is set correctly per-frame
                    // memset(gBack_screen->pixels, 0, gBack_screen->row_bytes * gBack_screen->height);
                    DRSetPalette(gCurrent_palette);
                    PDScreenBufferSwap(0);
                    EnsurePaletteUp();
                }

                SmackDoFrame(smk);
                if (i != smk->Frames) {
                    SmackNextFrame(smk);
                }
                PDScreenBufferSwap(0);

                do {
                    fuck_off = AnyKeyDown() || EitherMouseButtonDown();
                } while (!fuck_off && SmackWait(smk));
                if (fuck_off) {
                    break;
                }
            }
            FadePaletteDown();
            ClearEntireScreen();
            SmackClose(smk);
        } else {
            dr_dprintf("Smack file '%s' failed to open", pSmack_name);
        }
        StartMusic();
    }
}

// IDA: void __cdecl DoOpeningAnimation()
// FUNCTION: CARM95 0x004a5d73
void DoOpeningAnimation(void) {

    PlaySmackerFile("LOGO.SMK");
    PlaySmackerFile(harness_game_info.defines.INTRO_SMK_FILE);
    WaitForNoKeys();
}

// IDA: void __cdecl DoNewGameAnimation()
// FUNCTION: CARM95 0x004a5de6
void DoNewGameAnimation(void) {
}

// IDA: void __cdecl DoGoToRaceAnimation()
// FUNCTION: CARM95 0x004a5d9d
void DoGoToRaceAnimation(void) {

    if (!gNet_mode) {
        if (PercentageChance(50)) {
            PlaySmackerFile("GARAGE2.SMK");
        } else {
            PlaySmackerFile("GARAGE1.SMK");
        }
    }
}

// IDA: void __cdecl DoEndRaceAnimation()
// FUNCTION: CARM95 0x004a5df1
void DoEndRaceAnimation(void) {
    int made_a_profit;
    int went_up_a_rank;

    made_a_profit = gProgram_state.credits_earned >= gProgram_state.credits_lost;
    went_up_a_rank = gProgram_state.credits_earned >= gProgram_state.credits_per_rank;

    FadePaletteDown();

    if (gAusterity_mode || gNet_mode != eNet_mode_none) {
        return;
    }
    if (gProgram_state.credits + gProgram_state.credits_earned - gProgram_state.credits_lost >= 0) {
        if (made_a_profit && went_up_a_rank) {
            PlaySmackerFile("SUCCESS.SMK");
        } else if (made_a_profit || went_up_a_rank) {
            PlaySmackerFile("MUNDANE.SMK");
        } else {
            PlaySmackerFile("UNSUCSES.SMK");
        }
    }
}

// IDA: void __cdecl DoGameOverAnimation()
// FUNCTION: CARM95 0x004a5ed6
void DoGameOverAnimation(void) {

    StopMusic();
    PlaySmackerFile("CRASH.SMK");
    StartMusic();
}

// IDA: void __cdecl DoGameCompletedAnimation()
// FUNCTION: CARM95 0x004a5ef8
void DoGameCompletedAnimation(void) {

    StopMusic();
    PlaySmackerFile("TOPRANK.SMK");
    StartMusic();
}

// DEMO only
void DoFeatureUnavailableInDemo(void) {

    PrintMemoryDump(0, "BEFORE DEMO-ONLY SCREEN");

    SuspendPendingFlic();
    FadePaletteDown();
    ShowCutScene(7, 1, 8502, gCut_delay_3);
    FadePaletteDown();

    PrintMemoryDump(0, "AFTER DEMO-ONLY SCREEN");
}

// DEMO only
void DoFullVersionPowerpoint(void) {

    FadePaletteDown();
    DRSetPalette(gRender_palette);
    if (harness_game_info.mode == eGame_splatpack_demo) {
        PlaySmackerFile("DEMOEND.SMK");
    } else {
        ShowCutScene(9, 0, 8503, gCut_delay_4);
    }
    FadePaletteDown();

    gLast_demo_end_anim = PDGetTotalTime();
}

// DEMO only
void DoDemoGoodbye(void) {
    if (PDGetTotalTime() - gLast_demo_end_anim > 90000) {
        DoFullVersionPowerpoint();
    }
}

// IDA: void __cdecl StartLoadingScreen()
// FUNCTION: CARM95 0x004a5f1a
void StartLoadingScreen(void) {

    PossibleService();
    if (gProgram_state.sausage_eater_mode) {
        SplashScreenWith(harness_game_info.defines.GERMAN_LOADSCRN);
    } else {
        SplashScreenWith("LOADSCRN.PIX");
    }
}
