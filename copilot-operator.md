# Copilot Operator MAVLink Protocol

`copilot_operator_mavlink_dialect.xml` defines operator requests to Copilot IO,
request responses, and IO-to-client status streams. It is a standalone
MAVLink 2 XML source for onboard and ground-station consumers. Operator API
version is `1`; the protocol name is
`copilot-operator-mavlink`.

The API uses the same separation of message operations and parameter groups
seen in DigiView's native `msg_defs.hpp`: enums identify operations and
fixed-width fields carry their arguments. The Copilot XML is authoritative
for these MAVLink message IDs, enums, and field types.

## Messages

| ID | Message | Purpose |
| --- | --- | --- |
| 41100 | `COPILOT_OPERATOR_HELLO` | Discover protocol version, IO session, capabilities, and snapshot schema version. |
| 41101 | `COPILOT_OPERATOR_PING` | Exchange client and IO timestamps for operator connection liveness. |
| 41102 | `COPILOT_OPERATOR_REQUEST` | Identify an operation and its fixed arguments, expected revision, and optional body. |
| 41103 | `COPILOT_OPERATOR_PAYLOAD` | Transfer length-delimited request or response body fragments. |
| 41104 | `COPILOT_OPERATOR_REPLY` | Report upload progress, protocol rejection, or final operator result. |

All messages address a specific system and component. IO accepts only its
exact configured destination; broadcast control requests are rejected.
Responses address the requesting client's source system/component. Receivers
validate frame CRCs before interpreting any message.

## Operator input fields

The action enum covers the complete current operator IPC control contract.
Request UUIDs are carried as exactly 32 lowercase hexadecimal characters,
matching the client-generated request identity. Text fields are UTF-8,
zero-padded to their declared size. A full-width field does not require a
terminating zero. Senders reject oversized values before packing.

| Action | Operator API name | Fixed fields | Request JSON body |
| --- | --- | --- | --- |
| `COMMAND_START` | `command.start` | `command` | Optional command `payload` object. |
| `COMMAND_STOP` | `command.stop` | `command`; empty selects active command. | Empty. |
| `RADAR_SET` | `operator.radar.set` | `radar_mode`: `REL` or `GLOBAL`. | Empty. |
| `COMMANDS_GET` | `operator.commands.get` | None. | Empty. |
| `COMMANDS_SAVE` | `operator.commands.save` | `revision`. | Ordered `items` array of `{command, enabled}` records. |
| `PARAMETERS_GET` | `operator.parameters.get` | None. | Empty. |
| `PARAMETERS_SAVE` | `operator.parameters.save` | `revision`. | `items` array of `{name, value}` records. |
| `RESTART` | `restart.request` | None. | Empty. |
| `SHUTDOWN` | `shutdown.request` | None. | Empty. |

Unused command and revision fields are empty; unused radar mode is
`UNSPECIFIED`. In particular, an empty START command is invalid. START with
no arguments has a zero-length body. An explicit empty arguments object is
`{}`. A save always has an array body, including `[]` for an empty list.
The order of command items is preserved.

Command references are at most 64 UTF-8 bytes. Catalog revisions are opaque
strings of at most 64 UTF-8 bytes. An empty revision uses the operator API's
unconditional-save semantics. The IO catalog service remains responsible for
revision checking, name resolution, editability, bounds, and persistence.
Parameter values retain their JSON types, including empty strings, booleans,
zero, and null. Command arguments may contain nested objects and arrays.
All numbers must be finite; duplicate JSON object keys are rejected.

## Handshake and liveness

The client sends HELLO with role `CLIENT`, the protocol name and version,
a nonzero `client_session`, and `server_session=0`. IO replies with role `IO`,
the echoed client session, a nonzero server boot token, capabilities, and an
optional snapshot schema version. Unsupported versions are not admitted.
Action capability bits are `1 << (action - 1)` for actions 1 through 9.
Capability `STREAMS` (512) advertises the output stream contract below. Clients send
only actions advertised by IO.

