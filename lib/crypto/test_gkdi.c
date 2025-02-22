/*
 * Unix SMB/CIFS implementation.
 *
 * Copyright (C) Catalyst.Net Ltd 2024
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include "replace.h"
#include <talloc.h>
#include "libcli/util/ntstatus.h"
#include "lib/crypto/gkdi.h"
#include "lib/crypto/gmsa.h"

#include "librpc/ndr/libndr.h"
#include "librpc/gen_ndr/gmsa.h"
#include "librpc/gen_ndr/ndr_gmsa.h"
#include "lib/util/time.h"
#include "libcli/security/dom_sid.h"

static void test_password_based_on_key_id(void **state)
{
	static const uint8_t root_key_data[] = {
		152, 203, 215, 84,  113, 216, 118, 177, 81,  128, 50,  160, 148,
		132, 82,  244, 65,  179, 164, 219, 209, 14,  33,  131, 178, 193,
		80,  248, 126, 23,  66,	 227, 45,  221, 171, 12,  247, 15,  62,
		179, 164, 217, 123, 179, 106, 118, 228, 74,  12,  2,   241, 229,
		139, 55,  237, 155, 220, 122, 200, 245, 129, 222, 37,  15};
	static const struct ProvRootKey root_key = {
		.version = root_key_version_1,
		.id = {.time_low = 0x170e972,
		       .time_mid = 0xa035,
		       .time_hi_and_version = 0xaf4c,
		       .clock_seq = {0x99, 0x3b},
		       .node = {0xe0, 0x52, 0xd5, 0xbd, 0x12, 0x16}},
		.data =
			{
				.data = discard_const_p(uint8_t, root_key_data),
				.length = sizeof root_key_data,
			},
		.create_time = 0,
		.use_start_time = 0,
		.domain_id = "example.com",
		.kdf_algorithm = {
			.id = KDF_ALGORITHM_SP800_108_CTR_HMAC,
			.param.sp800_108 = KDF_PARAM_SHA512,
		}};
	static const uint8_t expected[GMSA_PASSWORD_LEN] = {
		114, 92,  31,  204, 138, 249, 1,   76,	192, 65,  52,  248, 247,
		191, 30,  213, 25,  38,	 81,  21,  183, 167, 154, 102, 190, 234,
		234, 116, 114, 18,  141, 208, 143, 38,	178, 115, 195, 26,  199,
		214, 176, 229, 128, 160, 147, 249, 245, 67,  165, 191, 192, 78,
		224, 50,  115, 8,   207, 124, 178, 121, 67,  135, 125, 113, 79,
		0,   131, 43,  74,  48,	 171, 239, 183, 228, 50,  212, 202, 215,
		188, 182, 94,  127, 117, 217, 91,  17,	90,  80,  158, 176, 204,
		151, 244, 107, 139, 65,	 94,  148, 216, 212, 97,  53,  54,  253,
		6,   201, 94,  93,  250, 213, 12,  82,	162, 246, 197, 254, 205,
		8,   19,  153, 66,  72,	 60,  167, 28,	65,  39,  218, 147, 82,
		162, 11,  177, 78,  231, 200, 66,  121, 9,   196, 240, 7,   148,
		190, 151, 96,  214, 246, 7,   110, 85,	0,   246, 28,  121, 3,
		61,  212, 204, 101, 153, 121, 100, 91,	65,  28,  225, 241, 123,
		115, 105, 138, 74,  187, 74,  188, 59,	17,  201, 229, 158, 170,
		184, 141, 237, 179, 246, 135, 104, 204, 56,  228, 156, 182, 26,
		90,  151, 147, 25,  142, 47,  172, 183, 165, 222, 240, 95,  63,
		79,  88,  35,  205, 76,	 26,  229, 107, 46,  16,  202, 102, 196,
		18,  140, 211, 242, 226, 198, 154, 97,	199, 44,  220, 186, 76,
		215, 54,  196, 44,  140, 145, 252, 99,	229, 179, 74,  150, 154,
		70,  226, 45,  122, 156, 156, 75,  83,	24};
	uint8_t out[GMSA_PASSWORD_NULL_TERMINATED_LEN];
	TALLOC_CTX *mem_ctx = NULL;

	mem_ctx = talloc_new(NULL);
	assert_non_null(mem_ctx);

	{
		/* An arbitrary GKID. */
		const struct Gkid gkid = {362, 0, 23};
		/* An arbitrary time in the past. */
		const NTTIME current_nt_time = 133524411756072082;
		struct dom_sid account_sid;
		NTSTATUS status;
		bool ok;

		ok = dom_sid_parse(
			"S-1-5-21-4119591868-3001629103-3445594911-48026",
			&account_sid);
		assert_true(ok);

		/* Derive a password from the root key. */
		status = gmsa_password_based_on_key_id(mem_ctx,
						       gkid,
						       current_nt_time,
						       &root_key,
						       &account_sid,
						       out);
		assert_true(NT_STATUS_IS_OK(status));
		assert_memory_equal(expected, out, GMSA_PASSWORD_LEN);
	}

	{
		uint64_t query_interval = 0;
		uint64_t unchanged_interval = 0;
		const struct MANAGEDPASSWORD_BLOB managed_pwd = {
			.passwords = {
				.current = out,
				.query_interval = &query_interval,
				.unchanged_interval = &unchanged_interval}};
		DATA_BLOB packed_blob = {};
		enum ndr_err_code err;

		err = ndr_push_struct_blob(
			&packed_blob,
			mem_ctx,
			&managed_pwd,
			(ndr_push_flags_fn_t)ndr_push_MANAGEDPASSWORD_BLOB);
		assert_int_equal(NDR_ERR_SUCCESS, err);
	}

	talloc_free(mem_ctx);
}

int main(int argc, char *argv[])
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_password_based_on_key_id),
	};

	if (argc == 2) {
		cmocka_set_test_filter(argv[1]);
	}
	cmocka_set_message_output(CM_OUTPUT_SUBUNIT);
	return cmocka_run_group_tests(tests, NULL, NULL);
}
