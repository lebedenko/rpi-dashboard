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

- The entire page, including its header and master/detail content, is enclosed by one flat frame
  with four equal `Theme.chamferMedium` corners. The list, detail, project rows, and stage cards use
  `Theme.chamferSmall`. Metric tiles use square frames. All decorative borders use the reusable
  `Frame` component. The page uses no gradient, shadow, blur, or `Canvas` effects.
- The translated source title is `Projects`, is visually rendered as `PROJECTS`, uses the primary
  accent, and uses the Rajdhani heading role. Project, health, stage, and outcome labels render
  uppercase; list branches and relative ages render lowercase.
- The summary styles `TRACKED`, `RUNNING`, and `FAILED` independently. Tracked content is secondary;
  a non-zero running number is cyan; and a non-zero failed number and label use the failure color.
  Zero running and failed values use the regular information color.
- The content uses an 8 px inset, a 48 px header with vertically centered content, a 330 px
  scrollable project list, 56 px rows with 6 px gaps, and a detail pane that consumes the remaining
  width. The list viewport is inset 8 px from its frame so the complete chamfered border remains
  visible.
- Project row frames have only passive and selected visual states and never encode CI health. Each
  row uses two aligned text lines: repository opposite a disc-prefixed health label, then branch
  opposite relative age. A thick left rail, the status disc, and the translated status label are
  the only row elements colored by CI health, so color is never the only status indication. Project
  names use native, fully hinted text rendering so thin uppercase strokes remain intact at the
  target pixel size.
- Rows are selectable by pointer and by Up/Down followed by Space or Enter. F5 focuses the selected
  project row. Global Left, Right, and Home page navigation remains available.
- Detail shows repository, default branch, short revision, run number and age, at most four pipeline
  cards, duration, successful/total jobs, non-expired artifact bytes, inferred deploy state, and the
  latest 20 outcomes. Missing values render as unavailable rather than fabricated metrics.
- Detail separates uppercase repository/branch/revision/health from right-aligned run number/age.
  Stage cards show uppercase names and outcomes with semantic color and lightweight connector/check
  cues. Metrics are labelled `DURATION`, `JOBS`, `ARTIFACTS`, and `DEPLOY`, while values preserve
  natural technical casing. History is introduced by `RUN HISTORY (%1)` and `%1 / %2 SUCCESS`.
- Four or fewer jobs map directly to cards. More jobs show the first three plus a `+N jobs` card
  whose status is the worst hidden job status. Deploy jobs contain `deploy`, `release`, or `publish`
  case-insensitively.
- Health mapping is: incomplete → Running; success → Healthy (Stale after seven days); failure,
  timed-out, startup-failure, or action-required → Failed; neutral, cancelled, or skipped →
  Attention; GitHub stale → Stale; missing or invalid data → Unknown.
- Projects sort by latest CI execution time in descending order, regardless of outcome. Reordering
  preserves the selected repository key, keeps the list highlight synchronized with the detail
  pane, and retains the selected project's loaded metrics while refreshed details are in flight.
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
- Each response is limited to 4 MiB and validated against its endpoint schema before any model
  mutation. Oversized or malformed successful responses follow the normal retained-stale-data path.
- All custom controls expose accessible roles/names, touch targets are at least 48 logical pixels,
  and visible strings are translatable.

## Non-goals

- No protocol schema changes or credential-management UI.
- No stage-detail or pipeline-history page, log viewer, retry, deploy, terminal, edit, or overflow
  actions.
- No additional CI provider.
- No settings UI or configurable polling interval.
- No inferred external-service uptime, generic coverage, or compiler-warning totals.
- No fabricated coverage, warning, Raspberry Pi 5, or other mockup-only values.
- No gradients, shadows, blur, or canvas-based decoration.
- Runner availability is best-effort and never affects repository health.

## Deferred performance follow-ups

- Avoid rebuilding the translated status-label map on every `statusLabel()` call.
- Explicitly select `Text.PlainText` for content that is guaranteed to contain plain text only.
