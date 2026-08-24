# Dashboard Typography

## Context

The dashboard mockups use a condensed, technical visual style, but their AI-generated lettering does not identify a dependable real font. The dashboard needs a consistent typography contract that preserves that character when the preferred fonts are installed and remains usable with system fonts when they are not.

## Functional requirements

- UI headings and labels prefer Rajdhani at weight 600.
- Regular information prefers Rajdhani at weight 500.
- Large metric values prefer Rajdhani at weight 300.
- Technical values, including addresses and identifiers, prefer JetBrains Mono, then IBM Plex Mono.
- UI typography falls back to the platform sans-serif font family.
- Technical typography falls back to the platform fixed-width font family.
- Typography families and weights are centralized in the dashboard theme.

## Acceptance criteria

- Existing dashboard headings and navigation labels use the heading typography tokens.
- Existing placeholder information uses the regular-information typography tokens.
- Theme tokens are available for future large metrics and technical values.
- The dashboard initializes when none of the preferred font families are installed.

## Non-goals

- Bundling or downloading font files.
- Adding telemetry or other content solely to demonstrate typography.
- Using IBM Plex Sans Condensed as a second visual theme.

## Verification

- Verify the QML source uses the semantic theme tokens for every existing text role.
- Launch the dashboard with the offscreen Qt platform and verify QML initialization succeeds.
- Run the project test and static-analysis tasks.
