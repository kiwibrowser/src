/*
 * Copyright © 2017 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"

static int parse_one_line(const char *line, struct amdgpu_asic_id *id)
{
	char *buf, *saveptr;
	char *s_did;
	char *s_rid;
	char *s_name;
	char *endptr;
	int r = 0;

	buf = strdup(line);
	if (!buf)
		return -ENOMEM;

	/* ignore empty line and commented line */
	if (strlen(line) == 0 || line[0] == '#') {
		r = -EAGAIN;
		goto out;
	}

	/* device id */
	s_did = strtok_r(buf, ",", &saveptr);
	if (!s_did) {
		r = -EINVAL;
		goto out;
	}

	id->did = strtol(s_did, &endptr, 16);
	if (*endptr) {
		r = -EINVAL;
		goto out;
	}

	/* revision id */
	s_rid = strtok_r(NULL, ",", &saveptr);
	if (!s_rid) {
		r = -EINVAL;
		goto out;
	}

	id->rid = strtol(s_rid, &endptr, 16);
	if (*endptr) {
		r = -EINVAL;
		goto out;
	}

	/* marketing name */
	s_name = strtok_r(NULL, ",", &saveptr);
	if (!s_name) {
		r = -EINVAL;
		goto out;
	}
	/* trim leading whitespaces or tabs */
	while (isblank(*s_name))
		s_name++;
	if (strlen(s_name) == 0) {
		r = -EINVAL;
		goto out;
	}

	id->marketing_name = strdup(s_name);
	if (id->marketing_name == NULL) {
		r = -EINVAL;
		goto out;
	}

out:
	free(buf);

	return r;
}

int amdgpu_parse_asic_ids(struct amdgpu_asic_id **p_asic_id_table)
{
	struct amdgpu_asic_id *asic_id_table;
	struct amdgpu_asic_id *id;
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t n;
	int line_num = 1;
	size_t table_size = 0;
	size_t table_max_size = AMDGPU_ASIC_ID_TABLE_NUM_ENTRIES;
	int r = 0;

	fp = fopen(AMDGPU_ASIC_ID_TABLE, "r");
	if (!fp) {
		fprintf(stderr, "%s: %s\n", AMDGPU_ASIC_ID_TABLE,
			strerror(errno));
		return -EINVAL;
	}

	asic_id_table = calloc(table_max_size + 1,
			       sizeof(struct amdgpu_asic_id));
	if (!asic_id_table) {
		r = -ENOMEM;
		goto close;
	}

	/* 1st valid line is file version */
	while ((n = getline(&line, &len, fp)) != -1) {
		/* trim trailing newline */
		if (line[n - 1] == '\n')
			line[n - 1] = '\0';

		/* ignore empty line and commented line */
		if (strlen(line) == 0 || line[0] == '#') {
			line_num++;
			continue;
		}

		drmMsg("%s version: %s\n", AMDGPU_ASIC_ID_TABLE, line);
		break;
	}

	while ((n = getline(&line, &len, fp)) != -1) {
		if (table_size > table_max_size) {
			/* double table size */
			table_max_size *= 2;
			id = realloc(asic_id_table, (table_max_size + 1) *
				     sizeof(struct amdgpu_asic_id));
			if (!id) {
				r = -ENOMEM;
				goto free;
			}
                        asic_id_table = id;
		}

		id = asic_id_table + table_size;

		/* trim trailing newline */
		if (line[n - 1] == '\n')
			line[n - 1] = '\0';

		r = parse_one_line(line, id);
		if (r) {
			if (r == -EAGAIN) {
				line_num++;
				continue;
			}
			fprintf(stderr, "Invalid format: %s: line %d: %s\n",
				AMDGPU_ASIC_ID_TABLE, line_num, line);
			goto free;
		}

		line_num++;
		table_size++;
	}

	if (table_size != table_max_size) {
		id = realloc(asic_id_table, (table_size + 1) *
			     sizeof(struct amdgpu_asic_id));
		if (!id) {
			r = -ENOMEM;
			goto free;
		}
		asic_id_table = id;
        }

	/* end of table */
	id = asic_id_table + table_size;
	memset(id, 0, sizeof(struct amdgpu_asic_id));

free:
	free(line);

	if (r && asic_id_table) {
		while (table_size--) {
			id = asic_id_table + table_size;
			free(id->marketing_name);
		}
		free(asic_id_table);
		asic_id_table = NULL;
	}
close:
	fclose(fp);

	*p_asic_id_table = asic_id_table;

	return r;
}
