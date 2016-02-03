#include <pebble.h>

#define MSG_KEY_DATA_SIZE 0

static Window *window;
static MenuLayer *s_menu_layer;

static uint8_t friend_amt = 0;
static bool loaded = false;
static char** friend_names;
static uint32_t* friend_steps;

static bool selection_was_changed = false;

typedef enum {
    TimePeriodMorning,
    TimePeriodEvening,
} TimePeriod;

static TimePeriod time_period;

static uint32_t your_steps = 0;

static void send_user_steps() {
    // This used to run only when launch_reason() == APP_LAUNCH_WAKEUP.
    DictionaryIterator* iter;
    app_message_outbox_begin(&iter);
    dict_write_uint32(iter, 25973, your_steps);
    app_message_outbox_send();
}

static uint16_t main_menu_row_amt(MenuLayer *menu_layer,
        uint16_t section_num, void *callback_context) {
    if (!loaded) {
        return 2;
    } else if (loaded && friend_amt == 0) {
        return 2;
    } else {
        return 1 + friend_amt;
    }
}

static int16_t main_menu_row_size(MenuLayer *menu_layer,
        MenuIndex *cell_index, void *callback_context) {
    if (cell_index->row == 0) {
        return 50;
    }
    if (menu_layer_get_center_focused(menu_layer)) {
        if (menu_layer_get_selected_index(menu_layer).row == cell_index->row) {
            return 96;
        } else {
            return 42;
        }
    }
    return 95;
}

