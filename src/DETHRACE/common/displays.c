#include "displays.h"
#include "brender.h"
#include "constants.h"
#include "controls.h"
#include "depth.h"
#include "errors.h"
#include "flicplay.h"
#include "globvars.h"
#include "globvrbm.h"
#include "globvrkm.h"
#include "globvrpb.h"
#include "grafdata.h"
#include "graphics.h"
#include "harness/trace.h"
#include "netgame.h"
#include "pd/sys.h"
#include "utility.h"
#include <stdlib.h>

// GLOBAL: CARM95 0x00521678
int gLast_fancy_index;

// GLOBAL: CARM95 0x0052167c
int gLast_credit_headup__displays; // suffix added to avoid duplicate symbol

// GLOBAL: CARM95 0x00521680
int gLast_time_credit_headup;

// GLOBAL: CARM95 0x00521684
tDR_font* gCached_font;

// GLOBAL: CARM95 0x00541598
br_font* gBR_fonts[4];

// GLOBAL: CARM95 0x00541160
tQueued_headup gQueued_headups[4];

// GLOBAL: CARM95 0x00544e30
int gOld_times[10];

// GLOBAL: CARM95 0x00541590
int gLast_fancy_headup;

// GLOBAL: CARM95 0x00541594
tU32 gLast_time_earn_time;

// GLOBAL: CARM95 0x005415a8
tU32 gLast_centre_headup;

// GLOBAL: CARM95 0x005415ac
tU32 gLast_fancy_time;

// GLOBAL: CARM95 0x00541158
int gQueued_headup_count;

// GLOBAL: CARM95 0x0054114c
tU32 gLast_earn_time;

// GLOBAL: CARM95 0x00541154
tU32 gLast_time_credit_amount;

// GLOBAL: CARM95 0x00541150
int gLast_credit_amount;

// GLOBAL: CARM95 0x0053fdd8
tHeadup gHeadups[15];

// GLOBAL: CARM95 0x00544e60
int gLaps_headup;

// GLOBAL: CARM95 0x00544e68
int gCar_kill_count_headup;

// GLOBAL: CARM95 0x00544e5c
int gTimer_headup;

// GLOBAL: CARM95 0x00544e6c
int gTime_awarded_headup;

// GLOBAL: CARM95 0x00544e64
int gPed_kill_count_headup;

// GLOBAL: CARM95 0x00544eec
int gDim_amount;

// GLOBAL: CARM95 0x00544e70
br_pixelmap* gHeadup_images[32]; // Modified by DethRace for the demo

// GLOBAL: CARM95 0x00544e58
int gNet_cash_headup;

// GLOBAL: CARM95 0x00544e1c
int gNet_ped_headup;

int gCredits_lost_headup;

// GLOBAL: CARM95 0x00544e20
int gCredits_won_headup;

// IDA: void __usercall GetTimerString(char *pStr@<EAX>, int pFudge_colon@<EDX>)
// FUNCTION: CARM95 0x004c4030
void GetTimerString(char* pStr, int pFudge_colon) {

    TimerString(gTimer, pStr, pFudge_colon, 0);
}

// IDA: void __cdecl InitHeadups()
// FUNCTION: CARM95 0x004c4053
void InitHeadups(void) {
    int i;

    for (i = 0; i < 15; i++) {
        gHeadups[i].type = eHeadup_unused;
    }
    gBR_fonts[0] = BrFontProp7x9;
    gBR_fonts[1] = BrFontFixed3x5;
    gBR_fonts[2] = gFont_7;
    gBR_fonts[3] = gHeadup_font;
}

// IDA: void __usercall ClearHeadup(int pIndex@<EAX>)
// FUNCTION: CARM95 0x004c40c1
void ClearHeadup(int pIndex) {

    gHeadups[pIndex].type = eHeadup_unused;
}

// IDA: void __usercall ClearHeadupSlot(int pSlot_index@<EAX>)
// FUNCTION: CARM95 0x004c40e6
void ClearHeadupSlot(int pSlot_index) {
    int i;
    tHeadup* the_headup;

    the_headup = gHeadups;
    for (i = 0; i < COUNT_OF(gHeadups); i++) {
        if (the_headup->type != eHeadup_unused && the_headup->slot_index == pSlot_index) {
            ClearHeadup(i);
            return;
        }
        the_headup++;
    }
}

// IDA: void __cdecl ClearHeadups()
// FUNCTION: CARM95 0x004c414c
void ClearHeadups(void) {
    int i;

    for (i = 0; i < COUNT_OF(gHeadups); i++) {
        if (gHeadups[i].type) {
            ClearHeadup(i);
        }
    }
    gLast_fancy_index = -1;
    gLast_credit_headup__displays = -1;
    gLast_time_credit_headup = -1;
    gLast_earn_time = 0;
    gLast_fancy_time = 0;
    gLast_time_earn_time = 0;
    for (i = 0; i < COUNT_OF(gOld_times); i++) {
        gOld_times[i] = 0;
    }
    gQueued_headup_count = 0;
    gLast_centre_headup = 0;
}

// IDA: int __usercall HeadupActive@<EAX>(int pIndex@<EAX>)
// FUNCTION: CARM95 0x004c421d
int HeadupActive(int pIndex) {

    return gHeadups[pIndex].type != eHeadup_unused;
}

// IDA: void __usercall DRPixelmapText(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, tDR_font *pFont@<ECX>, char *pText, int pRight_edge)
// FUNCTION: CARM95 0x004c4256
void DRPixelmapText(br_pixelmap* pPixelmap, int pX, int pY, tDR_font* pFont, char* pText, int pRight_edge) {
    int i;
    int x;
    int len;
    int chr;
    int ch_width;
    unsigned char* ch;

    len = strlen(pText);
    ch = (unsigned char*)pText;
    if (pX >= 0 && pPixelmap->width >= pRight_edge && pY >= 0 && (pY + pFont->height) <= pPixelmap->height) {
        x = pX;
        for (i = 0; i < len; i++) {
            chr = ch[i] - pFont->offset;
            ch_width = pFont->width_table[chr];
            DRPixelmapRectangleOnscreenCopy(
                gBack_screen,
                x,
                pY,
                pFont->images,
                0,
                pFont->height * chr,
                ch_width,
                pFont->height);

            x += ch_width + pFont->spacing;
        }
    } else {
        x = pX;
        for (i = 0; i < len; i++) {
            chr = ch[i] - pFont->offset;
            ch_width = pFont->width_table[chr];
            DRPixelmapRectangleMaskedCopy(
                gBack_screen,
                x,
                pY,
                pFont->images,
                0,
                pFont->height * chr,
                ch_width,
                pFont->height);

            x += ch_width + pFont->spacing;
        }
    }
}

// IDA: void __usercall DRPixelmapCleverText2(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, tDR_font *pFont@<ECX>, signed char *pText, int pRight_edge)
// FUNCTION: CARM95 0x004c43cf
void DRPixelmapCleverText2(br_pixelmap* pPixelmap, int pX, int pY, tDR_font* pFont, char* pText, int pRight_edge) {
    int i;
    int x;
    int len;
    int chr;
    int ch_width;
    unsigned char* ch;
    tDR_font* new_font;

    x = pX;
    len = strlen(pText);
    ch = (unsigned char*)pText;
    if (pX >= 0 && pPixelmap->width >= pRight_edge && pY >= 0 && pY + pFont->height <= pPixelmap->height) {
        for (i = 0; i < len; i++) {
            if (*ch < 224) {
                chr = *ch - pFont->offset;
                ch_width = pFont->width_table[chr];
                DRPixelmapRectangleOnscreenCopy(
                    gBack_screen,
                    x,
                    pY,
                    pFont->images,
                    0,
                    chr * pFont->height,
                    ch_width,
                    pFont->height);
                x += ch_width + pFont->spacing;
            } else {
                new_font = &gFonts[-*ch + 256];
                pY -= (new_font->height - pFont->height) / 2;
                pFont = new_font;
            }
            ch++;
        }
    } else {
        for (i = 0; i < len; i++) {
            if (*ch < 224) {
                chr = *ch - pFont->offset;
                ch_width = pFont->width_table[chr];
                DRPixelmapRectangleMaskedCopy(
                    gBack_screen,
                    x,
                    pY,
                    pFont->images,
                    0,
                    chr * pFont->height,
                    ch_width,
                    pFont->height);
                x += ch_width + pFont->spacing;
            } else {
                new_font = &gFonts[-*ch + 256];
                pY -= (new_font->height - pFont->height) / 2;
                pFont = new_font;
            }
            ch++;
        }
    }
}

