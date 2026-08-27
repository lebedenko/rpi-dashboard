# Remote telemetry UI and portable Python sender

## Behavior

The dashboard presents local and registered remote devices through one ordered model. The local
device is always `01`; remotes retain registry order as `02` onward. Registration appends a
collapsed card and does not change selection. Expanding selects a card, while collapsing it leaves
that card selected. Overview and Systems share the selected index.

Remote cards use the registered display name, falling back to hostname, and display `Registered`,
`Online`, `Stale`, or `Offline` with distinct semantic colors. Missing values render as `—`.
The latest complete remote snapshot populates system panels. Remote history is not retained and an
expanded remote card states “Remote history unavailable”; local history remains available.

The Python 3.8+ sender has no third-party dependencies. It persists a device UUID in the XDG data
directory, creates an instance UUID per run, registers before sending complete version-1 CBOR
snapshots, repeats registration every ten seconds, and samples every one to five seconds. Missing
or failed metric collections are omitted rather than invented. Temporary network failures are
retried; invalid configuration, rejected registration, and unsupported protocol responses fail.

## Acceptance criteria

- Model membership, roles, and updates follow the ordering and freshness behavior above.
- New remote telemetry does not reset card selection, expansion, or focus.
- Both device pages display the selected remote snapshot at 1480×320 without overflow.
- `scripts/rpi-dashboard-telemetry.py --once` interoperates with the C++ UDP receiver.
- Sender fixtures cover deterministic CBOR, identity persistence, Linux collection, rates, and CLI validation.

## Non-goals

- Remote metric history or persistence of live session state.
- Fabricated GPU metrics or a general GPU abstraction.
- Authentication, encryption, or non-Linux collection.
