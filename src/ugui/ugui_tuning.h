/* ugui_tuning.h -- every behaviour the spec marks as unknown / guessed (G)
 * lives here as a named constant, so it can be corrected in one place once
 * reference screenshots of the original arrive.  Each entry names the spec
 * section that leaves it open (SPEC 11 "Unknowns") and the DESIGN.md
 * paragraph that documents it.  Nothing here is retail data.
 *
 * Override any value at compile time with -DUG_TUNE_NAME=value. */
#ifndef UGUI_TUNING_H
#define UGUI_TUNING_H

/* ---- 1.3 / 11.1: equal-depth tie order --------------------------------- */
/* 1: earlier objects in file order are drawn first (behind). */
#ifndef UG_TUNE_TIE_LIST_ORDER
#define UG_TUNE_TIE_LIST_ORDER 1
#endif

/* ---- 6.4 (amendment 3, R): icon scroller --------------------------------- */
#ifndef UG_TUNE_ICON_SPACING
#define UG_TUNE_ICON_SPACING 158.0f      /* R: neighbours sit at +-158 */
#endif
#ifndef UG_TUNE_ICON_VISIBLE
#define UG_TUNE_ICON_VISIBLE 3           /* R: the selected icon and one neighbour each side */
#endif
#ifndef UG_TUNE_ICON_WRAP_IF_MORE
#define UG_TUNE_ICON_WRAP_IF_MORE 0      /* R (4.5): no wrap, the end arrow is hidden */
#endif
#ifndef UG_TUNE_ICON_FADE_DIST
#define UG_TUNE_ICON_FADE_DIST 0.8f      /* G: while the strip slides, an icon fades out between |d| = 1
                                            and 1 + this (stills only show |d| <= 1; the original clips it
                                            to the light band) */
#endif

/* ---- 6.5 (amendment 3, R): option list ------------------------------------ */
#ifndef UG_TUNE_ROW_PITCH
#define UG_TUNE_ROW_PITCH 36.0f          /* R: measured on two screens */
#endif
#ifndef UG_TUNE_ROW_VISIBLE
#define UG_TUNE_ROW_VISIBLE 8            /* R: plain list; the PC controls page shows 7 (UG_R_CTL_VISIBLE) */
#endif
#ifndef UG_TUNE_ROW_BACKING_ALPHA
#define UG_TUNE_ROW_BACKING_ALPHA 172    /* G: black alpha that composites to about (65,65,65) on the light
                                            panel (R: focused row backing is dark grey) */
#endif
#ifndef UG_TUNE_SELECTOR_WRAP
#define UG_TUNE_SELECTOR_WRAP 1          /* selector rows wrap at the ends */
#endif
#ifndef UG_TUNE_LIST_WRAP
#define UG_TUNE_LIST_WRAP 0              /* focus wraps from last to first row */
#endif

/* ---- 4.5 / 11.3: key repeat for held directions ------------------------- */
#ifndef UG_TUNE_REPEAT_DELAY_MS
#define UG_TUNE_REPEAT_DELAY_MS 400.0f
#endif
#ifndef UG_TUNE_REPEAT_PERIOD_MS
#define UG_TUNE_REPEAT_PERIOD_MS 110.0f
#endif

/* ---- 4.5: automatic directional focus movement -------------------------- */
#ifndef UG_TUNE_NAV_CONE            /* lateral <= forward * NAV_CONE (1 = 45 deg) */
#define UG_TUNE_NAV_CONE 1.0f
#endif
#ifndef UG_TUNE_NAV_LATERAL_WEIGHT  /* score = forward + weight * lateral */
#define UG_TUNE_NAV_LATERAL_WEIGHT 2.0f
#endif

/* ---- 4.6 / 11.5: pointer model (hover / wheel / click) ------------------ */
#ifndef UG_TUNE_HOVER_FOCUS
#define UG_TUNE_HOVER_FOCUS 1            /* hovering a focusable button focuses it */
#endif
#ifndef UG_TUNE_WHEEL_AS_UPDOWN
#define UG_TUNE_WHEEL_AS_UPDOWN 1        /* wheel over a list = Up / Down */
#endif

/* ---- 6.4 / 7.1: scene (original screen) presentation -------------------- */
#ifndef UG_TUNE_HIDE_FLAG4_TEXT
#define UG_TUNE_HIDE_FLAG4_TEXT 1        /* text with object flag 0x4 (hint labels,
                                            player strip) is replaced by the
                                            mouse hint bar (SPEC 3.4.2, 7.1) */