#ifdef DETHRACE_3DFX_PATCH
// IDA: void __usercall DeviouslyDimRectangle(br_pixelmap *pPixelmap@<EAX>, int pLeft@<EDX>, int pTop@<EBX>, int pRight@<ECX>, int pBottom, int pKnock_out_corners)
void DeviouslyDimRectangle(br_pixelmap* pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pKnock_out_corners) {

    if (pPixelmap != gBack_screen) {
        FatalError(kFatalError_CanOnlyDimRectanglesOfgBack_screen);
    }

    gDim_model->vertices[1].p.v[0] = pLeft;
    gDim_model->vertices[0].p.v[0] = pLeft;
    gDim_model->vertices[3].p.v[0] = pRight;
    gDim_model->vertices[2].p.v[0] = pRight;
    gDim_model->vertices[3].p.v[1] = -pTop;
    gDim_model->vertices[0].p.v[1] = -pTop;
    gDim_model->vertices[2].p.v[1] = -pBottom;
    gDim_model->vertices[1].p.v[1] = -pBottom;
    BrModelUpdate(gDim_model, BR_MODU_VERTEX_POSITIONS);
    gDim_actor->render_style = BR_RSTYLE_FACES;
    PDUnlockRealBackScreen(1);
    BrZbSceneRender(g2d_camera, g2d_camera, gBack_screen, gDepth_buffer);
    PDLockRealBackScreen(1);
    gDim_actor->render_style = BR_RSTYLE_NONE;
}
#endif

// IDA: void __cdecl DimRectangle(br_pixelmap *pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pKnock_out_corners)
// FUNCTION: CARM95 0x004c4604
void DimRectangle(br_pixelmap* pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pKnock_out_corners) {
    tU8* ptr;
    tU8* depth_table_ptr;
    tU8* right_ptr;
    int x;
    int y;
    int line_skip;
    int width;

    #ifdef DETHRACE_3DFX_PATCH
    if (gDevious_2d) {
        DeviouslyDimRectangle(pPixelmap, pLeft, pTop, pRight, pBottom, pKnock_out_corners);
        return;
    }
    #endif

    ptr = (tU8*)pPixelmap->pixels + pLeft + pPixelmap->row_bytes * pTop;
    line_skip = pPixelmap->row_bytes - pRight + pLeft;
    depth_table_ptr = gDepth_shade_table->pixels;
    x = gDepth_shade_table->row_bytes * gDim_amount;
    width = pRight - pLeft;

    if (pKnock_out_corners) {
        ptr++;
        for (right_ptr = ptr + width - 2; ptr < right_ptr; ptr++) {
            *ptr = depth_table_ptr[*ptr + x];
        }
        ptr += line_skip + 1;
        for (y = pTop + 1; y < (pBottom - 1); y++, ptr += line_skip) {
            for (right_ptr = ptr + width; ptr < right_ptr; ptr++) {
                *ptr = depth_table_ptr[*ptr + x];
            }
        }
        ptr++;
        for (right_ptr = ptr + width - 2; ptr < right_ptr; ptr++) {
            *ptr = depth_table_ptr[*ptr + x];
        }
    } else {
        for (y = pTop; y < pBottom; y++) {
            for (right_ptr = ptr + width; ptr < right_ptr; ptr++) {
                *ptr = depth_table_ptr[*ptr + x];
            }
            ptr += line_skip;
        }
    }
}

// IDA: void __cdecl DimAFewBits()
// FUNCTION: CARM95 0x004c479c
void DimAFewBits(void) {
    int i;

    int dim_index; // Added
    dim_index = gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0;
    for (i = 0; i < gProgram_state.current_car.dim_count[dim_index]; i++) {
        DimRectangle(
            gBack_screen,
            gProgram_state.current_car.dim_left[dim_index][i],
            gProgram_state.current_car.dim_top[dim_index][i],
            gProgram_state.current_car.dim_right[dim_index][i],
            gProgram_state.current_car.dim_bottom[dim_index][i],
            1);
    }
}

// IDA: void __cdecl KillOldestQueuedHeadup()
// FUNCTION: CARM95 0x004c524c
void KillOldestQueuedHeadup(void) {

    gQueued_headup_count--;
    memmove(&gQueued_headups[0], &gQueued_headups[1], gQueued_headup_count * sizeof(tQueued_headup));
}

// IDA: void __usercall DubreyBar(int pX_index@<EAX>, int pY@<EDX>, int pColour@<EBX>)
// FUNCTION: CARM95 0x004c5438
void DubreyBar(int pX_index, int pY, int pColour) {
    int x;

    x = gCurrent_graf_data->ps_bar_left - gCurrent_graf_data->ps_x_pitch * pX_index;
    BrPixelmapLine(gBack_screen, x, pY, x, gCurrent_graf_data->ps_bar_height + pY, pColour);
}

// IDA: void __usercall DoPSPowerHeadup(int pY@<EAX>, int pLevel@<EDX>, char *pName@<EBX>, int pBar_colour@<ECX>)
// FUNCTION: CARM95 0x004c530b
void DoPSPowerHeadup(int pY, int pLevel, char* pName, int pBar_colour) {
    char s[16];
    int i;

#ifdef DETHRACE_3DFX_PATCH
    if (gBack_screen->type == BR_PMT_RGB_565) {
        pBar_colour = PaletteEntry16Bit(gRender_palette, pBar_colour);
    }
#endif

    DimRectangle(gBack_screen, gCurrent_graf_data->ps_dim_left, pY, gCurrent_graf_data->ps_dim_right, gCurrent_graf_data->ps_dim_height + pY, 1);
    TransDRPixelmapText(gBack_screen, gCurrent_graf_data->ps_name_left, gCurrent_graf_data->ps_name_top_border + pY, gFonts + 6, pName, gBack_screen->width);

    for (i = (6 - gCurrent_graf_data->ps_bars_per_level) * gCurrent_graf_data->ps_bars_per_level + 1; i > (gCurrent_graf_data->ps_bars_per_level * pLevel + 1); i--) {
        DubreyBar(i, pY + gCurrent_graf_data->ps_bar_top_border, 0);
    }
    for (i = gCurrent_graf_data->ps_bars_per_level * pLevel + 1; i >= 0; i--) {
        DubreyBar(i, pY + gCurrent_graf_data->ps_bar_top_border, pBar_colour);
    }
}

// IDA: void __cdecl DoPSPowerupHeadups()
// FUNCTION: CARM95 0x004c5288
void DoPSPowerupHeadups(void) {

    DoPSPowerHeadup(gCurrent_graf_data->armour_headup_y[gProgram_state.cockpit_on], gProgram_state.current_car.power_up_levels[0], "A", 45);
    DoPSPowerHeadup(gCurrent_graf_data->power_headup_y[gProgram_state.cockpit_on], gProgram_state.current_car.power_up_levels[1], "P", 99);
    DoPSPowerHeadup(gCurrent_graf_data->offense_headup_y[gProgram_state.cockpit_on], gProgram_state.current_car.power_up_levels[2], "O", 4);
}

