/* gui_timing.c -- see gui_timing.h. Data: SPEC_GUI_ART 1.4 (authored enter/exit scripts of the
 * event-handler object per package). */
#include "gui_timing.h"

#include <stddef.h>
#include <strings.h>

static const GtPackage k_packages[] = {
    {"UI_pause.fng",
     {"call", 320, 2, {{0xBDFD2DF9U, GT_TO_TARGETS, 0}, {0x37EDF39BU, GT_TO_APP, 320}}},
     {"Forward", 320, 2, {{0xAADD0187U, GT_TO_TARGETS, 0}, {0xE1FDE1D1U, GT_TO_APP, 320}}},
     320},
    {"UI_PauseOptionsMain.fng",
     {"CALL", 320, 2, {{0xBDFD2DF9U, GT_TO_TARGETS, 0}, {0x37EDF39BU, GT_TO_APP, 320}}},
     {"Forward", 320, 2, {{0xAADD0187U, GT_TO_TARGETS, 0}, {0xE1FDE1D1U, GT_TO_APP, 320}}},
     320},
    {"UI_PauseOptions.fng",
     {"CALL", 320, 2, {{0xBDFD2DF9U, GT_TO_TARGETS, 0}, {0x35F8620BU, GT_TO_APP, 320}}},
     {"FORWARD", 320, 3, {{0xAADD0187U, GT_TO_TARGETS, 0}, {0x84378BEFU, GT_TO_APP, 0}, {0xE1FDE1D1U, GT_TO_APP, 320}}},
     320},
    {"UI_Options.fng",
     {"call", 500, 2, {{0xBDFD2DF9U, GT_TO_TARGETS, 0}, {0x35F8620BU, GT_TO_APP, 320}}},
     {"Forward", 500, 3, {{0xAADD0187U, GT_TO_TARGETS, 0}, {0x84378BEFU, GT_TO_APP, 0}, {0xE1FDE1D1U, GT_TO_APP, 140}}},
     140},
    {"UI_PC_Customize_Options.fng",
     {"call", 500, 2, {{0xBDFD2DF9U, GT_TO_TARGETS, 0}, {0x35F8620BU, GT_TO_APP, 320}}},
     {"FORWARD", 500, 3, {{0xAADD0187U, GT_TO_TARGETS, 0}, {0x84378BEFU, GT_TO_APP, 0}, {0xE1FDE1D1U, GT_TO_APP, 140}}},
     140},
    {"UI_InGameDialog.fng",
     {"call", 180, 2, {{0xBDFD2DF9U, GT_TO_ALL, 0}, {0x35F8620BU, GT_TO_APP, 180}}},
     {"FORWARD", 140, 2, {{0xAADD0187U, GT_TO_ALL, 0}, {0xE1FDE1D1U, GT_TO_APP, 140}}},
     140},
    {"GenericDialog_SMALL.fng",
     {"call", 180, 2, {{0xBDFD2DF9U, GT_TO_ALL, 0}, {0x35F8620BU, GT_TO_APP, 180}}},
     {"Forward", 140, 2, {{0xAADD0187U, GT_TO_ALL, 0}, {0xE1FDE1D1U, GT_TO_APP, 140}}},
     140},
};
#define N_PACKAGES ((int)(sizeof k_packages / sizeof k_packages[0]))

const GtPackage *gt_find(const char *package) {
    if (!package) return NULL;
    for (int i = 0; i < N_PACKAGES; i++)
        if (!strcasecmp(k_packages[i].package, package)) return &k_packages[i];
    return NULL;
}
int gt_count(void) { return N_PACKAGES; }
const GtPackage *gt_at(int i) { return i >= 0 && i < N_PACKAGES ? &k_packages[i] : NULL; }
int gt_exit_complete_ms(const char *package) {
    const GtPackage *p = gt_find(package);
    return p ? p->exit_complete_ms : 0;
}
uint32_t gt_exit_script_hash(const GtPackage *p) {
    uint32_t h = 0xFFFFFFFFu;                       /* name hash: h = h*33 + toupper(c) */
    if (!p) return 0;
    for (const char *s = p->exit.name; *s; s++)
        h = h * 33u + (uint32_t)((*s >= 'a' && *s <= 'z') ? *s - 32 : *s);
    return h;
}
