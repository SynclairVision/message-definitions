# DCLink MAVLink protocol

`dclink_mavlink_dialect.xml` is the canonical source for the MAVLink 2 operator
messages shared by DCLink and QGroundControl. Consumers pin this repository to
the same commit and generate their language bindings with the pinned
`mavlink/pymavlink` generator.

| ID | Message | Payload bytes | CRC extra | Operations |
| --- | --- | --- | --- | --- |
| 41000 | DCLINK_STATUS | 79 | 119 | Query high-level runtime status. |
| 41001 | DCLINK_COMMAND | 157 | 121 | GET indexed commands, START, STOP, ITEM, ACK, ERROR. |
| 41002 | DCLINK_PARAMETER | 221 | 27 | GET indexed parameters, SET a value, ITEM, ACK, ERROR. |

Payload sizes are the complete layouts before MAVLink 2 trailing-zero trimming.
CRC validation is required. Do not combine this dialect with
`copilot_mavlink_dialect.xml`: both allocate IDs 41000 through 41002.

## Requests and replies

Send an all-zero STATUS body except for `request_id` to query runtime health.
Responses echo `request_id` and report runtime fields. For indexed catalogs,
GET `id=0` returns ACK with `total`; GET `id=1` through `total` returns one ITEM.
START and STOP select a command by name or numeric catalog ID. An empty STOP
selects the active command. Text fields contain UTF-8 within their fixed byte
limits. QGC keeps its small vehicle-specific command menu and intersects that
menu with the returned command catalog.

ACK reports request acceptance. Follow subsequent status replies to observe
the active command; acceptance does not establish completion of a flight
command or its safe cancellation.

The command and parameter messages carry `access_role` as a base field, not
a MAVLink extension. The metadata bitmask uses `INTEGRATOR=1` and `PILOT=2`;
zero means unspecified metadata. It is not client authentication or a command
authorization mechanism. DCLink validates requests, and QGC retains its local
pilot-command allowlist even when role metadata is unspecified.

## Routing and liveness

The default DCLink source identity is system/component `252/67`. Each operator
uses a unique source identity and a dedicated router endpoint. Native messages
contain no destination fields; use one DCLink instance and one operator client
per routed operator bus. QGC also requires fresh aircraft telemetry on that
connection and rejects an ambiguous or mismatched aircraft.

Match replies against the DCLink source, request ID, message type and expected
catalog item. Only a correlated STATUS reply establishes runtime freshness.
QGC polls status once per second and expires it after three seconds. Other
traffic must not keep a stale DCLink session online.

Catalog reads may be retried. Command mutations are sent once, without
automatic retry or reconnect replay. A missing acknowledgement leaves the
outcome unknown; obtain fresh status before another operator decision. DCLink
retains up to 1024 mutation identities and responses for its boot lifetime,
rejects conflicting reuse, and returns BUSY when the cache is full. Clients
do not reuse request IDs within a client lifetime. There is no persistent
replay protection across service restarts.

Native parameter SET validates, saves and applies one value. Atomic,
revision-checked catalog saves require DCLink's local operator interface.
The native messages do not carry flight telemetry, target geometry, full
command diagnostics, logs or parameter editability metadata.

## Generation

QGC stages the pinned standard XML plus `sv_mavlink_dialect.xml` and
`dclink_mavlink_dialect.xml` in its build directory. Its generated `all.xml`
includes both custom dialects, checks message-ID collisions, and produces C
headers. Source XML and vendored submodules remain unchanged by generation.

DCLink generates its Python bus from the pinned `ardupilotmega.xml`,
`sv_mavlink_dialect.xml` and `dclink_mavlink_dialect.xml` using
`python -B -m src.tools.generate_dclink_bindings` from its repository root.
Its finalization step keeps CRC validation unconditional.
