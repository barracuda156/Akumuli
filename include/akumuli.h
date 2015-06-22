/** 
 * PUBLIC HEADER
 *
 * Akumuli API.
 * Contains only POD definitions that can be used from "C" code.
 *
 * Copyright (c) 2013 Eugene Lazin <4lazin@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <stdint.h>
#include <apr_errno.h>
#include "akumuli_config.h"
#include "akumuli_def.h"

#ifdef __unix__
#ifdef __cplusplus
#define AKU_EXPORT  extern "C" __attribute__((visibility("default")))
#else
#define AKU_EXPORT
#endif
#else
#define AKU_EXPORT __declspec(dllexport)
#endif

//-----------------
// Data structures
//-----------------

typedef uint64_t    aku_Timestamp;    //< Timestamp
typedef uint64_t    aku_ParamId;      //< Parameter (or sequence) id
typedef int         aku_Status;       //< Status code of any operation

//! Structure represents memory region
typedef struct {
    void* address;
    uint32_t length;
} aku_MemRange;


//! Payload data
typedef struct {
    //! Value
    union {
        //! BLOB type
        struct {
            const void *begin;
            uint32_t    size;
        } blob;
        //! Float type
        double       float64;
    } value;
    //! Data element type
    enum {
        FLOAT,
        BLOB
    } type;
} aku_PData;


//! Cursor result type
typedef struct {
    aku_Timestamp timestamp;
    aku_ParamId     paramid;
    aku_PData       payload;
} aku_CursorResult;


//! Database instance.
typedef struct {
    int padding;
} aku_Database;


/**
 * @brief The aku_Cursor struct
 */
typedef struct {
    int padding;
} aku_Cursor;


//! Search stats
typedef struct {
    struct {
        uint64_t n_times;               //< How many times interpolation search was performed
        uint64_t n_steps;               //< How many interpolation search steps was performed
        uint64_t n_overshoots;          //< Number of overruns
        uint64_t n_undershoots;         //< Number of underruns
        uint64_t n_matches;             //< Number of matches by interpolation search only
        uint64_t n_reduced_to_one_page;
        uint64_t n_page_in_core_checks; //< Number of page in core checks
        uint64_t n_page_in_core_errors; //< Number of page in core check errors
        uint64_t n_pages_in_core_found; //< Number of page in core found
        uint64_t n_pages_in_core_miss;  //< Number of page misses
    } istats;
    struct {
        uint64_t n_times;               //< How many times binary search was performed
        uint64_t n_steps;               //< How many binary search steps was performed
    } bstats;
    struct {
        uint64_t fwd_bytes;             //< Number of scanned bytes in forward direction
        uint64_t bwd_bytes;             //< Number of scanned bytes in backward direction
    } scan;
} aku_SearchStats;


//! Storage stats
typedef struct {
    uint64_t n_entries;       //< Total number of entries
    uint64_t n_volumes;       //< Total number of volumes
    uint64_t free_space;      //< Free space total
    uint64_t used_space;      //< Space in use
} aku_StorageStats;


//-------------------
// Utility functions
//-------------------

/** This function must be called before any other library function.
  * @param optional_panic_handler function to alternative panic handler
  */
AKU_EXPORT void aku_initialize(aku_panic_handler_t optional_panic_handler);

/** Convert error code to error message.
  * Function returns pointer to statically allocated string
  * there is no need to free it.
  */
AKU_EXPORT const char* aku_error_message(int error_code);

/** Default logger that is used if no logging function is
  * specified. Exported for testing reasons, no need to use it
  * explicitly.
  */
AKU_EXPORT void aku_console_logger(int tag, const char* message);

/**
 * @brief Destroy any object created with aku_make_*** function
 */
AKU_EXPORT void aku_destroy(void* any);


//------------------------------
// Storage management functions
//------------------------------


/**
 * @brief Creates storage for new database on the hard drive
 * @param file_name database file name
 * @param metadata_path path to metadata file
 * @param volumes_path path to volumes
 * @param num_volumes number of volumes to create
 * @return APR errorcode or APR_SUCCESS
 * TODO: move from apr_status_t to aku_Status
 */
AKU_EXPORT apr_status_t aku_create_database
                                ( const char     *file_name
                                , const char     *metadata_path
                                , const char     *volumes_path
                                , int32_t         num_volumes
                                // optional args
                                , uint32_t  compression_threshold
                                , uint64_t  window_size
                                , uint32_t  max_cache_size
                                , aku_logger_cb_t logger
                                );


/** Remove all volumes.
  * @param file_name
  * @param logger
  * @returns status
  */
AKU_EXPORT apr_status_t aku_remove_database(const char* file_name, aku_logger_cb_t logger);


/** Open recenlty create storage.
  * @param path path to storage metadata file
  * @param parameters open parameters
  * @return pointer to new db instance, null if db doesn't exists.
  */
AKU_EXPORT aku_Database* aku_open_database(const char *path, aku_FineTuneParams parameters);


/** Check status of previous open operation
  * @param db pointer to database
  */
AKU_EXPORT aku_Status aku_open_status(aku_Database* db);


//! Close database. Free resources.
AKU_EXPORT void aku_close_database(aku_Database* db);


//---------
// Writing
//---------

/** Write measurement to DB
  * @param db opened database instance
  * @param type is a string that contains type descriptor of the tuple  
  * @param tuple is an array of values
  * @returns operation status
  * @note type descriptor is a string of the following format:
  * 1. It should contain len(tuple) characters
  * 2. Character at i'th position corresponds to tuple element at i'th position
  * 3. Each character defines the type 'u' - unsigned long (64-bit), 'i' - signed
  *    long (64-bit), 'b' - blob, 'f' - float (64-bit).
  */
AKU_EXPORT aku_Status aku_write(aku_Database* db, const char* type, const aku_PData* tuple);

//---------
// Queries
//---------

/**
 * @brief Execute query
 * @param query string
 * @return cursor
 */
AKU_EXPORT aku_Cursor* aku_select(aku_Database* db, const char* query);

/**
 * @brief Close cursor
 * @param pcursor pointer to cursor
 */
AKU_EXPORT void aku_cursor_close(aku_Cursor* pcursor);

/** Read the values under cursor.
  * @param cursor should point to active cursor instance
  * @param dest is an output buffer
  * @param dest_size is an output buffer size
  * @returns error code
  */
AKU_EXPORT aku_Status aku_cursor_read( aku_Cursor       *cursor
                                     , aku_CursorResult *dest
                                     , size_t            dest_size);

//! Check cursor state. Returns zero value if not done yet, non zero value otherwise.
AKU_EXPORT aku_Status aku_cursor_is_done(aku_Cursor* pcursor);

//! Check cursor error state. Returns zero value if everything is OK, non zero value otherwise.
AKU_EXPORT aku_Status aku_cursor_is_error(aku_Cursor* pcursor, int* out_error_code_or_null);

//--------------------
// Stats and counters
//--------------------

/** Get search counters.
  * @param rcv_stats pointer to `aku_SearchStats` structure that will be filled with data.
  * @param reset reset all counter if not zero
  */
AKU_EXPORT void aku_global_search_stats(aku_SearchStats* rcv_stats, int reset);

/** Get storage stats.
  * @param db database instance.
  * @param rcv_stats pointer to destination
  */
AKU_EXPORT void aku_global_storage_stats(aku_Database *db, aku_StorageStats* rcv_stats);
