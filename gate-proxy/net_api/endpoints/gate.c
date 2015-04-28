#include "cbor.h"
#include "cc110x_legacy.h"
#include "gate_control.h"
#include "gate_telemetry.h"

const coap_endpoint_path_t path_gate = {1, {"gate"}};

static uint8_t resp_buffer[CC1100_MAX_DATA_LENGTH];
cbor_stream_t cbor;


int handle_get_gate(
    coap_rw_buffer_t    *scratch,
    const coap_packet_t *inpkt,
    coap_packet_t       *outpkt,
    uint8_t             id_hi,
    uint8_t             id_lo)
{
    uint16_t length = 0;
    // cbor_clear(&cbor);
    cbor_init(&cbor, resp_buffer, sizeof(resp_buffer));

    length += cbor_serialize_map(&cbor, 2);
    length += cbor_serialize_byte_string(&cbor, "pos");
    length += cbor_serialize_int(&cbor, 1234);
    length += cbor_serialize_byte_string(&cbor, "mvt");
    length += cbor_serialize_byte_string(&cbor, "closing");

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

    if (action == 1) {
        err = gate_open();
    } else if (action == -1) {
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
    length += cbor_serialize_byte_string(&cbor, "st");
    length += cbor_serialize_byte_string(&cbor, gate_state_to_s(gate_get_state()));

    if (err < 0) {
        length += cbor_serialize_byte_string(&cbor, "err");
        length += cbor_serialize_byte_string(&cbor, err * -1);
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