Subsequent messages carry both session tokens. PING uses direction `TO_IO`;
IO echoes the double-precision `sent_at` value and supplies `server_at` in a
`FROM_IO` reply. Both are Unix timestamps in seconds. Operator ping does not
refresh vehicle/GCS heartbeat safety state.

## Body transfer

Request identity is the tuple of source system/component, server session,
client session, and request UUID. A body is finite UTF-8 JSON without a JSON
Lines delimiter, limited to 1,048,576 bytes. `payload_crc32` is unsigned
CRC-32/ISO-HDLC over exactly those bytes, equivalent to `zlib.crc32(body)`.
A zero-length body has checksum zero.

1. Send REQUEST with the complete body length and checksum.
2. IO validates the header and reserves bounded reassembly space. For a
   nonempty body it replies `RECEIVING`, with `received_bytes=0` initially.
3. Send PAYLOAD fragments with direction `TO_IO`, explicit byte offsets,
   and 1 through 196 valid bytes. The remaining data bytes are padding.
4. Progress replies report the first missing body offset. The sender may
   retransmit fragments from that point. Duplicate fragments with identical
   bytes are accepted. Conflicting overlaps are rejected without dispatch.
5. After all bytes arrive, IO verifies size, checksum, UTF-8, JSON, and the
   action-specific body type. It queues exactly one operator dispatch on
   the runtime main thread.

Fragments can arrive out of order after header admission. Fragments without
an admitted header, invalid lengths, offsets outside the declared body,
wrong sessions, and invalid direction are rejected. A UTF-8 character can
cross fragment boundaries; text is decoded after reassembly.
Incomplete transfers expire after 5 seconds without progress, or 60 seconds
total. An expired incomplete transfer can be restarted with the same header
and identity because no operator action has executed. Receivers admit at
most four incomplete bodies and one per client session; resource exhaustion
returns `BUSY` before allocating or dispatching. Transfer traffic must be
bounded so it cannot starve telemetry, heartbeat, or control output.

Final response bodies are JSON objects containing `ok`, `result`, and `error`
from the operator service. The REPLY header declares response length and
checksum. Body fragments use direction `FROM_IO` and the same transaction
identity. A client can request retransmission of a retained final response
by resending the original REQUEST header and body unchanged. Complete JSON
catalog and parameter results preserve their fields and metadata. The
48-byte `error` header field contains a bounded machine-readable error;
the complete operator error and details remain in the JSON body.

## Dispatch and retries

A successful final response uses `OK`; a negative operator outcome uses
`ERROR`. Protocol failures use `INVALID`, `BUSY`, `SESSION_CHANGED`, or
`CONFLICT`. Bodyless protocol failures are described by their result and
error fields. Progress replies have no response body. Clients require a
final result and verify any declared response body before reporting an
operator outcome.

The receiver records transaction admission before dispatch. Identical retries
return progress or the retained final response and do not execute again.
Reusing an identity with a different header or body is a conflict. Completed
mutating request identities retain replay barriers for the IO boot lifetime;
response-body eviction must never make an old mutation executable again.
If a retained body is unavailable, the receiver returns `CONFLICT` with
`error=response_expired`. Resource limits return `BUSY` for new requests
instead of dropping replay barriers. Implementations must bound retained
response bytes and admitted mutation identities.

A restarted IO process changes its server token and rejects old-session
requests before dispatch. The client rediscovers the session and current
state, then obtains a new operator decision before issuing a replacement
mutation. Session tokens provide correlation, not authentication.

Operator dispatch retains normal planner ownership and fallback checks.
`ok=true` is request acceptance, not a certificate of flight completion.
A cancellation can return `accepted=true`, `pending=true`, and
`reason=cancel_queued`. Failed starts preserve `reason`, `fallback`, and
`fallback_confirmed`. Parameter saves preserve `saved.changed` and
`saved.restart_required`. Restart/shutdown replies preserve deferred
acceptance. Runtime services own those meanings and safety decisions.

## Catalog and action responses

A final REPLY body preserves the operator response's `ok`, `result`, and
`error` values. It can also carry the complete IPC envelope (`type=response`,
request UUID, and action name); these must agree with the typed header when
present. Successful catalog get/save responses carry the full catalog,
including metadata needed by the editors:

