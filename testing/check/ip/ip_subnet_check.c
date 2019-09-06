/* test subnets, for libreswan
 *
 * Copyright (C) 2000  Henry Spencer.
 * Copyright (C) 2018, 2019  Andrew Cagney
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/lgpl-2.1.txt>.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 */

#include <stdio.h>

#include "lswcdefs.h"		/* for elemsof() */
#include "constants.h"		/* for streq() */
#include "ipcheck.h"
#include "ip_subnet.h"

static void check_str_subnet(void)
{
	static const struct test {
		int family;
		char *in;
		char *out;	/* NULL means error expected */
	} tests[] = {
		{ 4, "1.2.3.0/255.255.255.0", "1.2.3.0/24" },
		{ 4, "1.2.3.0/24", "1.2.3.0/24" },
		{ 4, "1.2.3.1/255.255.255.240", "1.2.3.0/28" },
		{ 4, "1.2.3.1/32", "1.2.3.1/32" },
		{ 4, "1.2.3.1/0", "0.0.0.0/0" },
/*	{4, "1.2.3.1/255.255.127.0",	"1.2.3.0/255.255.127.0"}, */
		{ 4, "1.2.3.1/255.255.127.0", NULL },
		{ 4, "128.009.000.032/32", "128.9.0.32/32" },
		{ 4, "128.0x9.0.32/32", NULL },
		{ 4, "0x80090020/32", "128.9.0.32/32" },
		{ 4, "0x800x0020/32", NULL },
		{ 4, "128.9.0.32/0xffFF0000", "128.9.0.0/16" },
		{ 4, "128.9.0.32/0xff0000FF", NULL },
		{ 4, "128.9.0.32/0x0000ffFF", NULL },
		{ 4, "128.9.0.32/0x00ffFF0000", NULL },
		{ 4, "128.9.0.32/0xffFF", NULL },
		{ 4, "128.9.0.32.27/32", NULL },
		{ 4, "128.9.0k32/32", NULL },
		{ 4, "328.9.0.32/32", NULL },
		{ 4, "128.9..32/32", NULL },
		{ 4, "10/8", "10.0.0.0/8" },
		{ 4, "10.0/8", "10.0.0.0/8" },
		{ 4, "10.0.0/8", "10.0.0.0/8" },
		{ 4, "10.0.1/24", "10.0.1.0/24" },
		{ 4, "_", NULL },
		{ 4, "_/_", NULL },
		{ 4, "1.2.3.1", NULL },
		{ 4, "1.2.3.1/_", NULL },
		{ 4, "1.2.3.1/24._", NULL },
		{ 4, "1.2.3.1/99", NULL },
		{ 4, "localhost/32", NULL },
		{ 4, "%default", "0.0.0.0/0" },
		{ 6, "3049:1::8007:2040/0", "::/0" },
		{ 6, "3049:1::8007:2040/128", "3049:1::8007:2040/128" },
		{ 6, "3049:1::192.168.0.1/128", NULL },	/*"3049:1::c0a8:1/128",*/
		{ 6, "3049:1::8007::2040/128", NULL },
		{ 6, "3049:1::8007:2040/ffff:0", NULL },
		{ 6, "3049:1::8007:2040/64", "3049:1::/64" },
		{ 6, "3049:1::8007:2040/ffff:", NULL },
		{ 6, "3049:1::8007:2040/0000:ffff::0", NULL },
		{ 6, "3049:1::8007:2040/ff1f:0", NULL },
		{ 6, "3049:1::8007:x:2040/128", NULL },
		{ 6, "3049:1t::8007:2040/128", NULL },
		{ 6, "3049:1::80071:2040/128", NULL },
		{ 6, "::/21", "::/21" },
		{ 6, "::1/128", "::1/128" },
		{ 6, "1::/21", "1::/21" },
		{ 6, "1::2/128", "1::2/128" },
		{ 6, "1:0:0:0:0:0:0:2/128", "1::2/128" },
		{ 6, "1:0:0:0:3:0:0:2/128", "1::3:0:0:2/128" },
		{ 6, "1:0:0:3:0:0:0:2/128", "1:0:0:3::2/128" },
		{ 6, "1:0:3:0:0:0:0:2/128", "1:0:3::2/128" },
		{ 6, "abcd:ef01:2345:6789:0:00a:000:20/128",
		  "abcd:ef01:2345:6789:0:a:0:20/128" },
		{ 6, "3049:1::8007:2040/ffff:ffff:", NULL },
		{ 6, "3049:1::8007:2040/ffff:88:", NULL },
		{ 6, "3049:12::9000:3200/ffff:fff0", NULL },
		{ 6, "3049:12::9000:3200/28", "3049:10::/28" },
		{ 6, "3049:12::9000:3200/ff00:", NULL },
		{ 6, "3049:12::9000:3200/ffff:", NULL },
		{ 6, "3049:12::9000:3200/128_", NULL },
		{ 6, "3049:12::9000:3200/", NULL },
		{ 6, "%default", "::/0" },
	};

	const char *oops;

	for (size_t ti = 0; ti < elemsof(tests); ti++) {
		const struct test *t = &tests[ti];
		PRINT_IN(stdout, " -> '%s'",
			 t->out ? t->out : "<error>");

		sa_family_t af = SA_FAMILY(t->family);

		ip_subnet s;
		oops = ttosubnet(t->in, 0, af, &s);
		if (oops != NULL && t->out == NULL) {
			/* Error was expected, do nothing */
			continue;
		} else if (oops != NULL && t->out != NULL) {
			/* Error occurred, but we didn't expect one  */
			FAIL_IN("ttosubnet failed: %s", oops);
		} else if (oops == NULL && t->out == NULL) {
			/* If no errors, but we expected one */
			FAIL_IN("ttosubnet succeeded unexpectedly");
		}

		CHECK_TYPE(FAIL_IN, subnet_type(&s), t->family);

		subnet_buf buf;
		const char *out = str_subnet(&s, &buf);
		if (!streq(t->out, out)) {
			FAIL_IN("str_subnet() returned '%s', expected '%s'",
				out, t->out);
		}
	}
}

