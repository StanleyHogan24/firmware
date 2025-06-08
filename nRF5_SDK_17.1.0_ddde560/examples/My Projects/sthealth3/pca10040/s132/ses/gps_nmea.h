// gps_nmea.h
#ifndef GPS_NMEA_H_
#define GPS_NMEA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset NMEA parser state.
 */
void gps_nmea_init(void);

/**
 * @brief Feed incoming GPS character to the NMEA parser.
 *
 * @param c  Byte from GPS module.
 */
void gps_nmea_parse_char(uint8_t c);

#ifdef __cplusplus
}
#endif

#endif // GPS_NMEA_H_