| Result | Published fields |
| --- | --- |
| Command catalog | `path`, opaque `revision`, and ordered `items[]` records with `command`, `label`, `enabled`, and `group`. |
| Parameter catalog | `path`, opaque `revision`, ordered `sections[]`, and `counts` with `copilot` and `command` parameter counts. |
| Parameter section | `id`, `scope` (`copilot` or `command`), `label`, `description`, `command`, and ordered `parameters[]`. |
| Parameter record | `name`, `value`, `default`, `label`, `description`, `type`, `category`, `apply`, `editable`, `unit`, `choices`, `min`, `max`, and `overridden`. |
| Parameter save | Full catalog plus `saved.changed` and `saved.restart_required`. |

Current/default parameter values in catalog records are strings. Their type
and editor behavior come from `type`, `choices`, optional bounds, `editable`,
and `apply` (`live` or `restart`). Empty strings, absent bounds represented
by null, and false flags must survive transfer. Paths are informational;
clients persist edits through IO. Catalog revision conflicts and other
validation failures preserve the machine-readable `error` and `result.detail`.

Command responses preserve `command`, `reason`, `accepted`, `pending`,
`fallback`, and `fallback_confirmed` whenever the operator service supplies
them. Lifecycle results preserve `restart` or `shutdown` acceptance and
`deferred`. A transport acknowledgement never substitutes for these operator
outcomes, and omitted optional fields are not fabricated.

## IO-to-client delivery

The `STREAMS` capability (bit 9, value 512) advertises IO output subscription.
The nine action capability bits retain their action mapping. Output delivery
uses the following additional messages:

| ID | Message | Purpose |
| --- | --- | --- |
| 41105 | `COPILOT_OPERATOR_STREAM_REQUEST` | Replace stream selection, requested rates, lease, and log replay policy. |
| 41106 | `COPILOT_OPERATOR_STREAM_STATUS` | Acknowledge effective subscription settings, snapshot contract/version, log retention cursors, and gaps. |
| 41107 | `COPILOT_OPERATOR_EVENT` | Describe a complete snapshot, log batch, or service error body. |
| 41108 | `COPILOT_OPERATOR_EVENT_DATA` | Carry 196-byte fragments of that event body. |
| 41109 | `COPILOT_OPERATOR_RUNTIME_TICK` | Publish a small, independent main-loop liveness sample. |
| 41110 | `COPILOT_OPERATOR_EVENT_ACK` | Acknowledge an event, request missing bytes, or report expired event retention. |

The output contract covers every service message consumed by the TUI:
HELLO carries protocol identity/version, endpoint role, capabilities, and
snapshot schema version; PING in the reply direction carries pong timestamps;
REPLY and its body carry the request response; EVENT carries snapshot,
log.batch, and error envelopes; RUNTIME_TICK carries runtime.tick. Local
connection state, local receipt times, pending request timers, and the
client-specific SERVICE alert are computed by the client.

### Subscription and reconnect

After HELLO, the client sends STREAM_REQUEST with its sessions and a fresh
subscription request UUID. `streams` is the complete desired bitmask:
SNAPSHOT=1, LOG=2, RUNTIME_TICK=4. Zero unsubscribes. IO accepts a replacement
atomically and returns STREAM_STATUS with effective intervals, lease,
retained log bounds, current snapshot sequence, and snapshot contract/version.
A failed replacement preserves the active subscription. Exact request retries
return the original acknowledgement without starting another history replay.

Each accepted replacement gets a new nonzero `subscription_id`, issued by IO
and never reused during the IO boot. Exact retries retain the same token.
EVENT, EVENT_DATA, EVENT_ACK, and RUNTIME_TICK carry it. The client accepts
stream data only for the configured IO source, current server/client sessions,
and current subscription token. Reordered acknowledgements for an older
subscription request cannot replace the current selection. Unsubscribe returns
subscription id zero and retires the previous token.

