#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <app_priv.h>
#include <app_reset.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

static const char *TAG = "app_main";
uint16_t light_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void save_matter_pin(uint32_t pin) {
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("matter", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_u32(handle, "pin_code", pin));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

static uint32_t generate_random_pin() {
    return 10000000 + (esp_random() % 90000000);
}

static uint32_t load_matter_pin() {
    nvs_handle_t handle;
    uint32_t pin = 0;
    if (nvs_open("matter", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u32(handle, "pin_code", &pin); 
        nvs_close(handle);
    }
    return pin;
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        {
            ESP_LOGI(TAG, "Fabric removed successfully");
            if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
            {
                uint32_t pinCode = load_matter_pin();
                if (pinCode == 0) {
                    pinCode = generate_random_pin();
                    save_matter_pin(pinCode);
                }
                
                constexpr uint16_t discriminator = 0x6252;
                constexpr uint32_t iterations = 1000;
                chip::ByteSpan salt;
                chip::FabricIndex fabricIndex = 0;
                chip::VendorId vendorId = static_cast<chip::VendorId>(0xFFF1);

                chip::Crypto::Spake2pVerifier verifier;
                CHIP_ERROR err = verifier.Generate(iterations, salt, pinCode);
                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to generate verifier: %" CHIP_ERROR_FORMAT, err.Format());
                    return;
                }

                err = chip::Server::GetInstance().GetCommissioningWindowManager().OpenEnhancedCommissioningWindow(
                    chip::System::Clock::Seconds32(k_timeout_seconds),
                    discriminator,
                    verifier,
                    iterations,
                    salt,
                    fabricIndex,
                    vendorId
                );

                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to open commissioning window: %" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        break;
        }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}

/*
 * This is a tutorial, but it doesn't
 * really have any other features other
 * than on/off. From my understanding,
 * this is a matter application, which 
 * Google Home Supports. 
 * 
 * What it does is that it Initializes 
 * the NVS which is the ESP32's flash.
 * It then initializes my light driver
 * which uses GPIO p5. Then it creates
 * a matter node. Then it creates an
 * endpoint which is like the settings
 * that Google Home can access and use
 * , so for example, now it just has a
 * setting to turn it off/on. But now 
 * it can change the brightness, etc.
 * Then it starts the Matter Stack and
 * sets the LED's defaults.
 * 
 * This means that when I try to flash
 * the ESP32 and boot it, it will open
 * the commissioning window for 300sec,
 * which allows me to connect to it.
*/
extern "C" void app_main() {
    esp_err_t err = ESP_OK;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    app_driver_handle_t light_handle = app_driver_light_init();

    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);

    extended_color_light::config_t light_config;

    /*
     * Brightness stuff
    */
    light_config.on_off.on_off = DEFAULT_POWER;
    light_config.level_control.current_level = DEFAULT_BRIGHTNESS;

    /*
     * Temperature stuff
    */
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control_color_temperature.startup_color_temperature_mireds = 153;

    /*
     * Hue/Sat Stuff
    */
    light_config.color_control_xy.current_x = 0x616b;
    light_config.color_control_xy.current_y = 0x607d;
    endpoint_t *endpoint = extended_color_light::create(node, &light_config, ENDPOINT_FLAG_NONE, light_handle);

    if (!node || !endpoint) {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    light_endpoint_id = endpoint::get_id(endpoint);

    ESP_LOGI(TAG, "Light created with endpoint_id %d", light_endpoint_id);

    ESP_LOGI(TAG, "Matter App created with endpoint_id");

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Matter start failed: %d", err);
    }

    app_driver_light_set_defaults(light_endpoint_id);

#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err);
    }
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
#if CONFIG_OPENTHREAD_CLI
    esp_matter::console::otcli_register_commands();
#endif
    esp_matter::console::init();
#endif
}
