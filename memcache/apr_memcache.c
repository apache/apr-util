/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_memcache.h"
#include "apr_poll.h"
#include "apr_version.h"
#include <stdlib.h>

struct apr_memcache_conn_t
{
    apr_size_t blen;
    apr_pool_t *p;
    apr_socket_t *sock;
    apr_bucket_alloc_t *balloc;
    apr_bucket_brigade *bb;
    apr_bucket_brigade *tb;
    apr_memcache_server_t *ms;
};                                                          

#define REQ_MAGIC_BYTE (0xF)

/* XXXX: sync these with memcached server impl */
enum mc_cmd_type {
    CMD_GET = 0,
    CMD_SET = 1,
    CMD_ADD = 2,
    CMD_REPLACE = 3,
    CMD_DELETE = 4,
    CMD_INCR = 5,
    CMD_DECR = 6,
    CMD_QUIT = 7,
};

#define MC_STATUS_SUCCESS (0)
#define MC_STATUS_STORED (1)
#define MC_STATUS_NOT_STORED (2)
#define MC_STATUS_DELETED (3)
#define MC_STATUS_NOT_FOUND (4)

/* XXXX: why isn't this in apr.h? */
typedef  unsigned char apr_uint8_t;

typedef struct {
    apr_uint8_t magic;
    apr_uint8_t cmd;
    union {
        apr_uint8_t rep_status;
        apr_uint8_t req_keylen;
    };
    union {
        apr_uint8_t req_reserved;
        apr_uint8_t rep_reserved;
    };
    apr_uint32_t opaque_id;
    apr_uint32_t lenleft;
} apr_mc_hdr_t;

#define MC_HDR_LEN (sizeof(apr_mc_hdr_t))

/** Server and Query Structure for a multiple get */
struct cache_server_query_t {
    apr_memcache_server_t* ms;
    apr_memcache_conn_t* conn;
    struct iovec* query_vec;
    unsigned int query_vec_count;
};

#define MULT_GET_TIMEOUT 50000

static apr_status_t make_server_dead(apr_memcache_t *mc, apr_memcache_server_t *ms)
{
#if APR_HAS_THREADS
    apr_thread_mutex_lock(ms->lock);
#endif
    ms->status = APR_MC_SERVER_DEAD;
    ms->btime = apr_time_now();
#if APR_HAS_THREADS
    apr_thread_mutex_unlock(ms->lock);
#endif
    return APR_SUCCESS;
}

