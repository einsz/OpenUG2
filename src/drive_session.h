/* OM-1/2a: independent local-host lifecycle policy. No event-family numbers,
 * pedals, contacts or vehicle/driver selectors participate in this boundary. */
#ifndef OPENUG2_DRIVE_SESSION_H
#define OPENUG2_DRIVE_SESSION_H
typedef enum {
    DRIVE_SESSION_SUPPLIED, /* pass an actual snapshot (or fixture) unchanged */
    DRIVE_SESSION_ACTIVE,
    DRIVE_SESSION_COUNTDOWN,
    DRIVE_SESSION_SUSPENDED, /* no dynamics; state is retained for resumption */
    DRIVE_SESSION_RETAIN /* excluded route: retain established host scheduling/status */
} DriveSessionPhase;
typedef struct { DriveSessionPhase phase;int status; } DriveSession;
/* Ready means body initialized AND preceding force/torque preparation complete.
 * A supplied snapshot is independent of this host policy's readiness test. */
static inline int drive_session_status(DriveSession session,int ready,int *status) {
    if(session.phase==DRIVE_SESSION_SUPPLIED){*status=session.status;return 1;}
    if(!ready)return 0;
    if(session.phase==DRIVE_SESSION_ACTIVE){*status=4;return 1;}
    if(session.phase==DRIVE_SESSION_COUNTDOWN){*status=3;return 1;}
    return 0;
}
/* The application has its own countdown clock and scheduling. Other routes
 * retain the previously supplied selector; this assigns them no new status. */
static inline DriveSession drive_session_local(int active,int countdown) {
    return (DriveSession){active?DRIVE_SESSION_ACTIVE:countdown?DRIVE_SESSION_COUNTDOWN:DRIVE_SESSION_RETAIN,0};
}
#endif
