# DCLink mission planning contract

The version-2 mission format supports waypoint routes, ordered rules and
geographic behavior areas in the shared QGC and Maintenance library. The running
DCLink planner evaluates the saved revision. The IPC and MAVLink exchange stays
at protocol version 1.

## Ownership

DCLink owns the saved mission library, validation, revisions, and execution.
QGC and Maintenance use the same validated, revisioned SQLite mission library:

```text
QGC Plan view -- MAVLink --> running DCLink mission service --+
                                                            +--> SQLite library
Maintenance web server --> local DCLink filesystem helper ---+
```

Maintenance invokes the installed DCLink Python helper from its authenticated
web server, following its configuration-file helper pattern. It does not open a
runtime socket or require the DCLink process to be running. The helper and
runtime reuse DCLink's storage validation and transaction functions. Neither
frontend writes SQL or mission configuration files itself.
Saving a mission never starts or changes a flight.
Editing, importing, listing, loading, and saving do not require an autopilot.

The shared library supports named waypoint routes, multiple saved missions, and
rules that react to waypoint arrivals or progress along a leg. A rule can
require a current detection, aircraft presence inside a drawn area, or both.

## Mission document, identity, and limits

[mission.schema.json](mission.schema.json) defines the JSON mission content.
[examples/route.json](examples/route.json) is synthetic, not an operational route.

| Field | Contract |
| --- | --- |
| `schema_version` | Integer `2` for new and editable missions. |
| `name` | 1–80 Unicode code points, without control characters or leading/trailing whitespace. Names are labels, not identity. |
| `altitude_reference` | `relative_home`: metres above the admitted aircraft HOME altitude, not terrain clearance or height above the launch map point. |
| `waypoints` | Ordered list of 0–1000 waypoints. An empty mission is a saved draft and cannot execute. |
| Waypoint `id` | Lowercase UUIDv4, unique within this mission and stable through coordinate edits and reordering. |
| `latitude_deg`, `longitude_deg` | WGS84 degrees in [-90, 90] and [-180, 180]. Zero is valid. |
| `altitude_m` | Finite JSON number in metres relative to HOME. Execution checks configured flight limits separately. |
| `acceptance_radius_m` | Finite positive metres for that waypoint. The editor initially offers 30 m and exposes the value. |
| `rules` | Ordered list of 0–16 rules. An empty list gives normal waypoint navigation. |
| `areas` | Ordered list of 0–8 geographic behavior areas. |

All listed fields are required. Unknown fields, duplicate JSON object keys,
non-finite numbers, and unsupported versions are invalid on save. The schema
validates structure. The service also checks unique waypoint, rule and area IDs,
waypoint and area references, polygon geometry, command compatibility, the wait
budget, and a maximum encoded document size of 262144 UTF-8 bytes.
The same limits apply to IPC and MAVLink. No production JSON Schema dependency
is required: each implementation may validate these rules with existing tools.

A saved record is an envelope, separate from editable content:

```json
{
  "mission_id": "44444444-4444-4444-8444-444444444444",
  "revision": 1,
  "mission": {
    "schema_version": 2,
    "name": "Example",
    "altitude_reference": "relative_home",
    "waypoints": [],
    "rules": [],
    "areas": []
  }
}
```

DCLink assigns the mission UUIDv4 and revision 1 on create. Revisions are positive
uint32 values, increase on every successful update, and never wrap. Identity is
scoped to the selected DCLink service; identical labels do not merge missions.
Clients distinguish duplicate labels with a short ID in the picker when needed.
Waypoint, rule and area IDs are scoped to their mission and may be retained
when making a copy.
At revision exhaustion, reject updates with `busy` and reason
`revision_exhausted`; the operator can save a copy with a new identity.

Updates replace the complete mission content atomically and require the exact
current `expected_revision`. Only a successful committed save changes the record
or revision. A failed validation, transfer, or conflict leaves the record intact.
Use one serialized service mutation path for both adapters. Storage replacement
must be durable before reporting success. A reader sees a complete saved record.

