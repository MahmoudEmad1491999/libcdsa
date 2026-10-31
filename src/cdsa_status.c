#include "cdsa/cdsa_status.h"

const char *cdsa_strerror(cdsa_status_t status)
{
    switch (status) {
    case CDSA_OK:
        return "CDSA_OK";
    case CDSA_ERR_NOMEM:
        return "CDSA_ERR_NOMEM: memory allocation failed";
    case CDSA_ERR_INVAL:
        return "CDSA_ERR_INVAL: invalid argument";
    case CDSA_ERR_NOTFOUND:
        return "CDSA_ERR_NOTFOUND: key or element not found";
    case CDSA_ERR_EMPTY:
        return "CDSA_ERR_EMPTY: container is empty";
    case CDSA_ERR_FULL:
        return "CDSA_ERR_FULL: container is full";
    case CDSA_ERR_RANGE:
        return "CDSA_ERR_RANGE: index out of bounds";
    case CDSA_ERR_DUPLICATE:
        return "CDSA_ERR_DUPLICATE: duplicate key";
    default:
        return "CDSA_ERR_UNKNOWN";
    }
}
