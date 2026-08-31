# Weather screensaver

## Intent

Show current weather as an atmospheric, full-screen idle view without replacing or navigating the
dashboard page the user was viewing.

## Behavior

The inactivity timer starts with the application and uses
`display.screensaver_timeout_seconds`, which defaults to 600. Keyboard, mouse movement/buttons/wheel,
and touch activity restart the timer. A value of zero disables both activation and wake filtering;
negative values are invalid. There is no command-line override.

When the timer expires, a lazily loaded view covers the complete window. Its hero begins 64 pixels
from the left, uses a 110-pixel condition icon, a 152-pixel temperature, and 32-pixel gaps between
the icon, temperature, and condition block. The condition sits 6 pixels above the location line.
The location baseline aligns with the temperature baseline. A stale marker appears beside the
location on that same baseline. The view contains no clock, navigation, forecasts, or controls.
Entry and exit use a 300 ms black transition.

The lower-right details line presents independently styled labels and values for feels-like
temperature, daily high and low, and the next solar event. Labels and vertical separators use muted
text; the dot between high and low uses the violet accent. Temperature values use the focus accent
and the solar-event time uses the attention status color.
Its text edge is 32 pixels from the right and 14 pixels from the bottom. A borderless rounded
background provides 12 pixels of horizontal and 6 pixels of vertical padding. Its background alpha
is 0.78 for day scenes and 0.62 for night scenes.

The current validated OpenWeather icon code selects the corresponding bundled day/night wallpaper.
All 18 supported codes (`01`–`04`, `09`–`11`, `13`, and `50`, each day and night) map directly to
their wallpaper. Missing or invalid codes use the neutral `03d` scene. Weather continues refreshing
while the view is present. Unavailable values use placeholders; stale and unavailable state remain
visible.

The validated icon suffix also selects a wallpaper contrast treatment. A frameless, full-height
left scrim extends 1120 pixels and fades at gradient positions 0.0, 0.85, and 1.0. Day scenes use
background alpha values 0.68, 0.58, and 0.0; night scenes use 0.24, 0.18, and 0.0. Wallpapers are
not modified and the composition does not use shadows, outlines, blur, or shader effects.

The screensaver shows the earliest sunrise or sunset timestamp later than the current UTC time from
the available daily forecasts. This yields sunrise before dawn, sunset during daylight, and the
following sunrise after sunset. The selected event switches at its boundary without requiring a
weather refresh. Its time is formatted in the forecast location's UTC offset using the current
locale. If forecasts contain no future solar timestamp, the current validated day/night icon chooses
`SUNSET` or `SUNRISE` respectively and the time is shown as `—`. Cache files without daily solar
timestamps remain readable. The weather sidebar continues to use the existing current-day sunset.

The first keyboard, mouse, or touch activity dismisses the view and is consumed. A touch wake consumes
the complete active touch sequence. The underlying page stack and focus state are not recreated or
changed, and the screensaver view unloads after its exit transition.

## Acceptance criteria

- The default timeout is 600 seconds; explicit non-negative values parse and negative values fail.
- Zero disables activation and application-wide input filtering.
- Inactivity activates the controller, activity resets an inactive timer, and the active state emits
  change notification.
- Keyboard, mouse movement/buttons/wheel, and touch can wake the view without operating dashboard UI.
- The view fills 1480×320, matches the weather hierarchy above, and preserves the selected page.
- Every supported icon maps exactly to its packaged, decodable wallpaper and changes immediately with
  the service icon; invalid icons select `03d`.
- The hero uses the specified size, spacing, and baseline alignment, including the adjacent
  stale marker.
- Detail tokens use their specified colors, backing, and 32-pixel right/14-pixel bottom placement.
- Day and night wallpapers use their specified scrim and detail-backing strengths.
- The next chronological solar event is selected across day boundaries, is locale-formatted at the
  forecast UTC offset, and switches when its timestamp is reached without a network refresh.
- Missing and legacy-cached solar timestamps use the icon-derived label and an em-dash time.
- Entry and exit transition through black for 300 ms, and the view unloads after exit.

## Non-goals

- Animated clouds, particles, panning, scaling, or other continuous motion effects.
- A clock, forecast rail, navigation, controls, or a command-line timeout override.
- Pausing weather collection or changing dashboard navigation APIs.
- Changing the weather sidebar's current-day sunset behavior or modifying wallpaper assets.