A snapshot subscription requires the exact supported `snapshot_schema_version`
(currently 2). A mismatch returns INVALID with `snapshot_schema_mismatch`.
STREAM_STATUS supplies `snapshot_contract` (`src.copilot.operator.snapshot`);
clients validate it before accepting snapshots. A client can subscribe only
to logs/ticks if it does not support the snapshot schema.

Zero requested intervals select the IO defaults: snapshots 200 ms, log batches
50 ms, runtime ticks 1000 ms. Snapshot/log intervals are at least 50 ms. Ticks
use an interval from 100 through 1000 ms so the current TUI's three-second
runtime freshness threshold can be retained. Effective rates are explicit in
the acknowledgement and may be reduced by configured transport budgets.
Clients must use a freshness threshold consistent with the acknowledged tick
interval; the current TUI policy is three seconds. Stream rates do not alter
vehicle telemetry, command rates, or flight safety policies.

A zero lease requests a 10000 ms lease. IO admits leases from 3000 through
60000 ms. Valid client PING renews an active lease; clients send it at least
once per second. Lease expiry retires the subscription and bounds resources
when a UDP client disappears. Expiry does not stop the running vehicle command.
Clients rediscover and establish a new subscription before accepting new data.
Server-session changes clear all partial transfers and source cursors; a log
cursor from a previous server session must not be reused.

### Complete snapshots

On snapshot subscription IO sends the latest available full snapshot even if
the view is unchanged. If none exists it waits for a real runtime capture.
Subsequent unchanged views may be suppressed. The body is exactly the finite
JSON envelope published by the operator snapshot service:

```json
{
  "type": "snapshot",
  "contract": "src.copilot.operator.snapshot",
  "schema_version": 2,
  "sequence": 42,
  "generated_at": 1800000000.125,
  "view": {
    "flight": {},
    "target": {},
    "command": {},
    "actuators": {},
    "cas": [],
    "actions": [],
    "radar_mode": "REL",
    "control_owner": ""
  }
}
```

The empty groups above identify the envelope shape. Published groups contain
the fields below. `sequence` increases independently of log record sequences;
`generated_at` is snapshot Unix generation time, not the age of every sensor.
EVENT's schema version and generation time must agree with its snapshot body.
Unknown additive fields, including arbitrary keys inside `command.status`,
are preserved or ignored without rejecting an otherwise compatible snapshot.
Clients apply a snapshot only after validating its whole body.

| View group | Fields and representation |
| --- | --- |
| `flight` identity | `autopilot`, `flight_mode`, `navigation_source`, `vehicle_id`: strings; `armed`: boolean or null. |
| `flight` attitude | `roll_deg`, `pitch_deg`, `heading_deg`: degrees or null. |
| `flight` altitude/speed | `altitude_m`, `target_altitude_m`: meters; `horizontal_speed_mps`, `airspeed_mps`, `vertical_speed_mps`: meters/second; each may be null. |
| `flight` Myggnav | `myggnav_yaw_deg`, `myggnav_pitch_deg`, `myggnav_roll_deg`: degrees; `myggnav_thrust`: normalized thrust; each may be null. |
| `flight` battery/GPS | `battery_v`, `battery_pct`: volts and percent or null; `gps_sats`, `gps_fix`: integers or null. |
| `flight` position | `latitude_deg`, `longitude_deg`, `navigation_latitude_deg`, `navigation_longitude_deg`: decimal degrees; `local_north_m`, `local_east_m`, `local_down_m`: NED meters; each may be null. |
| `target` state | `present`, `control_active`: booleans; `control_command`: string; `age_s`, `source_age_s`: seconds or null. |
| `target` direction | `bearing_deg`, `bearing_relative_deg`, `bearing_global_deg`, `yaw_deg`, `pitch_deg`: degrees or null. Relative bearing is clockwise from the vehicle nose; global bearing is clockwise from north. |
| `target` range/motion | `distance_m`, `slant_distance_m`, `height_m`: meters; `speed_mps`: meters/second; `moving_probability`: number; numeric fields may be null. `height_source`, `motion_mode`: strings. |
| `target` quality | `triangulation_quality`: string; `triangulation_score`, `observability_score`: numbers or null; `observability_maneuver`: boolean or null. |
| `target` position | `latitude_deg`, `longitude_deg`: decimal degrees; `local_north_m`, `local_east_m`, `relative_north_m`, `relative_east_m`: meters; each may be null. |
| `target` diagnostics | `stt_moss_global_yaw_deg`, `stt_moss_global_pitch_deg`, `stt_moss_delta_yaw_deg`, `stt_moss_delta_pitch_deg`: degrees or null. |
| `command` | `active_id`, `label`, `phase`, `reason`, `safe_command`: strings; `status`: complete command-defined JSON object, including nested arrays and optional diagnostics. |
| `actuators` | `source`: string; `age_s`: seconds or null; `values`: array of normalized numbers or null; `channels`: array of channel records. |
| `actuators.channels[]` | `label`: string; `value`: normalized number or null; `positive_only`: boolean; `channel`: source channel integer or null. Values are 0..1; centered controls have neutral 0.5. |
| `cas[]` | `label`, `value`, `detail`: strings; `level`: `normal`, `info`, `caution`, `warning`, `critical`, or `unknown`. Order is preserved. |
| `actions[]` | `command`, `label`, `group`: strings; order is preserved. |
| View selection/ownership | `radar_mode`: `REL` or `GLOBAL`; `control_owner`: `manual`, `autopilot`, `copilot`, or empty when unknown. |

