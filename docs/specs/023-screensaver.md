# Weather screensaver

## Intent

Show current weather as an atmospheric, full-screen idle view without replacing or navigating the
dashboard page the user was viewing.

## Behavior

The inactivity timer starts with the application and uses
`display.screensaver_timeout_seconds`, which defaults to 600. Keyboard, mouse movement/buttons/wheel,
and touch activity restart the timer. A value of zero disables both activation and wake filtering;
negative values are invalid. There is no command-line override.

When the timer expires, a lazily loaded view covers the complete window. The view follows the
composition in `docs/mockups/m6.png`: condition icon, dominant temperature, condition and location at
left, with feels-like temperature, daily high/low, and sunset at lower right. It contains no clock,
navigation, forecasts, or controls. Entry and exit use a 300 ms black transition.

The current validated OpenWeather icon code selects the corresponding bundled day/night wallpaper.
All 18 supported codes (`01`–`04`, `09`–`11`, `13`, and `50`, each day and night) map directly to
their wallpaper. Missing or invalid codes use the neutral `03d` scene. Weather continues refreshing
while the view is present. Unavailable values use placeholders; stale and unavailable state remain
visible.

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
- Entry and exit transition through black for 300 ms, and the view unloads after exit.

## Non-goals

- Animated clouds, particles, panning, scaling, or other continuous motion effects.
- A clock, forecast rail, navigation, controls, or a command-line timeout override.
- Pausing weather collection or changing dashboard navigation APIs.
