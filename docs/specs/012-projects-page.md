# Projects health page

## Intent

Replace the Projects placeholder with the compact master/detail CI health board shown in
[`docs/mockups/m5.png`](../mockups/m5.png). The page is designed first for the dashboard's
1480×320 landscape display and displays live GitHub Actions data for repositories owned by the
configured GitHub user or organization.

## Configuration and data source

- `--github-owner <login>` selects the account and defaults to `lebedenko`.
- `GITHUB_TOKEN` is optional. Public repositories remain usable without it and the token must never
  appear in diagnostics or logs.
- Fine-grained tokens require repository Metadata and Actions read access for private workflow
  data. Repository Administration read access enables runner inventory; its absence is non-fatal.
- Discover all non-archived owner repositories, including token-visible private repositories, and
  retain only repositories with a default-branch workflow run.
- The displayed pipeline is the latest default-branch run across all workflows. Retain the latest
  20 outcomes and load jobs and artifacts for the selected run.

## Observable acceptance criteria

- The content uses an 8 px inset, a 30 px header, a 330 px scrollable project list, 56 px rows with
  6 px gaps, and a detail pane that consumes the remaining width.
- Project rows expose a semantic edge, dot, and translated status label. Color is never the only
  status indication.
- Rows are selectable by pointer and by Up/Down followed by Space or Enter. F5 focuses the selected
  project row. Global Left, Right, and Home page navigation remains available.
- Detail shows repository, default branch, short revision, run number and age, at most four pipeline
  cards, duration, successful/total jobs, non-expired artifact bytes, inferred deploy state, and the
  latest 20 outcomes. Missing values render as unavailable rather than fabricated metrics.
- Four or fewer jobs map directly to cards. More jobs show the first three plus a `+N jobs` card
  whose status is the worst hidden job status. Deploy jobs contain `deploy`, `release`, or `publish`
  case-insensitively.
- Health mapping is: incomplete → Running; success → Healthy (Stale after seven days); failure,
  timed-out, startup-failure, or action-required → Failed; neutral, cancelled, or skipped →
  Attention; GitHub stale → Stale; missing or invalid data → Unknown.
- Projects sort Failed, Attention, Running, Stale, Unknown, then Healthy. Reordering preserves the
  selected repository key.
- Loading, empty, first-load error, retained-stale-data, and unavailable-field states are explicit.
- The global status rail appears on every page, remains outside the focus chain, reports worst
  tracked-project CI health, and reports unique online/total self-hosted runners only when every
  required runner request succeeds; otherwise it shows `— RUNNERS`.
- Requests time out independently after 10 seconds with at most four in flight. Authenticated data
  refreshes every minute. Anonymous polling is calculated from the requests made during repository
  discovery and run loading to remain within 50 requests/hour and is never faster than 15 minutes.
- `Retry-After` delays polling only for `403` and `429` responses. `X-RateLimit-Reset` delays polling
  only when `X-RateLimit-Remaining` is zero. The normal authenticated or anonymous interval is
  restored after every successful refresh. Conditional responses are used where available. A
  failed refresh retains the last successful snapshot, marks it stale, and exposes the last-success
  UTC time and a concise diagnostic.
- All custom controls expose accessible roles/names, touch targets are at least 48 logical pixels,
  and visible strings are translatable.

## Non-goals

- No protocol schema changes or credential-management UI.
- No stage-detail or pipeline-history page, log viewer, retry, deploy, terminal, edit, or overflow
  actions.
- No additional CI provider.
- No settings UI or configurable polling interval.
- No inferred external-service uptime, generic coverage, or compiler-warning totals.
- Runner availability is best-effort and never affects repository health.
