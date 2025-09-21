#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <esp_matter.h>
#include <led_driver.h>

#include <app_priv.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t light_endpoint_id;
static led_driver_handle_t handle;

/*
 * Translation Functions
*/
int map_brightness(uint8_t matter_val) {
    return (matter_val * 100) / MATTER_BRIGHTNESS;
}

int map_hue(uint8_t matter_val) {
    return (matter_val * 360) / MATTER_HUE;
}

int map_saturation(uint8_t matter_val) {
    return (matter_val * 100) / MATTER_SATURATION;
}

int mired_to_kelvin(uint16_t mireds) {
    return MATTER_TEMPERATURE_FACTOR / mireds;
}

static esp_err_t app_driver_light_set_brightness(esp_matter_attr_val_t *val) {
    int pwm_val = map_brightness(val->val.u8);
    led_driver_set_brightness(handle, pwm_val);
    return ESP_OK;
}

static esp_err_t app_driver_light_set_color_temp(esp_matter_attr_val_t *val) {
    int kelvin = mired_to_kelvin(val->val.u16);
    led_driver_set_temperature(handle, kelvin);
    return ESP_OK;
}

static esp_err_t app_driver_light_set_hue(esp_matter_attr_val_t *val) {
    int hue_deg = map_hue(val->val.u8);
    led_driver_set_hue(handle, hue_deg);
    return ESP_OK;
}

static esp_err_t app_driver_light_set_saturation(esp_matter_attr_val_t *val) {
    int sat = map_saturation(val->val.u8);
    led_driver_set_saturation(handle, sat);

    return ESP_OK;
}

static esp_err_t app_driver_light_set_on_off(esp_matter_attr_val_t *val) {
    ESP_LOGI(TAG, "Changing the GPIO LED!");

    led_driver_set_power(handle, val->val.b);

    return ESP_OK;
}

esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val) {
    esp_err_t err = ESP_OK;

    if (endpoint_id == light_endpoint_id) {
        if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id) {
            err = app_driver_light_set_on_off(val);
        } 
        else if (cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            err = app_driver_light_set_brightness(val);
        }
        else if (cluster_id == ColorControl::Id) {
            if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
                err = app_driver_light_set_hue(val);
            } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
                err = app_driver_light_set_saturation(val);
            } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
                err = app_driver_light_set_color_temp(val);
            }
        }
    }

    return err;
}

app_driver_handle_t app_driver_light_init() {
    led_driver_config_t config = led_driver_get_config();

    config.gpio = 5;

    handle = led_driver_init(&config);

    return (app_driver_handle_t)handle;
}

esp_err_t app_driver_light_set_defaults(uint16_t endpoint_id) {
    esp_err_t err = ESP_OK;
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = NULL;
    attribute_t *attribute = NULL;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    // Set On/Off
    cluster = cluster::get(endpoint, OnOff::Id);
    attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_on_off(&val);

    // Set Brightness
    cluster = cluster::get(endpoint, LevelControl::Id);
    attribute = attribute::get(cluster, LevelControl::Attributes::CurrentLevel::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_brightness(&val);

    cluster = cluster::get(endpoint, ColorControl::Id);
    attribute = attribute::get(cluster, ColorControl::Attributes::CurrentHue::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_hue(&val);

    attribute = attribute::get(cluster, ColorControl::Attributes::CurrentSaturation::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_saturation(&val);

    attribute = attribute::get(cluster, ColorControl::Attributes::ColorTemperatureMireds::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_light_set_color_temp(&val);

    return err;
}