/* GET,PUT /gate */
const coap_endpoint_path_t path_gate = {1, {"gate"}};
int handle_get_gate(coap_rw_buffer_t*, const coap_packet_t*, coap_packet_t*, uint8_t, uint8_t);
int handle_put_gate(coap_rw_buffer_t*, const coap_packet_t*, coap_packet_t*, uint8_t, uint8_t);

/* GET /.well-known/core */
const coap_endpoint_path_t path_well_known_core = {2, {".well-known", "core"}};
int handle_get_well_known_core(coap_rw_buffer_t*, const coap_packet_t*, coap_packet_t*, uint8_t, uint8_t);
void well_known_core_build(const coap_endpoint_t*);

const coap_endpoint_t endpoints[] =
{
    {COAP_METHOD_GET, handle_get_well_known_core, &path_well_known_core, "ct=40"},
    {COAP_METHOD_GET, handle_get_gate, &path_gate, "ct=42"},
    // {COAP_METHOD_PUT, handle_put_gate, &path_gate, NULL},
    {(coap_method_t)0, NULL, NULL, NULL}
};