Optional measurements remain JSON null when unavailable. Nonfinite numeric
source values are serialized to null by the operator snapshot publisher.
False, zero, and empty strings retain their meanings. Clients do not turn
missing measurements into plausible flight values or infer control ownership
from a stale snapshot. Cached baseline timestamps and measurement ages are
preserved; replay does not make old sensor data new.

### Logs, retention, and replay

Log bodies preserve the operator log envelope:

```json
{
  "type": "log.batch",
  "reset": true,
  "events": [
    {"sequence": 7, "line": "[fake] waiting for telemetry", "severity": "warning"}
  ]
}
```

`reset` is optional and defaults to false. True clears the client history and
its last accepted log record sequence before applying the batch. `events`
is ordered. Record fields are `sequence` (positive integer), `line` (complete
string, including formatted timestamps and newlines), and `severity` (`debug`,
`info`, `success`, `warning`, `error`, `critical`, or null for neutral/unknown).
Logs have no invented structured measurement timestamp. Snapshot, transfer,
and log record sequences are independent. Duplicate record sequences are
ignored; accepted record order must not regress.

STREAM_REQUEST selects initial delivery:

- NONE starts with new records and an empty reset batch. The acknowledgement's
  `log_newest_sequence` establishes the deliberately skipped history cursor.
- HISTORY sends a reset batch with up to `log_history_limit` retained records;
  zero limit requests IO's configured history bound (normally 1000 records).
- AFTER sends records newer than `log_after_sequence` for the same IO boot.
  If requested records are missing to retention or the requested limit,
  STREAM_STATUS has `log_gap=1` and IO sends a reset batch containing the
  available tail. A cursor ahead of the latest record is INVALID with
  `log_cursor_ahead`. An empty history has oldest/newest sequence zero.

`log_gap` must be visible to the operator as unavailable history. Clients
must not interpret a gap as a clean continuous log. IO keeps log batches
ordered per subscription, with at most one unacknowledged log batch. If
ongoing retention overtakes delivery, IO reports a service error
`log_history_gap` and pauses log delivery until the client establishes a
replacement subscription. Snapshot and runtime-tick streams continue.
An empty reset batch is meaningful and must be delivered.

### Runtime freshness and errors

A runtime tick carries the original `runtime.tick.generated_at` value and a
positive main-loop `sequence`. It is emitted from the runtime main thread,
bypasses bulk-transfer queues, and is never replayed from a cache. Unchanged
snapshots, logs, pongs, successful request responses, subscriptions, and event
acknowledgements do not establish runtime freshness. Clients require a newer
tick for the current subscription and track its local monotonic receipt time.
Remote wall-clock skew does not control the local timeout.

