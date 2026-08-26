# System page

## Purpose

Replace the Systems placeholder with a compact live view of the selected device. The composition follows the hierarchy and visual language of `docs/mockups/m2.png`, compressed into the dashboard's 1252×320 content region and optimized for the 1480×320 Raspberry Pi touchscreen.

## Observable behavior

- Overview and System use one selected-device index owned by `Main`.
- The System header shows one 48-logical-pixel device tab per model entry. Pointer activation and Space or Enter on a focused tab select that device.
- F5 on the System page focuses the selected device tab. Existing Home, Left, Right, and global status/clock behavior remain unchanged.
- The selected model entry supplies six horizontally arranged panels: CPU, GPU, Memory, Thermals, Network, and Uptime.
- CPU shows usage and the average of reported logical-CPU frequencies.
- GPU shows values from the first reported GPU only.
- Memory shows physical and swap usage derived from total and available byte counts.
- Network rates are summed across non-loopback interfaces. The interface name is shown only when exactly one contributing interface is reported.
- Uptime shows elapsed uptime and an inferred boot time based on the successful collection timestamp.
- Live service changes update the local model and visible values without replacing the model entry or changing selection/focus state.
- All content and focus indicators remain inside the 1480×320 display safe area.

## Unavailable values

- Missing frequency, GPU usage or temperature, board temperature, swap, network rates, and boot time render as `—`.
- Missing values never render as zero and no simulated values are used.
- Invalid memory totals or available values do not produce a usage ratio.
- A failed collection preserves the most recent successful snapshot and inferred boot time, while the service state and diagnostics still report the failure.

## Accessibility

- Device tabs expose button roles, translated accessible names, visible focus frames, and pointer/keyboard activation.
- Each metric panel exposes one translated semantic summary containing its visible values.
- User-visible labels are translatable and custom decoration is ignored by accessibility.
- Touch targets are at least 48 logical pixels.

## Non-goals

- Remote device discovery, transport, and production telemetry for non-local entries.
- Protocol-schema or collector-source changes.
- IP-address collection or new thermal-zone semantics.
- Fabricated Raspberry Pi GPU data.
- Gradients, blur, shadows, decorative animation, or an exact reproduction of the mockup's 1910×823 geometry.
- Physical touchscreen validation; that remains a hardware acceptance check.

## Acceptance criteria

- Service tests cover present and absent projections, memory calculations, average CPU frequency, first-GPU selection, non-loopback network aggregation/identity, and boot-time derivation/preservation.
- QML tests cover shared selection, pointer and keyboard device tabs, F5 focus, six-panel containment, accessible summaries, live formatting updates, and unavailable states.
- Focused tests, the full test suite, and static checks pass.
- A 1480×320 software-backend render is inspected for hierarchy, spacing, contrast, and safe-area containment.
