// gps_nmea.c
#include "gps_nmea.h"
#include "nrf_log.h"
#include <string.h>
#include <stdlib.h>

#define NMEA_BUF_SIZE   256
static char    nmea_buf[NMEA_BUF_SIZE];
static size_t  nmea_index = 0;

static double nmea_to_decimal(const char * coord, char hemi)
{
    if (coord == NULL || coord[0] == '\0')
    {
        return 0.0;
    }

    double val = strtod(coord, NULL);
    int degrees = (int)(val / 100);
    double minutes = val - (degrees * 100);
    double decimal = degrees + minutes / 60.0;

    if (hemi == 'S' || hemi == 'W')
    {
        decimal = -decimal;
    }

    return decimal;
}


static void parse_nmea_sentence(const char * sentence)
{
    if (strncmp(sentence, "$GPRMC", 6) == 0)
    {
        char buf[NMEA_BUF_SIZE];
        strncpy(buf, sentence, sizeof(buf));
        char * fields[12] = {0};
        size_t idx = 0;
        char * token = strtok(buf, ",");
        while (token && idx < 12)
        {
            fields[idx++] = token;
            token = strtok(NULL, ",");
        }
        if (idx > 6 && fields[2][0] == 'A')
        {
            double lat = nmea_to_decimal(fields[3], fields[4][0]);
            double lon = nmea_to_decimal(fields[5], fields[6][0]);
            NRF_LOG_INFO("Latitude:" NRF_LOG_FLOAT_MARKER ", Longitude:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(lat), NRF_LOG_FLOAT(lon));
        }
    }
    else if (strncmp(sentence, "$GPGGA", 6) == 0)
    {
        char buf[NMEA_BUF_SIZE];
        strncpy(buf, sentence, sizeof(buf));
        char * fields[10] = {0};
        size_t idx = 0;
        char * token = strtok(buf, ",");
        while (token && idx < 10)
        {
            fields[idx++] = token;
            token = strtok(NULL, ",");
        }
        if (idx > 5 && fields[2] && fields[4])
        {
            double lat = nmea_to_decimal(fields[2], fields[3][0]);
            double lon = nmea_to_decimal(fields[4], fields[5][0]);
            NRF_LOG_INFO("Latitude:" NRF_LOG_FLOAT_MARKER ", Longitude:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(lat), NRF_LOG_FLOAT(lon));
        }
    }
}

void gps_nmea_init(void)
{
    nmea_index = 0;
    memset(nmea_buf, 0, sizeof(nmea_buf));
}

void gps_nmea_parse_char(uint8_t c)
{
    if (c == '\r' || c == '\n')
    {
        if (nmea_index > 0)
        {
            nmea_buf[nmea_index] = '\0';
            NRF_LOG_INFO("NMEA: %s", nrf_log_push(nmea_buf));
            parse_nmea_sentence(nmea_buf);
            nmea_index = 0;
        }
    }
    else if (nmea_index < (NMEA_BUF_SIZE - 1))
    {
        nmea_buf[nmea_index++] = (char)c;
    }
    else
    {
        nmea_index = 0;
    }
}
