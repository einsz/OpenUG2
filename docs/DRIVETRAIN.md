# Stock drivetrain opt-in

Run the independent native OpenUG2 executable with `--drivetrain` to select the
approved bounded Stage1–5 engine, clutch, automatic gearbox, tyre, service brake,
steering, resistance, suspension-contact and handbrake composition. The default
without that flag remains the legacy model. Drag/drift race families use legacy
physics because their alternate producers are outside these specifications.

W accelerates forward; S brakes forward motion and selects reverse near rest.
W brakes reverse motion before returning to automatic forward gears. Space now
requests the specified rear handbrake torque and frame grip blend. It does not
hold body velocity, erase yaw or assign wheel spin. Initial locks are explicitly
supplied as zero; release reevaluates them through the Stage3 motion predicate.
Engine sound, HUD RPM/gear, wheel spin/steering, pause, reset, collision velocity
and car switching continue to use the composed simulation state. Stock tuning
is required at loading and switching; malformed fields reject the candidate.
Upgrades, nitrous and alternate race handling are outside this opt-in.

During a forward/reverse handoff, the host withholds accelerator input while
either the actual gear or its pending target opposes the requested direction.
The selector and clutch continue their normal transition; service braking still
opposes actual motion. This independent keyboard policy can delay acceleration
until the next 60 Hz tick after a handoff. Existing wheel/engine energy and the
preceding prepared force remain in the simulation, so it does not erase motion
instantly or establish a fix for every intermittent post-braking stall.

## Contact and body boundary

Stage5 computes frame preload from stock mass, centre, wheel mounts and the
preceding aero-enable/signed-speed snapshot. Ordered wheel queries produce
travel/rate, progressive spring/damping/paired loads, contact flags, alignment,
contact points, and distinct tyre/spin/body surface channels. The aligned load
feeds every tyre/brake-lock/cap calculation. Final world forces and all three
moments are available in `Stage1Drive.contact_force/contact_moment` and the body
accumulators. Gravity, actual aero and each wheel force enter those accumulators
once. Preload's separate specified aero feed-forward is not the reported aero
load added again.

The playable body is a prescribed-ride host. Existing independent ride heave,
pitch, roll and rates supply the vertical/angular trajectory. It integrates XY
momentum and yaw from the full transformed wheel forces/moments. Within a tick,
ride height/pitch/roll are extrapolated from their supplied rates; cosmetic
`body_roll` is never a contact input. World vertical force and pitch/roll moments
are calculated but do not drive that ride trajectory. Its own heave/gravity
remains the sole producer of playable vertical motion. Consequently **dynamic
braking/throttle pitch and full 3D load-transfer feedback are not implemented**.
Calculated loads change with actual road/ride/contact geometry. Prescribed
nose-down/nose-up/roll tests demonstrate the consequences of those poses,
not a claim that the pedals produce them dynamically.

The host identifies XY as body-centre coordinates. Its vertical body centre is
`carpos.z + stock centre.z - mean(stock mount.z - radius)`, aligning the mean
uncompressed wheel bottom with the independent ride reference on a level plane.
This is an explicit host coordinate bridge. Stock centre subtraction still sets
all mount/coupling arms. The stock unscaled box yaw inertia is the supplied host
mode; Stage5 exposes both specified inertia diagonals for other callers.
Additional yaw damping is explicitly zero. Full angular mode/damping/momentum
and impulse caps remain unknown. This host selects global bottoming-impulse
suppression, reports requests, and still runs the required extra zero sweep.
The component API supports separate supplied linear/angular impulse caps.

The external world adapter uses independent `world_wheel_support` selection,
unit triangle normals, an upward 0.25 m window and a downward window at least
stock droop/current host swept-fall reach. Missing queries return valid default
normal and numeric surface as explicit host policy. Every selected triangle is
assigned **numeric ID 1 as a fixture**; no ROAD/TERRAIN/name-to-retail-ID mapping
is claimed. Injected queries can provide all 29 specified IDs. Peak, sliding,
spin drag, body drag and bump channels remain separate even when fixture 1 makes
them unit/zero. Immediate wheel preparation uses fresh IDs; frame body drag uses
the previously exported contact/ID snapshot.

