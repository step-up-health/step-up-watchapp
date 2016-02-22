#include <pebble.h>
#include "health-utils.h"

#define UPDATE_INTERVAL 800
// over 720 because why not.

uint32_t health_sum_timeframe(HealthMetric metric, time_t time_start, time_t time_end) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Heap: %i", heap_bytes_free());
    uint32_t output = 0;
    APP_LOG(APP_LOG_LEVEL_INFO, "Metric: %i", metric);
    if (health_service_metric_accessible(metric, time_start, time_end) ==
            HealthServiceAccessibilityMaskAvailable) {
        HealthMinuteData * minute_data = malloc(sizeof(HealthMinuteData) * UPDATE_INTERVAL);
        time_t time_start_calc = time_start;
        while (time_start_calc < time_end &&
               time_start_calc < time(NULL)) {
            time_t time_start_temp = time_start_calc;
            time_t time_end_temp = time_start_calc + UPDATE_INTERVAL*60;
            if (time_end_temp > time_end) {
                time_end_temp = time_end;
            }
            if (time_end_temp > time(NULL)) {
                time_end_temp = time(NULL);
            }
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "start %lu", time_start_temp);
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "end %lu", time_end_temp);
            uint32_t minutes_gotten =
                    health_service_get_minute_history(minute_data, UPDATE_INTERVAL, &time_start_temp, &time_end_temp);
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "-> start %lu", time_start_temp);
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "-> end %lu", time_end_temp);
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "mins gotten %lu", minutes_gotten);
            for (uint32_t i = 0; i < minutes_gotten; i++) {
                if (!minute_data[i].is_invalid) {
                    output += minute_data[i].steps;
                }
            }
            APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "output now %lu", output);
            time_start_calc += UPDATE_INTERVAL*60;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "OUTPUT: %lu", output);
        free(minute_data);
    }
    return output;
}
