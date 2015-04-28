/**
* @brief Saves state as defined in persistence.c 'state_elements' to persistent
* storage (probably ROM).
*/
void persist_state(void);

/**
* @brief Restores state as defined in persistence.c 'state_elements' from
* persistent storage. If nothing is found in storage, persist_state() is
* invoked.
*
* @return 1 on existing state restored
* @return 0 on existing state not found
*/
int restore_state(void);