/** @file
 *
 * @defgroup ble_door_lock main.c
 * @{
 * @ingroup ble_door_lock
 * @brief Door Lock main file.
 *
 * DESCRIPTION HERE
 */

#include "config.h"

#include "nrf_sdh_ble.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"

#include "bsp.h"
#include "bsp_btn_ble.h"

#include "board_service/board_services.h"
#include "ble_service/ble_services.h"
#include "ble_service/ble_dls/ble_dls.h"


BLE_DLS_DEF(m_door);  /**< Define the door service instance */
APP_TIMER_DEF(m_door_timer); /**< Define the door lock timer */

static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    //{DLS_UUID_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}
    //{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}



/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Called when the the timer for the auto door lock times out
 *
 * @param[in] p_context  Unused
*/
static void door_lock_timeout(void* p_context)
{
    NRF_LOG_INFO("Door autolock engaged");
    ble_dls_lock_state_set(&m_door, true);
}


/**@brief Function for starting the door autolock timer.
 */
static void door_timer_start(void)
{
    ret_code_t err_code;
    err_code = app_timer_start(m_door_timer, APP_TIMER_TICKS(5000), NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Door Service Service events.
 *
 * @details This function will be called for all Door Service events which are passed to
 *          the application.
 *
 * @param[in]   p_door       Door Service structure.
 * @param[in]   p_evt          Event received from the Custom Service.
 *
 */
static void on_door_evt(ble_dls_t* p_door, ble_dls_evt_t* p_evt) {
    uint32_t err_code;
    uint8_t door_locked;

    switch(p_evt->evt_type) {
        case BLE_DLS_EVT_NOTIFICATION_ENABLED:
            break;

        case BLE_DLS_EVT_NOTIFICATION_DISABLED:
            break;

        case BLE_DLS_EVT_CONNECTED:
            break;

        case BLE_DLS_EVT_DISCONNECTED:
            break;

        case BLE_DLS_EVT_WRITE: {
            err_code = ble_dls_lock_state_get(p_door, &door_locked);
            APP_ERROR_CHECK(err_code);
            if (door_locked) {
                NRF_LOG_INFO("Door locked");
                bsp_board_led_on(DOOR_LOCK_LED);
            }
            else {
                NRF_LOG_INFO("Door unlocked");
                bsp_board_led_off(DOOR_LOCK_LED);
                door_timer_start();
                sd_ble_gap_disconnect(p_door->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            }
            break;
        }

        default:
            break;
    }
}


/**@brief User function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated when button is pressed.
 */
void bsp_event_handler(bsp_event_t event) {
    switch (event) {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case DOOR_LOCK_BUTTON_EVT:
            ble_dls_lock_state_set(&m_door, true);
            break;

        /* Don't want these for now
        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break; // BSP_EVENT_KEY_0
        */
    }
}


/**@brief User function for handling BLE advertising events.
 *
 * @param[in]   ble_adv_evt   Advertising event.
 */
void ble_adv_evt_handler(ble_adv_evt_t ble_adv_evt) {
    switch (ble_adv_evt) {
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief User function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
void ble_evt_handler(const ble_evt_t* p_ble_evt, void* p_context) {
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED: {
            bsp_board_led_on(CONNECTED_LED);
        } break;

        case BLE_GAP_EVT_DISCONNECTED: {
            bsp_board_led_off(CONNECTED_LED);
        } break;
    }
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


/**@brief Function for application timer initialization.
 *
 * @details Initializes any necessary application timers
 */
static void application_timers_init(void)
{
    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by one. */
    ret_code_t err_code;
    err_code = app_timer_create(&m_door_timer, APP_TIMER_MODE_SINGLE_SHOT, door_lock_timeout);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE Door Lock service.
 */
static void door_service_init(void) {
    ble_dls_init_t door_init = {0};
    
    door_init.evt_handler = on_door_evt;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&door_init.lock_state_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&door_init.lock_state_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&door_init.lock_state_char_attr_md.write_perm);

    const ret_code_t err_code = ble_dls_init(&m_door, &door_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    // Board services config
    board_services_init_t board_init = {0};
    board_init.bsp_evt_handler = bsp_event_handler;
    board_init.erase_bonds = &erase_bonds;

    // BLE services config
    ble_services_init_t ble_init = {0};
    ble_service_init_func_t init_funcs[] = {
        door_service_init
    };

    ble_init.adv_evt_handler         = ble_adv_evt_handler;
    ble_init.ble_evt_handler         = ble_evt_handler;
    ble_init.service_init_funcs      = init_funcs;
    ble_init.service_init_func_count = sizeof(init_funcs) / sizeof(init_funcs[0]);
    ble_init.adv_uuids               = NULL;//m_adv_uuids;
    ble_init.adv_uuid_count          = 0;//sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);

    // Initialize
    board_services_init(&board_init);
    ble_services_init(&ble_init);
    application_timers_init();

    const ret_code_t err_code = ble_dls_lock_state_set(&m_door, true);
    APP_ERROR_CHECK(err_code);

    // Start execution
    NRF_LOG_INFO("Door lock server started");
    advertising_start(erase_bonds);

    // Enter main loop
    for (;;) {
        idle_state_handle();
    }
}


/**
 * @}
 */
