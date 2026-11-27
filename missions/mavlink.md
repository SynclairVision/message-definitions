# Mission MAVLink transport, version 1

This profile carries the [mission service](README.md) over MAVLink 2. Library
and supported execution operations use the same exchange. The canonical layouts are
in [dclink_mavlink_dialect.xml](../dclink_mavlink_dialect.xml).

The mission content version and the exchange protocol version are separate.
Version-2 rules are part of the existing JSON mission body, and rule status is
part of the existing JSON run report.

| ID | Message | Full payload bytes | CRC extra |
| --- | --- | --- | --- |
| 41010 | `DCLINK_MISSION_REQUEST` | 108 | 198 |
| 41011 | `DCLINK_MISSION_RESULT` | 109 | 10 |
| 41012 | `DCLINK_MISSION_DATA` | 232 | 193 |

Existing messages 41000–41003 retain their layouts and checksums. Consumers pin
the same definitions commit and regenerate their bindings. CRC validation is
required. MAVLink 2 trailing-zero trimming does not change the layouts above.

## Identity and bounds

Every packet carries `protocol_version=1`, exact nonzero destination
`target_system/target_component`, and four 16-byte identities. UUIDs use RFC 4122
byte order, corresponding to Python `UUID.bytes` and Qt `QUuid::toRfc4122()`.
Client, logical request and transfer UUIDs must be UUIDv4. The client identity
survives transport reconnects for that client process.

`service_session` in requests is the discovered service UUID. Discovery uses
sixteen zero bytes throughout its BEGIN, DATA, COMMIT, READ and ABORT exchanges.
An admitted zero-session transfer can execute only `missions.capabilities`.
Responses always carry the actual current service UUID. The client adopts a
discovered session only after verifying the complete capabilities reply.

Bind transfers to source system/component, physical endpoint, client, logical
request and transfer identities. Reject broadcast requests and ignore packets
addressed elsewhere. Match replies against that endpoint, expected source and
destination, protocol, all identities, stage and offset. Routing identities and
SHA-256 integrity are not authentication; transport access controls still apply.

An upload is strict UTF-8 JSON with exactly two keys:

```json
{"action":"missions.list","payload":{}}
```

The logical identity comes from the packet header. Permit only advertised
`missions.*` actions and object payloads. Execution admits a saved revision and
aircraft identity, or cancels a specific run, through the planner. Both this adapter and local IPC
invoke the same mission service, session and mutation receipt cache.

An upload or complete logical reply is at most 264192 bytes. Editable mission
content remains limited to 262144 bytes. Chunks carry 1–160 meaningful bytes and
zero padding; `data_length` preserves meaningful trailing zero bytes. SHA-256
covers exact complete bytes, including whitespace, without compression.

The endpoint retains at most eight peer/client RPC records. Each client uses
one RPC at a time. Uploads and reply snapshots expire after 30 seconds without a
valid exchange. A client's next BEGIN may reclaim its completed reply snapshot,
so losing the final best-effort ABORT does not consume additional slots.
Background work and bounded ingress/egress queues yield to normal control/status
traffic; library persistence never runs on the flight-control loop.

## Exchange

Stage values are BEGIN=1, CHUNK=2, COMMIT=3, READ=4 and ABORT=5. CHUNK appears only
in an acknowledgement to client DATA. Transfer results are OK=0, INVALID=1,
NOT_FOUND=2, BUSY=3, STALE_SESSION=4 and ERROR=5.

1. **BEGIN:** REQUEST includes a fresh transfer UUID, `offset=0`, complete upload
   `total_size` and `sha256`. RESULT BEGIN OK echoes that size/digest and the next
   upload offset. Repeating the same descriptor resumes at the acknowledged
   offset; changed descriptors under the same transfer are invalid.
2. **Upload:** Send DATA at the next offset. RESULT CHUNK OK reports the next
   expected offset and upload size/digest. Identical duplicate slices are harmless;
   conflicting bytes, gaps, nonzero padding, or ranges outside the descriptor are
   invalid. Ignore acknowledgements for another stage or offset.
3. **COMMIT:** REQUEST repeats the upload `total_size` and digest, with
   `offset=total_size`. The endpoint requires exact length/digest, strict JSON,
   valid session and an allowed operation before invoking the service. Revision
   checks use the service's storage transaction, including after a competing
   library edit during upload. Start acceptance queues runtime admission.
4. **Reply descriptor:** RESULT COMMIT OK has `offset=0` and the immutable logical
   reply's size/digest. This means the reply is available, not that the operation
   succeeded. The downloaded JSON envelope's `result` is the operation outcome.
5. **Read:** REQUEST READ specifies the next reply offset, `total_size=0` and a
   zero digest. Server DATA contains the corresponding slice. Duplicate reads
   return the same bytes. Errors are RESULT READ with the requested offset and
   no descriptor. Verify complete size, digest, UTF-8, JSON and logical identities
   before publishing any result or mission content.
6. **Release:** Send best-effort REQUEST ABORT with zero offset/size/digest after
   reading the result, or when abandoning the transfer. RESULT ABORT OK has no
   descriptor. ABORT is idempotent and never reverses a committed library change.

Other transfer errors have zero size/digest and never imply a successful save.
Normal acknowledgements echo client/request/transfer identities; their session
is always the current server session. A STALE_SESSION response terminates the
operation and requires rediscovery. A session change during any exchange also
terminates the operation. An old response must not establish a new session.

## Retry and uncertainty

Clients use a two-second exchange timeout and at most three resends of the same
exchange. Do not advance on a delayed acknowledgement for a preceding chunk.
Repeated COMMIT returns its cached reply descriptor until release, reclamation
or expiry. A new transfer for the same logical request and exact body can replace
its incomplete transfer. Another normal request cannot replace an incomplete
upload. A fresh zero-session discovery can abandon that same client's incomplete
upload to recover a reconnect; its COMMIT still permits only capabilities and
never undoes an operation already committed by the service.

If staging has expired, an explicit same-session retry uploads the same body
under the original logical request UUID with a fresh transfer UUID. The shared
service returns its retained mutation outcome, preventing duplicate creation or
revision increments. Changing content under that logical identity is invalid.

After a mutation COMMIT was sent, a lost reply or connection change leaves its
outcome unknown. Preserve the draft or run request identity. For execution, read
run status to reconcile a matching Start or Cancel, or require operator review.
Read-only status polling never repeats an execution request.
Never automatically replay a whole operation across reconnects or service boots.
After a boot change, refresh the library and require an operator decision before
a new mutation identity. An explicit retry of an old identity must stay bound to
its original service, endpoint, sender and content.
