# Copilot MAVLink protocol

`copilot_mavlink_dialect.xml` defines three MAVLink 2 messages:

| ID | Message | Operations |
| --- | --- | --- |
| 41000 | COPILOT_STATUS | Query high-level runtime status. |
| 41001 | COPILOT_COMMAND | GET indexed commands, START, STOP, ITEM, ACK, ERROR. |
| 41002 | COPILOT_PARAMETER | GET indexed parameters, SET a value, ITEM, ACK, ERROR. |

Send an all-zero STATUS body except for request_id to query runtime health.
Responses echo request_id and report runtime fields. For indexed catalogs,
GET id=0 returns ACK with total; GET id=1 through total returns one ITEM.
START and STOP select a command by name or numeric catalog id. An empty STOP
selects the active command. ACK means dispatch acceptance, not completion.
Text fields contain UTF-8 within their fixed byte limits.

Native fields do not contain destination identities, session tokens, command
arguments, full snapshots, telemetry, logs, catalog revisions or parameter
editability metadata. Use one IO instance and one operator client per routed operator bus. Replies
do not identify the requesting client; simultaneous clients can collide on IDs. Match replies against IO source, request_id, message
type, and the expected item. A catalog can change while it is being read.

The Copilot TUI polls status once per second, reads indexed catalogs, and sends
command START/STOP once without automatic retries or reconnect replay. Missing
ACKs leave outcomes unknown. IO caches up to 1024 mutation identities and
responses for its boot lifetime, rejects conflicting reuse, and returns BUSY
when full. The protocol has no persistent replay protection across IO restarts.
Client objects never reuse their 16-bit request IDs; restarting a client selects
a random initial ID. Native parameters can be SET individually; atomic,
revision-checked catalog saves require the IPC operator interface. The TUI
exposes native catalogs as read-only.