static void check_str_subnet_port(void)
{
	/*
	 * XXX: can't yet do invalid ports.
	 */
	static const struct test {
		int family;
		char *in;
		char *out;	/* NULL means error expected */
	} tests[] = {
		/* no port as in :0 should not appear (broken as uint16_t port) */
		{ 4, "0.0.0.0/0", "0.0.0.0/0:0" },
		{ 6, "::/0", "::/0:0", },
		/* any */
		{ 4, "0.0.0.0/0:0", "0.0.0.0/0:0" },
		{ 6, "::/0:0", "::/0:0", },
		/* longest */
		{ 4, "101.102.103.104/32:65535", "101.102.103.104/32:65535" },
		{ 6, "1001:1002:1003:1004:1005:1006:1007:1008/128:65535", "1001:1002:1003:1004:1005:1006:1007:1008/128:65535", },
	};

	const char *oops;

	for (size_t ti = 0; ti < elemsof(tests); ti++) {
		const struct test *t = &tests[ti];
		PRINT_IN(stdout, " -> '%s'",
			 t->out ? t->out : "<error>");

		sa_family_t af = SA_FAMILY(t->family);

		ip_subnet s;
		oops = ttosubnet(t->in, 0, af, &s);
		if (oops != NULL && t->out == NULL) {
			/* Error was expected, do nothing */
			continue;
		} else if (oops != NULL && t->out != NULL) {
			/* Error occurred, but we didn't expect one  */
			FAIL_IN("ttosubnet failed: %s", oops);
		} else if (oops == NULL && t->out == NULL) {
			/* If no errors, but we expected one */
			FAIL_IN("ttosubnet succeeded unexpectedly");
		}

		CHECK_TYPE(FAIL_IN, subnet_type(&s), t->family);

		subnet_buf buf;
		const char *out = str_subnet_port(&s, &buf);
		if (!streq(t->out, out)) {
			FAIL_IN("str_subnet_port() returned '%s', expected '%s'",
				out, t->out);
		}
	}
}

