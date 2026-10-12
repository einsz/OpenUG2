/* Original synthetic tests of OpenUG2's public scheduling contract.
 * No retail data or recovered game implementation is used. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "physics.h"

#ifdef NDEBUG
#error "physics_clock_test requires assertions; remove -DNDEBUG"
#endif

typedef struct {
    float position[3], velocity[2], heading, speed, steering;
    int ticks;
} DriveTrace;

static void drive_tick(DriveTrace *trace) {
    /* Inputs belong to simulation ticks, independent of render cadence. */
    int tick = trace->ticks++;
    float target = tick < 120 ? 0 : tick < 240 ? .4f : tick < 360 ? -.4f : 0;
    float throttle = tick < 420 ? 1 : tick < 480 ? -1 : 0;
    trace->steering = phys_steer_response(trace->steering, target);
    phys_drive_step(trace->position, trace->velocity, &trace->heading,
                    &trace->speed, throttle, trace->steering,
                    tick >= 480 && tick < 540, NULL, NULL, NULL);
    for (int i = 0; i < 3; ++i) assert(isfinite(trace->position[i]));
    for (int i = 0; i < 2; ++i) assert(isfinite(trace->velocity[i]));
    assert(isfinite(trace->heading) && isfinite(trace->speed));
}

static void drive_frame(PhysClock *clock, DriveTrace *trace, double elapsed) {
    int steps = phys_clock_steps(clock, elapsed);
    assert(steps >= 0 && steps <= 8);
    for (int i = 0; i < steps; ++i) drive_tick(trace);
}

static void same_trace(const DriveTrace *a, const DriveTrace *b) {
    assert(a->ticks == b->ticks);
    /* Same tick sequence must produce identical floats on this build. This
       does not require identical results across compilers or architectures. */
    assert(!memcmp(a->position, b->position, sizeof a->position));
    assert(!memcmp(a->velocity, b->velocity, sizeof a->velocity));
    assert(a->heading == b->heading && a->speed == b->speed);
    assert(a->steering == b->steering);
}

static void test_render_cadence(void) {
    DriveTrace reference = {0};
    for (int i = 0; i < 600; ++i) drive_tick(&reference);
    assert(hypotf(reference.position[0], reference.position[1]) > 1);
    const int rates[] = {30, 60, 90, 144, 240};
    for (unsigned i = 0; i < sizeof rates / sizeof rates[0]; ++i) {
        PhysClock clock = {0};
        DriveTrace actual = {0};
        for (int frame = 0; frame < 10 * rates[i]; ++frame)
            drive_frame(&clock, &actual, 1.0 / rates[i]);
        same_trace(&reference, &actual);
        assert(fabs(clock.remainder) < 1e-9);
    }
    /* Irregular frame batches, including a frame that cannot produce a tick. */
    PhysClock clock = {0};
    DriveTrace actual = {0};
    const double tick = 1.0 / PHYS_TICKRATE;
    for (int batch = 0; batch < 100; ++batch) {
        drive_frame(&clock, &actual, .25 * tick);
        drive_frame(&clock, &actual, 2.50 * tick);
        drive_frame(&clock, &actual, 3.25 * tick);
    }
    same_trace(&reference, &actual);
    assert(fabs(clock.remainder) < 1e-9);
}

static void test_invalid_time_and_stalls(void) {
    const double tick = 1.0 / PHYS_TICKRATE;
    PhysClock clock = {.remainder = .5 * tick};
    const double invalid[] = {0, -tick, NAN, INFINITY, -INFINITY};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        double saved = clock.remainder;
        assert(phys_clock_steps(&clock, invalid[i]) == 0);
        assert(clock.remainder == saved);
    }
    assert(phys_clock_steps(&clock, 10) == 8);
    assert(fabs(clock.remainder - .5 * tick) < 1e-12);
    assert(phys_clock_steps(&clock, .5 * tick) == 1);
    assert(fabs(clock.remainder) < 1e-12);
    /* Discarded stall time must not cause a later catch-up burst. */
    assert(phys_clock_steps(&clock, tick) == 1);
    assert(fabs(clock.remainder) < 1e-12);
}

int main(void) {
    phys_selftest();
    test_render_cadence();
    test_invalid_time_and_stalls();
    puts("physics_clock_test: PASS (synthetic OpenUG2 contracts; no retail equivalence claim)");
    return 0;
}
