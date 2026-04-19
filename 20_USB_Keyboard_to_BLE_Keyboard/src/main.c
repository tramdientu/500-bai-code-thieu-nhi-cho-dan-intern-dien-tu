#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"

static const char *TAG = "SPRINT1";

/* ============================================================
 *  HID Keycode → ASCII (Boot Protocol, US layout)
 * ============================================================ */
static const char hid_keycode_to_ascii[2][0x68] = {
    // [0] = không Shift
    {
        0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '\n', 0, '\b', '\t', ' ', '-', '=', '[', ']', '\\', 0, ';', '\'', '`', ',', '.', '/',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    // [1] = có Shift
    {
        0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        '\n', 0, '\b', '\t', ' ', '_', '+', '{', '}', '|', 0, ':', '"', '~', '<', '>', '?',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

/* ============================================================
 *  ⭐ ENUM FILTER CALLBACK - Force enumerate mọi device
 *  Cho phép ESP32 enumerate cả composite keyboards
 * ============================================================ */
static bool enum_filter_cb(const usb_device_desc_t *dev_desc, uint8_t *bConfigurationValue)
{
    ESP_LOGI(TAG, "🔍 Device detected:");
    ESP_LOGI(TAG, "   VID: 0x%04X, PID: 0x%04X", dev_desc->idVendor, dev_desc->idProduct);
    ESP_LOGI(TAG, "   Class: 0x%02X, SubClass: 0x%02X, Protocol: 0x%02X",
             dev_desc->bDeviceClass, dev_desc->bDeviceSubClass, dev_desc->bDeviceProtocol);
    ESP_LOGI(TAG, "   Num configs: %d", dev_desc->bNumConfigurations);

    // ⭐ Ép chọn configuration 1 (mặc định) - bypass abort
    *bConfigurationValue = 1;
    return true;  // ✅ Cho phép enumerate
}

/* ============================================================
 *  USB Host library task
 * ============================================================ */
static void usb_lib_task(void *arg)
{
    ESP_LOGI(TAG, "USB Lib task started on core %d", xPortGetCoreID());
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_LOGW(TAG, "⚠️ USB: No clients");
            usb_host_device_free_all();
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGW(TAG, "⚠️ USB: All devices freed");
        }
    }
}

/* ============================================================
 *  Parse HID keyboard report (Boot Protocol)
 * ============================================================ */
static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length)
{
    if (length < 8) {
        ESP_LOGW(TAG, "HID report quá ngắn: %d bytes", length);
        return;
    }

    uint8_t modifier = data[0];
    bool shift_pressed = (modifier & 0x22) != 0;

    ESP_LOGI(TAG, "📋 Raw: mod=0x%02X keys=[%02X %02X %02X %02X %02X %02X]",
             modifier, data[2], data[3], data[4], data[5], data[6], data[7]);

    for (int i = 2; i < 8; i++) {
        uint8_t keycode = data[i];
        if (keycode == 0) continue;
        if (keycode < 0x04 || keycode > 0x67) continue;

        char ch = hid_keycode_to_ascii[shift_pressed ? 1 : 0][keycode];
        if (ch != 0) {
            if (ch == '\n') ESP_LOGI(TAG, "🎹 Key: [ENTER]");
            else if (ch == '\b') ESP_LOGI(TAG, "🎹 Key: [BACKSPACE]");
            else if (ch == '\t') ESP_LOGI(TAG, "🎹 Key: [TAB]");
            else if (ch == ' ') ESP_LOGI(TAG, "🎹 Key: [SPACE]");
            else ESP_LOGI(TAG, "🎹 Key: '%c' (keycode=0x%02X)", ch, keycode);
        } else {
            ESP_LOGI(TAG, "🎹 Key: keycode=0x%02X (chưa map)", keycode);
        }
    }
}

/* ============================================================
 *  Interface callback
 * ============================================================ */