Other schema versions, including version 1, may be read and inspected as
unsupported library entries. Editing or starting a mission requires version 2.

## Shared service operations

Runtime IPC and MAVLink use the following logical request and result envelopes.
The IPC adapter maps them onto its existing request/response envelope; MAVLink
maps them onto addressed requests and bounded data transfers. Maintenance uses
the separate local-helper identity profile below with the same mission schema
and supported storage operations.

```json
{
  "client_id": "55555555-5555-4555-8555-555555555555",
  "request_id": "66666666-6666-4666-8666-666666666666",
  "service_session": "77777777-7777-4777-8777-777777777777",
  "action": "missions.get",
  "payload": {
    "mission_id": "44444444-4444-4444-8444-444444444444"
  }
}
```

UUIDs above are synthetic. `client_id` identifies one client process session
and survives its transport reconnects; it is not authentication.
`request_id` identifies one logical operation, independent of command request IDs.

| Action | Payload | Successful result data |
| --- | --- | --- |
| `missions.capabilities` | Empty; the discovery request omits `service_session`. | Session, versions, supported operations, features, limits, readiness, and the mission command catalog when supported. |
| `missions.list` | Optional opaque `cursor`, `limit` from 1 to 50 (default 50). | `items` of `{mission_id, revision, name, schema_version, waypoint_count}`, `next_cursor` or null. |
| `missions.get` | `mission_id`. | Latest complete saved record. |
| `missions.create` | `mission` content. | `{mission_id, revision: 1}`. |
| `missions.update` | `mission_id`, `expected_revision`, complete `mission`. | `{mission_id, revision}` for the committed content. |
| `missions.delete` | `mission_id`, `expected_revision`. | Deleted ID and revision. An admitted run retains its own snapshot. |
| `missions.import_route` | `name`, optional positive `acceptance_radius_m`. | `{mission_id, revision: 1}` for a one-time import of DCLink's stored route. |
| `missions.start` | `mission_id`, exact saved `revision`, and `vehicle_system_id` (1–255). | `{run}` with a new run ID and queued state, pending runtime admission. |
| `missions.run_status` | Empty. | `{fresh, execution, run}`. Latest run on this runtime boot or null, plus current start readiness. |
| `missions.cancel` | Exact `run_id`. | `{run}` with cancellation requested, or the retained state of that cancellation. |

Rename is an update of `name`. Duplicate/Save as copy is a create using the draft
content and a chosen name. There is no mutable service-wide selected mission.

Import reads the fixed route source owned by DCLink; clients cannot supply a
filesystem path. It accepts only representable HOME-relative waypoint records
and preserves each positive stored radius. Zero or missing radii require the
explicit supplied radius. Unsupported commands, frames, holds, yaw, pass radii,
or continuation behavior reject the whole import. The source file is preserved.
The import marker and mission commit together; another import returns `conflict`
with `reason: "route_already_imported"` and the original `mission_id`. The marker
survives restarts and deletion of that mission. Import never changes execution.

List cursors bind a consistent catalog generation and stable mission-ID order.
If another client changes the catalog between pages, the service returns
`conflict`; the client restarts the list rather than mixing generations.
The read limit bounds each page, not the number of saved missions.

Every result echoes `client_id`, `request_id`, `service_session`, and `action`,
and includes `result` plus `data`. Result codes are `ok`, `not_found`,
`invalid_value`, `unsupported`, `conflict`, `busy`, `stale_session`, and `error`.
Validation failures include `{path, message}` entries using JSON Pointer paths.
A revision conflict includes `current_revision`; it never returns success for
an overwritten edit. A storage failure reports `error` without claiming a save.
Mission content is capped at 262144 bytes; a complete logical result is capped
at 264192 bytes to allow its identity and revision envelope. Limit validation
details to fit and set `data.errors_truncated` when some errors are omitted.
Discovery returns the current `service_session` because its request has none.
A `stale_session` response echoes the requested session for correlation and
includes `data.current_service_session`; clients rediscover rather than treating
that response as a successful operation in the new session.