// IDA: void __usercall DoHeadups(tU32 pThe_time@<EAX>)
// FUNCTION: CARM95 0x004c4906
void DoHeadups(tU32 pThe_time) {
    int i;
    int x_offset;
    int y_offset;
    tHeadup* the_headup;
    int time_factor;

    if (gNet_mode) {
        DoNetScores();
    }
    if (gQueued_headup_count && PDGetTotalTime() - gLast_centre_headup >= 1000) {
        NewTextHeadupSlot(eHeadupSlot_misc, gQueued_headups[0].flash_rate,
            gQueued_headups[0].lifetime,
            gQueued_headups[0].font_index,
            gQueued_headups[0].text);
        KillOldestQueuedHeadup();
    }

    for (i = 0; i < COUNT_OF(gHeadups); i++) {
        the_headup = &gHeadups[i];
        if (the_headup->type != eHeadup_unused
            && (gProgram_state.which_view == eView_forward || !the_headup->cockpit_anchored)
            && (the_headup->type == eHeadup_image
                || the_headup->type == eHeadup_fancy
                || (the_headup->type == eHeadup_text && the_headup->data.text_info.text[0] != '\0')
                || ((the_headup->type == eHeadup_coloured_text || the_headup->type == eHeadup_box_text)
                    && the_headup->data.text_info.text[0] != '\0'))) {
            if (the_headup->type == eHeadup_fancy || the_headup->end_time == 0 || pThe_time < the_headup->end_time) {
                if (the_headup->dimmed_background) {
                    DimRectangle(
                        gBack_screen,
                        the_headup->dim_left,
                        the_headup->dim_top,
                        the_headup->dim_right,
                        the_headup->dim_bottom,
                        1);
                }
                if (the_headup->flash_period == 0
                    || Flash(the_headup->flash_period, &the_headup->last_flash, &the_headup->flash_state)) {
                    switch (the_headup->type) {
                    case eHeadup_text:
                        if (the_headup->cockpit_anchored) {
                            y_offset = gScreen_wobble_y;
                        } else {
                            y_offset = 0;
                        }
                        if (the_headup->cockpit_anchored) {
                            x_offset = gScreen_wobble_x;
                        } else {
                            x_offset = 0;
                        }
                        TransBrPixelmapText(
                            gBack_screen,
                            x_offset + the_headup->x,
                            y_offset + the_headup->y,
                            the_headup->data.text_info.colour,
                            the_headup->data.text_info.font,
                            the_headup->data.text_info.text);
                        break;
                    case eHeadup_coloured_text:
                        if (the_headup->clever) {
                            if (the_headup->cockpit_anchored) {
                                y_offset = gScreen_wobble_y;
                            } else {
                                y_offset = 0;
                            }
                            if (the_headup->cockpit_anchored) {
                                x_offset = gScreen_wobble_x;
                            } else {
                                x_offset = 0;
                            }
                            TransDRPixelmapCleverText(
                                gBack_screen,
                                x_offset + the_headup->x,
                                y_offset + the_headup->y,
                                the_headup->data.coloured_text_info.coloured_font,
                                the_headup->data.coloured_text_info.text,
                                the_headup->right_edge);
                        } else {
                            if (the_headup->cockpit_anchored) {
                                y_offset = gScreen_wobble_y;
                            } else {
                                y_offset = 0;
                            }
                            if (the_headup->cockpit_anchored) {
                                x_offset = gScreen_wobble_x;
                            } else {
                                x_offset = 0;
                            }
                            TransDRPixelmapText(
                                gBack_screen,
                                x_offset + the_headup->x,
                                y_offset + the_headup->y,
                                the_headup->data.coloured_text_info.coloured_font,
                                the_headup->data.coloured_text_info.text,
                                the_headup->right_edge);
                        }
                        break;
                    case eHeadup_image:
                        if (the_headup->cockpit_anchored) {
                            y_offset = gScreen_wobble_y;
                        } else {
                            y_offset = 0;
                        }
                        if (the_headup->cockpit_anchored) {
                            x_offset = gScreen_wobble_x;
                        } else {
                            x_offset = 0;
                        }
                        DRPixelmapRectangleMaskedCopy(
                            gBack_screen,
                            x_offset + the_headup->x,
                            y_offset + the_headup->y,
                            the_headup->data.image_info.image,
                            0,
                            0,
                            the_headup->data.image_info.image->width,
                            the_headup->data.image_info.image->height);
                        break;

                    case eHeadup_fancy:
                        switch (the_headup->data.fancy_info.fancy_stage) {
                        case eFancy_stage_incoming:
                            the_headup->data.fancy_info.offset -= 500 * gFrame_period / 1000;
                            if (the_headup->data.fancy_info.offset <= the_headup->data.fancy_info.shear_amount) {
                                the_headup->data.fancy_info.offset = the_headup->data.fancy_info.shear_amount;
                                the_headup->data.fancy_info.fancy_stage = eFancy_stage_halting;
                                the_headup->data.fancy_info.start_time = GetTotalTime();
                            }
                            DRPixelmapRectangleShearedCopy(
                                gBack_screen,
                                the_headup->x + the_headup->data.fancy_info.offset,
                                the_headup->y,
                                the_headup->data.fancy_info.image,
                                0,
                                0,
                                the_headup->data.fancy_info.image->width,
                                the_headup->data.fancy_info.image->height,
                                -BrIntToFixed(1));
                            break;
                        case eFancy_stage_halting:
                            time_factor = 1000 * (pThe_time - the_headup->data.fancy_info.start_time) / 100;
                            if (time_factor > 1000) {
                                if (time_factor > 1500) {
                                    time_factor = 1500;
                                    the_headup->data.fancy_info.fancy_stage = eFancy_stage_waiting;
                                    the_headup->data.fancy_info.start_time = GetTotalTime();
                                }
                                DRPixelmapRectangleShearedCopy(
                                    gBack_screen,
                                    the_headup->x - (1500 - time_factor) * the_headup->data.fancy_info.shear_amount / 500,
                                    the_headup->y,
                                    the_headup->data.image_info.image,
                                    0,
                                    0,
                                    the_headup->data.image_info.image->width,
                                    the_headup->data.image_info.image->height,
                                    BrIntToFixed((1500 - time_factor) * the_headup->data.fancy_info.shear_amount / 500)
                                        / the_headup->data.image_info.image->height);
                            } else {
#if DETHRACE_FIX_BUGS
                                time_factor = MAX(time_factor, 0);
#endif
                                DRPixelmapRectangleShearedCopy(
                                    gBack_screen,
                                    the_headup->x - the_headup->data.fancy_info.shear_amount * (time_factor - 500) / 500,
                                    the_headup->y,
                                    the_headup->data.image_info.image,
                                    0,
                                    0,
                                    the_headup->data.image_info.image->width,
                                    the_headup->data.image_info.image->height,
                                    BrIntToFixed(the_headup->data.fancy_info.shear_amount * (time_factor - 500) / 500)
                                        / the_headup->data.image_info.image->height);
                            }
                            break;
                        case eFancy_stage_waiting:
                            if (pThe_time - the_headup->data.fancy_info.start_time > 1000) {
                                the_headup->data.fancy_info.fancy_stage = eFancy_stage_readying;
                                the_headup->data.fancy_info.start_time = GetTotalTime();
                            }
                            DRPixelmapRectangleMaskedCopy(
                                gBack_screen,
                                the_headup->x,
                                the_headup->y,
                                the_headup->data.image_info.image,
                                0,
                                0,
                                the_headup->data.image_info.image->width,
                                the_headup->data.image_info.image->height);
                            break;
                        case eFancy_stage_readying:
                            time_factor = 1000 * (pThe_time - the_headup->data.fancy_info.start_time) / 100;
                            if (time_factor > 1000) {
                                time_factor = 1000;
                                the_headup->data.fancy_info.fancy_stage = eFancy_stage_leaving;
                                the_headup->data.fancy_info.start_time = GetTotalTime();
                                the_headup->data.fancy_info.offset = 0;
                            }
                            DRPixelmapRectangleShearedCopy(
                                gBack_screen,
                                the_headup->x,
                                the_headup->y,
                                the_headup->data.image_info.image,
                                0,
                                0,
                                the_headup->data.image_info.image->width,
                                the_headup->data.image_info.image->height,
                                -BrIntToFixed(time_factor * the_headup->data.fancy_info.shear_amount / 1000)
                                    / the_headup->data.image_info.image->height);
                            break;
                        case eFancy_stage_leaving:
                            the_headup->data.fancy_info.offset -= 500 * gFrame_period / 1000;
                            if (the_headup->data.fancy_info.offset <= the_headup->data.fancy_info.end_offset) {
                                ClearHeadup(i);
                            } else {
                                DRPixelmapRectangleShearedCopy(
                                    gBack_screen,
                                    the_headup->data.fancy_info.offset + the_headup->x,
                                    the_headup->y,
                                    the_headup->data.image_info.image,
                                    0,
                                    0,
                                    the_headup->data.image_info.image->width,
                                    the_headup->data.image_info.image->height,
                                    -BrIntToFixed(1));
                            }
                            break;
                        default:
                            break;
                        }
                        break;

                    case eHeadup_box_text:
                        if (the_headup->cockpit_anchored) {
                            y_offset = gScreen_wobble_y;
                        } else {
                            y_offset = 0;
                        }
                        if (the_headup->cockpit_anchored) {
                            x_offset = gScreen_wobble_x;
                        } else {
                            x_offset = 0;
                        }
                        OoerrIveGotTextInMeBoxMissus(
                            the_headup->data.coloured_text_info.coloured_font - gFonts,
                            the_headup->data.coloured_text_info.text,
                            gBack_screen,
                            gBack_screen->width / 10,
                            x_offset + the_headup->y,
                            9 * gBack_screen->width / 10,
                            y_offset + the_headup->y + 60,
                            1);
                        break;
                    default:
                        break;
                    }
                }
            } else {
                ClearHeadup(i);
            }
        }
    }
    DoPSPowerupHeadups();
}

// IDA: int __usercall FindAHeadupHoleWoofBarkSoundsABitRude@<EAX>(int pSlot_index@<EAX>)
// FUNCTION: CARM95 0x004c5493
int FindAHeadupHoleWoofBarkSoundsABitRude(int pSlot_index) {
    int i;
    int empty_one;
    tHeadup* the_headup;

    empty_one = -1;
    for (i = 0, the_headup = gHeadups; i < COUNT_OF(gHeadups); i++, the_headup++) {
        if (pSlot_index >= 0 && the_headup->slot_index == pSlot_index) {
            return i;
        }
        if (the_headup->type == eHeadup_unused) {
            empty_one = i;
        }
    }
    return empty_one;
}

// IDA: int __usercall DRTextWidth@<EAX>(tDR_font *pFont@<EAX>, char *pText@<EDX>)
// FUNCTION: CARM95 0x004c5514
int DRTextWidth(tDR_font* pFont, char* pText) {
    int i;
    int len;
    int result;
    char* c;

    c = pText;
    result = 0;
    len = strlen(pText);

    for (i = 0; i < len; i++) {
        result += pFont->width_table[*c - pFont->offset];
        c++;
    }
    return result + pFont->spacing * (len - 1);
}

