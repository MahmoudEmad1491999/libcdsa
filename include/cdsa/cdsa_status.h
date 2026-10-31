#ifndef CDSA_STATUS_H
#define CDSA_STATUS_H

/**
 * @file cdsa_status.h
 * @brief Error codes returned by all fallible cdsa functions.
 *
 * Every cdsa function that can fail returns cdsa_status_t.
 * CDSA_OK (0) means success; negative values indicate specific errors.
 */

/** Return type for all fallible cdsa operations. */
typedef enum cdsa_status {
    CDSA_OK            =  0, /**< Success. */
    CDSA_ERR_NOMEM     = -1, /**< Memory allocation failed. */
    CDSA_ERR_INVAL     = -2, /**< Invalid argument (NULL pointer, bad size, …). */
    CDSA_ERR_NOTFOUND  = -3, /**< Key / element not present. */
    CDSA_ERR_EMPTY     = -4, /**< Operation requires a non-empty container. */
    CDSA_ERR_FULL      = -5, /**< Fixed-capacity container has no room. */
    CDSA_ERR_RANGE     = -6, /**< Index out of bounds. */
    CDSA_ERR_DUPLICATE = -7, /**< Duplicate key where uniqueness is required. */
} cdsa_status_t;

/**
 * @brief Returns a constant human-readable string for @p status.
 *
 * Always returns a non-NULL, NUL-terminated string in static storage.
 * For unrecognised values the string is "CDSA_ERR_UNKNOWN".
 * The returned pointer must not be freed or modified.
 *
 * @param status  Any cdsa_status_t value.
 * @return        Static string describing the status code.
 *
 * @complexity O(1) time, O(1) extra space.
 */
const char *cdsa_strerror(cdsa_status_t status);

#endif /* CDSA_STATUS_H */