Capabilities return `protocol_version: 1`, `schema_versions: [2]`, the current
supported `operations`, and the features `waypoint_routes` and `waypoint_rules`.
`mission_detection_rules` advertises support for the optional condition and
`leg_progress` trigger within version 2. Clients require this feature to save
documents using either addition. `mission_behavior_areas` advertises support
for area definitions, rule references, and area status. Clients require it to
save any document with areas or rules that reference them.
`mission_command_steps` advertises the `command` step and adds a
`mission_commands` array of `{id, label}` to capability discovery. `id` is the
canonical saved command ID and `label` is the pilot-facing name. The catalog is
available for planning without an autopilot and is independent of start
readiness. Editors populate their command choices from this catalog. Saving or
retrying a write with command steps, including Save as copy, requires the
feature and each referenced command in the current catalog. DCLink validates
each command against its mission catalog when accepting the document. Earlier
version 2 documents remain valid.
The service advertises `max_document_bytes: 262144`, `max_waypoints: 1000`,
`max_name_characters: 80`, `max_list_page: 50`, and a MAVLink `max_chunk_bytes`
no greater than 160.
IPC additionally reports `max_result_bytes: 264192` and the bounded
`max_session_mutations`. The IPC-only delivery advertises `transports: ["ipc"]`
and omits MAVLink chunk support. `stored_route_import` advertises the explicit
import operation. Execution capabilities contain `supported`, `ready`,
`reason` and, when supported, `vehicle_system_id`. Execution operations are
advertised independently of library
operations. Unimplemented features are omitted. Clients never infer support
from a connection, a vehicle heartbeat, or the presence of a schema file.

### Concurrency, retries, and reconnects

DCLink issues a random UUIDv4 `service_session` at service start. All operations
after discovery target that session. The service checks this before admitting a
request; old-session requests receive `stale_session` without mutation. Clients
correlate replies with endpoint, source, session, client, request, and action.

Within a service session, repeat mutation requests with the same identity and
identical content return the retained result, including the original assigned
mission ID. Different content under a retained request ID is invalid. Retain
completed mutation identities for the entire session; use a bounded cache and
reject new mutations with `busy` when full instead of evicting replay protection.
Pending identical requests join the same operation and cannot commit twice.
Cancel instead uses the run ID as its idempotence boundary. A repeat can only
read or request cancellation of that same run.

Read requests can retry. A library mutation can retry with its original identity
while the service session is unchanged. A client timeout means outcome unknown,
not failure: retain the draft and identity while reconciling the result. After
a service-session change, refresh the library and require an operator decision
before sending an uncertain mutation as a new operation. Never replay a pending
save or execution request automatically across a service restart.

Conflicts present **Reload** or **Save as copy**. Preserve local work until the
operator chooses; do not merge route edits or overwrite another revision
automatically. A new local draft may have no endpoint, mission ID, or base
revision until the operator explicitly selects a destination. A loaded draft
retains its endpoint, mission ID, and saved base revision. A copy has no saved
identity/base revision and is created only on Save. Switching aircraft or
service must not silently send a bound draft to another destination.

## MAVLink mapping for the transfer phase

The [MAVLink transport profile](mavlink.md) defines exact layouts, identities,
exchange stages, integrity, limits and retry behavior for the three implemented
message families in `dclink_mavlink_dialect.xml`. Existing status, commands,
parameters and target-view messages retain their layouts. Mission identity and
content are not encoded in command names.

| Family | Purpose |
| --- | --- |
| `DCLINK_MISSION_REQUEST` | Discovery, service operation, transfer stage, read offset, or abort. |
| `DCLINK_MISSION_RESULT` | Correlated outcome, metadata, transfer descriptor, or acknowledged offset. |
| `DCLINK_MISSION_DATA` | Explicit-length bytes at an offset in a bounded transfer. |