// IDA: int __usercall DRTextCleverWidth@<EAX>(tDR_font *pFont@<EAX>, signed char *pText@<EDX>)
// FUNCTION: CARM95 0x004c5591
int DRTextCleverWidth(tDR_font* pFont, signed char* pText) {
    int i;
    int len;
    int result;
    unsigned char* c;

    result = 0;
    len = strlen((char*)pText) + 1;

    for (i = 0, c = (unsigned char*)pText; i < len; i++, c++) {
        if (*c < 224) {
            if (i < (len - 1)) {
                result += pFont->spacing;
            }
            result += pFont->width_table[*c - pFont->offset];
        } else {
            pFont = &gFonts[256 - *c];
        }
    }
    return result;
}

// IDA: void __usercall DRPixelmapCentredText(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, tDR_font *pFont@<ECX>, char *pText)
// FUNCTION: CARM95 0x004c5665
void DRPixelmapCentredText(br_pixelmap* pPixelmap, int pX, int pY, tDR_font* pFont, char* pText) {
    int width_over_2;

    width_over_2 = DRTextWidth(pFont, pText) / 2;
    TransDRPixelmapText(pPixelmap, pX - width_over_2, pY, pFont, pText, width_over_2 + pX);
}

// IDA: int __usercall IsHeadupTextClever@<EAX>(signed char *pText@<EAX>)
// FUNCTION: CARM95 0x004c5bd4
int IsHeadupTextClever(signed char* pText) {

    while (*pText) {
        if (*pText < 0) {
            return 1;
        }
        pText++;
    }
    return 0;
}

// IDA: int __usercall MungeHeadupWidth@<EAX>(tHeadup *pHeadup@<EAX>)
// FUNCTION: CARM95 0x004c5981
int MungeHeadupWidth(tHeadup* pHeadup) {
    int width;

    width = 0;
    if (pHeadup->type == eHeadup_box_text) {
        return 0;
    }
    if (pHeadup->type == eHeadup_coloured_text) {
        pHeadup->clever = IsHeadupTextClever((signed char*)(&pHeadup->data.text_info.text));
        if (pHeadup->justification) {
            if (pHeadup->justification == eJust_right) {
                if (pHeadup->clever) {
                    width = DRTextCleverWidth(
                        pHeadup->data.coloured_text_info.coloured_font,
                        (signed char*)(&pHeadup->data.text_info.text));
                } else {
                    width = DRTextWidth(pHeadup->data.coloured_text_info.coloured_font, pHeadup->data.text_info.text);
                }
                pHeadup->x = pHeadup->original_x - width;
            } else if (pHeadup->justification == eJust_centre) {
                if (pHeadup->clever) {
                    width = DRTextCleverWidth(
                        pHeadup->data.coloured_text_info.coloured_font,
                        (signed char*)(&pHeadup->data.text_info.text));
                } else {
                    width = DRTextWidth(pHeadup->data.coloured_text_info.coloured_font, pHeadup->data.text_info.text);
                }
                pHeadup->x = pHeadup->original_x - width / 2;
            }
        } else {
            pHeadup->x = pHeadup->original_x;
        }
    } else {
        pHeadup->clever = 0;
        if (pHeadup->justification) {
            if (pHeadup->justification == eJust_right) {
                width = BrPixelmapTextWidth(gBack_screen, pHeadup->data.text_info.font, pHeadup->data.text_info.text);
                pHeadup->x = pHeadup->original_x - width;
            } else if (pHeadup->justification == eJust_centre) {
                width = BrPixelmapTextWidth(gBack_screen, pHeadup->data.text_info.font, pHeadup->data.text_info.text);
                pHeadup->x = pHeadup->original_x - width / 2;
            }
        } else {
            pHeadup->x = pHeadup->original_x;
        }
    }
    return width;
}

// IDA: int __usercall NewTextHeadupSlot2@<EAX>(int pSlot_index@<EAX>, int pFlash_rate@<EDX>, int pLifetime@<EBX>, int pFont_index@<ECX>, char *pText, int pQueue_it)
// FUNCTION: CARM95 0x004c56b1
int NewTextHeadupSlot2(int pSlot_index, int pFlash_rate, int pLifetime, int pFont_index, char* pText, int pQueue_it) {
    int index;
    tHeadup* the_headup;
    tHeadup_slot* headup_slot;
    tU32 time;

    time = PDGetTotalTime();
    if (pQueue_it && pSlot_index == 4 && time - gLast_centre_headup < 1000) {
        if (gQueued_headup_count == 4) {
            KillOldestQueuedHeadup();
        }
        gQueued_headups[gQueued_headup_count].flash_rate = pFlash_rate;
        gQueued_headups[gQueued_headup_count].lifetime = pLifetime;
        gQueued_headups[gQueued_headup_count].font_index = pFont_index;
        strcpy(gQueued_headups[gQueued_headup_count].text, pText);
        gQueued_headup_count++;
        index = -1;
    } else {
        index = FindAHeadupHoleWoofBarkSoundsABitRude(pSlot_index);
        if (index >= 0) {
            if (pSlot_index == 4) {
                gLast_centre_headup = time;
            }
            headup_slot = &gProgram_state.current_car.headup_slots[gProgram_state.cockpit_on][pSlot_index];
            the_headup = &gHeadups[index];
            the_headup->data.coloured_text_info.coloured_font = &gFonts[-pFont_index];
            if (pSlot_index == 4) {
                the_headup->type = eHeadup_box_text;
            } else {
                the_headup->type = eHeadup_coloured_text;
            }
            if (!pText) {
                LOG_PANIC("panic");
            }
            strcpy(the_headup->data.text_info.text, pText);

            the_headup->slot_index = pSlot_index;
            the_headup->justification = headup_slot->justification;
            if (pSlot_index < 0) {
                the_headup->cockpit_anchored = 0;
            } else {
                the_headup->cockpit_anchored = headup_slot->cockpit_anchored;
            }
            the_headup->dimmed_background = headup_slot->dimmed_background;
            the_headup->dim_left = headup_slot->dim_left;
            the_headup->dim_top = headup_slot->dim_top;
            the_headup->dim_right = headup_slot->dim_right;
            the_headup->dim_bottom = headup_slot->dim_bottom;
            the_headup->original_x = headup_slot->x;
            the_headup->right_edge = MungeHeadupWidth(the_headup) + the_headup->x;
            the_headup->y = headup_slot->y;
            if (pFlash_rate) {
                the_headup->flash_period = 1000 / pFlash_rate;
            } else {
                the_headup->flash_period = 0;
            }
            the_headup->last_flash = 0;
            the_headup->flash_state = 0;
            if (pLifetime) {
                the_headup->end_time = GetTotalTime() + pLifetime;
            } else {
                the_headup->end_time = 0;
            }
        }
    }
    return index;
}

// IDA: int __usercall NewTextHeadupSlot@<EAX>(int pSlot_index@<EAX>, int pFlash_rate@<EDX>, int pLifetime@<EBX>, int pFont_index@<ECX>, char *pText)
// FUNCTION: CARM95 0x004c5c1d
int NewTextHeadupSlot(int pSlot_index, int pFlash_rate, int pLifetime, int pFont_index, char* pText) {

    return NewTextHeadupSlot2(pSlot_index, pFlash_rate, pLifetime, pFont_index, pText, 1);
}

// IDA: int __usercall NewImageHeadupSlot@<EAX>(int pSlot_index@<EAX>, int pFlash_rate@<EDX>, int pLifetime@<EBX>, int pImage_index@<ECX>)
// FUNCTION: CARM95 0x004c5c4b
int NewImageHeadupSlot(int pSlot_index, int pFlash_rate, int pLifetime, int pImage_index) {
    int index;
    tHeadup* the_headup;
    tHeadup_slot* headup_slot;

    index = FindAHeadupHoleWoofBarkSoundsABitRude(pSlot_index);
    if (index >= 0) {
        headup_slot = &gProgram_state.current_car.headup_slots[gProgram_state.cockpit_on][pSlot_index];
        the_headup = &gHeadups[index];
        the_headup->type = eHeadup_image;
        the_headup->slot_index = pSlot_index;
        the_headup->justification = headup_slot->justification;
        if (pSlot_index < 0) {
            the_headup->cockpit_anchored = 0;
        } else {
            the_headup->cockpit_anchored = headup_slot->cockpit_anchored;
        }
        the_headup->dimmed_background = headup_slot->dimmed_background;
        the_headup->dim_left = headup_slot->dim_left;
        the_headup->dim_top = headup_slot->dim_top;
        the_headup->dim_right = headup_slot->dim_right;
        the_headup->dim_bottom = headup_slot->dim_bottom;
        the_headup->original_x = headup_slot->x;

        if (headup_slot->justification) {
            if (headup_slot->justification == eJust_right) {
                the_headup->x = the_headup->original_x - gHeadup_images[pImage_index]->width;
            } else if (headup_slot->justification == eJust_centre) {
                the_headup->x = the_headup->original_x - gHeadup_images[pImage_index]->width / 2;
            }
        } else {
            the_headup->x = the_headup->original_x;
        }
        the_headup->y = headup_slot->y;
        if (pFlash_rate) {
            the_headup->flash_period = 1000 / pFlash_rate;
        } else {
            the_headup->flash_period = 0;
        }
        the_headup->last_flash = 0;
        the_headup->flash_state = 0;
        if (pLifetime) {
            the_headup->end_time = GetTotalTime() + pLifetime;
        } else {
            the_headup->end_time = 0;
        }
        the_headup->data.image_info.image = gHeadup_images[pImage_index];
    }
    return index;
}

