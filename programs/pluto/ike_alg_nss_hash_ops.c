/*
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
 */

#include <stdlib.h>

#include "libreswan.h"
#include "lswalloc.h"
#include "lswlog.h"
#include "ike_alg.h"
#include "crypt_symkey.h"
#include "crypto.h"
#include "lswfips.h"
#include "lswnss.h"

/*
 * These probably fail in FIPS mode.
 */
struct hash_context {
	lset_t debug;
	const char *name;
	PK11Context *context;
	const struct hash_desc *desc;
};

static struct hash_context *init(const struct hash_desc *hash_desc,
				 const char *name, lset_t debug)
{
	SECOidTag tag = PK11_MechanismToAlgtag(hash_desc->common.nss_mechanism);
	if (DBGP(debug)) {
		DBG_log("%s %s hasher: mapped mechanism %lx to tag %x",
			name, hash_desc->common.name,
			hash_desc->common.nss_mechanism, tag);
	}
	struct hash_context *hash = alloc_thing(struct hash_context, "hasher");
	*hash = (struct hash_context) {
		.context = PK11_CreateDigestContext(tag),
		.name = name,
		.debug = debug,
		.desc = hash_desc,
	};
	if (DBGP(hash->debug)) {
		DBG_log("%s %s hasher: context %p",
			name, hash_desc->common.name,
			hash->context);
	}
	passert(hash->context);
	SECStatus rc = PK11_DigestBegin(hash->context);
	passert(rc == SECSuccess);
	return hash;
}

static void digest_symkey(struct hash_context *hash,
			  const char *name UNUSED,
			  PK11SymKey *symkey)
{
	passert(digest_symkey == hash->desc->hash_ops->digest_symkey);
	SECStatus rc = PK11_DigestKey(hash->context, symkey);
	passert(rc == SECSuccess);
}

static void digest_bytes(struct hash_context *hash,
			 const char *name UNUSED,
			 const u_int8_t *bytes, size_t sizeof_bytes)
{
	passert(digest_bytes == hash->desc->hash_ops->digest_bytes);
	SECStatus rc = PK11_DigestOp(hash->context, bytes, sizeof_bytes);
	passert(rc == SECSuccess);
}

static void final_bytes(struct hash_context **hashp,
			u_int8_t *bytes, size_t sizeof_bytes)
{
	passert(final_bytes == (*hashp)->desc->hash_ops->final_bytes);
	unsigned out_len = 0;
	passert(sizeof_bytes == (*hashp)->desc->hash_digest_len);
	SECStatus rc = PK11_DigestFinal((*hashp)->context, bytes,
					&out_len, sizeof_bytes);
	passert(rc == SECSuccess);
	passert(out_len <= sizeof_bytes);
	PK11_DestroyContext((*hashp)->context, PR_TRUE);
	DBG(DBG_CRYPT, DBG_dump((*hashp)->name, bytes, sizeof_bytes));
	pfree(*hashp);
	*hashp = NULL;
}

/*
 * Some magic never dies.
 */
static CK_MECHANISM_TYPE nss_digest_derivation(CK_MECHANISM_TYPE hash)
{
	CK_MECHANISM_TYPE mechanism;
	switch (hash) {
	case CKM_MD5:
		mechanism = CKM_MD5_KEY_DERIVATION;
		break;
	case CKM_SHA_1:
		mechanism = CKM_SHA1_KEY_DERIVATION;
		break;
	case CKM_SHA224:
		mechanism = CKM_SHA224_KEY_DERIVATION;
		break;
	case CKM_SHA256:
		mechanism = CKM_SHA256_KEY_DERIVATION;
		break;
	case CKM_SHA384:
		mechanism = CKM_SHA384_KEY_DERIVATION;
		break;
	case CKM_SHA512:
		mechanism = CKM_SHA512_KEY_DERIVATION;
		break;
	default:
		libreswan_log("NSS hash %lx not supported", hash);
		mechanism = CKM_VENDOR_DEFINED;
		break;
	}
	if (DBGP(DBG_CRYPT)) {
		DBG_log("NSS: hash mechanism %lx derivation %lx",
			hash, mechanism);
	}
	return mechanism;
}

static PK11SymKey *symkey_to_symkey(const struct hash_desc *hash_desc,
				    const char *name, lset_t debug,
				    const char *symkey_name, PK11SymKey *symkey)
{
	CK_MECHANISM_TYPE derive = nss_digest_derivation(hash_desc->common.nss_mechanism);
	SECItem *param = NULL;
	CK_MECHANISM_TYPE target = CKM_CONCATENATE_BASE_AND_KEY; /* bogus */
	CK_ATTRIBUTE_TYPE operation = CKA_DERIVE;
	int key_size = 0;

	if (DBGP(debug)) {
		DBG_log("%s hash(%s) symkey %s(%p) to symkey - derive(%s)",
			name, hash_desc->common.name,
			symkey_name, symkey,
			lsw_nss_ckm_to_string(derive));
		DBG_symkey(symkey_name, symkey);
	}
	PK11SymKey *result = PK11_Derive(symkey, derive, param, target,
					 operation, key_size);
	if (DBGP(debug)) {
		DBG_symkey(name, result);
	}
	return result;
}

const struct hash_ops ike_alg_nss_hash_ops = {
	init,
	digest_symkey,
	digest_bytes,
	final_bytes,
	symkey_to_symkey,
};
