#ifndef APU_COMPAT_H
#define APU_COMPAT_H

/* Include the apr compatibility changes, since apr-util users are
 * always apr users.
 */
#include "apr_compat.h"

/* redefine 1.3.x symbols to those that now live in libapr-util */

#define ap_base64decode apr_base64decode
#define ap_base64decode_binary apr_base64decode_binary
#define ap_base64decode_len apr_base64decode_len
#define ap_base64encode apr_base64encode
#define ap_base64encode_binary apr_base64encode_binary
#define ap_base64encode_len apr_base64encode_len
#define ap_hook_deregister_all apr_hook_deregister_all
#define ap_hook_sort_register apr_hook_sort_register
#define ap_show_hook apr_show_hook

#endif /* APU_COMPAT_H */
