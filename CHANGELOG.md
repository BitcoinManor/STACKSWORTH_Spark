# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog], and this project adheres to [Semantic Versioning].

## [Unreleased]
### Planned
- HTTPS robustness for mempool/market APIs (use `WiFiClientSecure::setInsecure()` or add NTP + CA).
- Pre-screen visual polish (remove stray bars, tidy layout).

## [v0.0.3] - 2025-09-09
### Added
- Captive-portal Wi-Fi onboarding via SoftAP + SPIFFS-served page.
- SoftAP SSID `STACKSWORTH-SPARK-<MAC>`; captive DNS redirect; `/ping` health probe.
- `/save` endpoint accepts `application/x-www-form-urlencoded` **and** JSON; `/reboot` endpoint.
- On-device pre-screen showing SSID and setup URL during portal mode.

### Changed
- Boot flow: try saved Wi-Fi → else start portal; initialize LVGL **after** that decision.
- `loop()` pumps DNS while portal is active; normal 30s data fetch cadence in STA mode.

### Notes
- Requires SPIFFS upload with `/STACKS_Wifi_Portal.html.gz` before flashing.
- Matrix firmware is unchanged.

[Unreleased]: https://github.com/<org>/<repo>/compare/v0.0.3...HEAD
[v0.0.3]: https://github.com/<org>/<repo>/releases/tag/v0.0.3
[Keep a Changelog]: https://keepachangelog.com/en/1.0.0/
[Semantic Versioning]: https://semver.org/spec/v2.0.0.html