## Scheduling and supplied choices

The application retains its independent fixed 60 Hz accumulator with fractional
residual, bounded catch-up, and paused time discarded. Each tick follows the
specified substeps. Its first derivative consumes the preceding prepared forces;
new pedals, missing terrain or surfaces affect subsequent preparations. Frame
preload precedes mode refresh; control exclusivity occurs once, then corrected
signed brake distribution and handbrake/grip refresh once. No extra Euler step
occurs for the bottoming zero sweep. Export happens after all substeps.

Other host inputs are ordinary state 0, response scale 1, supplied reduction 0,
base grip 1, drafting/event/calibration inputs zero, no damage/bonus/launch,
initial locks zero and relief enabled. The original timed steering-history
producer remains unknown; the independent host retains 33-sample input and
nine-sample rate box histories at 60 Hz. Reset preserves tuning/query/environment
configuration and clears travel/rate, blend, histories, yaw, locks and preparation
using their explicit creation/reset fixtures. Lower Stage1–4 standalone adapters
remain available to preserve their approved supplied-input fixture boundaries;
the application opt-in configures Stage5.

## Repeatable checks

```sh
make contact-test drive-stage5-test lateral-test drive-stage4-test braking-test \
  resistance-test drivetrain-test drive-stage1-test physics-clock-test district-collision-test
make debug
DISPLAY=:0 SDL_AUDIODRIVER=dummy ./nfsu2 '/path/to/your/NFSU2 installation' \
  --car 240SX --drivetrain --track STREAML4RA --resolution 640x360 \
  --drive-audit /tmp/stage5 --frames 420
```

The bounded native audit drives the normal controls and world queries. With
`--drivetrain`, ticks 170–184 request moving handbrake and a small steer input,
then release; steering pulses occur at 210–239 and 255–284; service braking at
300–344 is followed by acceleration. No physical state is forced. `STAGE5` rows
show handbrake torque, blend/grip, contact mask, travel, aligned loads, prescribed
ride pose, body forces/moments, signed speed and wall/rail counts. Later scripted
segments can meet obstacles; S near rest may select reverse. `--frames` controls
the bound (default audit 2,220). The audit writes spawn/end captures under the
selected prefix. Dummy audio may be omitted for normal interactive sound.

For an intermittent problem during manual driving, record a CSV capture:

```sh
./nfsu2 '/path/to/your/NFSU2 installation' --car 240SX \
  --drivetrain-trace /tmp/240sx-driving.csv
```

The trace option enables the stock drivetrain and keeps W/S/Space and steering
under your control. Reproduce the problem, wait a few seconds, then quit normally.
It writes one row per simulation tick, flushes every second of simulated time,
and replaces the named file. Pause produces no rows. Each row includes pedal,
gear/clutch/RPM, wheel spin/lock/torque, suspension travel/load, tyre forces and
ground/wall/rail/vehicle collision observations. `host_vx/host_vy` are the final
world velocity after collisions; `body_vx/body_vy` are the drivetrain's velocity
before those host responses. `incoming_f*` is the previous preparation consumed
at tick entry, while `prepared_f*` is the next prepared force at tick exit.
Forces are kN, torques kN·m, velocities m/s, angular speeds rad/s and travel m.
`active=0` marks a legacy race mode where drivetrain fields are inactive snapshots.
The capture reads only independent simulation state; it does not dump car assets.
`make drive-trace-test` checks the CSV schema and that recording leaves state intact.

All vectors are synthetic calculations from static-inferred approved contracts.
Component agreement, host tests and native captures do not establish original
runtime/differential, full world/material or full 3D handling parity. Detailed
architecture and validation are in
[Stage5 implementation design](cleanroom-driving-physics/stage5/IMPL_DESIGN.md).