Execution operations are logical requests over the same addressed exchange.
They are advertised by a service with a running planner.

Requests and replies carry destination system/component fields and service,
client, and request identities. MAVLink routing uses unique source identities
and the selected endpoint. Reject broadcasts for mutations and ignore messages
addressed to another peer. Replies from another service cannot replace a draft.
These routing identities do not replace existing transport access controls.

Encode mission content as strict UTF-8 JSON; transfer exact bytes without
compression. A descriptor binds a random transfer ID, operation identity,
mission ID/revision where applicable, byte length, and SHA-256 digest. The digest
checks complete transfer integrity; it is not authentication or authorization.
The server snapshot used for a download remains fixed for that transfer.
Structured result bodies that exceed a result packet, including catalog pages,
capabilities, and validation details, use the same bounded download mechanism.

Use a simple bounded, stop-and-wait exchange first:

1. Begin a read or staged write and receive its descriptor. A staged write binds
   create/update, the logical request identity, and the expected saved revision.
2. Transfer at most the advertised chunk size per exchange. Chunks specify offset
   and actual byte length, including trailing zero bytes. Acknowledge the next
   expected offset. Repeated identical chunks are harmless; conflicting bytes or
   invalid ranges fail the transfer. Correlate each exchange with its stage and
   offset so a delayed acknowledgement cannot advance the wrong chunk.
3. Commit a write only after exact length, digest, schema, semantic validation,
   and a fresh expected-revision check. A competing save during upload produces
   a conflict at commit. Only then invoke the shared create/update operation.
4. Abort or expire incomplete transfers without modifying saved content. Retain
   completed mutation results so repeating begin/commit returns the same outcome.

Allow one upload and one download per client, with service-wide bounded staging
and explicit `busy` responses. Expire an idle transfer after 30 seconds. Initial
clients use a 2-second exchange timeout and at most three resends of the same
exchange while the service session is unchanged. Timeout ends the transfer UI;
a timed-out commit remains uncertain and is reconciled as described above.
Background transfer work yields to command/status traffic and the runtime loop.

The transport profile is authoritative for its packet stages and bounds.
Consumers generate their language bindings from the same pinned definitions.

## Maintenance local-helper profile

Maintenance invokes `python -B -m src.tools.mission_library` from the selected
DCLink installation. One bounded strict UTF-8 JSON object arrives on stdin and
one correlated JSON object is returned on stdout. The helper opens only the
mission library; it does not start DCLink, connect to a vehicle, or use IPC.

| Request field | Meaning |
| --- | --- |
| `action`, `payload` | The storage operation and its payload from the shared operation table. |
| `client_id`, `request_id` | Lowercase UUIDv4 identities retained for an explicit retry. |
| `library_id` | Stable SQLite-library UUID, omitted only for capabilities discovery. |

Capabilities advertise `transports: ["local_helper"]` and the six operations
capabilities, list, get, create, update and delete, with `schema_versions: [2]`
and the `waypoint_rules`, `mission_detection_rules`, `mission_behavior_areas`
and `mission_command_steps` editing features. The helper advertises the shared
`mission_commands` catalog and validates and stores the rules and areas. The
running planner evaluates them during a run.
The response echoes action, client/request IDs, `library_id`, `result` and
`data`. Input is limited to 270336 bytes and logical output to 264192 bytes.
A replaced library with a different identity returns `stale_library` and its
current ID without admitting the write.
`library_id` identifies stored data and survives process restarts; it is separate
from the runtime boot's `service_session` and is not a MAVLink field.

The default library is `<installed DCLink>/config/mission-library.sqlite3`.
It contains named mission documents, revisions, catalog generation, the stable
library identity and compact helper mutation receipts. The helper commits the
mutation and its receipt in one SQLite transaction. Repeating an identical
client/request identity returns that retained result across helper or runtime
restarts; different content under the same identity is rejected. A missing reply
requires an explicit original-request retry or operator review, never automatic
creation of another mission. Runtime MAVLink/IPC retain their boot-scoped replay
rules above.