static void check_subnet_mask(void)
{
	static const struct test {
		int family;
		const char *in;
		const char *mask;
	} tests[] = {
		{ 4, "1.2.3.4/1", "128.0.0.0", },
		{ 4, "1.2.3.4/23", "255.255.254.0", },
		{ 4, "1.2.3.4/24", "255.255.255.0", },
		{ 4, "1.2.3.4/25", "255.255.255.128", },
		{ 4, "1.2.3.4/31", "255.255.255.254", },
		{ 4, "1.2.3.4/32", "255.255.255.255", },
		{ 6, "1:2:3:4:5:6:7:8/1", "8000::", },
		{ 6, "1:2:3:4:5:6:7:8/63", "ffff:ffff:ffff:fffe::", },
		{ 6, "1:2:3:4:5:6:7:8/64", "ffff:ffff:ffff:ffff::", },
		{ 6, "1:2:3:4:5:6:7:8/65", "ffff:ffff:ffff:ffff:8000::", },
		{ 6, "1:2:3:4:5:6:7:8/127", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe", },
		{ 6, "1:2:3:4:5:6:7:8/128", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", },
	};

	for (size_t ti = 0; ti < elemsof(tests); ti++) {
		const struct test *t = &tests[ti];
		PRINT_IN(stdout, " -> %s", t->mask);

		sa_family_t af = SA_FAMILY(t->family);

		ip_subnet s;
		err_t oops = ttosubnet(t->in, 0, af, &s);
		if (oops != NULL) {
			FAIL_IN("ttosubnet() failed: %s", oops);
		}

		CHECK_TYPE(FAIL_IN, subnet_type(&s), t->family);

		address_buf buf;
		const char *out;

		ip_address mask = subnet_mask(&s);
		out = str_address(&mask, &buf);
		if (!streq(t->mask, out)) {
			FAIL_IN("subnet_mask() returned '%s', expected '%s'",
				out, t->mask);
		}
		CHECK_TYPE(FAIL_IN, address_type(&mask), t->family);
	}
}

static void check_subnet_prefix(void)
{
	static const struct test {
		int family;
		const char *in;
		const char *out;
	} tests[] = {
		{ 4, "255.255.255.255/1", "128.0.0.0", },
		{ 6, "ffff:2:3:4:5:6:7:8/1", "8000::", },

		{ 4, "1.2.255.255/23", "1.2.254.0", },
		{ 4, "1.2.255.255/24", "1.2.255.0", },
		{ 4, "1.2.255.255/25", "1.2.255.128", },
		{ 6, "1:2:3:ffff:ffff:6:7:8/63", "1:2:3:fffe::", },
		{ 6, "1:2:3:ffff:ffff:6:7:8/64", "1:2:3:ffff::", },
		{ 6, "1:2:3:ffff:ffff:6:7:8/65", "1:2:3:ffff:8000::", },

		{ 4, "1.2.3.255/31", "1.2.3.254", },
		{ 4, "1.2.3.255/32", "1.2.3.255", },
		{ 6, "1:2:3:4:5:6:7:ffff/127", "1:2:3:4:5:6:7:fffe", },
		{ 6, "1:2:3:4:5:6:7:ffff/128", "1:2:3:4:5:6:7:ffff", },
	};

	for (size_t ti = 0; ti < elemsof(tests); ti++) {
		const struct test *t = &tests[ti];
		PRINT_IN(stdout, " -> %s", t->out);

		sa_family_t af = SA_FAMILY(t->family);

		ip_subnet s;
		err_t oops = ttosubnet(t->in, 0, af, &s);
		if (oops != NULL) {
			FAIL_IN("ttosubnet() failed: %s", oops);
		}

		CHECK_TYPE(FAIL_IN, subnet_type(&s), t->family);

		ip_address prefix = subnet_prefix(&s);
		CHECK_TYPE(FAIL_IN, address_type(&prefix), t->family);

		address_buf buf;
		const char *out = str_address(&prefix, &buf);
		if (!streq(out, t->out)) {
			FAIL_IN("subnet_prefix() returned '%s', expected '%s'",
				out, t->out);
		}
	}
}

static void check_subnet_port(void)
{
	static const struct test {
		int family;
		const char *in;
		const char *out;
		int hport;
	} tests[] = {
		{ 4, "1.2.3.0/24", "1.2.3.0/24", 0, },
		{ 4, "1.2.3.0/24:0", "1.2.3.0/24", 0, },
		{ 4, "1.2.3.0/24:10", "1.2.3.0/24", 10, },
		{ 4, "1.2.3.0/24:-1", NULL },
		{ 4, "1.2.3.0/24:none", NULL },
		{ 4, "1.2.3.0/24:", NULL },
		{ 4, "1.2.3.0/24:0x10", "1.2.3.0/24", 16, },
		{ 4, "1.2.3.0/24:0X10", "1.2.3.0/24", 16, },
		{ 4, "1.2.3.0/24:010", "1.2.3.0/24", 8, },
		{ 6, "3049:1::8007:2040/64", "3049:1::/64", 0, },
		{ 6, "3049:1::8007:2040/64:0", "3049:1::/64", 0, },
		{ 6, "3049:1::8007:2040/64:53", "3049:1::/64", 53, },
	};

	for (size_t ti = 0; ti < elemsof(tests); ti++) {
		const struct test *t = &tests[ti];
		PRINT_IN(stdout, " -> %d", t->hport);

		sa_family_t af = SA_FAMILY(t->family);

		ip_subnet s;
		err_t oops = ttosubnet(t->in, 0, af, &s);
		if (oops != NULL && t->out == NULL) {
			/* Error was expected, do nothing */
			continue;
		} else if (oops != NULL && t->out != NULL) {
			/* Error occurred, but we didn't expect one  */
			FAIL_IN("ttosubnet failed: %s", oops);
		} else if (oops == NULL && t->out == NULL) {
			/* If no errors, but we expected one */
			FAIL_IN("ttosubnet succeeded unexpectedly");
		}
		CHECK_TYPE(FAIL_IN, subnet_type(&s), t->family);

		subnet_buf out_buf;
		const char *out = str_subnet(&s, &out_buf);
		if (!streq(t->out, out)) {
			FAIL_IN("str_subnet() returned '%s', expected '%s'",
				out, t->out);
		}

		int hport = subnet_hport(&s);
		if (hport != t->hport) {
			FAIL_IN("subnet_hport() returned '%d', expected '%d'",
				hport, t->hport);
		}

		int nport = subnet_nport(&s);
		if (nport != htons(t->hport)) {
			FAIL_IN("subnet_nport() returned '%04x', expected '%04x'",
				nport, htons(t->hport));
		}

		ip_subnet ps = set_subnet_port(&s, t->hport+1);
		int pport = subnet_hport(&ps);
		if (pport != t->hport+1) {
			FAIL_IN("subnet_hport()+1 returned '%d', expected '%d'",
				pport, t->hport+1);
		}
	}
}

void ip_subnet_check(void)
{
	check_str_subnet();
	check_str_subnet_port();
	check_subnet_prefix();
	check_subnet_mask();
	check_subnet_port();
}