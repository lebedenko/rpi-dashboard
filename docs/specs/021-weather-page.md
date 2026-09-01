# Weather page

## Goal

The dashboard provides a compact weather page for the fixed 1480×320 Raspberry Pi 5 display. It
shows current conditions, the next eight hourly observations, five daily forecasts, and a
weather-specific status rail without blocking the GUI thread.

## Configuration

The application discovers `rpi-dashboard/config.toml` through the XDG configuration directories.
`--config PATH` overrides discovery; an unreadable or malformed explicit file is fatal. A missing
discovered file leaves weather unconfigured while the rest of the dashboard remains usable.

```toml
[weather]
provider = "openweather"
refresh_interval_seconds = 600

[weather.location]
automatic_provider = "ipgeolocation"
# city = "Lviv,UA"
# latitude = 49.8397
# longitude = 24.0297
```

Latitude and longitude must occur together and be in range. Coordinates conflict with `city`.
Otherwise a non-empty city uses OpenWeather direct geocoding; otherwise the configured automatic
provider is used. Unknown providers and refresh intervals below 60 seconds are rejected.

Secrets are read from `OPENWEATHER_API_KEY_FILE` and `IPGEOLOCATION_API_KEY_FILE`, with
`OPENWEATHER_API_KEY` and `IPGEOLOCATION_API_KEY` as development fallbacks. File values take
precedence and trailing newlines are removed. Diagnostics contain neither keys nor complete
credential-bearing URLs.

## Behaviour

- OpenWeather One Call 3.0 and current air pollution are requested concurrently, each with an
  independent timeout. Forecast success is publishable when AQI fails.
- Automatic and city locations are resolved until successful. Once resolved, scheduled refreshes
  request forecasts without resolving the location again. Location and forecast requests never
  overlap. Authentication failures stop timed retries, but F5/manual refresh retries immediately;
  transient failures use bounded exponential backoff and respect Retry-After.
- Provider-neutral snapshots use Celsius, km/h, millimetres, percentages, hPa, and μg/m³.
- Matching snapshots are atomically cached below `QStandardPaths::CacheLocation`. A loaded cache is
  marked stale by age and is never used for another provider or location. Cache schema version 1
  is validated completely before publication; legacy, truncated, wrongly typed, or out-of-range
  cache data is ignored atomically.
- Weather and geolocation response bodies are limited to 1 MiB. Oversized responses are reported
  through the existing sanitized failure paths.
- The service caches the provider's complete hourly forecast while its public hourly model exposes
  only the current observation and next seven hourly entries. The daily list contains the first
  five days.

## Presentation and interaction

- The shell is inset 10 px vertically, uses 8 px horizontal gaps, and all shell/card corners use
  the large chamfer. The header restores the `WEATHER` hierarchy followed by resolved city/country,
  a status marker, `UPDATED`, and relative age (`NOW`, minutes, hours, or days). Its left and right
  padding are equal and its minute clock runs only while the page is visible.
- Current conditions use 16 px horizontal content padding and show icon, temperature, condition,
  feels-like, high/low, humidity, and compass wind direction/speed. The separator and bottom metric
  row extend to those content edges, and the row has an 8 px bottom margin. High/low are centered
  inline label/value pairs with cyan labels and primary-colored temperature values; humidity and
  wind remain centered label/value stacks.
- Hourly columns show `NOW` followed by the location's zero-padded numeric 24-hour local hour
  (`00`–`23`), icon, temperature, a min/max-normalized cyan trend (centered when all values are
  equal), and proportional precipitation bars hidden at zero percent. Hour and precipitation labels
  use the body text size; hour labels use the primary accent. Straight graph connections use the
  Qt Quick Shapes curve renderer with round caps and cyan points.
- Daily rows and their horizontal separators have equal 8 px left/right margins. Rows use 8 px gaps
  between cyan `TODAY`/weekday labels, icon, violet
  precipitation probability, minimum, range rail, and maximum. Minimum temperatures are
  right-aligned and maximum temperatures left-aligned so the visual gaps beside the rail match. The
  rail knob uses the provider-derived mean of available morning/day/evening/night temperatures.
  Legacy cached rows without a mean omit the knob.
- The page shell and current, hourly, and daily forecast panels use the reusable `Frame` component
  with their established fills, border colors, 1 px widths, and medium rounded corners. Chart and
  range indicators remain simple rectangles rather than decorative frames.
- Daily rain and snow millimetres are optional, cached fields. Today's precipitation kind remains
  classified from daily rain/snow data. Its displayed probability is the maximum hourly probability
  from the bucket containing the current local hour through local end of day; elapsed buckets,
  invalid timestamps, and the next local day are excluded. It is `0%` when no qualifying bucket
  exists, and is recalculated after publication, cache loading, and at every local-hour boundary.
  The former rain probability property remains a compatibility alias to this derived value.
- The contextual rail always shows date/time. It is empty on Overview/System, shows CI/runners only
  on Projects, and on Weather shows OpenWeather AQI category/native 1–5 index, the next sunrise or
  sunset, and classified precipitation wording with a numeric percentage. A zero remaining-day
  probability instead displays `PRECIPITATION` and `NONE`.
- Loading, unconfigured, stale, and unavailable states do not blank previously available data.
- Cached data remains visible and stale while location or forecast recovery is underway.
- F5 refreshes weather. Left, Right, and Home retain page navigation. Custom controls have useful
  accessible names, and touch targets are at least 48 logical pixels.

## Acceptance criteria

- Valid automatic, city, and coordinate configurations load; conflicts, partial/out-of-range
  coordinates, invalid intervals, TOML errors, and unknown providers fail deterministically.
- Provider fixture parsing handles optional values and timezone offsets without live API access.
- Weather state, diagnostics, cache freshness, eight hourly rows, five daily rows, partial AQI
  failure, location/forecast recovery, non-overlap, and manual retry after transient or
  authentication failures are observable through `WeatherService`.
- Runtime location, forecast, and AQI failures, plus relevant startup credential failures, reach
  the journal as sanitized warnings without API keys or credential-bearing URL query data.
- All 18 weather SVGs are packaged, use neutral fallback styles, are safely recolored through the
  local image provider, and invalid icon identifiers resolve to a neutral fallback.
- At 1480×320 all four content regions and the weather rail are visible with approximately 340 px
  for current conditions and 500 px for hourly forecast. Weather states,
  accessibility, navigation, and F5 are covered by deterministic startup/QML tests.

## Deferred review findings

- Explicit `Text.PlainText` formatting is deferred to a separate text-rendering cleanup.
- Synchronous weather-icon decoding must be profiled before considering asynchronous image loading.

## Non-goals

One Call 4.0, alerts, minute forecasts, imperial units, provider selection UI, a second weather
provider, selectable daily details, ambient mode, and live-network tests are not included.
