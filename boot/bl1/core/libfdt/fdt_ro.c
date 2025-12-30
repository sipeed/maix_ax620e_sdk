/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libfdt_env.h"

#include <fdt.h>
#include <libfdt.h>
#include "libfdt_internal.h"

static int _fdt_nodename_eq(const void *fdt, int offset,
			    const char *s, int len)
{
	const char *p = fdt_offset_ptr(fdt, offset + FDT_TAGSIZE, len+1);

	if (! p)
		/* short match */
		return 0;

	if (memcmp(p, s, len) != 0)
		return 0;

	if (p[len] == '\0')
		return 1;
	else if (!memchr(s, '@', len) && (p[len] == '@'))
		return 1;
	else
		return 0;
}

const char *fdt_string(const void *fdt, int stroffset)
{
	return (const char *)fdt + fdt_off_dt_strings(fdt) + stroffset;
}

int fdt_subnode_offset_namelen(const void *fdt, int offset,
			       const char *name, int namelen)
{
	int depth;

	FDT_CHECK_HEADER(fdt);

	for (depth = 0, offset = fdt_next_node(fdt, offset, &depth);
	     (offset >= 0) && (depth > 0);
	     offset = fdt_next_node(fdt, offset, &depth)) {
		if (depth < 0)
			return -FDT_ERR_NOTFOUND;
		else if ((depth == 1)
			 && _fdt_nodename_eq(fdt, offset, name, namelen))
			return offset;
	}

	if (offset < 0)
		return offset; /* error */
	else
		return -FDT_ERR_NOTFOUND;
}

int fdt_subnode_offset(const void *fdt, int parentoffset,
		       const char *name)
{
	return fdt_subnode_offset_namelen(fdt, parentoffset, name, strlen(name));
}

const char *fdt_get_name(const void *fdt, int nodeoffset, int *len)
{
	const struct fdt_node_header *nh = _fdt_offset_ptr(fdt, nodeoffset);
	int err;

	if (((err = fdt_check_header(fdt)) != 0)
	    || ((err = _fdt_check_node_offset(fdt, nodeoffset)) < 0))
			goto fail;

	if (len)
		*len = strlen(nh->name);

	return nh->name;

 fail:
	if (len)
		*len = err;
	return NULL;
}

const struct fdt_property *fdt_get_property(const void *fdt,
					    int nodeoffset,
					    const char *name, int *lenp)
{
	uint32_t tag;
	const struct fdt_property *prop;
	int namestroff;
	int offset, nextoffset;
	int err;

	if (((err = fdt_check_header(fdt)) != 0)
	    || ((err = _fdt_check_node_offset(fdt, nodeoffset)) < 0))
			goto fail;

	nextoffset = err;
	do {
		offset = nextoffset;

		tag = fdt_next_tag(fdt, offset, &nextoffset);
		switch (tag) {
		case FDT_END:
			err = -FDT_ERR_TRUNCATED;
			goto fail;

		case FDT_BEGIN_NODE:
		case FDT_END_NODE:
		case FDT_NOP:
			break;

		case FDT_PROP:
			err = -FDT_ERR_BADSTRUCTURE;
			prop = fdt_offset_ptr(fdt, offset, sizeof(*prop));
			if (! prop)
				goto fail;
			namestroff = fdt32_to_cpu(prop->nameoff);
			if (strcmp(fdt_string(fdt, namestroff), name) == 0) {
				/* Found it! */
				int len = fdt32_to_cpu(prop->len);
				prop = fdt_offset_ptr(fdt, offset,
						      sizeof(*prop)+len);
				if (! prop)
					goto fail;

				if (lenp)
					*lenp = len;

				return prop;
			}
			break;

		default:
			err = -FDT_ERR_BADSTRUCTURE;
			goto fail;
		}
	} while ((tag != FDT_BEGIN_NODE) && (tag != FDT_END_NODE));

	err = -FDT_ERR_NOTFOUND;
 fail:
	if (lenp)
		*lenp = err;
	return NULL;
}

const void *fdt_getprop(const void *fdt, int nodeoffset,
		  const char *name, int *lenp)
{
	const struct fdt_property *prop;

	prop = fdt_get_property(fdt, nodeoffset, name, lenp);
	if (! prop)
		return NULL;

	return prop->data;
}