Before the first runtime tick a connected client is waiting for runtime data.
After three seconds without a fresh tick the current TUI treats the runtime
as stale and clears displayed control ownership. Link disconnection and IO
restart also clear runtime freshness. These states are derived locally;
operator ping cannot substitute for runtime or vehicle/GCS heartbeat state.

Service errors use EVENT type ERROR with schema version 1 and the complete
error envelope, for example:

```json
{"type":"error","error":"unknown_message","message_type":"unknown.input"}
```

An error can be addressed before stream establishment using subscription id
zero. It is accepted only from the configured IO identity for the addressed
client session. It cannot establish runtime freshness or snapshot validity.
Additional error detail fields are preserved.

### Event transfer and recovery

An event body is finite UTF-8 JSON of 1 through 1,048,576 bytes. Header
`payload_crc32` uses CRC-32/ISO-HDLC over exactly those bytes. EVENT_DATA
fragments carry offsets and 1 through 196 valid bytes. Padding is ignored,
and UTF-8 is decoded only after reassembly. A transfer matches source
system/component, server/client sessions, subscription id, and event id.
Request UUIDs, event ids, snapshot sequences, log sequences, and tick
sequences occupy separate identity spaces.

Event ids increase within a server/client session pair and are never reused
for a different body. Retransmissions keep the header and body unchanged.
The client accepts fragments only after their header and rejects invalid
bounds, conflicting duplicate bytes, wrong identities, and failed checksums.
It never publishes a partial snapshot, log batch, or error.

EVENT_ACK direction TO_IO reports PROGRESS with the first missing byte,
COMPLETE after verifying/accepting the body, or INVALID to reject it. PROGRESS
at offset zero requests a missing header and full body. IO answers with
retained header/fragments, or FROM_IO UNAVAILABLE with the same identity
when retention has expired. The client then establishes a new subscription
for a full snapshot and an explicit log replay cursor. A lost subscription
acknowledgement is recovered by retrying the same STREAM_REQUEST.

If an event header itself is lost, a client can infer a missing event id
from later headers or replace its subscription after a delivery timeout.
Runtime tick traffic remains independent and can still show that the runtime
is alive. A new full snapshot may supersede an incomplete older snapshot;
late superseded data cannot overwrite the latest applied snapshot. Log
records require ordered completion or explicit replay/gap handling.

Each client keeps at most one incomplete snapshot, one log batch, and one
service-error body, each at most one MiB. Incomplete bodies expire after five
seconds without progress or 60 seconds total. IO retains at most the latest
snapshot and one log/error event per subscription, bounded by body-size
limits and the subscription lease. Unsupported resource requirements are
rejected with BUSY. Counters must not wrap within a session. Emission budgets
prioritize runtime ticks and flight/runtime traffic over bulk event data.

## Generation and local validation

Consumers generate this XML together with the standard and SV definitions
they require. It has no standard-dialect includes. Generated artifacts belong
to the consuming build. Message and enum names use the `COPILOT_OPERATOR_`
prefix. The existing DigiView CMake target generates its configured SV input;
a consumer of this operator API explicitly adds the operator XML to its
MAVLink generation inputs.

From the Copilot repository, run:

```bash
venv/bin/python -B -m src.tools.smoke.operator_dialect
```

These checks generate temporary Python and C bindings, check ID collisions
against standard/SV/current Copilot definitions, compile a C consumer, and
round-trip fake requests for every operator action. They also check a
maximum-size body, UTF-8 fragment boundaries, CRC rejection, handshake,
ping, full operator response values, complete operator snapshots, log replay
envelopes, runtime ticks, subscriptions, service errors, and event integrity.
They validate the schema without
opening a socket, persisting configuration, or invoking a vehicle command.
To validate a definitions checkout before it is pinned by Copilot, select
its XML explicitly:

```bash
COPILOT_OPERATOR_XML=../message-definitions/copilot_operator_mavlink_dialect.xml \
  venv/bin/python -B -m src.tools.smoke.operator_dialect
```

The flight runtime and QGC need implementations of this wire contract.
