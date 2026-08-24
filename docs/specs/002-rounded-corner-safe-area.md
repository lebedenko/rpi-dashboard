# Rounded-Corner Display Safe Area

## Context

The validated Waveshare product 25623 panel has rounded physical viewport corners that obscure framebuffer pixels. Backgrounds should remain full bleed, while meaningful content needs one shared, measured inset from the physical outer edges.

This specification covers measurement on the validated panel and application of the resulting token. It does not authorize estimating the inset from product photographs or nominal glass dimensions.

## Measurement procedure

1. Temporarily replace the normal dashboard root view with a fullscreen calibration view at the logical 1480×320 geometry. Do not add a permanent launcher option or runtime diagnostic.
2. Render a high-contrast solid background, known-coordinate fiducials near all four corners, and an adjustable rectangular outline. Show the current integer inset in the center. Provide 1 px and 5 px increase/decrease controls that work without placing meaningful labels in the corner cutouts.
3. With reflections minimized and the camera approximately perpendicular to the glass, capture at original resolution:
   - one straight-on photograph containing the entire illuminated viewport and all fiducials;
   - one close photograph of each corner containing that corner's fiducials and visible physical boundary.
4. Preserve the original photographs as validation evidence outside the runtime application. Do not commit resized or recompressed substitutes as measurement sources.
5. Use the known fiducial coordinates to perspective-correct each photograph into logical display coordinates.
6. Trace the visible boundary at each corner and fit a circle. Record the fitted radius and root-mean-square fit error in logical pixels. If the residuals show that a circle is not an adequate description, also fit and report effective horizontal and vertical radii.
7. For each corner, find the smallest equal horizontal and vertical integer inset for which the calibration rectangle's complete outline is visible.
8. Set the final safe inset to `ceil(maximum per-corner safe inset) + 2 px`.
9. Re-display the calibration view with the final value and confirm that the complete outline is visible at all four corners.
10. Remove the calibration component, restore the normal dashboard root view, and record the dated results in `docs/hardware-validation.md`.

## Functional requirements

- `Theme.displaySafeInset` is a read-only integer containing the measured final inset.
- The sidebar remains exactly 88 logical pixels wide.
- The sidebar and page background surfaces continue to fill their complete rectangular regions beneath the rounded glass.
- Full-bleed surfaces do not draw decorative borders along physical display edges; the sidebar draws only its internal separator against the page region.
- Sidebar navigation frames remain fully inside `Theme.displaySafeInset` on the physical left, top, and bottom edges.
- Page content uses `max(existing margin, Theme.displaySafeInset)` on the physical top, right, and bottom edges.
- Page spacing at the internal sidebar/page boundary remains unchanged; the compact navigation rail centers its button frames horizontally.
- Overview, Systems, Projects, and Weather use the same safe-area policy.
- Touch navigation and Left, Right, Home, and F5 behavior do not change.

## Acceptance criteria

- [x] The validation record identifies the Waveshare product 25623 panel and test date.
- [x] Original-resolution straight-on and four-corner photographs are available as measurement evidence.
- [x] Each corner has a recorded radius estimate, fit error, and verified safe inset; non-circular corners also have effective horizontal and vertical radii.
- [x] The recorded final value equals `ceil(maximum per-corner safe inset) + 2 px`.
- [x] The calibration outline at the measured 9 px inset is completely visible at all four corners; the final 11 px token adds clearance.
- [x] `Theme.displaySafeInset` equals the recorded final value.
- [ ] Dashboard title, page headings, controls, icons, and focus borders are unobscured on every page.
- [ ] The sidebar is exactly 88 px wide and all background surfaces remain edge-to-edge.
- [ ] Touch navigation and Left, Right, Home, and F5 behavior are unchanged.
- [ ] The focused dashboard startup test, `task test`, and `task check` pass.

The criteria remain unchecked until measurements and physical-panel verification are completed. Automated startup or layout checks cannot substitute for evidence of the glass boundary.

## Validation record

| Corner | Radius (px) | RMS fit error (px) | Effective X radius (px) | Effective Y radius (px) | Smallest safe inset (px) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Top left | ≈20 | ≈2.5 | ≈17 | ≈22 | ≤9 (9 verified) |
| Top right | ≈22 | ≈2.5 | ≈22 | ≈23 | ≤9 (9 verified) |
| Bottom right | ≈26 | ≈3 | ≈27 | ≈25 | ≤9 (9 verified) |
| Bottom left | ≈23 | ≈3 | ≈20 | ≈25 | ≤9 (9 verified) |

- Test date: 2026-08-24
- Maximum measured safe inset: 9 px; at 8 px the limiting magenta corner lost one edge pixel
- Guard: 2 px
- Final `Theme.displaySafeInset`: 11 px
- Measurement uncertainty: approximately ±1 px for the directly observed inset and ±4 px for photo-derived radii; the inset, not the radius estimate, controls layout
- Calibration-outline verification: Passed at 9 px in one full-display and four close-corner original-resolution photographs
- Final-dashboard verification: Pending

## Non-goals

- Deriving content clearance from the fitted radius instead of the directly observed safe inset.
- Assuming that the measurement applies to replacement panels or other display models.
- Permanently shipping calibration UI, command-line switches, launch modes, photographs, or image-analysis tooling.
- Changing page navigation, keyboard shortcuts, touch mapping, or output rotation.
- Preventing touch hit areas from extending into the inset when all visible and meaningful content remains unobscured.

## Verification

Run the focused startup test first, followed by `task test` and `task check`. On the physical panel, verify the final calibration outline, then inspect Overview, Systems, Projects, and Weather with keyboard focus moved through all available controls. Record every physical acceptance criterion as passed or failed in this specification and copy the measurements, uncertainty, date, and outcome to the hardware validation report.
