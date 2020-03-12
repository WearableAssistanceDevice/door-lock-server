#ifndef BLE_DLS_H
#define BLE_DLS_H

#include <stdint.h>
#include <stdbool.h>

#include "ble.h"
#include "ble_srv_common.h"


#ifdef __cplusplus
extern "C" {
#endif


// Base 128-bit UUID for the door lock service
#define DLS_UUID_BASE { 0x7D, 0x94, 0xA7, 0x59, 0x10, 0xD8, 0x0B, 0xAB, \
                        0x4D, 0x4E, 0xA9, 0x10, 0x0D, 0xA3, 0xE1, 0xD1 }

// 16-bit UUID for the service and its characteristics
#define DLS_UUID_SERVICE        0x2000
#define DLS_UUID_DOOR_LOCK_CHAR 0x2001


/**@brief   Macro for defining an door lock service instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_DLS_DEF(_name)                        \
static ble_dls_t _name;                          \
NRF_SDH_BLE_OBSERVER(_name ## _obs,              \
                     BLE_HRS_BLE_OBSERVER_PRIO,  \
                     ble_dls_on_ble_evt, &_name)


typedef enum {
    BLE_DLS_EVT_NOTIFICATION_ENABLED,   /**< Door lock notification enabled event. */
    BLE_DLS_EVT_NOTIFICATION_DISABLED,  /**< Door lock notification disabled event. */
    BLE_DLS_EVT_DISCONNECTED,
    BLE_DLS_EVT_CONNECTED,
    BLE_DLS_EVT_WRITE
} ble_dls_evt_type_t;

/**@brief Door Lock Service event. */
typedef struct {
    ble_dls_evt_type_t evt_type;  /**< Type of event. */
} ble_dls_evt_t;


// Forward declaration of the ble_dls_t type.
typedef struct ble_dls_s ble_dls_t;


/**@brief Door Lock Service event handler type. */
typedef void (*ble_dls_evt_handler_t)(ble_dls_t* p_dls, ble_dls_evt_t* p_evt);


/**@brief Door Lock Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct {
    ble_dls_evt_handler_t        evt_handler;               /**< Event handler to be called for handling events in the Door Lock Service */
    uint8_t                      initial_door_lock_value;  /**< Initial value for the door lock */
    ble_srv_cccd_security_mode_t door_lock_char_attr_md;   /**< Initial security level for Door Lock characteristics attribute */
} ble_dls_init_t;


/**@brief Door Lock Service structure. This contains various status information for the service. */
struct ble_dls_s {
    ble_dls_evt_handler_t    evt_handler;         /**< Event handler to be called for handling events in the Door Lock Service */
    uint16_t                 service_handle;      /**< Handle of Door Lock Service (as provided by the BLE stack) */
    ble_gatts_char_handles_t door_lock_handles;  /**< Handles related to the Door locked characteristic */
    uint16_t                 conn_handle;         /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection) */
    uint8_t                  uuid_type; 
};


/**@brief Function for initializing the Door Lock Service.
 *
 * @param[out]  p_dls       Door Lock Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_dls_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_dls_init(ble_dls_t* p_dls, const ble_dls_init_t* p_dls_init);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Door Lock Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Door Lock Service structure.
 */
void ble_dls_on_ble_evt(const ble_evt_t* p_ble_evt, void* p_context);



/**@brief Function for updating the door lock value.
 *
 * @details The application calls this function when the cutom value should be updated.
 *
 * @note 
 *       
 * @param[in]   p_dls            Door Lock Service structure
 * @param[in]   door_lock_value  Door lock value
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_dls_door_lock_set(ble_dls_t* p_dls, uint8_t door_lock_value);



/**@brief Function for retrieving the door lock value.
 *
 * @details The application calls this function when it wants to retrieve the value of
 *          the door lock characteristic.
 *
 * @note 
 *       
 * @param[in]   p_dls              Door Lock Service structure
 * @param[in]   p_door_lock_value  Pointer to the location to store the door lock value
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_dls_door_lock_get(ble_dls_t* p_dls, uint8_t* p_door_lock_value);


#ifdef __cplusplus
}
#endif


#endif //BLE_DLS_H