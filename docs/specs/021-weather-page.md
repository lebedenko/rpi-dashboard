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
  marked stale by age and is never used for another provider or location.
- The timeline contains the current observation and next seven hourly entries. The daily list
  contains the first five days.

## Presentation and interaction

- The header shows WEATHER, resolved city/country, update age, and persistent stale/error state.
- Current conditions show icon, temperature, condition, feels-like, high/low, humidity, and compass
  wind direction/speed.
- Hourly columns show localized time, icon, temperature, a cyan axis-free temperature trend, and
  violet precipitation bars.
- Daily rows show localized weekday, icon, rain probability, min/max, and a compact range rail.
- The contextual rail shows OpenWeather AQI category/index, local sunset, today's rain probability,
  and visible OpenWeather attribution with a link.
- Loading, unconfigured, stale, and unavailable states do not blank previously available data.
- Cached data remains visible and stale while location or forecast recovery is underway.
- F5 refreshes weather. Left, Right, and Home retain page navigation. Custom controls have useful
  accessible names, and touch targets are at least 48 logical pixels.

## Acceptance criteria

- Valid automatic, city, and coordinate configurations load; conflicts, partial/out-of-range
  coordinates, invalid intervals, TOML errors, and unknown providers fail deterministically.
- Provider fixture parsing handles optional values and timezone offsets without live API access.
- Weather state, diagnostics, cache freshness, eight hourly rows, five daily rows, partial AQI
- Weather state, diagnostics, cache freshness, eight hourly rows, five daily rows, partial AQI
  failure, location/forecast recovery, non-overlap, and manual retry after transient or
  authentication failures are observable through `WeatherService`.
- Runtime location, forecast, and AQI failures, plus relevant startup credential failures, reach
  the journal as sanitized warnings without API keys or credential-bearing URL query data.
- All 18 weather SVGs are packaged, use neutral fallback styles, are safely recolored through the
  local image provider, and invalid icon identifiers resolve to a neutral fallback.
- At 1480×320 all four content regions and the weather rail are visible. Weather states,
  accessibility, navigation, and F5 are covered by deterministic startup/QML tests.

## Non-goals

One Call 4.0, alerts, minute forecasts, imperial units, provider selection UI, a second weather
provider, selectable daily details, ambient mode, and live-network tests are not included.