// IDA: void __usercall DoFancyHeadup(int pIndex@<EAX>)
// FUNCTION: CARM95 0x004c5e50
void DoFancyHeadup(int pIndex) {
    tU32 the_time;
    tHeadup* the_headup;
    int temp_ref;

    the_time = GetTotalTime();
    if (!gMap_mode && (gLast_fancy_index < 0 || the_time - gLast_fancy_time > 2000 || gLast_fancy_index <= pIndex)) {
        temp_ref = NewImageHeadupSlot(eHeadupSlot_fancies, 0, 2000, pIndex + 10);
        if (temp_ref >= 0) {
            gLast_fancy_headup = temp_ref;
            gLast_fancy_index = pIndex;
            gLast_fancy_time = the_time;
            the_headup = &gHeadups[temp_ref];
            the_headup->type = eHeadup_fancy;
            the_headup->data.fancy_info.offset = (the_headup->data.image_info.image->width + gBack_screen->width) / 2;
            the_headup->data.fancy_info.end_offset = -the_headup->data.fancy_info.offset;
            the_headup->data.fancy_info.fancy_stage = eFancy_stage_incoming;
            the_headup->data.fancy_info.shear_amount = the_headup->data.image_info.image->height;
        }
    }
}

// IDA: void __cdecl AdjustHeadups()
// FUNCTION: CARM95 0x004c5f58
void AdjustHeadups(void) {
    int i;
    int delta_x;
    int delta_y;
    tHeadup* the_headup;

    the_headup = gHeadups;
    for (i = 0; i < COUNT_OF(gHeadups); i++) {
        the_headup = &gHeadups[i];
        if (the_headup->type == eHeadup_unused) {
            continue;
        }
        delta_x = gProgram_state.current_car.headup_slots[gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0][the_headup->slot_index].x
            - gProgram_state.current_car.headup_slots[!gProgram_state.cockpit_on || gProgram_state.cockpit_image_index < 0][the_headup->slot_index].x;
        delta_y = gProgram_state.current_car.headup_slots[gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0][the_headup->slot_index].y
            - gProgram_state.current_car.headup_slots[!gProgram_state.cockpit_on || gProgram_state.cockpit_image_index < 0][the_headup->slot_index].y;
        the_headup->x += delta_x;
        the_headup->original_x += delta_x;
        the_headup->y += delta_y;
        the_headup->dim_left += delta_x;
        the_headup->dim_top += delta_y;
        the_headup->dim_right += delta_x;
        the_headup->dim_bottom += delta_y;
    }
}

// IDA: void __usercall MoveHeadupTo(int pHeadup_index@<EAX>, int pNew_x@<EDX>, int pNew_y@<EBX>)
// FUNCTION: CARM95 0x004c6107
void MoveHeadupTo(int pHeadup_index, int pNew_x, int pNew_y) {
    int delta_x;
    tHeadup* the_headup;

    if (pHeadup_index >= 0) {
        delta_x = gHeadups[pHeadup_index].x - gHeadups[pHeadup_index].original_x;
        gHeadups[pHeadup_index].original_x = pNew_x;
        gHeadups[pHeadup_index].x = pNew_x + delta_x;
        gHeadups[pHeadup_index].y = pNew_y;
    }
}

// IDA: void __usercall ChangeHeadupText(int pHeadup_index@<EAX>, char *pNew_text@<EDX>)
// FUNCTION: CARM95 0x004c6169
void ChangeHeadupText(int pHeadup_index, char* pNew_text) {
    tHeadup* the_headup;

    if (pHeadup_index >= 0) {
        the_headup = &gHeadups[pHeadup_index];
        strcpy(the_headup->data.text_info.text, pNew_text);
        MungeHeadupWidth(the_headup);
    }
}

// IDA: void __usercall ChangeHeadupImage(int pHeadup_index@<EAX>, int pNew_image@<EDX>)
// FUNCTION: CARM95 0x004c61d2
void ChangeHeadupImage(int pHeadup_index, int pNew_image) {
    tHeadup* the_headup;

    if (pHeadup_index >= 0) {
        the_headup = &gHeadups[pHeadup_index];
        the_headup->data.image_info.image = gHeadup_images[pNew_image];
        switch (the_headup->justification) {
        case eJust_left:
            the_headup->x = the_headup->original_x;
            break;
        case eJust_right:
            the_headup->x = the_headup->original_x - the_headup->data.image_info.image->width;
            break;
        case eJust_centre:
            the_headup->x = the_headup->original_x - the_headup->data.image_info.image->width / 2;
            break;
        }
    }
}

// IDA: void __usercall ChangeHeadupColour(int pHeadup_index@<EAX>, int pNew_colour@<EDX>)
// FUNCTION: CARM95 0x004c629e
void ChangeHeadupColour(int pHeadup_index, int pNew_colour) {

    if (pHeadup_index >= 0) {
        gHeadups[pHeadup_index].data.text_info.colour = gColours[pNew_colour];
    }
}

// IDA: void __usercall DoDamageScreen(tU32 pThe_time@<EAX>)
// FUNCTION: CARM95 0x004c62d8
void DoDamageScreen(tU32 pThe_time) {
    int i;
    int y_pitch;
    int the_step;
    int the_wobble_x;
    int the_wobble_y;
    br_pixelmap* the_image;
    tDamage_unit* the_damage;

    if (&gProgram_state.current_car != gCar_to_view || gProgram_state.current_car_index != gProgram_state.current_car.index) {
        return;
    }
    if (gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0) {
        if (gProgram_state.which_view != eView_forward) {
            return;
        }
        the_wobble_x = gScreen_wobble_x;
        the_wobble_y = gScreen_wobble_y;
    } else {
        the_wobble_x = gProgram_state.current_car.damage_x_offset;
        the_wobble_y = gProgram_state.current_car.damage_y_offset;
        if (gProgram_state.current_car.damage_background != NULL) {
            DRPixelmapRectangleMaskedCopy(
                gBack_screen,
                gProgram_state.current_car.damage_background_x,
                gProgram_state.current_car.damage_background_y,
                gProgram_state.current_car.damage_background,
                0,
                0,
                gProgram_state.current_car.damage_background->width,
                gProgram_state.current_car.damage_background->height);
        }
    }
    for (i = 0; i < COUNT_OF(gProgram_state.current_car.damage_units); i++) {
        the_damage = &gProgram_state.current_car.damage_units[i];
        if (i != eDamage_driver) {
            the_image = the_damage->images;
            the_step = 5 * the_damage->damage_level / 100;
            y_pitch = (the_image->height / 2) / 5;
            DRPixelmapRectangleMaskedCopy(
                gBack_screen,
                the_wobble_x + gProgram_state.current_car.damage_units[i].x_coord,
                the_wobble_y + gProgram_state.current_car.damage_units[i].y_coord,
                the_image,
                0,
                y_pitch * (2 * the_step + ((pThe_time / the_damage->periods[the_step]) & 1)),
                the_image->width,
                y_pitch);
        }
    }
}

// IDA: void __cdecl PoshDrawLine(float pAngle, br_pixelmap *pDestn, int pX1, int pY1, int pX2, int pY2, int pColour)
// FUNCTION: CARM95 0x004c70fd
void PoshDrawLine(float pAngle, br_pixelmap* pDestn, int pX1, int pY1, int pX2, int pY2, int pColour) {

    if (pColour < 0) {
        if (pAngle >= 0.785 && pAngle <= 5.498 && (pAngle <= 2.356 || pAngle >= 3.926)) {
            if ((pAngle <= 0.785 || pAngle >= 1.57) && (pAngle <= 3.926 || pAngle >= 4.712)) {
                DRDrawLine(pDestn, pX1 - 1, pY1, pX2 - 1, pY2, -pColour - 1);
                DRDrawLine(pDestn, pX1 + 1, pY1, pX2 + 1, pY2, 1 - pColour);
            } else {
                DRDrawLine(pDestn, pX1 - 1, pY1, pX2 - 1, pY2, 1 - pColour);
                DRDrawLine(pDestn, pX1 + 1, pY1, pX2 + 1, pY2, -pColour - 1);
            }
        } else {
            DRDrawLine(pDestn, pX1, pY1 + 1, pX2, pY2 + 1, -pColour - 1);
            DRDrawLine(pDestn, pX1, pY1 - 1, pX2, pY2 - 1, 1 - pColour);
        }
        DRDrawLine(pDestn, pX1, pY1, pX2, pY2, -pColour);
    } else {
        DRDrawLine(pDestn, pX1, pY1, pX2, pY2, pColour);
    }
}