static void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,                                        const hid_host_interface_event_t event,                                        void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(
            hid_device_handle, data, sizeof(data), &data_length));

        if (dev_params.proto == HID_PROTOCOL_KEYBOARD) {
            hid_host_keyboard_report_callback(data, data_length);
        } else {
            ESP_LOGI(TAG, "📋 Non-keyboard report (%d bytes, proto=%d)", data_length, dev_params.proto);
            ESP_LOG_BUFFER_HEX(TAG, data, data_length);
        }
        break;

    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "🔌 HID Device DISCONNECTED");
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;

    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "⚠️ HID Transfer error");
        break;

    default:
        ESP_LOGW(TAG, "HID unhandled event: %d", event);
        break;
    }
}

/* ============================================================
 *  Device event - khi cắm/rút HID device
 * ============================================================ */
static void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                                    const hid_host_driver_event_t event,
                                    void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        const char *proto_str =
            (dev_params.proto == HID_PROTOCOL_KEYBOARD) ? "KEYBOARD" :
            (dev_params.proto == HID_PROTOCOL_MOUSE) ? "MOUSE" : "OTHER";

        ESP_LOGI(TAG, "🎹 HID Device CONNECTED - proto: %s, sub_class: %d, addr=%d, iface=%d",
                 proto_str, dev_params.sub_class, dev_params.addr, dev_params.iface_num);

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        esp_err_t ret = hid_host_device_open(hid_device_handle, &dev_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "❌ hid_host_device_open failed: 0x%x", ret);
            return;
        }

        // SET_PROTOCOL và SET_IDLE là OPTIONAL
        if (dev_params.sub_class == HID_SUBCLASS_BOOT_INTERFACE) {
            ret = hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "⚠️ SET_PROTOCOL failed (0x%x) - bỏ qua", ret);
            } else {
                ESP_LOGI(TAG, "✅ SET_PROTOCOL = BOOT OK");
            }

            if (dev_params.proto == HID_PROTOCOL_KEYBOARD) {
                ret = hid_class_request_set_idle(hid_device_handle, 0, 0);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "⚠️ SET_IDLE failed (0x%x) - bỏ qua", ret);
                } else {
                    ESP_LOGI(TAG, "✅ SET_IDLE OK");
                }
            }
        }

        ret = hid_host_device_start(hid_device_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "❌ hid_host_device_start failed: 0x%x", ret);
        } else {
            ESP_LOGI(TAG, "🚀 HID device started - GÕ PHÍM ĐỂ TEST!");
        }
    }
}

/* ============================================================
 *  Main entry point
 * ============================================================ */
void app_main(void)
{
    ESP_LOGI(TAG, "=====================================");
    ESP_LOGI(TAG, " ESP32-S3 USB-BLE Keyboard Bridge");
    ESP_LOGI(TAG, " Sprint 1: USB Host HID + Enum Filter");
    ESP_LOGI(TAG, "=====================================");
    ESP_LOGI(TAG, "USB OTG: GPIO19 (D-), GPIO20 (D+)");

    // 1. Install USB Host library ⭐ VỚI ENUM FILTER
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
        .enum_filter_cb = enum_filter_cb,  // ⭐ CHỐT QUAN TRỌNG
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));
    ESP_LOGI(TAG, "✅ USB Host installed (with enum filter)");

    // 2. Tạo USB library task
    xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, NULL, 2, NULL, 0);

    // 3. Install HID Host driver
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 8192,
        .core_id = 0,
        .callback = hid_host_device_event,
        .callback_arg = NULL
    };
    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));
    ESP_LOGI(TAG, "✅ HID Host driver installed");

    ESP_LOGI(TAG, "🔌 Ready! Plug in a USB keyboard...");

    // 4. Main loop
    int count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "💓 Alive [%d] | heap=%" PRIu32 " bytes",
                 count++, esp_get_free_heap_size());
    }
}
