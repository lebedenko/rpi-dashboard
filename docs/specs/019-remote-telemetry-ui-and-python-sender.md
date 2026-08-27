# Remote telemetry UI and retired Python sender

## Behavior

The dashboard presents local and registered remote devices through one ordered model. The local
device is always `01`; remotes retain registry order as `02` onward. Registration appends a
collapsed card and does not change selection. Expanding selects a card, while collapsing it leaves
that card selected. Overview and Systems share the selected index.

Remote cards use the registered display name, falling back to hostname, and display `Registered`,
`Online`, `Stale`, or `Offline` with distinct semantic colors. Missing values render as `—`.
The latest complete remote snapshot populates system panels. Remote history is not retained and an
expanded remote card states “Remote history unavailable”; local history remains available.

The former Python sender has been superseded by the static C++ `dashboard-daemon` described in
specification 020.

## Acceptance criteria

- Model membership, roles, and updates follow the ordering and freshness behavior above.
- New remote telemetry does not reset card selection, expansion, or focus.
- Both device pages display the selected remote snapshot at 1480×320 without overflow.
- `dashboard-daemon` interoperates with the C++ UDP receiver.
- Daemon fixtures cover deterministic CBOR, identity persistence, Linux collection, and configuration validation.

## Non-goals

- Remote metric history or persistence of live session state.
- Fabricated GPU metrics or a general GPU abstraction.
- Authentication, encryption, or non-Linux collection.
