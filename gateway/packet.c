// There is only one UART and it
// receives only one packet at a time,
// this cannot parse incoming packets concurently

typedef enum {
    DELIMIT,
    LENGTH,
    DATA,
    CRC,
    COMPLETE
} parse_state_t;

const uint8_t DELIMITER = 0;
const uint8_t CRC_LEN = sizeof(uint16_t);

parse_state_t parse_state = DELIMIT;
static uint8_t data_len;
static uint8_t data_cursor;
static uint16_t crc;

void parser_push_byte(uint8_t byte)
{
    size_t i = 0;

    switch (parse_state) {
        case DELIMIT:
            if (byte == DELIMITER)
            {
                parse_state = LENGTH;
                break;
            }
        case LENGTH:
            data_len = c;
            parse_state = DATA;
        case DATA:
            bytes_avail = len - i;
            // determine how many bytes we can read
            if (data_len < bytes_avail)
            {
                data_len = 0;
            }
            else {
                data_len -= bytes_avail;
            }
            if (data_len == 0)
            {
                parse_state = CRC;
            }
            data_cursor =
            break;
        case CRC:
            crc_i += 1;
            crc_i %= CRC_LEN;
            break;
        case DONE:

    }

    return parse_state;
}

void packet_push_bytes(xport_packet_t *pkt, uint8_t *bytes, size_t len)
{
    size_t i = 0;

    switch (pkt->parse_state) {
        case DELIMIT:
            while (i++ < len)
            {
                if (bytes[i] == DELIMITER)
                {
                    parse_state = LENGTH;
                    break;
                }
            }
        case LENGTH:
            pkt->len = bytes[i++];
            pkt->parse_state = DATA;
        case DATA:
            bytes_avail = len - i;
            // determine how many bytes we can read
            if (pkt->length < bytes_avail)
            {
                data_len = 0;
            }
            else {
                data_len -= bytes_avail;
            }
            if (data_len == 0)
            {
                parse_state = CRC;
            }
            data_cursor =
            break;
        case CRC:
            crc_i += 1;
            crc_i %= CRC_LEN;
            break;
        case DONE:

    }

}