// IDA: void __usercall DoInstruments(tU32 pThe_time@<EAX>)
// FUNCTION: CARM95 0x004c6474
void DoInstruments(tU32 pThe_time) {
    br_pixelmap* speedo_image;
    br_pixelmap* tacho_image;
    int the_wobble_x;
    int the_wobble_y;
    int gear;
    int gear_height; /* Added by dethrace. */
    double the_angle;
    double the_angle2;
    double sin_angle;
    double cos_angle;
    double speed_mph;

    if (gProgram_state.current_car_index == gProgram_state.current_car.index) {
        speed_mph = gCar_to_view->speedo_speed * WORLD_SCALE / 1600.0f * 3600000.0f;
        if (speed_mph < 0.0f) {
            speed_mph = 0.0f;
        }
        if (gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0) {
            if (gProgram_state.which_view != eView_forward) {
                return;
            }
            the_wobble_x = gScreen_wobble_x;
            the_wobble_y = gScreen_wobble_y;
        } else {
            the_wobble_x = 0;
            the_wobble_y = 0;
        }
        tacho_image = gProgram_state.current_car.tacho_image[gProgram_state.cockpit_on];
        if (gProgram_state.current_car.tacho_radius_2[gProgram_state.cockpit_on] >= 0) {
            if (gCar_to_view->red_line >= gCar_to_view->revs) {
                the_angle = DEG_TO_RAD((double)(gProgram_state.current_car.tacho_end_angle[gProgram_state.cockpit_on] - gProgram_state.current_car.tacho_start_angle[gProgram_state.cockpit_on]) * gCar_to_view->revs / (double)gCar_to_view->red_line + (double)gProgram_state.current_car.tacho_start_angle[gProgram_state.cockpit_on]);
            } else {
                the_angle = DEG_TO_RAD((double)gProgram_state.current_car.tacho_end_angle[gProgram_state.cockpit_on]);
            }
            if (the_angle >= 0.0) {
                if (the_angle >= TAU) {
                    the_angle -= TAU;
                }
            } else {
                the_angle += TAU;
            }
            the_angle2 = DR_PI_OVER_2 - the_angle;
            if (the_angle2 < 0) {
                the_angle2 += TAU;
            }
            if (the_angle2 > DR_3PI_OVER_2) {
                cos_angle = gCosine_array[(unsigned int)((TAU - the_angle2) / DR_PI * 128.0)];
            } else if (the_angle2 > DR_PI) {
                cos_angle = -gCosine_array[(unsigned int)((the_angle2 - DR_PI) / DR_PI * 128.0)];
#if defined(DETHRACE_FIX_BUGS)
            } else if (the_angle2 >= DR_PI_OVER_2) {
#else
            } else if (the_angle2 > DR_PI_OVER_2) {
#endif
                cos_angle = -gCosine_array[(unsigned int)((DR_PI - the_angle2) / DR_PI * 128.0)];
            } else {
                cos_angle = gCosine_array[(unsigned int)(the_angle2 / DR_PI * 128.0)];
            }
            if (the_angle > DR_3PI_OVER_2) {
                sin_angle = gCosine_array[(unsigned int)((TAU - the_angle) / DR_PI * 128.0)];
            } else if (the_angle > DR_PI) {
                sin_angle = -gCosine_array[(unsigned int)((the_angle - DR_PI) / DR_PI * 128.0)];
#if defined(DETHRACE_FIX_BUGS)
            } else if (the_angle >= DR_PI_OVER_2) {
#else
            } else if (the_angle > DR_PI_OVER_2) {
#endif
                sin_angle = -gCosine_array[(unsigned int)((DR_PI - the_angle) / DR_PI * 128.0)];
            } else {
                sin_angle = gCosine_array[(unsigned int)(the_angle / DR_PI * 128.0)];
            }
            if (tacho_image != NULL) {
                DRPixelmapRectangleMaskedCopy(
                    gBack_screen,
                    the_wobble_x + gProgram_state.current_car.tacho_x[gProgram_state.cockpit_on],
                    the_wobble_y + gProgram_state.current_car.tacho_y[gProgram_state.cockpit_on],
                    tacho_image,
                    0,
                    0,
                    tacho_image->width,
                    tacho_image->height);
            }

            PoshDrawLine(
                the_angle,
                gBack_screen,
                ((double)gProgram_state.current_car.tacho_radius_1[gProgram_state.cockpit_on] * sin_angle
                    + (double)gProgram_state.current_car.tacho_centre_x[gProgram_state.cockpit_on]
                    + (double)the_wobble_x),
                ((double)gProgram_state.current_car.tacho_centre_y[gProgram_state.cockpit_on]
                    - (double)gProgram_state.current_car.tacho_radius_1[gProgram_state.cockpit_on] * cos_angle
                    + (double)the_wobble_y),
                ((double)gProgram_state.current_car.tacho_radius_2[gProgram_state.cockpit_on] * sin_angle
                    + (double)gProgram_state.current_car.tacho_centre_x[gProgram_state.cockpit_on]
                    + (double)the_wobble_x),
                ((double)gProgram_state.current_car.tacho_centre_y[gProgram_state.cockpit_on]
                    - (double)gProgram_state.current_car.tacho_radius_2[gProgram_state.cockpit_on] * cos_angle
                    + (double)the_wobble_y),
                gProgram_state.current_car.tacho_needle_colour[gProgram_state.cockpit_on]);
        } else if (tacho_image != NULL) {
#ifdef DETHRACE_3DFX_PATCH
            DRPixelmapRectangleCopy(
#else
            BrPixelmapRectangleCopy(
#endif
                gBack_screen,
                the_wobble_x + gProgram_state.current_car.tacho_x[gProgram_state.cockpit_on],
                the_wobble_y + gProgram_state.current_car.tacho_y[gProgram_state.cockpit_on],
                gProgram_state.current_car.tacho_image[gProgram_state.cockpit_on],
                0,
                0,
                ((gCar_to_view->revs - 1.0) / (double)gCar_to_view->red_line * (double)gProgram_state.current_car.tacho_image[gProgram_state.cockpit_on]->width + 1.0),
                gProgram_state.current_car.tacho_image[gProgram_state.cockpit_on]->height);
        }
        if (!gProgram_state.cockpit_on || gProgram_state.cockpit_image_index < 0 || gProgram_state.which_view == eView_forward) {
            if (gCar_to_view->gear < 0) {
                gear = -1;
            } else {
                gear = gCar_to_view->gear;
            }
#if defined(DETHRACE_FIX_BUGS)
/*
 * The OG derives gear mask height of 16 or 28 by `gears_image->height / 8`, but
 * this is only valid for HGEARS.PIX, which contains 8 gear images. Hardcoding
 * this number fixes gear rendering for cars using HGEARS4.PIX, which consists
 * of 11 gear images.
 */
#define GEAR_HEIGHT 16
#define GEAR_HEIGHT_HIRES 28
#else
#define GEAR_HEIGHT ((int)gProgram_state.current_car.gears_image->height / 8)
#define GEAR_HEIGHT_HIRES GEAR_HEIGHT
#endif
            gear_height = gGraf_spec_index ? GEAR_HEIGHT_HIRES : GEAR_HEIGHT;
            DRPixelmapRectangleMaskedCopy(
                gBack_screen,
                the_wobble_x + gProgram_state.current_car.gear_x[gProgram_state.cockpit_on],
                the_wobble_y + gProgram_state.current_car.gear_y[gProgram_state.cockpit_on],
                gProgram_state.current_car.gears_image,
                0,
                (gear + 1) * gear_height,
                gProgram_state.current_car.gears_image->width,
                gear_height);
        }
        speedo_image = gProgram_state.current_car.speedo_image[gProgram_state.cockpit_on];
        if (gProgram_state.current_car.speedo_radius_2[gProgram_state.cockpit_on] >= 0) {
            if (speedo_image && (!gProgram_state.cockpit_on || gProgram_state.cockpit_image_index < 0)) {
                DRPixelmapRectangleMaskedCopy(
                    gBack_screen,
                    the_wobble_x + gProgram_state.current_car.speedo_x[gProgram_state.cockpit_on],
                    the_wobble_y + gProgram_state.current_car.speedo_y[gProgram_state.cockpit_on],
                    speedo_image,
                    0,
                    0,
                    speedo_image->width,
                    speedo_image->height);
            }
            if ((double)gProgram_state.current_car.max_speed >= speed_mph) {
                the_angle = DEG_TO_RAD((double)(gProgram_state.current_car.speedo_end_angle[gProgram_state.cockpit_on] - gProgram_state.current_car.speedo_start_angle[gProgram_state.cockpit_on]) * speed_mph / (double)gProgram_state.current_car.max_speed + (double)gProgram_state.current_car.speedo_start_angle[gProgram_state.cockpit_on]);
            } else {
                the_angle = DEG_TO_RAD((double)gProgram_state.current_car.speedo_end_angle[gProgram_state.cockpit_on]);
            }

            if (the_angle < 0.0) {
                the_angle = the_angle + TAU;
            } else if (the_angle >= TAU) {
                the_angle -= TAU;
            }
            the_angle2 = DR_PI_OVER_2 - the_angle;
            if (the_angle2 < 0.0) {
                the_angle2 += TAU;
            }
            if (the_angle2 > DR_3PI_OVER_2) {
                cos_angle = gCosine_array[(unsigned int)((TAU - the_angle2) / DR_PI * 128.0)];
            } else if (the_angle2 > DR_PI) {
                cos_angle = -gCosine_array[(unsigned int)((the_angle2 - DR_PI) / DR_PI * 128.0)];
            } else if (the_angle2 > DR_PI_OVER_2) {
                cos_angle = -gCosine_array[(unsigned int)((DR_PI - the_angle2) / DR_PI * 128.0)];
            } else {
                cos_angle = gCosine_array[(unsigned int)(the_angle2 / DR_PI * 128.0)];
            }

            if (the_angle > DR_3PI_OVER_2) {
                sin_angle = gCosine_array[(unsigned int)((TAU - the_angle) / DR_PI * 128.0)];
            } else if (the_angle > DR_PI) {
                sin_angle = -gCosine_array[(unsigned int)((the_angle - DR_PI) / DR_PI * 128.0)];
            } else if (the_angle > DR_PI_OVER_2) {
                sin_angle = -gCosine_array[(unsigned int)((DR_PI - the_angle) / DR_PI * 128.0)];
            } else {
                sin_angle = gCosine_array[(unsigned int)(the_angle / DR_PI * 128.0)];
            }

            PoshDrawLine(
                the_angle,
                gBack_screen,
                ((double)gProgram_state.current_car.speedo_radius_1[gProgram_state.cockpit_on] * sin_angle
                    + (double)gProgram_state.current_car.speedo_centre_x[gProgram_state.cockpit_on]
                    + (double)the_wobble_x),
                ((double)gProgram_state.current_car.speedo_centre_y[gProgram_state.cockpit_on]
                    - (double)gProgram_state.current_car.speedo_radius_1[gProgram_state.cockpit_on] * cos_angle
                    + (double)the_wobble_y),
                ((double)gProgram_state.current_car.speedo_radius_2[gProgram_state.cockpit_on] * sin_angle
                    + (double)gProgram_state.current_car.speedo_centre_x[gProgram_state.cockpit_on]
                    + (double)the_wobble_x),
                ((double)gProgram_state.current_car.speedo_centre_y[gProgram_state.cockpit_on]
                    - (double)gProgram_state.current_car.speedo_radius_2[gProgram_state.cockpit_on] * cos_angle
                    + (double)the_wobble_y),
                gProgram_state.current_car.speedo_needle_colour[gProgram_state.cockpit_on]);
            if (speedo_image != NULL && gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0) {
                DRPixelmapRectangleMaskedCopy(
                    gBack_screen,
                    the_wobble_x + gProgram_state.current_car.speedo_x[gProgram_state.cockpit_on],
                    the_wobble_y + gProgram_state.current_car.speedo_y[gProgram_state.cockpit_on],
                    speedo_image,
                    0,
                    0,
                    speedo_image->width,
                    speedo_image->height);
            }
        } else if (speedo_image != NULL) {
            DrawNumberAt(
                speedo_image,
                the_wobble_x + gProgram_state.current_car.speedo_x[gProgram_state.cockpit_on],
                the_wobble_y + gProgram_state.current_car.speedo_y[gProgram_state.cockpit_on],
                gProgram_state.current_car.speedo_x_pitch[gProgram_state.cockpit_on],
                gProgram_state.current_car.speedo_y_pitch[gProgram_state.cockpit_on],
                speed_mph,
                3,
                1);
        }
    }
}

// IDA: void __usercall DoSteeringWheel(tU32 pThe_time@<EAX>)
// FUNCTION: CARM95 0x004c72e1
void DoSteeringWheel(tU32 pThe_time) {
    br_pixelmap* hands_image;
    int hands_index;

    if (gProgram_state.current_car_index == gProgram_state.current_car.index && gProgram_state.cockpit_on && gProgram_state.cockpit_image_index >= 0 && gProgram_state.which_view == eView_forward) {
        hands_index = (int)floor(gProgram_state.current_car.number_of_hands_images * ((1.f - gProgram_state.current_car.steering_angle / 10.f) / 2.f));
        if (hands_index < 0) {
            hands_index = 0;
        } else if (hands_index >= gProgram_state.current_car.number_of_hands_images) {
            hands_index = gProgram_state.current_car.number_of_hands_images - 1;
        }
        hands_image = gProgram_state.current_car.lhands_images[hands_index];
        if (hands_image != NULL) {
            DRPixelmapRectangleMaskedCopy(gBack_screen,
                gProgram_state.current_car.lhands_x[hands_index] + gScreen_wobble_x,
                gProgram_state.current_car.lhands_y[hands_index] + gScreen_wobble_y,
                hands_image, 0, 0, hands_image->width, hands_image->height);
        }
        hands_image = gProgram_state.current_car.rhands_images[hands_index];
        if (hands_image != NULL) {
            DRPixelmapRectangleMaskedCopy(gBack_screen,
                gProgram_state.current_car.rhands_x[hands_index] + gScreen_wobble_x,
                gProgram_state.current_car.rhands_y[hands_index] + gScreen_wobble_y,
                hands_image, 0, 0, hands_image->width, hands_image->height);
        }
    }
}

// IDA: void __cdecl ChangingView()
// FUNCTION: CARM95 0x004c7455
void ChangingView(void) {
    tU32 the_time;

    if (gProgram_state.new_view == eView_undefined) {
        return;
    }
    the_time = PDGetTotalTime() - gProgram_state.view_change_start;
    gScreen_wobble_x = 0;
    gScreen_wobble_y = 0;
    if (the_time > 175 && gProgram_state.which_view == gProgram_state.new_view) {
        if (gProgram_state.pending_view != eView_undefined) {
            if (gProgram_state.pending_view == eView_left) {
                LookLeft();
                return;
            }
            if (gProgram_state.pending_view == eView_forward) {
                LookForward();
                return;
            }
            if (gProgram_state.pending_view == eView_right) {
                LookRight();
                return;
            }
            gScreen_wobble_x = 0;
            gScreen_wobble_y = 0;
            return;
        }
        if (gProgram_state.which_view == gProgram_state.new_view) {
            gProgram_state.new_view = eView_undefined;
            gScreen_wobble_x = 0;
            gScreen_wobble_y = 0;
            return;
        }
    }
    if (the_time < 88) {
        if (gProgram_state.old_view < gProgram_state.new_view) {
            gScreen_wobble_x = -gCurrent_graf_data->cock_margin_x * the_time * 2.f / 175.f;
        } else {
            gScreen_wobble_x = gCurrent_graf_data->cock_margin_x * the_time * 2.f / 175.f;
        }
    } else {
        gProgram_state.which_view = gProgram_state.new_view;
        switch (gProgram_state.new_view) {
        case eView_left:
            gProgram_state.cockpit_image_index = 1;
            break;
        case eView_forward:
            gProgram_state.cockpit_image_index = 0;
            break;
        case eView_right:
            gProgram_state.cockpit_image_index = 2;
            break;
        default:
            break;
        }
        AdjustRenderScreenSize();
        if (gProgram_state.old_view < gProgram_state.new_view) {
            gScreen_wobble_x = gCurrent_graf_data->cock_margin_x * (175 - the_time) * 2.f / 175.f;
        } else {
            gScreen_wobble_x = -gCurrent_graf_data->cock_margin_x * (175 - the_time) * 2.f / 175.f;
        }
    }
}

// IDA: void __usercall EarnCredits2(int pAmount@<EAX>, char *pPrefix_text@<EDX>)
// FUNCTION: CARM95 0x004c76d8
void EarnCredits2(int pAmount, char* pPrefix_text) {
    char s[256];
    int original_amount;
    tU32 the_time;

    if (gRace_finished) {
        return;
    }
    the_time = GetTotalTime();
    if (pAmount == 0) {
        return;
    }
    if (gNet_mode != eNet_mode_none && gProgram_state.credits_earned - gProgram_state.credits_lost + pAmount < 0) {
        pAmount = gProgram_state.credits_lost - gProgram_state.credits_lost;
    }
    original_amount = pAmount;
    if (gLast_credit_headup__displays >= 0 && the_time - gLast_earn_time < 2000) {
        pAmount += gLast_credit_amount;
    }
    gLast_credit_amount = pAmount;
    if (pAmount >= 2) {
        sprintf(s, "%s%d %s", pPrefix_text, pAmount, GetMiscString(kMiscString_Credits));
        gProgram_state.credits_earned += original_amount;
    } else if (pAmount >= 1) {
        sprintf(s, "%s1 %s", pPrefix_text, GetMiscString(kMiscString_Credit));
        gProgram_state.credits_earned += original_amount;
    } else if (pAmount >= -1) {
        sprintf(s, "%s%s 1 %s", pPrefix_text, GetMiscString(kMiscString_Lost), GetMiscString(kMiscString_Credit));
        gProgram_state.credits_lost -= original_amount;
    } else {
        sprintf(s, "%s%s %d %s", GetMiscString(kMiscString_Lost), pPrefix_text, -pAmount, GetMiscString(kMiscString_Credits));
        gProgram_state.credits_lost -= original_amount;
    }
    gLast_credit_headup__displays = NewTextHeadupSlot(eHeadupSlot_misc, 0, 2000, -4, s);
    gLast_earn_time = the_time;
}

// IDA: void __usercall EarnCredits(int pAmount@<EAX>)
// FUNCTION: CARM95 0x004c78a8
void EarnCredits(int pAmount) {

    EarnCredits2(pAmount, "");
}

// IDA: int __usercall SpendCredits@<EAX>(int pAmount@<EAX>)
// FUNCTION: CARM95 0x004c78c4
int SpendCredits(int pAmount) {
    int amount;

    gProgram_state.credits_lost += pAmount;
    if (gNet_mode == eNet_mode_none) {
        return 0;
    }
    amount = gProgram_state.credits_earned - gProgram_state.credits_lost;
    if (gProgram_state.credits_earned - gProgram_state.credits_lost >= 0) {
        return 0;
    }
    gProgram_state.credits_lost = gProgram_state.credits_earned;
    return amount;
}

// IDA: void __usercall AwardTime(tU32 pTime@<EAX>)
// FUNCTION: CARM95 0x004c791e
void AwardTime(tU32 pTime) {
    char s[256];
    tU32 original_amount;
    tU32 the_time;
    int i;

    if (gRace_finished || gFreeze_timer || gNet_mode != eNet_mode_none || pTime == 0) {
        return;
    }

    original_amount = pTime;
    the_time = GetTotalTime();
    for (i = COUNT_OF(gOld_times) - 1; i > 0; i--) {
        gOld_times[i] = gOld_times[i - 1];
    }
    gOld_times[0] = pTime;
    if (gLast_time_credit_headup >= 0 && (the_time - gLast_time_earn_time) < 2000) {
        pTime += gLast_time_credit_amount;
    }
    gLast_time_credit_amount = pTime;
    gTimer += original_amount * 1000;
    s[0] = '+';
    TimerString(1000 * pTime, &s[1], 0, 0);
    gLast_time_credit_headup = NewTextHeadupSlot(eHeadupSlot_time_award, 0, 2000, -2, s);
    gLast_time_earn_time = the_time;
}

// IDA: void __usercall DrawRectangle(br_pixelmap *pPixelmap@<EAX>, int pLeft@<EDX>, int pTop@<EBX>, int pRight@<ECX>, int pBottom, int pColour)
// FUNCTION: CARM95 0x004c7a61
void DrawRectangle(br_pixelmap* pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pColour) {

    BrPixelmapLine(pPixelmap, pLeft, pTop, pRight, pTop, pColour);
    BrPixelmapLine(pPixelmap, pLeft, pBottom, pRight, pBottom, pColour);
    BrPixelmapLine(pPixelmap, pLeft, pTop, pLeft, pBottom, pColour);
    BrPixelmapLine(pPixelmap, pRight, pTop, pRight, pBottom, pColour);
}

// IDA: void __usercall DrawRRectangle(br_pixelmap *pPixelmap@<EAX>, int pLeft@<EDX>, int pTop@<EBX>, int pRight@<ECX>, int pBottom, int pColour)
// FUNCTION: CARM95 0x004c7aec
void DrawRRectangle(br_pixelmap* pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pColour) {

    BrPixelmapLine(pPixelmap, pLeft + 1, pTop, pRight - 1, pTop, pColour);
    BrPixelmapLine(pPixelmap, pLeft + 1, pBottom, pRight - 1, pBottom, pColour);
    BrPixelmapLine(pPixelmap, pLeft, pTop + 1, pLeft, pBottom - 1, pColour);
    BrPixelmapLine(pPixelmap, pRight, pTop + 1, pRight, pBottom - 1, pColour);
}

// IDA: void __usercall OoerrIveGotTextInMeBoxMissus(int pFont_index@<EAX>, char *pText@<EDX>, br_pixelmap *pPixelmap@<EBX>, int pLeft@<ECX>, int pTop, int pRight, int pBottom, int pCentred)
// FUNCTION: CARM95 0x004c7b7f
void OoerrIveGotTextInMeBoxMissus(int pFont_index, char* pText, br_pixelmap* pPixelmap, int pLeft, int pTop, int pRight, int pBottom, int pCentred) {
    tDR_font* font;
    int width;
    int current_width;
    int i;
    int centre;
    int line_char_index;
    int input_str_index;
    int start_line;
    int current_y;
    int font_needed_loading;
    char line[256];

    font = &gFonts[pFont_index];
    current_width = 0;
    font_needed_loading = font->images == NULL;
    if (font_needed_loading) {
        LoadFont(pFont_index);
    }
    centre = (pRight + pLeft) / 2;
    current_y = pTop;
    width = pRight - pLeft;
    line_char_index = 0;
    input_str_index = 0;
    start_line = 0;

    while (pText[input_str_index]) {
        line[line_char_index] = pText[input_str_index];
        line[line_char_index + 1] = 0;
        current_width += font->spacing + font->width_table[pText[input_str_index] - font->offset];
        if (current_width > width) {
            for (i = input_str_index; i >= start_line; i--) {
                if (pText[i] == ' ') {
                    break;
                }
            }
            if (i == start_line) {
                i = input_str_index;
            }
            line_char_index += i - input_str_index;
            input_str_index = i;
            if (pText[input_str_index] == ' ') {
                input_str_index++;
            }
            line[line_char_index] = 0;
            if (pCentred) {
                DRPixelmapCentredText(gBack_screen, centre, current_y, font, line);
            } else {
                TransDRPixelmapText(gBack_screen, pLeft, current_y, font, line, pRight);
            }
            current_width = 0;
            current_y += 3 * (font->height - (TranslationMode() ? 2 : 0)) / 2;
            line_char_index = 0;
            start_line = input_str_index;
        } else {
            line_char_index++;
            input_str_index++;
        }
    }
    if (line_char_index != 0) {
        if (pCentred) {
            TransDRPixelmapText(gBack_screen, centre - (DRTextWidth(font, line) / 2), current_y, font, line, (DRTextWidth(font, line) / 2) + centre);
        } else {
            TransDRPixelmapText(gBack_screen, pLeft, current_y, font, line, pRight);
        }
    }
    if (font_needed_loading) {
        DisposeFont(pFont_index);
    }
}

// IDA: void __usercall TransBrPixelmapText(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, br_uint_32 pColour@<ECX>, br_font *pFont, signed char *pText)
// FUNCTION: CARM95 0x004c7ec5
void TransBrPixelmapText(br_pixelmap* pPixelmap, int pX, int pY, br_uint_32 pColour, br_font* pFont, char* pText) {
    int len;

    len = TranslationMode() ? 2 : 0;
    BrPixelmapText(pPixelmap, pX, pY - len, pColour, pFont, (char*)pText);
}

// IDA: void __usercall TransDRPixelmapText(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, tDR_font *pFont@<ECX>, char *pText, int pRight_edge)
// FUNCTION: CARM95 0x004c7f08
void TransDRPixelmapText(br_pixelmap* pPixelmap, int pX, int pY, tDR_font* pFont, char* pText, int pRight_edge) {

    if (gAusterity_mode && FlicsPlayedFromDisk() && pFont != gCached_font) {
        if (gCached_font != NULL && gCached_font - gFonts > 13) {
            DisposeFont(gCached_font - gFonts);
        }
        gCached_font = pFont;
    }
    LoadFont(pFont - gFonts);
    DRPixelmapText(pPixelmap, pX, pY - (TranslationMode() ? 2 : 0), pFont, pText, pRight_edge);
}

// IDA: void __usercall TransDRPixelmapCleverText(br_pixelmap *pPixelmap@<EAX>, int pX@<EDX>, int pY@<EBX>, tDR_font *pFont@<ECX>, char *pText, int pRight_edge)
// FUNCTION: CARM95 0x004c7fd5
void TransDRPixelmapCleverText(br_pixelmap* pPixelmap, int pX, int pY, tDR_font* pFont, char* pText, int pRight_edge) {

    if (gAusterity_mode && FlicsPlayedFromDisk() && gCached_font != pFont) {
        if (gCached_font && gCached_font - gFonts > 13) {
            DisposeFont(gCached_font - gFonts);
        }
        gCached_font = pFont;
    }
    LoadFont(pFont - gFonts);
    DRPixelmapCleverText2(pPixelmap, pX, pY - (TranslationMode() == 0 ? 0 : 2), pFont, pText, pRight_edge);
}
