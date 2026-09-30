#ifndef DS5DONGLE_CRC_H
#define DS5DONGLE_CRC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32_seeded(const uint8_t *data, size_t size, uint32_t seed);
uint32_t crc32(const uint8_t *data, size_t size);
void fill_output_report_checksum(uint8_t *data, size_t len);

uint32_t crc32_feature(const uint8_t *data, size_t size);
void fill_feature_report_checksum(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