SQLite serializes helper and runtime writes and checks the expected revision in
the write transaction. A successful save reply follows commit; there is no
shutdown-time mission flush. A concurrent edit produces a conflict and leaves
the losing draft intact. QGC sees helper changes on its next read/Refresh, and
Maintenance sees QGC changes on its next read. Refreshing does not replace local
draft content; Reload requires an explicit discard decision.

QGC retains its drafts in its application-data file; Maintenance retains drafts
and pending identities in browser storage. These are local working copies,
separate from the onboard saved library. Stopping DCLink preserves committed
missions. Maintenance can continue editing while the runtime is stopped if
WebMenu and the computer remain available; QGC remote requests require DCLink
to be running. Neither client automatically starts a mission after reconnection.

## Execution

Start checks the requested revision against one SQLite read snapshot. If a save
committed first, Start returns `conflict` and the current revision. A later save
or deletion changes the library while the run keeps its original route and name.
Start is retained under the boot-scoped mutation identity. A repeated Start with
that identity returns the original acceptance and can never launch a second run.

DCLink queues one run at a time and rechecks readiness on the runtime thread.
The supported profile is an already armed, airborne PX4 multicopter in Hold
(MAVLink mode LOITER) or Position. The aircraft must match the requested system
ID, have valid fresh heartbeat, airborne state, HOME and global position, and
pass existing command and runtime safety admission. Every waypoint must be
within the configured HOME-relative altitude limits and above zero. Start
supplies no automatic arming, takeoff or landing.

Run reports contain `run_id`, `start_request_id`, `mission_id`, `revision`,
`name`, `vehicle_system_id`, `state`, `waypoint_id` (null after the last point),
`waypoints_reached`, `waypoint_count` and `reason`. States are `queued`,
`starting`, `running`, `stopping`, `completed`, `canceled`, `rejected`,
`interrupted` and `failed`. The latest report is kept in memory for this boot.
An absent report for an expected run, a stale session, or `fresh: false` means
the client should treat that run's state as unknown.

The planner uses the aircraft's existing external-control and Hold lifecycle.
Progress requires fresh feedback inside each waypoint's horizontal acceptance
radius and within 2 m vertically. The final waypoint requests Hold after its
rules finish. Cancel targets one run and is safe to repeat for that run. It can
cancel a queued start or request the usual confirmed Hold handoff. `stopping`
is pending. The planner reports `completed` or an in-flight `canceled` after
confirmed handoff. The operator controls are Start, status and Cancel. The
planner resumes the route automatically after a successful command step.

## Waypoint and leg rules

Each rule has a lowercase UUIDv4 `id`, one `trigger`, and an ordered `actions`
list of one to four steps. Optional `condition` and `area_id` fields limit when
the rule can run. After the last step, the route continues.

| When | Trigger fields | Runs at |
| --- | --- | --- |
| Reaching a waypoint | `type: "waypoint_reached"`, `waypoint_id` | Confirmed arrival at the named waypoint, using normal mission progress checks. |
| Starting a leg | `type: "leg_started"`, `from_waypoint_id`, `to_waypoint_id` | Transition to the next waypoint after the previous waypoint and its rules finish. |
| Anywhere on a leg | `type: "leg_progress"`, `from_waypoint_id`, `to_waypoint_id`, `position: {"type":"anywhere"}` | First eligible point on the active leg, including an accepted arrival at its end. Evaluates any saved conditions until the first match. |
| At a percentage of a leg | `type: "leg_progress"`, `from_waypoint_id`, `to_waypoint_id`, `position: {"type":"percent","percent":50}` | First time a fresh aircraft position reaches or crosses the specified whole percentage from 0 to 100. |