#endif
#ifndef UG_TUNE_SHOW_CONSOLE_STRIP
#define UG_TUNE_SHOW_CONSOLE_STRIP 1     /* D (20.3): original console strip selected for desktop demo */
#endif
#ifndef UG_TUNE_SCENE_PC_BUILD
#define UG_TUNE_SCENE_PC_BUILD 1         /* hide console-only, show PC-only */
#endif
#ifndef UG_TUNE_SCENE_BILINEAR
#define UG_TUNE_SCENE_BILINEAR 1         /* 15.9: level 0, bilinear */
#endif
#ifndef UG_TUNE_TEX_MIPMAPS
#define UG_TUNE_TEX_MIPMAPS 0            /* 15.9: level 0 with bilinear filtering; 1 = CPU box-filter mip chain */
#endif
#ifndef UG_TUNE_EXIT_TIMEOUT_MS
#define UG_TUNE_EXIT_TIMEOUT_MS 900.0f   /* viewer waits this long for "exit complete" */
#endif

/* ---- 6.9: dialog layout the data does not give --------------------------- */
#ifndef UG_TUNE_DLG3_BUTTON_W
#define UG_TUNE_DLG3_BUTTON_W 170.0f     /* three-button variant: width and pitch */
#endif
#ifndef UG_TUNE_DLG3_BUTTON_PITCH
#define UG_TUNE_DLG3_BUTTON_PITCH 180.0f
#endif

/* ---- 6.11 / 6.13: meters and grids -------------------------------------- */
#ifndef UG_TUNE_STAT_ANIM_MS
#define UG_TUNE_STAT_ANIM_MS 300.0f
#endif
#ifndef UG_TUNE_GRID_SCALE
#define UG_TUNE_GRID_SCALE 2.0f          /* swatch group scale (6.13): 16 local units = 32 on screen */
#endif
#ifndef UG_TUNE_SWATCH_PITCH
#define UG_TUNE_SWATCH_PITCH 42.0f       /* 6.13 (amendment 2): ~42 on screen */
#endif

/* ---- 6.18: player strip --------------------------------------------------- */
#ifndef UG_TUNE_STRIP_SWAP_MS
#define UG_TUNE_STRIP_SWAP_MS 4000.0f
#endif

/* ---- 7.6: mouse hint bar layout (design, not data) ------------------------ */
#ifndef UG_TUNE_HINT_AREA_LEFT
#define UG_TUNE_HINT_AREA_LEFT (-108.0f)  /* left bound of the hint area (7.1: dark box starts at -107) */
#endif
#ifndef UG_TUNE_HINT_ITEM_GAP
#define UG_TUNE_HINT_ITEM_GAP 24.0f
#endif

/* ---- 8.4 / 11.16: text layout choices ------------------------------------- */
#ifndef UG_TUNE_MULTILINE_ALIGN_EACH
#define UG_TUNE_MULTILINE_ALIGN_EACH 1   /* centre/right each line individually */
#endif
#ifndef UG_TUNE_GLYPH_CELL_PER_CAP
#define UG_TUNE_GLYPH_CELL_PER_CAP 1.2f  /* 7.3: inline glyph cell = cap height x this */
#endif

/* ---- 7.5 / 11.6: key-cap binding of BUTTON0..3 (letters) ------------------ */
#ifndef UG_TUNE_BUTTON_LETTERS
#define UG_TUNE_BUTTON_LETTERS "QCDR"
#endif


/* ---- amendment 3: guesses that remain around the measured values ---------- */
#ifndef UG_TUNE_DIM_BELOW
#define UG_TUNE_DIM_BELOW 0              /* 6.9 / 11.18: 1 = the page beneath a dialog also plays its dim fade
                                            (the captures show only the 0.40 overlay, so 0) */
