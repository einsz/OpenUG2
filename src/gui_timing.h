/* gui_timing.h -- authored enter/exit timing of the screen packages (SPEC_GUI_ART 1.4, tvg11).
 *
 * Pure data, no GL/SDL. Every retail package has an event-handler object (name hash 0x47FF4E7C)
 * whose Init script chains to an enter script; the application starts the exit script by name and
 * waits for "exit complete" (0xE1FDE1D1) before it switches package or performs a deferred action.
 * The state machines use `exit_complete_ms` as their clock; the presentation layer starts the
 * authored exit script on the handler object in parallel. Design log: IMPL_DESIGN_GUI.md. */
#ifndef OPENUG2_GUI_TIMING_H
#define OPENUG2_GUI_TIMING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GT_HANDLER_HASH      0x47FF4E7Cu   /* event-handler object name, every package */
#define GT_MSG_ENTER         0xBDFD2DF9u   /* objects run FadeIn */
#define GT_MSG_EXIT          0xAADD0187u   /* objects run FadeOut */
#define GT_MSG_EXIT_COMPLETE 0xE1FDE1D1u
#define GT_MSG_ENTER_PAUSE   0x37EDF39Bu   /* enter-complete of the pause/hub packages */
#define GT_MSG_INIT_COMPLETE 0x35F8620Bu   /* enter-complete of the other packages */
#define GT_MSG_EXIT_START    0x84378BEFu
#define GT_MSG_UNDIM_COMPLETE 0x0153E234u
#define GT_SCRIPT_DIM        0x00009E99u   /* handler script "DIM" */
#define GT_SCRIPT_UNDIM      0x03D8EABCu   /* handler script "UNDIM" */

typedef enum { GT_TO_TARGETS = 0, GT_TO_ALL, GT_TO_APP } GtTarget;

typedef struct GtEvent {
    uint32_t msg;
    GtTarget target;
    int t_ms;
} GtEvent;

typedef struct GtScript {
    const char *name;            /* script name as authored (matched case-insensitively) */
    int length_ms;
    int nevents;
    GtEvent events[4];
} GtScript;

typedef struct GtPackage {
    const char *package;         /* lookup name, e.g. "UI_pause.fng" */
    GtScript enter, exit;
    int exit_complete_ms;        /* time of the GT_MSG_EXIT_COMPLETE event of `exit` */
} GtPackage;

const GtPackage *gt_find(const char *package);           /* case-insensitive; NULL when unknown */
int gt_count(void);
const GtPackage *gt_at(int i);
/* Exit duration the state machines wait for; 0 for an unknown package. */
int gt_exit_complete_ms(const char *package);
/* Hash of the exit script's name (what the application starts on the handler object). */
uint32_t gt_exit_script_hash(const GtPackage *p);

#ifdef __cplusplus
}
#endif
#endif