References use stable waypoint IDs. Both leg IDs must be consecutive in route
order. Moving a point keeps its ID. Removing a referenced point or breaking a
leg requires fixing its rules before saving.

Leg progress is relative to the straight horizontal segment between the saved
waypoints, calculated by projecting the aircraft position along that segment.
0% is the first fresh sample after steering toward the next waypoint begins.
100% is confirmed arrival under the normal waypoint checks. A leg can finish
inside its arrival radius before reaching an interior percentage. An interior
percentage that was never observed is skipped. A leg with coincident horizontal
endpoints can reach 0% and 100% and can use Anywhere.

The first sample at or beyond a percentage is its only check in that run.
If its conditions fail at that sample, the rule is skipped for that run.
Anywhere checks each eligible sample until it matches once or the leg ends.

Set `condition: {"type":"detection_present"}` to require any fresh, active
classified detection reported to DCLink. Add `class_id` to match an exact
integer class ID from the camera model, from 0 to 32767. While a mission uses
conditions, DCLink polls its configured DigiView for a complete classified
detection snapshot. A detection is fresh for at most 1.5 seconds. Empty reports
clear presence. The route continues while a rule waits for a match.

Set `area_id` to the ID of a saved area to require the aircraft's current
horizontal position to be inside it. This combines with a detection condition
when both are present. Waypoint arrival, leg start and percentage rules check
both conditions once at their event. A wait that starts after a match continues
for its saved duration if the aircraft drifts outside the area. The original
Cancel, telemetry checks and safety handoff still apply.

| Step | Action fields | Behavior |
| --- | --- | --- |
| Notify | `type: "notify"`, `text` | Record an operator message and continue to the next step. Text is 1–100 Unicode scalar values without surrounding whitespace, edge BOMs, or C0/C1 control characters. |
| Wait at waypoint | `type: "wait"`, `duration_s` | Keep the reached waypoint as the current navigation target for 1–60 whole seconds, then continue to the next step. Available only on waypoint arrival. |
| Run compatible command | `type: "command"`, `command_id` | Pause route progress, run the selected command within the saved mission, then continue to the next step on success. Available at waypoint and leg triggers. |

Rules that become eligible together run in saved order. Each rule runs at most
once per execution, with a combined wait budget of at most 300 seconds per
mission. For example, an arrival rule can notify, wait five seconds, and
notify again before the next leg begins. Wait uses the same aircraft ownership
and readiness checks as normal route motion. Cancel and safety handoff apply
during the wait and any command step.

Command IDs contain up to 64 characters. Use lowercase slash-separated
segments that start with a letter and continue with letters, ASCII digits, or
underscores. Each step stores only its command ID, and DCLink supplies its
bounded implementation. The current catalog contains **Face north**
(`copter/rotate_north`). It holds the current captured position and altitude
while turning north. The mission retains movement ownership and normal flight
eligibility checks. The step has a 20-second deadline and completes when fresh
heading and position feedback confirm the turn and the hold. The planner then
finishes the remaining steps and resumes the current waypoint or leg. Timeout
or failure ends the run through the normal safe Hold handoff. Failure reasons
include `mission_command_timeout`, `mission_command_heading_unavailable` and
`mission_command_position_out_of_bounds`. Cancel uses the same confirmed Hold
path as Cancel during route navigation.

A run report may include `rule_event` while a wait or command is active. It
contains `rule_id`, `action_index` (starting at 1 within the rule), `type`
(`"wait"` or `"command"`), and whole seconds left in `remaining_s`. A command
event also includes `command_id`. Each notification is logged by DCLink.
`last_notification` retains the latest notification for this run and includes
`rule_id`, `action_index`, `type: "notify"` and `text`.

## Geographic behavior areas

Draw the boundary on the QGC Plan map. Give it a name, optionally enter an
advisory, then select it under the rule's area condition. A named area can be
reused by several rules. Keep its ID when editing its name, advisory or
vertices. If an area is removed, repair the affected rules before saving.