static void main_menu_draw_row(GContext *ctx, const Layer *cell_layer,
        MenuIndex *cell_index, void *callback_context) {
    MenuLayer *menu_layer = (MenuLayer*) callback_context;
    GRect bounds = layer_get_bounds(cell_layer);
    if (cell_index->row == 0) {
        GRect draw_rect = layer_get_bounds(cell_layer);
        draw_rect.size.w -= 20;
        draw_rect.origin.x += 10;
        draw_rect.origin.y += 10;
        char user_info[40];
        snprintf(user_info, 40, "You took %lu steps this %s.",
            your_steps,
            time_period == TimePeriodMorning ? "morning" : "evening");
        graphics_draw_text(ctx, user_info,
            fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
            draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else if (cell_index->row == 1 && !loaded) {
        GRect draw_rect = layer_get_bounds(cell_layer);
        draw_rect.size.w -= 20;
        draw_rect.origin.x += 10;
        if (menu_layer_get_center_focused(menu_layer) &&
            menu_layer_get_selected_index(menu_layer).row != cell_index->row) {
            draw_rect.origin.y += 2;
        } else {
            draw_rect.origin.y += PBL_IF_ROUND_ELSE(26 + 16, 26 + 8);
            graphics_draw_text(ctx, "Have you selected a username in the settings?",
                fonts_get_system_font(FONT_KEY_GOTHIC_18),
                draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
            draw_rect.origin.y -= PBL_IF_ROUND_ELSE(33, 30);
            draw_rect.size.h -= 50;
        }
        graphics_draw_text(ctx, "Loading...",
            fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
            draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else if (cell_index->row == 1 && friend_amt == 0) { // No friends yet
        GRect draw_rect = layer_get_bounds(cell_layer);
        draw_rect.size.w -= 20;
        draw_rect.origin.x += 10;
        draw_rect.origin.y += 8;
        graphics_draw_text(ctx, "You can add friends in the app settings. "
                                "They'll show up here about a day later.",
            fonts_get_system_font(FONT_KEY_GOTHIC_18),
            draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    } else if (cell_index->row - 1 < friend_amt) {
        uint8_t friend_num = cell_index->row - 1;
        GRect draw_rect = layer_get_bounds(cell_layer);
        if (menu_layer_get_center_focused(menu_layer) &&
            menu_layer_get_selected_index(menu_layer).row != cell_index->row) {
            graphics_draw_text(ctx, friend_names[friend_num],
                fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
            return;
        } else {
            graphics_draw_text(ctx, friend_names[friend_num],
                fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
        }
        draw_rect.origin.y += 28;
        char friendly_info_buff_1[10];
        char friendly_info_buff_2[50];
        uint8_t top_line_width = 0;
        uint8_t second_text_width = 0;
        snprintf(friendly_info_buff_1, 20,
            "%lu",
            friend_steps[friend_num]);
        GTextAttributes *text_attrs = graphics_text_attributes_create();
        graphics_text_attributes_enable_screen_text_flow(text_attrs, 10);
        {
            GSize text_size;
            text_size = graphics_text_layout_get_content_size("took ",
                fonts_get_system_font(FONT_KEY_GOTHIC_18), draw_rect,
                GTextOverflowModeWordWrap, GTextAlignmentCenter);
            top_line_width += text_size.w;
            text_size = graphics_text_layout_get_content_size(friendly_info_buff_1,
                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), draw_rect,
                GTextOverflowModeWordWrap, GTextAlignmentCenter);
            top_line_width += text_size.w;
            second_text_width = text_size.w;
        }
        snprintf(friendly_info_buff_2, 50,
            "steps last %s.\nCan you beat them?",
            time_period == TimePeriodMorning ? "morning" : "evening");

        graphics_draw_text(ctx, "took ",
            fonts_get_system_font(FONT_KEY_GOTHIC_18),
            (GRect) {
                .origin = GPoint((bounds.size.w - top_line_width) / 2,
                                 draw_rect.origin.y),
                .size = draw_rect.size
            }, GTextOverflowModeWordWrap, GTextAlignmentLeft, text_attrs);
        graphics_draw_text(ctx, friendly_info_buff_1,
            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
            (GRect) {
                .origin = GPoint((bounds.size.w + top_line_width - 2 * second_text_width) / 2,
                                 draw_rect.origin.y),
                .size = draw_rect.size
            }, GTextOverflowModeWordWrap, GTextAlignmentLeft, text_attrs);

        draw_rect.origin.y += 17;
        graphics_draw_text(ctx, friendly_info_buff_2,
            fonts_get_system_font(FONT_KEY_GOTHIC_18),
            draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, text_attrs);
        // draw_rect.origin.y += 24 + 10;

        // This code was used to display a split screen view of steps - you/person

        // graphics_draw_line(ctx, GPoint(bounds.size.w / 2, draw_rect.origin.y),
        //                         GPoint(bounds.size.w / 2, draw_rect.origin.y + 40));
        //
        // const uint8_t text_offset = 5;

        // draw_rect.size.w = bounds.size.w / 2 - text_offset;
        // draw_rect.origin.x = text_offset;
        // char buffer_they_steps[6];
        // snprintf(buffer_they_steps, 5, "%lu", friend_steps[friend_num]);
        // graphics_draw_text(ctx, buffer_they_steps,
        //     fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS),
        //     draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
        // draw_rect.origin.x = bounds.size.w / 2;
        //
        // char buffer_your_steps[6];
        // snprintf(buffer_your_steps, 5, "%lu", your_steps);
        // graphics_draw_text(ctx, buffer_your_steps,
        //     fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS),
        //     draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
        // draw_rect.origin.y += 22;
        //
        // draw_rect.size.w = bounds.size.w / 2 - text_offset;
        // draw_rect.origin.x = text_offset;
        // graphics_draw_text(ctx, "THEY",
        //     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
        //     draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
        // draw_rect.origin.x = bounds.size.w / 2;
        // graphics_draw_text(ctx, "YOU",
        //     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
        //     draw_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
}

static void main_menu_selection_changed(MenuLayer *menu_layer, MenuIndex to_index,
                            MenuIndex from_index, void *callback_context) {
    selection_was_changed = true;
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, s_menu_layer, (MenuLayerCallbacks) {
        .get_num_rows = main_menu_row_amt,
        .draw_row = main_menu_draw_row,
        .get_cell_height = main_menu_row_size,
        .selection_changed = main_menu_selection_changed,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorVividCerulean, GColorWhite);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
}

static void inbox_handler(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Inbox handler");
    Tuple *friend_amt_tup = dict_find(iterator, MSG_KEY_DATA_SIZE);
    friend_amt = friend_amt_tup->value->uint8;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%u friends sent.", friend_amt);
    friend_names = malloc(sizeof(char*) * friend_amt);
    friend_steps = malloc(sizeof(uint32_t) * friend_amt);
    for (uint8_t i = 0; i < friend_amt; i++) {
        Tuple *this_friend_name_tup = dict_find(iterator, 100 + i);
        friend_names[i] = malloc(this_friend_name_tup->length);
        strcpy(friend_names[i], this_friend_name_tup->value->cstring);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Friend name: %s", friend_names[i]);

        Tuple *this_friend_data_tup = dict_find(iterator, 200 + i);
        friend_steps[i] = this_friend_data_tup->value->uint32;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Friend steps: %lu", friend_steps[i]);
    }
    loaded = true;
    menu_layer_reload_data(s_menu_layer);

    send_user_steps();
}

static time_t today_beginning() {
    time_t now = time(NULL);
    struct tm here_time = *localtime(&now);
    here_time.tm_hour = 0;
    here_time.tm_min = 0;
    here_time.tm_sec = 0;
    return mktime(&here_time);
}

static void update_steps() {
    HealthServiceAccessibilityMask availability = health_service_metric_accessible(HealthMetricStepCount,
                                         time(NULL) - 360,
                                         time(NULL));
    time_t now_ts = time(NULL);
    struct tm *now = localtime(&now_ts);
    now->tm_hour = 0;
    now->tm_min = 0;
    now->tm_sec = 0;
    time_t midnight_ts = mktime(now);
    if (availability == HealthServiceAccessibilityMaskAvailable) {
        if (midnight_ts + 12*60*60 > time(NULL)) { // At or before 11:59am
            your_steps = health_service_sum(HealthMetricStepCount,
                midnight_ts,
                time(NULL));
            time_period = TimePeriodMorning;
        } else {
            your_steps = health_service_sum(HealthMetricStepCount,
                midnight_ts + 12*60*60,
                time(NULL));
            time_period = TimePeriodEvening;
        }
        // your_steps = health_service_sum_today(HealthMetricStepCount);
    }
    if (s_menu_layer != NULL) {
        menu_layer_reload_data(s_menu_layer);
    }
}

static void health_update(HealthEventType event, void *context) {
    update_steps();
}

static void maybe_auto_quit(void *callback_data) {
    if (!selection_was_changed) {
        window_stack_remove(window, false);
    }
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushing");
    window_stack_push(window, animated);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushed");
    update_steps();
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "available: %u", availability == HealthServiceAccessibilityMaskAvailable);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "nopermission: %u", availability & HealthServiceAccessibilityMaskNoPermission);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "notsupported: %u", availability & HealthServiceAccessibilityMaskNotSupported);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "notavailable: %u", availability & HealthServiceAccessibilityMaskNotAvailable);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "availability: %u", availability);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Steps gotten");

    app_timer_register(60 * 1000, maybe_auto_quit, NULL);

    app_message_register_inbox_received(inbox_handler);
    app_message_open(1024, 128);
    health_service_events_subscribe(health_update, NULL);
}

static void deinit(void) {
    window_destroy(window);
    time_t now_ts = time(NULL);
    struct tm *now = localtime(&now_ts);
    now->tm_hour = 0;
    now->tm_min = 0;
    now->tm_sec = 0;
    time_t push_ts = mktime(now);
    while (push_ts < now_ts) {
        push_ts += 24*60*60;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "first wakeup pushed at %lu", push_ts);
    wakeup_schedule(push_ts, 40, false);
    push_ts += 12*60*60;
    wakeup_schedule(push_ts, 40, false);
    push_ts += 12*60*60;
    wakeup_schedule(push_ts, 40, false);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "last wakeup pushed at %lu", push_ts);
}

int main(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initing");
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}
