# Dashboard Typography

## Context

The dashboard uses a condensed, technical visual style. Its canonical Rajdhani and JetBrains Mono faces are application resources so rendering does not depend on the fonts installed on the host.

## Functional requirements

- Bundle the OFL-licensed Rajdhani 300, 400, 500, and 600 faces and JetBrains Mono 300, 400, and 500 faces with the dashboard QML module.
- Load every bundled face through `FontLoader` in the theme and expose whether all faces are ready.
- UI headings use Rajdhani at weight 400.
- Regular labels use Rajdhani at weight 400 and ordinary information uses Rajdhani at weight 500.
- Large metric values use Rajdhani at weight 300.
- Technical values, including branches, revisions, identifiers, ages, durations, counts, and sizes, use JetBrains Mono at the matching semantic weight.
- UI typography falls back to the platform sans-serif font family.
- Technical typography falls back to the platform fixed-width font family.
- Typography families and weights are centralized in the dashboard theme.

## Acceptance criteria

- All seven bundled faces report `FontLoader.Ready` during normal startup.
- Theme family tokens resolve to the bundled families when ready and retain platform sans-serif and fixed-width fallbacks if a resource fails.
- Existing dashboard headings, labels, information, metrics, and technical values use their semantic family and weight tokens.
- The dashboard initializes when none of the preferred font families are installed.

## Non-goals

- Adding telemetry or other content solely to demonstrate typography.
- Using IBM Plex Sans Condensed as a second visual theme.

## Verification

- Verify every font resource is packaged and loads, and QML source uses semantic theme tokens.
- Launch the dashboard with the offscreen Qt platform and verify QML initialization succeeds.
- Run the project test and static-analysis tasks.