static apr_status_t make_server_live(apr_memcache_t *mc, apr_memcache_server_t *ms)
{
    ms->status = APR_MC_SERVER_LIVE; 
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_memcache_add_server(apr_memcache_t *mc, apr_memcache_server_t *ms)
{
    apr_status_t rv = APR_SUCCESS;

    if(mc->ntotal >= mc->nalloc) {
        return APR_ENOMEM;
    }

    mc->live_servers[mc->ntotal] = ms;
    mc->ntotal++;
    make_server_live(mc, ms);
    return rv;
}

static apr_status_t mc_version_ping(apr_memcache_server_t *ms);

APR_DECLARE(apr_memcache_server_t *) 
apr_memcache_find_server_hash(apr_memcache_t *mc, const apr_uint32_t hash)
{
    if (mc->server_func) {
        return mc->server_func(mc->server_baton, mc, hash);
    }
    else {
        return apr_memcache_find_server_hash_default(NULL, mc, hash);
    }
}   

APR_DECLARE(apr_memcache_server_t *) 
apr_memcache_find_server_hash_default(void *baton, apr_memcache_t *mc,
                                      const apr_uint32_t hash)
{
    apr_memcache_server_t *ms = NULL;
    apr_uint32_t h = hash ? hash : 1;
    apr_uint32_t i = 0;
    apr_time_t curtime = 0;
   
    if(mc->ntotal == 0) {
        return NULL;
    }

    do {
        ms = mc->live_servers[h % mc->ntotal];
        if(ms->status == APR_MC_SERVER_LIVE) {
            break;
        }
        else {
            if (curtime == 0) {
                curtime = apr_time_now();
            }
#if APR_HAS_THREADS
            apr_thread_mutex_lock(ms->lock);
#endif
            /* Try the the dead server, every 5 seconds */
            if (curtime - ms->btime >  apr_time_from_sec(5)) {
                if (mc_version_ping(ms) == APR_SUCCESS) {
                    ms->btime = curtime;
                    make_server_live(mc, ms);
#if APR_HAS_THREADS
                    apr_thread_mutex_unlock(ms->lock);
#endif
                    break;
                }
            }
#if APR_HAS_THREADS
            apr_thread_mutex_unlock(ms->lock);
#endif
        }
        h++;
        i++;
    } while(i < mc->ntotal);

    if (i == mc->ntotal) {
        ms = NULL;
    }

    return ms;
}

APR_DECLARE(apr_memcache_server_t *) apr_memcache_find_server(apr_memcache_t *mc, const char *host, apr_port_t port)
{
    int i;

    for (i = 0; i < mc->ntotal; i++) {
        if (strcmp(mc->live_servers[i]->host, host) == 0
            && mc->live_servers[i]->port == port) {

            return mc->live_servers[i];
        }
    }

    return NULL;
}

static apr_status_t ms_find_conn(apr_memcache_server_t *ms, apr_memcache_conn_t **conn) 
{
#if APR_HAS_THREADS
    return apr_reslist_acquire(ms->conns, (void **)conn);
#else
    *conn = ms->conn;
    return APR_SUCCESS;
#endif
}

static apr_status_t ms_bad_conn(apr_memcache_server_t *ms, apr_memcache_conn_t *conn) 
{
#if APR_HAS_THREADS
    return apr_reslist_invalidate(ms->conns, conn);
#else
    return APR_SUCCESS;
#endif
}

static apr_status_t ms_release_conn(apr_memcache_server_t *ms, apr_memcache_conn_t *conn) 
{
#if APR_HAS_THREADS
    return apr_reslist_release(ms->conns, conn);
#else
    return APR_SUCCESS;
#endif
}

APR_DECLARE(apr_status_t) apr_memcache_enable_server(apr_memcache_t *mc, apr_memcache_server_t *ms)
{
    apr_status_t rv = APR_SUCCESS;

    if (ms->status == APR_MC_SERVER_LIVE) {
        return rv;
    }

    rv = make_server_live(mc, ms);
    return rv;
}

APR_DECLARE(apr_status_t) apr_memcache_disable_server(apr_memcache_t *mc, apr_memcache_server_t *ms)
{
    return make_server_dead(mc, ms);
}

static apr_status_t conn_connect(apr_memcache_conn_t *conn)
{
    apr_status_t rv = APR_SUCCESS;
    apr_sockaddr_t *sa;

    rv = apr_sockaddr_info_get(&sa, conn->ms->host, APR_INET, conn->ms->port, 0, conn->p);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_socket_timeout_set(conn->sock, 1 * APR_USEC_PER_SEC);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_socket_connect(conn->sock, sa);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_socket_timeout_set(conn->sock, -1);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    return rv;
}


static apr_status_t
mc_conn_construct(void **conn_, void *params, apr_pool_t *pool)
{
    apr_status_t rv = APR_SUCCESS;
    apr_memcache_conn_t *conn;
    apr_bucket *e;
    apr_pool_t *np;
    apr_memcache_server_t *ms = params;

    rv = apr_pool_create(&np, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    conn = apr_palloc(np, sizeof( apr_memcache_conn_t ));

    conn->p = np;

    rv = apr_socket_create(&conn->sock, APR_INET, SOCK_STREAM, 0, np);

    if (rv != APR_SUCCESS) {
        apr_pool_destroy(np);
        return rv;
    }

    conn->balloc = apr_bucket_alloc_create(conn->p);
    conn->bb = apr_brigade_create(conn->p, conn->balloc);
    conn->tb = apr_brigade_create(conn->p, conn->balloc);
    conn->blen = 0;
    conn->ms = ms;

    e = apr_bucket_socket_create(conn->sock, conn->balloc);
    APR_BRIGADE_INSERT_TAIL(conn->bb, e);

    rv = conn_connect(conn);
    if (rv != APR_SUCCESS) {
        apr_pool_destroy(np);
    }
    else {
        *conn_ = conn;
    }
    
    return rv;
}

static void setup_req_hdr(apr_mc_hdr_t *hdr, apr_uint8_t cmd)
{
    hdr->magic = REQ_MAGIC_BYTE;
    hdr->cmd = cmd;
    hdr->req_keylen = 0;
    hdr->req_reserved = 0;
    hdr->opaque_id = 0;
    hdr->lenleft = 0;
}

static void finalize_req_hdr(apr_mc_hdr_t *hdr)
{
    /* this isn't strictly needed, but will be nicer for protocol analyzers */
    hdr->opaque_id = htonl(hdr->opaque_id);
    hdr->lenleft = htonl(hdr->lenleft);
}

static void repair_hdr_rep(apr_mc_hdr_t *hdr)
{
    hdr->opaque_id = ntohl(hdr->opaque_id);
    hdr->lenleft = ntohl(hdr->lenleft);
}

static apr_status_t brigade_split_offset(apr_bucket_brigade *bbOut,
                                         apr_bucket_brigade *bbIn,
                                         apr_read_type_e block,
                                         apr_off_t offset)
{
    apr_off_t readbytes = 0;
    
    while (!APR_BRIGADE_EMPTY(bbIn)) {
        const char *str;
        apr_size_t len;
        apr_status_t rv;
        apr_bucket *e;

        e = APR_BRIGADE_FIRST(bbIn);
        rv = apr_bucket_read(e, &str, &len, block);

        if (rv != APR_SUCCESS) {
            /* if we didn't read the entire thing, stick what we have buffered
             * back into bbOut */
            if (!APR_BRIGADE_EMPTY(bbOut)) {
                APR_BRIGADE_PREPEND(bbIn, bbOut);
            }
            return rv;
        }
        
        if (readbytes+len > offset) {
            apr_bucket_split(e, offset - readbytes);
            APR_BUCKET_REMOVE(e);
            APR_BRIGADE_INSERT_TAIL(bbOut, e);
            return APR_SUCCESS;            
        }

        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(bbOut, e);
        readbytes += len;
    }
    
    return APR_SUCCESS;
}

static apr_status_t get_server_rep(apr_memcache_conn_t *conn, apr_mc_hdr_t *hdr)
{
    apr_size_t bsize = MC_HDR_LEN;
    apr_status_t rv = APR_SUCCESS;
    
    rv = brigade_split_offset(conn->tb, conn->bb, APR_BLOCK_READ, bsize);
    
    if (rv != APR_SUCCESS) {
        return rv;
    }
    
    rv = apr_brigade_flatten(conn->tb, (char*)hdr, &bsize);
    
    if (rv != APR_SUCCESS) {
        return rv;
    }
    
    repair_hdr_rep(hdr);
    
    return apr_brigade_cleanup(conn->tb);
}


static apr_status_t
mc_conn_destruct(void *conn_, void *params, apr_pool_t *pool)
{
    apr_memcache_conn_t *conn = (apr_memcache_conn_t*)conn_;
    apr_mc_hdr_t hdr;
    
    struct iovec vec[2];
    apr_size_t written;
    
    /* send a quit message to the memcached server to be nice about it. */
    setup_req_hdr(&hdr, CMD_QUIT);
    finalize_req_hdr(&hdr);
    vec[0].iov_base = &hdr;
    vec[0].iov_len = MC_HDR_LEN;

    /* Return values not checked, since we just want to make it go away. */
    apr_socket_sendv(conn->sock, vec, 1, &written);
    apr_socket_close(conn->sock);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_memcache_server_create(apr_pool_t *p, 
                                                     const char *host, apr_port_t port, 
                                                     apr_uint32_t min, apr_uint32_t smax,
                                                     apr_uint32_t max, apr_uint32_t ttl,
                                                     apr_memcache_server_t **ms)
{
    apr_status_t rv = APR_SUCCESS;
    apr_memcache_server_t *server;
    apr_pool_t *np;

    rv = apr_pool_create(&np, p);

    server = apr_palloc(np, sizeof(apr_memcache_server_t));

    server->p = np;
    server->host = apr_pstrdup(np, host);
    server->port = port;
    server->status = APR_MC_SERVER_DEAD;
#if APR_HAS_THREADS
    rv = apr_thread_mutex_create(&server->lock, APR_THREAD_MUTEX_DEFAULT, np);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_reslist_create(&server->conns, 
                               min,                     /* hard minimum */
                               smax,                    /* soft maximum */
                               max,                     /* hard maximum */
                               ttl,                     /* Time to live */
                               mc_conn_construct,       /* Make a New Connection */
                               mc_conn_destruct,        /* Kill Old Connection */
                               server, np);
#else
    rv = mc_conn_construct((void**)&(server->conn), server, np);
#endif

    if (rv != APR_SUCCESS) {
        return rv;
    }

    *ms = server;

    return rv;
}

APR_DECLARE(apr_status_t) apr_memcache_create(apr_pool_t *p,
                                              apr_uint16_t max_servers, apr_uint32_t flags,
                                              apr_memcache_t **memcache) 
{
    apr_status_t rv = APR_SUCCESS;
    apr_memcache_t *mc;
    
    mc = apr_palloc(p, sizeof(apr_memcache_t));
    mc->p = p;
    mc->nalloc = max_servers;
    mc->ntotal = 0;
    mc->live_servers = apr_palloc(p, mc->nalloc * sizeof(struct apr_memcache_server_t *));
    mc->hash_func = NULL;
    mc->hash_baton = NULL;
    mc->server_func = NULL;
    mc->server_baton = NULL;
    *memcache = mc;
    return rv;
}


/* The crc32 functions and data was originally written by Spencer
 * Garrett <srg@quick.com> and was gleaned from the PostgreSQL source
 * tree via the files contrib/ltree/crc32.[ch] and from FreeBSD at
 * src/usr.bin/cksum/crc32.c.
 */ 

static const apr_uint32_t crc32tab[256] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

APR_DECLARE(apr_uint32_t) apr_memcache_hash_crc32(void *baton, 
                                                    const char *data,
                                                    const apr_size_t data_len)
{
    apr_uint32_t i;
    apr_uint32_t crc;
    crc = ~0;
    
    for (i = 0; i < data_len; i++)
        crc = (crc >> 8) ^ crc32tab[(crc ^ (data[i])) & 0xff];
    
    return ~crc;
}

APR_DECLARE(apr_uint32_t) apr_memcache_hash_default(void *baton, 
                                                    const char *data,
                                                    const apr_size_t data_len)
{
    /* The default Perl Client doesn't actually use just crc32 -- it shifts it again
     * like this....
     */
    return ((apr_memcache_hash_crc32(baton, data, data_len) >> 16) & 0x7fff);
}

APR_DECLARE(apr_uint32_t) apr_memcache_hash(apr_memcache_t *mc,
                                            const char *data,
                                            const apr_size_t data_len)
{
    if (mc->hash_func) {
        return mc->hash_func(mc->hash_baton, data, data_len);
    }
    else {
        return apr_memcache_hash_default(NULL, data, data_len);
    }
}

static apr_status_t storage_cmd_write(apr_memcache_t *mc,
                                      apr_uint16_t cmdtype,
                                      const char *key,
                                      char *data,
                                      const apr_size_t data_size,
                                      apr_uint32_t timeout,
                                      apr_uint16_t flags)
{
    apr_mc_hdr_t hdr;
    apr_uint32_t hash;
    apr_memcache_server_t *ms;
    apr_memcache_conn_t *conn;
    apr_status_t rv;
    apr_size_t written;
    struct iovec vec[5];
    int klen;

    apr_size_t key_size = strlen(key);

    hash = apr_memcache_hash(mc, key, key_size);

    ms = apr_memcache_find_server_hash(mc, hash);

    if (ms == NULL)
        return APR_NOTFOUND;

    rv = ms_find_conn(ms, &conn);

    if (rv != APR_SUCCESS) {
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    setup_req_hdr(&hdr, cmdtype);
    hdr.req_keylen = key_size;
    hdr.opaque_id = 1;
    hdr.lenleft = 12 + key_size + data_size;
    
    finalize_req_hdr(&hdr);

    vec[0].iov_base = &hdr;
    vec[0].iov_len  = MC_HDR_LEN;

    flags = htonl(flags);
    vec[1].iov_base = (void*)&flags;
    vec[1].iov_len  = sizeof(apr_uint32_t);
    
    timeout = htonl(timeout);
    vec[2].iov_base = (void*)&timeout;
    vec[2].iov_len  = sizeof(apr_uint32_t);

    vec[3].iov_base = (void*)key;
    vec[3].iov_len  = key_size;
    
    vec[4].iov_base = data;
    vec[4].iov_len  = data_size;

    rv = apr_socket_sendv(conn->sock, vec, 5, &written);

    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = get_server_rep(conn, &hdr);

    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = APR_EGENERAL;
    if (hdr.rep_status) {
        if (hdr.rep_status == MC_STATUS_STORED) {
            rv = APR_SUCCESS;
        }
        else if (hdr.rep_status == MC_STATUS_NOT_STORED) {
            rv = APR_EEXIST;
        }
    }
    
    ms_release_conn(ms, conn);

    return rv;
}

APR_DECLARE(apr_status_t)
apr_memcache_set(apr_memcache_t *mc,
                 const char *key,
                 char *data,
                 const apr_size_t data_size,
                 apr_uint32_t timeout,
                 apr_uint16_t flags)
{
    return storage_cmd_write(mc,
                           CMD_SET,
                           key,
                           data, data_size,
                           timeout, flags);
}

APR_DECLARE(apr_status_t)
apr_memcache_add(apr_memcache_t *mc,
                 const char *key,
                 char *data,
                 const apr_size_t data_size,
                 apr_uint32_t timeout,
                 apr_uint16_t flags)
{
    return storage_cmd_write(mc, 
                           CMD_ADD,
                           key,
                           data, data_size,
                           timeout, flags);
}

APR_DECLARE(apr_status_t)
apr_memcache_replace(apr_memcache_t *mc,
                 const char *key,
                 char *data,
                 const apr_size_t data_size,
                 apr_uint32_t timeout,
                 apr_uint16_t flags)
{
    return storage_cmd_write(mc,
                           CMD_REPLACE,
                           key,
                           data, data_size,
                           timeout, flags);

}

APR_DECLARE(apr_status_t)
apr_memcache_getp(apr_memcache_t *mc,
                  apr_pool_t *p,
                  const char *key,
                  char **baton,
                  apr_size_t *new_length,
                  apr_uint32_t *flags)
{
    apr_mc_hdr_t hdr;
    apr_status_t rv;
    apr_memcache_server_t *ms;
    apr_memcache_conn_t *conn;
    apr_uint32_t hash;
    apr_size_t written;
    int klen = strlen(key);
    struct iovec vec[2];

    hash = apr_memcache_hash(mc, key, klen);
    ms = apr_memcache_find_server_hash(mc, hash);
    if (ms == NULL)
        return APR_NOTFOUND;
    
    rv = ms_find_conn(ms, &conn);

    if (rv != APR_SUCCESS) {
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    setup_req_hdr(&hdr, CMD_GET);
    hdr.req_keylen = klen;
    hdr.opaque_id = 1;
    hdr.lenleft = klen;
    
    finalize_req_hdr(&hdr);
    
    vec[0].iov_base = &hdr;
    vec[0].iov_len  = MC_HDR_LEN;
    
    vec[1].iov_base = (void*)key;
    vec[1].iov_len  = klen;
    
    rv = apr_socket_sendv(conn->sock, vec, 2, &written);

    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = get_server_rep(conn, &hdr);
    
    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }
    
    {
        apr_size_t len;

        rv = brigade_split_offset(conn->tb, conn->bb, APR_BLOCK_READ, hdr.lenleft);
        
        if (rv != APR_SUCCESS) {
            ms_bad_conn(ms, conn);
            apr_memcache_disable_server(mc, ms);
            return rv;
        }
        
        len = hdr.lenleft + 1;
        
        rv = apr_brigade_pflatten(conn->tb, baton, &len, p);

        if (rv != APR_SUCCESS) {
            ms_bad_conn(ms, conn);
            apr_memcache_disable_server(mc, ms);
            return rv;
        }

        apr_brigade_cleanup(conn->tb);
        
        *new_length = len - 1 - 4;
        memcpy(flags, *baton, 4);
        *flags = ntohl(*flags);
        *baton =  *baton + 4;
        (*baton)[*new_length] = '\0';
    }

    ms_release_conn(ms, conn);

    return rv;
}

APR_DECLARE(apr_status_t)
apr_memcache_delete(apr_memcache_t *mc,
                    const char *key,
                    apr_uint32_t timeout)
{
    apr_mc_hdr_t hdr;
    apr_status_t rv;
    apr_memcache_server_t *ms;
    apr_memcache_conn_t *conn;
    apr_uint32_t hash;
    apr_size_t written;
    struct iovec vec[3];
    int klen = strlen(key);

    hash = apr_memcache_hash(mc, key, klen);
    ms = apr_memcache_find_server_hash(mc, hash);
    if (ms == NULL)
        return APR_NOTFOUND;
    
    rv = ms_find_conn(ms, &conn);

    if (rv != APR_SUCCESS) {
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    setup_req_hdr(&hdr, CMD_DELETE);
    hdr.req_keylen = klen;
    hdr.opaque_id = 1;
    hdr.lenleft = klen;
    
    finalize_req_hdr(&hdr);
    
    vec[0].iov_base = &hdr;
    vec[0].iov_len  = MC_HDR_LEN;
    
    vec[1].iov_base = (void*)key;
    vec[1].iov_len  = klen;

    rv = apr_socket_sendv(conn->sock, vec, 2, &written);

    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = get_server_rep(conn, &hdr);
    
    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = APR_EGENERAL;
    if (hdr.rep_status) {
        if (hdr.rep_status == MC_STATUS_DELETED) {
            rv = APR_SUCCESS;
        }
        else if (hdr.rep_status == MC_STATUS_NOT_FOUND) {
            rv = APR_NOTFOUND;
        }
    }
    
    ms_release_conn(ms, conn);

    return rv;
}

static apr_status_t num_cmd_write(apr_memcache_t *mc,
                                  apr_uint8_t cmd,
                                  const char *key,
                                  const apr_int32_t inc,
                                  apr_uint32_t *new_value)
{
    apr_uint32_t tinc;
    apr_mc_hdr_t hdr;
    apr_status_t rv;
    apr_memcache_server_t *ms;
    apr_memcache_conn_t *conn;
    apr_uint32_t hash;
    apr_size_t written;
    struct iovec vec[3];
    int klen = strlen(key);

    hash = apr_memcache_hash(mc, key, klen);
    ms = apr_memcache_find_server_hash(mc, hash);
    if (ms == NULL)
        return APR_NOTFOUND;
    
    rv = ms_find_conn(ms, &conn);

    if (rv != APR_SUCCESS) {
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    setup_req_hdr(&hdr, CMD_DELETE);
    hdr.req_keylen = klen;
    hdr.opaque_id = 1;
    hdr.lenleft = klen + 4;
    
    finalize_req_hdr(&hdr);
    
    vec[0].iov_base = &hdr;
    vec[0].iov_len  = MC_HDR_LEN;
    
    tinc = htonl(inc);
    
    vec[1].iov_base = (void*)&tinc;
    vec[1].iov_len  = sizeof(apr_uint32_t);

    vec[2].iov_base = (void*)key;
    vec[2].iov_len  = klen;
    
    rv = apr_socket_sendv(conn->sock, vec, 3, &written);

    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }

    rv = get_server_rep(conn, &hdr);
    
    if (rv != APR_SUCCESS) {
        ms_bad_conn(ms, conn);
        apr_memcache_disable_server(mc, ms);
        return rv;
    }
    
    rv = APR_EGENERAL;
    if (hdr.rep_status) {
        if (hdr.rep_status == MC_STATUS_DELETED) {
            rv = APR_SUCCESS;
        }
        else if (hdr.rep_status == MC_STATUS_NOT_FOUND) {
            rv = APR_NOTFOUND;
        }
    }
    else {
        rv = brigade_split_offset(conn->tb, conn->bb, APR_BLOCK_READ, hdr.lenleft);
        if (rv) {
            ms_bad_conn(ms, conn);
            apr_memcache_disable_server(mc, ms);
            return rv;
        }

        if (new_value) {
            apr_size_t len = 4;
            rv = apr_brigade_flatten(conn->tb, (char*)new_value, &len);
            if (rv) {
                ms_bad_conn(ms, conn);
                apr_memcache_disable_server(mc, ms);
                return rv;
            }
            
            *new_value = ntohl(*new_value);
            
            apr_brigade_cleanup(conn->tb);
        }
    }

    ms_release_conn(ms, conn);

    return rv;
}

APR_DECLARE(apr_status_t)
apr_memcache_incr(apr_memcache_t *mc,
                    const char *key,
                    apr_int32_t inc,
                    apr_uint32_t *new_value)
{
    return num_cmd_write(mc,
                         CMD_INCR,
                         key,
                         inc, 
                         new_value);
}


APR_DECLARE(apr_status_t)
apr_memcache_decr(apr_memcache_t *mc,
                    const char *key,
                    apr_int32_t inc,
                    apr_uint32_t *new_value)
{
    return num_cmd_write(mc,
                         CMD_DECR,
                         key,
                         inc, 
                         new_value);
}



APR_DECLARE(apr_status_t)
apr_memcache_version(apr_memcache_server_t *ms,
                  apr_pool_t *p,
                  char **baton)
{
    return APR_ENOTIMPL;
}

apr_status_t mc_version_ping(apr_memcache_server_t *ms)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(void) 
apr_memcache_add_multget_key(apr_pool_t *data_pool,
                             const char* key,
                             apr_hash_t **values)
{
    apr_memcache_value_t* value;
    int klen = strlen(key);

    /* create the value hash if need be */
    if (!*values) {
        *values = apr_hash_make(data_pool);
    }

    /* init key and add it to the value hash */
    value = apr_pcalloc(data_pool, sizeof(apr_memcache_value_t));

    value->status = APR_NOTFOUND;
    value->key = apr_pstrdup(data_pool, key);

    apr_hash_set(*values, value->key, klen, value);
}

APR_DECLARE(apr_status_t)
apr_memcache_multgetp(apr_memcache_t *mc,
                      apr_pool_t *temp_pool,
                      apr_pool_t *data_pool,
                      apr_hash_t *values)
{
    return APR_ENOTIMPL;
}


APR_DECLARE(apr_status_t)
apr_memcache_stats(apr_memcache_server_t *ms,
                  apr_pool_t *p,
                  apr_memcache_stats_t **stats) 
{
    return APR_ENOTIMPL;
}

