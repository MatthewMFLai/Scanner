#ifndef GEOFENCE_HDLR_H__
#define GEOFENCE_HDLR_H__

#define GEOFENCE_HDLR_RC_FIRST_IN       0
#define GEOFENCE_HDLR_RC_IN             1
#define GEOFENCE_HDLR_RC_FIRST_OUT      2
#define GEOFENCE_HDLR_RC_OUT            3
#define GEOFENCE_HDLR_RC_NO_GEOFENCE    4

void geofence_hdlr_init(void);
void geofence_hdlr_fence_set(void);
void geofence_hdlr_fence_clr(void);
uint8_t geofence_hdlr_device_update(void);
uint8_t geofence_hdlr_timer_update(void);

#endif  /* _ GEOFENCE_HDLR_H__ */