| Area field | Contract |
| --- | --- |
| `id` | Lowercase UUIDv4, unique among areas in the mission. |
| `name` | Pilot-facing label of 1–80 Unicode scalar values, with the same constraints as the mission name. |
| `vertices` | 3–32 `{latitude_deg, longitude_deg}` corners in boundary order. The closing edge is implicit. |
| `advisory` | Optional message of 1–100 Unicode scalar values, with the same constraints as a Notify step. |

Each vertex uses finite WGS84 degrees, with latitude strictly between -90 and
90 and longitude between -180 and 180 inclusive. Rings require distinct
vertices, nonzero area and a longitude span under 180 degrees. Intersecting or
touching nonadjacent edges and adjacent edges that double back are invalid.
Consecutive corners that continue along the same straight edge are valid.
Clockwise and counterclockwise vertex orders have the same meaning.

Membership uses fresh aircraft latitude and longitude and includes the
boundary. Longitude 180° and -180° refer to the same meridian for membership.
Membership is independent of altitude. Areas can overlap and each is evaluated
independently. Advisories are for the operator, and area-scoped rules use the
same steps and command ownership as waypoint and leg rules. The route and
aircraft geofence remain separate from these areas.

While running, `run.active_areas` lists the areas containing the aircraft's
fresh position in saved order. Each item includes `area_id`, `name` and the
optional `advisory`. The field is omitted when no area is active. The run uses
area definitions and rule references from its admitted saved revision.

## Implementation acceptance

The planning handoff consists of this contract, the schema, a synthetic example,
and the [QGC editing design](https://github.com/SynclairVision/qgroundcontrol/blob/dev-0.8-qgc-missions/docs/en/qgc-dev-guide/plan/dclink_missions.md).
Later deliveries must demonstrate these outcomes using local validation and
simulation before operational evaluation:

- Create in QGC, load/edit/save in Maintenance, and refresh the saved revision
  in QGC. All library operations work without an autopilot.
- Simultaneous saves produce one committed update and one recoverable conflict;
  Reload/Save as copy preserve the pilot's choice.
- Invalid content, unsupported plan items, duplicate waypoint IDs, interrupted
  uploads, stale sessions, and failed storage do not partially replace a mission.
- Repeated create/commit requests do not create extra missions or revisions;
  reconnects never replay an uncertain mutation into a new service session.
- Reordering waypoints preserves IDs and saved values. Aircraft upload and
  current map/flight controls retain their behavior.
- Starting requires an explicit supported saved revision and execution readiness.
  Saving a newer revision leaves the active snapshot unchanged. Unsupported
  waypoint radii or backend behavior are reported before starting.
- A waypoint arrival sequence runs its steps in order and keeps the current
  waypoint target during a wait. A following leg rule starts only after the
  previous waypoint's rules complete. Cancel during a wait follows the normal
  flight handoff.
- Missing waypoint references, nonconsecutive leg endpoints, waits on legs,
  and excess wait time are rejected before saving. Both editors preserve the
  order and stable IDs of valid rules.
- In simulation, a matching detection can satisfy a waypoint or leg rule.
  Missing, stale, and mismatched detections leave navigation running normally.
  An Anywhere rule fires at most once, and a percentage rule checks only when
  the position reaches its specified progress.
- In simulation, aircraft entry and exit update active areas and advisories.
  A rule with both an area and a detection condition fires only if both match.
  Reject malformed polygons and missing area references before a save commits.
- Discover the compatible command list without an autopilot. Save and reload a
  command at a waypoint and on a leg in both editors. A syntactically valid
  command outside the catalog is rejected by the service.
- In PX4 SITL, start a saved route with Face north followed by Notify. Confirm
  that the command holds position and altitude, reports a running step, and
  resumes the same waypoint or leg after the turn. Cancel a separate run
  during the turn and verify confirmed Hold. With synthetic stale heading or
  an expired deadline, check that the failure requests Hold and later steps
  are skipped.
