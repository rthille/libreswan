/* crypto interfaces
 *
 * Copyright (C) 1998-2001,2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2003-2008 Michael C. Richardson <mcr@xelerance.com>
 * Copyright (C) 2003-2010 Paul Wouters <paul@xelerance.com>
 * Copyright (C) 2009-2012 Avesh Agarwal <avagarwa@redhat.com>
 * Copyright (C) 2012-2013 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013 Florian Weimer <fweimer@redhat.com>
 * Copyright (C) 2016 Andrew Cagney <cagney@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>

#include "libreswan.h"
#include "constants.h"
#include "lswlog.h"
#include "ike_alg.h"
#include "ike_alg_sha1.h"
#include "ike_alg_nss_hash_ops.h"
#include "ike_alg_nss_prf_ops.h"

struct hash_desc ike_alg_hash_sha1 = {
	.common = {
		.name = "sha",
		.officname = "sha1",
		.algo_type = IKE_ALG_HASH,
		.ikev1_oakley_id = OAKLEY_SHA1,
		.ikev2_id = IKEv2_PRF_HMAC_SHA1,
		.fips = TRUE,
		.nss_mechanism = CKM_SHA_1,
	},
	.hash_digest_len = SHA1_DIGEST_SIZE,
	.hash_block_size = 64,	/* B from RFC 2104 */
	.hash_ops = &ike_alg_nss_hash_ops,
};

struct prf_desc ike_alg_prf_sha1 = {
	.common = {
		.name = "sha",
		.officname = "sha1",
		.algo_type = IKE_ALG_PRF,
		.ikev1_oakley_id = OAKLEY_SHA1,
		.ikev2_id = IKEv2_PRF_HMAC_SHA1,
		.fips = TRUE,
		.nss_mechanism = CKM_SHA_1_HMAC,
	},
	.prf_key_size = SHA1_DIGEST_SIZE,
	.prf_output_size = SHA1_DIGEST_SIZE,
	.hasher = &ike_alg_hash_sha1,
	.prf_ops = &ike_alg_nss_prf_ops,
};

struct integ_desc ike_alg_integ_sha1 = {
	.common = {
		.name = "sha",
		.officname = "sha1",
		.algo_type = IKE_ALG_INTEG,
		.ikev1_oakley_id = OAKLEY_SHA1,
		.ikev1_esp_id = AUTH_ALGORITHM_HMAC_SHA1,
		.ikev2_id = IKEv2_AUTH_HMAC_SHA1_96,
		.fips = TRUE,
		.nss_mechanism = CKM_SHA_1_HMAC,
	},
#if 0
	.hash_ctx_size = sizeof(SHA1_CTX),
	.hash_key_size =   SHA1_DIGEST_SIZE,
	.hash_digest_len = SHA1_DIGEST_SIZE,
	.hash_block_size = 64,	/* B from RFC 2104 */
	.hash_init = SHA1Init_thunk,
	.hash_update = SHA1Update_thunk,
	.hash_final = SHA1Final_thunk,
#endif
	.integ_key_size = SHA1_DIGEST_SIZE,
	.integ_output_size = SHA1_DIGEST_SIZE_96,
	.prf = &ike_alg_prf_sha1,
};
