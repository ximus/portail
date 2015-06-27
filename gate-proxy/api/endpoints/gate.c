#include "coap.h"
#include "cbor.h"
#include "portail.h"
#include "gate_control.h"
#include "gate_telemetry.h"

/* PUT request value */
#define REQUEST_OPEN  (1)
#define REQUEST_CLOSE (-1)

/* response fields */
#define STATE_FIELD    "s"
#define ERROR_FIELD    "e"
#define POSITION_FIELD "p"

static uint8_t resp_buffer[PORTAIL_MAX_DATA_SIZE];
cbor_stream_t cbor;


int handle_get_gate(
    coap_rw_buffer_t    *scratch,
    const coap_packet_t *inpkt,
    coap_packet_t       *outpkt,
    uint8_t             id_hi,
    uint8_t             id_lo)
{
    uint8_t length = 0;
    // cbor_clear(&cbor);
    cbor_init(&cbor, resp_buffer, sizeof(resp_buffer));

    length += cbor_serialize_map_indefinite(&cbor);

    /* state */
    length += cbor_serialize_byte_string(&cbor, STATE_FIELD);
    length += cbor_serialize_int(&cbor, gate_get_state());

    /* position */
    uint16_t position;
    if (gate_get_position(&position) == 0) {
        length += cbor_serialize_byte_string(&cbor, POSITION_FIELD);
        length += cbor_serialize_int(&cbor, position);
    }

    length += cbor_write_break(&cbor);

    return coap_make_response(
        scratch,
        outpkt,
        (const uint8_t *)resp_buffer,
        length,
        id_hi,
        id_lo,
        &inpkt->tok,
        COAP_RSPCODE_CONTENT,
        COAP_CONTENTTYPE_OCTET_STREAM
    );
}

// for reference:
// int coap_make_response(coap_rw_buffer_t *scratch, coap_packet_t *pkt,
//   const uint8_t *content, size_t content_len, uint8_t msgid_hi,
//   uint8_t msgid_lo, const coap_buffer_t* tok, coap_responsecode_t rspcode,
//   coap_content_type_t content_type)

int handle_put_gate(
    coap_rw_buffer_t    *scratch,
    const coap_packet_t *inpkt,
    coap_packet_t       *outpkt,
    uint8_t             id_hi,
    uint8_t             id_lo)
{
    if (inpkt->payload.len <= 0) {
        // respond with bad request
        return coap_make_response(
            scratch, outpkt, NULL, 0, id_hi, id_lo,
            (const coap_buffer_t*) &inpkt->tok,
            COAP_RSPCODE_BAD_REQUEST,
            COAP_CONTENTTYPE_TEXT_PLAIN
        );
    }

    int8_t action = *inpkt->payload.p;
    int err;

    if (action == REQUEST_OPEN) {
        err = gate_open();
    } else if (action == REQUEST_CLOSE) {
        err = gate_close();
    } else {
        // respond with bad request
        return coap_make_response(
            scratch, outpkt, NULL, 0, id_hi, id_lo,
            (const coap_buffer_t*) &inpkt->tok,
            COAP_RSPCODE_BAD_REQUEST,
            COAP_CONTENTTYPE_TEXT_PLAIN
        );
    }


    uint16_t length = 0;
    // cbor_clear(&cbor);
    cbor_init(&cbor, resp_buffer, sizeof(resp_buffer));

    length += cbor_serialize_map(&cbor, 2);
    length += cbor_serialize_byte_string(&cbor, STATE_FIELD);
    length += cbor_serialize_int(&cbor, gate_get_state());

    if (err != 0) {
        length += cbor_serialize_byte_string(&cbor, ERROR_FIELD);
        length += cbor_serialize_int(&cbor, err * -1);
    }

    return coap_make_response(
        scratch,
        outpkt,
        (const uint8_t *)resp_buffer,
        length,
        id_hi,
        id_lo,
        &inpkt->tok,
        COAP_RSPCODE_CONTENT,
        COAP_CONTENTTYPE_OCTET_STREAM
    );
}