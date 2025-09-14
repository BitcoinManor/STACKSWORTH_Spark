// Adaptive time refresh: faster (5s) until NTP is valid, then 60s
static unsigned long t1 = 0;
unsigned long period = timeIsValid() ? 60000UL : 5000UL;
if (millis() - t1 > period) {
  ui_weather_set_time(now_time_12h());
  t1 = millis();
}
