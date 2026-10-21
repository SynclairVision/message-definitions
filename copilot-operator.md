# Copilot Operator MAVLink Protocol

`copilot_operator_mavlink_dialect.xml` defines operator requests to Copilot IO
and their responses. It is a standalone MAVLink 2 XML source for onboard and
ground-station consumers. Operator API version is `1`; the protocol name is
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
A capability bit is `1 << (action - 1)` for actions 1 through 9. Clients send
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
ping, and full operator response values. They validate the schema without
opening a socket, persisting configuration, or invoking a vehicle command.
The flight runtime and QGC need implementations of this wire contract.