#endif
#ifndef UG_TUNE_DLG_MSG_Y_MED
#define UG_TUNE_DLG_MSG_Y_MED (-16.0f)   /* G: message centre of the MED dialog (SMALL -3 and LARGE -33 are R) */
#endif
#ifndef UG_TUNE_PC_MOUSE_GLYPH
#define UG_TUNE_PC_MOUSE_GLYPH 0         /* 7.4.1 draws no glyph on a pill; 1 adds a small mouse glyph to
                                            clickable pills of the PC help bar (the library's own addition) */
#endif
#ifndef UG_TUNE_VSCR_THUMB_END
#define UG_TUNE_VSCR_THUMB_END 134.0f    /* G: option-list thumb centre with the last row selected (the first
                                            row's -130 is R); symmetric to the top inset */
#endif
#ifndef UG_TUNE_CTL_BACKING_ALPHA
#define UG_TUNE_CTL_BACKING_ALPHA 172    /* G, as UG_TUNE_ROW_BACKING_ALPHA */
#endif

/* ===========================================================================
 * SPEC-FIXED GEOMETRY (confidence R, amendment 3, SPEC 19.2)
 *
 * Values measured on reference captures of the original PC game, virtual units
 * (+-1 unless noted). They are not tunables in the sense of the entries above,
 * but live here so every number the spec now fixes sits in one header next to
 * the guesses it replaced. The tests in tools/ugui_test_amend3.inc check the
 * library against exactly these numbers.
 * ======================================================================== */

/* 6.4 icon scroller */
#define UG_R_ICON_SEL_W      118.0f   /* selected icon, centred (0, UG_R_ICON_CY) */
#define UG_R_ICON_SEL_H      59.0f
#define UG_R_ICON_NB_W       96.0f    /* neighbours, +-UG_TUNE_ICON_SPACING, full opacity, no backing */
#define UG_R_ICON_NB_H       48.0f
#define UG_R_ICON_CY         (-133.5f)
#define UG_R_FRAME_HW        60.0f    /* focus frame: line centres x +-60, y -161 / -106, ~2 thick */
#define UG_R_FRAME_TOP       (-161.0f)
#define UG_R_FRAME_BOT       (-106.0f)
#define UG_R_FRAME_LINE      2.0f
#define UG_R_ARROW_CX        270.0f   /* scroll-arrow badges centred (+-270, -132.5), visible disc ~23 */
#define UG_R_ARROW_CY        (-132.5f)
#define UG_R_BAND_X0         (-290.0f)  /* light icon band */
#define UG_R_BAND_X1         290.0f
#define UG_R_BAND_Y0         (-165.5f)
#define UG_R_BAND_Y1         (-103.3f)
#define UG_R_TRACK_Y0        (-176.0f)  /* horizontal scroller track, 8 thick (7.5 visible) */
#define UG_R_TRACK_Y1        (-168.5f)
#define UG_R_THUMB_X0        (-273.7f)  /* thumb centre x = X0 + SPAN * index / (N - 1) */
#define UG_R_THUMB_SPAN      547.4f
#define UG_R_THUMB_Y         (-171.0f)
#define UG_R_ITEM_TITLE_SCALE 0.7f      /* accent, centred x = 0, cap top y = -95 */
#define UG_R_ITEM_TITLE_CAPTOP (-95.0f)
#define UG_R_SCR_TITLE_SCALE 0.9f       /* accent, right aligned, ink right edge x 286.7, cap top -207.6 */
#define UG_R_SCR_TITLE_RIGHT 286.7f
#define UG_R_SCR_TITLE_CAPTOP (-207.6f)
#define UG_R_MAIN_ITEMS      7          /* main menu on PC */

/* 6.5 option list (plain, audio-page style) */
#define UG_R_ROW_TEXT_Y0     (-126.0f)  /* text centre of row k = -126 + 36 k */
#define UG_R_ROW_BACK_X0     (-286.0f)  /* focused backing x -286..+262, y -144.4..-109.6 (35 high) */
#define UG_R_ROW_BACK_X1     262.0f
#define UG_R_ROW_BACK_Y0     (-144.4f)
#define UG_R_ROW_BACK_H      34.8f
#define UG_R_ROW_NAME_X      (-269.0f)  /* name ink left edge, scale 0.7 */
#define UG_R_ROW_VALUE_X     127.0f     /* value text centre, scale 0.7 */
#define UG_R_ROW_ARROW_LX    6.0f       /* value arrows on the focused row only */
#define UG_R_ROW_ARROW_RX    246.0f
#define UG_R_SLIDER_X0       54.6f      /* option-row slider fill start; knob at 196.6 for the maximum */
#define UG_R_SLIDER_X1       196.6f
#define UG_R_SLIDER_FILL_H   7.0f
#define UG_R_VSCR_X0         269.0f     /* vertical scroller track x 269..277, y -147..+151 */
#define UG_R_VSCR_X1         277.0f
#define UG_R_VSCR_Y0         (-147.0f)
#define UG_R_VSCR_Y1         151.0f
#define UG_R_VSCR_ARROW_UP   (-163.5f)  /* arrow centres (273, -163.5) and (273, +167) */
#define UG_R_VSCR_ARROW_DOWN 167.0f
#define UG_R_VSCR_THUMB_TOP  (-130.0f)  /* thumb centre with the first row selected */
/* PC controls page variant (checklist 27, 28) */
#define UG_R_CTL_VISIBLE     7
#define UG_R_CTL_NAME_X      (-275.0f)  /* scale 0.5 */
#define UG_R_CTL_BACK_Y0     (-110.2f)  /* focused backing y -110.2..-85.3 */
#define UG_R_CTL_BACK_H      24.9f
#define UG_R_CTL_BAND_Y0     (-174.0f)  /* dark header band y -174..-142, caption scale 0.7, arrows x +-265.5 y -157 */
#define UG_R_CTL_BAND_Y1     (-142.0f)
#define UG_R_CTL_BAND_ARROW  265.5f
#define UG_R_CTL_BAND_ARROW_Y (-157.0f)

/* 6.17 car select (first car) and 6.11 stat bars */
#define UG_R_CAR_ARROW_RX    269.6f
#define UG_R_CAR_ARROW_RY    (-143.6f)
#define UG_R_CAR_THUMB_Y     (-188.0f)  /* scroller track y -192..-184 */
#define UG_R_CAR_LOGO_X0     (-223.0f)  /* manufacturer frame outer x -223..-92, y -178..-111 */
#define UG_R_CAR_LOGO_X1     (-92.0f)
#define UG_R_CAR_LOGO_Y0     (-178.0f)
#define UG_R_CAR_LOGO_Y1     (-111.0f)
#define UG_R_CAR_BADGE_X     93.0f      /* name badge centre (93, -144) */
#define UG_R_CAR_BADGE_Y     (-144.0f)
#define UG_R_CAR_FILTER_ARROW_L 95.0f   /* PC arrows at (95, -208) and (268, -208), caption centred x 185 */
#define UG_R_CAR_FILTER_ARROW_R 268.0f
#define UG_R_CAR_FILTER_Y    (-208.0f)
#define UG_R_CAR_FILTER_TEXT_X 185.0f
#define UG_R_TITLE_SCALE     0.8f       /* options / car select title: ink left -288, cap top -214 */
#define UG_R_TITLE_INK_LEFT  (-288.0f)
#define UG_R_TITLE_CAPTOP    (-214.0f)
#define UG_R_STAT_LABEL_SCALE 0.55f     /* ink left -280 / -77 / +121, cap top 157, fill centre y 176 */
#define UG_R_STAT_LABEL_Y    162.5f     /* text centre implied by cap top 157 (16.5: caps start 6 units into the cell) */
#define UG_R_STAT_FILL_Y     176.0f
#define UG_R_STAT_FILL_H     6.0f
#define UG_R_STAT_TRACK_LEN  170.0f     /* G within R: backing fades out about 165 right of the fill start */

/* 6.9 dialog (measured on SMALL and LARGE; MED derived from the same offsets) */
#define UG_R_DLG_BTN_DX      150.25f    /* visible button x = centre +-134.7, i.e. 15.5 .. 285 */
#define UG_R_DLG_BTN_HW      134.75f
#define UG_R_DLG_BTN_H       41.4f
#define UG_R_DLG_DIM         0.40f      /* brightness of everything beneath */
#define UG_R_DLG_TITLE_SCALE 0.75f
#define UG_R_DLG_LEADING     2.0f       /* authored text leading of the message: pitch (32 + 2) x 0.75 = 25.5 (16.5) */
#define UG_R_DLG_BTN_CAP_MARGIN 14.4f   /* transparent margin of the 42 x 42 end caps: authored 298 wide, 269.4 visible */
#define UG_R_DLG_TITLE_INK_LEFT (-289.0f)  /* no icon: starts at the light panel's left edge; cap top -198 on LARGE */

/* 7.4.1 PC help bar */
#define UG_R_PC_STRIP_TOP    189.2f     /* runs to the bottom edge (y 240; the quads are 51 high) */
#define UG_R_PC_STRIP_BOT    240.2f
#define UG_R_PC_LEFT_X0      (-300.4f)  /* 16 wide rounded end (16x16 corner over a 16-wide box) */
#define UG_R_PC_PLAYER_X0    (-284.4f)  /* player box -284.4 .. -151.8 */
#define UG_R_PC_PLAYER_X1    (-151.8f)
#define UG_R_PC_HINT_X0      (-148.9f)  /* hint box -148.9 .. +281.7 */
#define UG_R_PC_HINT_X1      281.7f
#define UG_R_PC_RIGHT_X1     297.6f
#define UG_R_PC_PLAYER_CX    (-225.7f)  /* player name centre, scale 0.5, grey 230 */
#define UG_R_PC_PLAYER_CY    212.2f
#define UG_R_PC_PILL_W       143.6f     /* visible pill outline */
#define UG_R_PC_PILL_H       18.7f
#define UG_R_PC_LABEL_DY     (-2.0f)    /* label centre = slot y - 2, scale 0.45 */
#define UG_R_PC_PLAYER_ALPHA 215        /* (128,128,128) at 215 on the black 60% box: dark (Q17) */
#define UG_R_PC_HINT_ALPHA   150        /* white at 150 on the black box */

#endif
