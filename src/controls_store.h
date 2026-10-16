/* controls_store.h -- persistence of the bindings (decision D4): the keyboard table in the OpenUG2 INI
 * store (settings.h), the joystick bindings in the device records of a profile (SPEC 6, CONTROLS_SPEC 6.6).
 * Pure: no SDL, no files except through the settings backend. Design log: CONTROLS_IMPL_DESIGN.md. */
#ifndef OPENUG2_CONTROLS_STORE_H
#define OPENUG2_CONTROLS_STORE_H

#include "controls_bind.h"
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CS_KB_VALUE "ControlsKeyboard"   /* INI value name of the keyboard table */
#define CS_PROFILE_NAME "OPENUG2"        /* profile that carries the joystick device records */

/* Writes the keyboard keys (ST_OK or a settings error). */
int cs_kb_save(StRegStore *s, const CbKeyTable *t);
/* Reads them back. Returns 1 when a well-formed value replaced the table, 0 when it is absent or malformed
 * (the table is then left unchanged). Keys outside the scancode range are dropped; a key held by several
 * entries stays with the first. */
int cs_kb_load(StRegStore *s, CbKeyTable *t);

/* Copies the record of every present pad into the profile (matched by GUID, the n-th pad with a GUID takes
 * the n-th record with it; a pad without a record takes a free slot from 1 on; slot 0 is the keyboard's). */
void cs_pads_store(StProfile *p, const CbBindings *b);
/* Loads the matching records into the present pads; an invalid record leaves the defaults. Returns the
 * number of pads that took a record. */
int cs_pads_load(const StProfile *p, CbBindings *b);

#ifdef __cplusplus
}
#endif
#endif
