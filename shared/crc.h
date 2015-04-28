uint16_t crc_calculate_sw(uint8_t*, uint8_t);
uint16_t crc_calculate_hw(uint8_t*, uint8_t);

static inline uint16_t crc16(uint8_t* data, uint8_t len) {
    return crc_calculate_hw(data, len);
}