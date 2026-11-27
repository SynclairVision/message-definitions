# DCLink mission planning contract

Status: version-1 contract. The shared mission library, runtime IPC/MAVLink
adapters, QGC route editor and Maintenance local helper implement planning and
storage. Rules, behavior areas and execution remain subsequent deliveries.
Existing operator messages retain their layouts.

## Ownership and first delivery

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

The first delivery supports named waypoint routes and multiple saved missions.
It reuses the QGC map and preserves aircraft upload and current flight controls.
Conditional rules and behavior areas are subsequent, capability-gated deliveries.

## Mission document, identity, and limits

[mission.schema.json](mission.schema.json) defines the JSON mission content.
[examples/route.json](examples/route.json) is synthetic, not an operational route.

| Field | Contract |
| --- | --- |
| `schema_version` | Integer `1`; a client must understand the complete version before editing. |
| `name` | 1–80 Unicode code points, without control characters or leading/trailing whitespace. Names are labels, not identity. |
| `altitude_reference` | `relative_home`: metres above the admitted aircraft HOME altitude, not terrain clearance or height above the launch map point. |
| `waypoints` | Ordered list of 0–1000 waypoints. An empty mission is a saved draft and cannot execute. |
| Waypoint `id` | Lowercase UUIDv4, unique within this mission and stable through coordinate edits and reordering. |
| `latitude_deg`, `longitude_deg` | WGS84 degrees in [-90, 90] and [-180, 180]. Zero is valid. |
| `altitude_m` | Finite JSON number in metres relative to HOME. Execution checks configured flight limits separately. |
| `acceptance_radius_m` | Finite positive metres for that waypoint. The editor initially offers 30 m and exposes the value. |
| `rules`, `areas` | Empty arrays in version 1. Nonempty content is rejected, never silently discarded. |

All listed fields are required. Unknown fields, duplicate JSON object keys,
non-finite numbers, duplicate waypoint IDs, and unsupported versions are invalid.
The schema validates structure; the service also checks unique IDs, trimmed
names, finite values, and a maximum encoded document size of 262144 UTF-8 bytes.
The same limits apply to IPC and MAVLink. No production JSON Schema dependency
is required: each implementation may validate these rules with existing tools.

A saved record is an envelope, separate from editable content:

```json
{
  "mission_id": "44444444-4444-4444-8444-444444444444",
  "revision": 1,
  "mission": {
    "schema_version": 1,
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
Waypoint IDs are scoped to their mission and may be retained when making a copy.
At revision exhaustion, reject updates with `busy` and reason
`revision_exhausted`; the operator can save a copy with a new identity.

Updates replace the complete mission content atomically and require the exact
current `expected_revision`. Only a successful committed save changes the record
or revision. A failed validation, transfer, or conflict leaves the record intact.
Use one serialized service mutation path for both adapters. Storage replacement
must be durable before reporting success. A reader sees a complete saved record.

Unknown schema versions remain visible as unsupported library entries. A client
may inspect available metadata but must not save a reduced or partially decoded
document over a version it cannot edit.

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
| `missions.capabilities` | Empty; the discovery request omits `service_session`. | Session, versions, supported operations, features, limits, and readiness. |
| `missions.list` | Optional opaque `cursor`, `limit` from 1 to 50 (default 50). | `items` of `{mission_id, revision, name, schema_version, waypoint_count}`, `next_cursor` or null. |
| `missions.get` | `mission_id`. | Latest complete saved record. |
| `missions.create` | `mission` content. | `{mission_id, revision: 1}`. |
| `missions.update` | `mission_id`, `expected_revision`, complete `mission`. | `{mission_id, revision}` for the committed content. |
| `missions.delete` | `mission_id`, `expected_revision`. | Deleted ID and revision. A mission used by a nonterminal run is busy. |
| `missions.import_route` | `name`, optional positive `acceptance_radius_m`. | `{mission_id, revision: 1}` for a one-time import of DCLink's stored route. |

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

Capabilities return `protocol_version: 1`, `schema_versions: [1]`, the current
supported `operations`, `features: ["waypoint_routes"]`, and these limits:
`max_document_bytes: 262144`, `max_waypoints: 1000`, `max_name_characters: 80`,
`max_list_page: 50`, and a MAVLink `max_chunk_bytes` no greater than 160.
IPC additionally reports `max_result_bytes: 264192` and the bounded
`max_session_mutations`. The IPC-only delivery advertises `transports: ["ipc"]`
and omits MAVLink chunk support. `stored_route_import` advertises the explicit
import operation; execution reports `supported: false` until implemented.
Execution operations and readiness are advertised independently of library
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

Execution requests and run status are a subsequent delivery with their own
definitions. They are not advertised by the library transport.

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
capabilities, list, get, create, update and delete. The response echoes action,
client/request IDs, `library_id`, `result` and `data`. Input is limited to 270336
bytes and logical output to 264192 bytes. A replaced library with a different
identity returns `stale_library` and its current ID without admitting the write.
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

## Execution boundary

The execution delivery adds `missions.start` with explicit `mission_id` and
`revision`. DCLink atomically checks that saved revision and captures an immutable
run snapshot. A concurrent save either precedes admission (causing conflict) or
creates a newer revision without changing the admitted run. Start never chooses
whichever mission another editor last selected.

Execution requires a nonempty supported route, an admitted matching vehicle,
fresh HOME/navigation data required by the backend, compatible vehicle/controller
support, and existing ownership and safety admission. Planning a relative-HOME
route does not invent HOME or authorize arming. A start does not add automatic
arming, takeoff, or landing behavior.

Execution requests use the planner's normal lifecycle. Future pause/resume/cancel
requests identify a `run_id`; capability/readiness states govern availability.
Their acknowledgements mean acceptance, not completed movement or handoff.
Run status reports mission ID/revision, run ID, state, current waypoint ID,
progress, and reason. A missing or stale run report means state unknown. It is
not evidence that a vehicle has stopped, and must not cause automatic restart.

The execution adapter must preserve waypoint IDs and honor each acceptance radius
or reject unsupported values before start. It must use the admitted mission
snapshot explicitly and retain existing route cleanup/restart requirements.
Unrelated aircraft plans and implicit route-source selection cannot substitute
for the named mission. No client or transport bypasses planner ownership,
telemetry freshness checks, cancellation, or the final flight safety envelope.

## Rules and areas extension

Future rules use a bounded **Where / When / Do / After** editor, with stable
waypoint/leg/area references rather than displayed waypoint numbers. Define a
route leg (progress between two points), a radius around a waypoint, and a
geographic corridor as distinct scopes. Advertise only implemented scopes.

Class conditions require a versioned model/class vocabulary associated with the
correct fresh track. Reacting to a selected track and automatically acquiring
a matching unselected detection are separate capabilities. Rule design must
specify dwell, rearming, action timeout, target loss, action ownership, and route
return behavior before enabling execution.

Behavior areas select supported profiles inside a geographic boundary. They are
distinct from aircraft fences. An avoidance profile is active only when its
backend confirms readiness; a drawn polygon alone does not provide avoidance.
Advertise these extensions only with an implemented versioned content schema,
validator, execution behavior, and both clients' preservation/editing behavior.

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
