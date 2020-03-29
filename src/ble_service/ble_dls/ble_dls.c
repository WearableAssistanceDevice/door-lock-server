#include "ble_dls.h"

#include <string.h>
#include "sdk_common.h"
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "bsp_config.h"
#include "boards.h"
#include "nrf_log.h"


/**@brief Function for adding the Door locked characteristic.
 *
 * @param[in]   p_dls        Door Lock Service structure.
 * @param[in]   p_dls_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t lock_state_request_char_add(ble_dls_t* p_dls, const ble_dls_init_t* p_dls_init) {
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.write_perm = p_dls_init->lock_state_char_attr_md.cccd_write_perm;
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 1; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; 
    char_md.p_sccd_md         = NULL;
		
    memset(&attr_md, 0, sizeof(attr_md));
    attr_md.read_perm  = p_dls_init->lock_state_char_attr_md.read_perm;
    attr_md.write_perm = p_dls_init->lock_state_char_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = p_dls->uuid_type;
    ble_uuid.uuid = DLS_UUID_LOCK_STATE_CHAR;

    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = sizeof(uint8_t);
    attr_char_value.p_value   = &p_dls_init->initial_lock_state_value;

    err_code = sd_ble_gatts_characteristic_add(p_dls->service_handle,
                                               &char_md,
                                               &attr_char_value,
                                               &p_dls->lock_state_handles);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}


uint32_t ble_dls_init(ble_dls_t* p_dls, const ble_dls_init_t* p_dls_init) {
    if (p_dls == NULL || p_dls_init == NULL) {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service struct
    p_dls->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_dls->evt_handler = p_dls_init->evt_handler;

    // Add Door Lock Service UUID
    ble_uuid128_t base_uuid = {DLS_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_dls->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_dls->uuid_type;
    ble_uuid.uuid = DLS_UUID_SERVICE;

    // Add the Door Lock Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_dls->service_handle);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return lock_state_request_char_add(p_dls, p_dls_init);
}


uint32_t ble_dls_lock_state_set(ble_dls_t* p_dls, uint8_t lock_state_value) {
    if (p_dls == NULL) {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct
    memset(&gatts_value, 0, sizeof(gatts_value));
    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &lock_state_value;

    // Update database
    err_code = sd_ble_gatts_value_set(p_dls->conn_handle,
                                      p_dls->lock_state_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Send value if connected and notifying
    if ((p_dls->conn_handle != BLE_CONN_HANDLE_INVALID)) {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_dls->lock_state_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset  = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(p_dls->conn_handle, &hvx_params);
    }

    // Send write signal
    if (p_dls->evt_handler != NULL) {
        ble_dls_evt_t evt;
        evt.evt_type = BLE_DLS_EVT_WRITE;
        p_dls->evt_handler(p_dls, &evt);
    }
  
    return err_code;
}

uint32_t ble_dls_lock_state_get(ble_dls_t* p_dls, uint8_t* p_lock_state_value) {
    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct
    memset(&gatts_value, 0, sizeof(gatts_value));
    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset   = 0;
    gatts_value.p_value = p_lock_state_value;

    // Retrieve the value
    err_code = sd_ble_gatts_value_get(p_dls->conn_handle,
                                      p_dls->lock_state_handles.value_handle,
                                      &gatts_value);

    return err_code;
}


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_dls       Door Lock Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_dls_t* p_dls, const ble_evt_t* p_ble_evt) {
    p_dls->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    if (p_dls->evt_handler != NULL) {
        ble_dls_evt_t evt;
        evt.evt_type = BLE_DLS_EVT_CONNECTED;
        p_dls->evt_handler(p_dls, &evt);
    }
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_dls       Door Lock Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_dls_t* p_dls, const ble_evt_t* p_ble_evt) {
    UNUSED_PARAMETER(p_ble_evt);
    p_dls->conn_handle = BLE_CONN_HANDLE_INVALID;

    if (p_dls->evt_handler != NULL) {
        ble_dls_evt_t evt;
        evt.evt_type = BLE_DLS_EVT_DISCONNECTED;
        p_dls->evt_handler(p_dls, &evt);
    }
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_dls       Door Lock Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_dls_t* p_dls, const ble_evt_t* p_ble_evt) {
    const ble_gatts_evt_write_t* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
    // Check if the handle passed with the event matches the Door locked Characteristic handle
    if (p_evt_write->handle == p_dls->lock_state_handles.value_handle) {
        if (p_dls->evt_handler != NULL) {
            ble_dls_evt_t evt;
            evt.evt_type = BLE_DLS_EVT_WRITE;
            p_dls->evt_handler(p_dls, &evt);
        }
    }

    // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == p_dls->lock_state_handles.cccd_handle) && (p_evt_write->len == 2)) {
        // CCCD written, call application event handler
        if (p_dls->evt_handler != NULL) {
            ble_dls_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data)) {
                evt.evt_type = BLE_DLS_EVT_NOTIFICATION_ENABLED;
            }
            else {
                evt.evt_type = BLE_DLS_EVT_NOTIFICATION_DISABLED;
            }

            // Call the application event handler
            p_dls->evt_handler(p_dls, &evt);
        }
    }
}


void ble_dls_on_ble_evt(const ble_evt_t* p_ble_evt, void* p_context) {
    ble_dls_t* p_dls = (ble_dls_t*)p_context;
    
    if (p_dls == NULL || p_ble_evt == NULL) {
        return;
    }

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_dls, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_dls, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_dls, p_ble_evt);
            break;

        default:
            break;
    }
}