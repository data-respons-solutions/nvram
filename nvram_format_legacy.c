#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "log.h"
#include "nvram_format.h"
#include "nvram_interface.h"
#include "libnvram/libnvram.h"

struct nvram {
	struct nvram_interface* interface;
	struct nvram_priv* interface_priv;
};

/* Returns pos of key in buf. */
/* Empty values are not supported and thus
 * a return value of 0 marks not found or too short */
static size_t find(uint8_t* buf, size_t buf_size, uint8_t key)
{
	for (size_t pos = 0; pos < buf_size; ++pos) {
		if (buf[pos] == key)
				return pos;
	}
	return 0;
}

/* entry->key and entry->value must be freed by caller */
static int append_null_terminator(struct libnvram_entry* entry)
{
	uint8_t* key_buf = malloc(entry->key_len + 1);
	if (key_buf == NULL)
		return -ENOMEM;
	uint8_t* value_buf = malloc(entry->value_len + 1);
	if (value_buf == NULL) {
		free(key_buf);
		return -ENOMEM;
	}
	memcpy(key_buf, entry->key, entry->key_len);
	key_buf[entry->key_len] = '\0';
	memcpy(value_buf, entry->value, entry->value_len);
	value_buf[entry->value_len] = '\0';
	entry->key = key_buf;
	entry->key_len++;
	entry->value = value_buf;
	entry->value_len++;
	return 0;
}

/* Returns pos in buffer AFTER extracted entry
 * 0 means no entry found */
static size_t find_entry(struct libnvram_entry* entry, uint8_t* buf, size_t buf_size)
{
	const size_t key_start = 0;
	const size_t key_end = find(buf + key_start, buf_size - key_start, '=');
	if (key_end == 0 || key_end > buf_size)
		return 0;
	/* verify no newline in key */
	if (find(buf + key_start, key_end - key_start, '\n') != 0)
		return 0;
	const size_t value_start = key_end + 1;
	/* No space left for value */
	if (value_start >= buf_size)
		return 0;
	const size_t r = find(buf + value_start, buf_size - value_start, '\n');
	/* Value empty if return 0 and last value is newline */
	if (r == 0 && buf[buf_size - 1] == '\n')
		return 0;
	/* No terminating newline at end-of-file if 0 and last char not newline */
	const size_t value_end = r == 0 ? buf_size : value_start + r;
	entry->key = buf + key_start;
	entry->key_len = key_end - key_start;
	entry->value = buf + value_start;
	entry->value_len = value_end - value_start;
	if (append_null_terminator(entry) != 0)
		return -ENOMEM;
	return value_end + 1;
}

static int populate_list(struct libnvram_list** list, uint8_t* buf, size_t buf_size)
{
	size_t pos = 0;
	struct libnvram_entry entry;
	while (pos < buf_size) {
		/* Skip whitespace in beginning of line and skip empty lines */
		if (buf[pos] == ' ' || buf[pos] == '\t' || buf[pos] == '\n') {
			pos++;
		}
		else {
			size_t r = find_entry(&entry, buf + pos, buf_size - pos);
			if (r == 0)
				return -EINVAL;
			if (libnvram_list_set(list, &entry) != 0)
				return -ENOMEM;
			free(entry.key);
			free(entry.value);
			pos	+= r;
		}
	}
	return 0;
}

static int legacy_init(struct nvram** nvram, struct nvram_interface* interface, struct libnvram_list** list, const char* section_a, const char* section_b)
{
	if (interface == NULL || interface->init == NULL || interface->destroy == NULL
		|| interface->size == NULL || interface->read == NULL
		|| interface->write == NULL || interface->section == NULL)
		return -EINVAL;

	if (!section_a || strlen(section_a) < 1)
		return -EINVAL;
	if (section_b && strlen(section_b) > 0) {
		pr_err("legacy interface supports single (A) section only\n");
		return -EINVAL;
	}
	struct nvram *pnvram = (struct nvram*) malloc(sizeof(struct nvram));
	if (!pnvram)
		return -ENOMEM;
	memset(pnvram, 0, sizeof(struct nvram));
	pnvram->interface = interface;

	int r = 0;
	uint8_t *buf = NULL;
	size_t buf_size = 0;

	r = pnvram->interface->init(&pnvram->interface_priv, section_a);
	if (r) {
		pr_err("%s: failed initializing [%d]: %s\n", section_a, -r, strerror(-r));
		goto exit;
	}
	r = pnvram->interface->size(pnvram->interface_priv, &buf_size);
	if (r) {
		pr_err("%s: failed checking size [%d]: %s\n", section_a, -r, strerror(-r));
		goto exit;
	}
	if (buf_size > 0) {
		buf = malloc(buf_size);
		if (buf == NULL) {
			r = -ENOMEM;
			pr_err("%s: failed allocating read buffer [%d]: %s\n", section_a, -r, strerror(-r));
			goto exit;
		}
		r = pnvram->interface->read(pnvram->interface_priv, buf, buf_size);
		if (r) {
			pr_err("%s: failed reading [%d]: %s\n", section_a, -r, strerror(-r));
			goto exit;
		}
		r = populate_list(list, buf, buf_size);
		if (r) {
			pr_err("%s: data corrupted [%d]: %s\n", section_a, -r, strerror(-r));
			goto exit;
		}
	}

	*nvram = pnvram;
	r = 0;
exit:
	if (r) {
		if (pnvram) {
			if (pnvram->interface_priv)
				pnvram->interface->destroy(&pnvram->interface_priv);
			free(pnvram);
		}
	}
	if (buf)
		free(buf);
	return r;
}

static int legacy_commit(struct nvram* nvram, const struct libnvram_list* list)
{
	const char* row_format = "%s=%s\n";
	size_t buf_size = 0;
	/* Calculate needed buffer for entries in list */
	for (libnvram_list_it it = libnvram_list_begin(list); it != libnvram_list_end(list); it = libnvram_list_next(it)) {
		const struct libnvram_entry* entry = libnvram_list_deref(it);
		/* legacy format only supports strings and all entries should be null-terminated */
		int r = snprintf(NULL, 0, row_format, entry->key, entry->value);
		if (r < 0)
			return -EINVAL;
		buf_size += r;
	}
	buf_size++; // include space for null-terminator
	uint8_t* buf = malloc(buf_size);
	if (buf == NULL) {
		pr_err("%s: failed allocating write buffer [%d]: %s\n", nvram->interface->section(nvram->interface_priv), ENOMEM, strerror(ENOMEM));
		return -ENOMEM;
	}

	/* Write buffer */
	size_t pos = 0;
	for (libnvram_list_it it = libnvram_list_begin(list); it != libnvram_list_end(list); it = libnvram_list_next(it)) {
		const struct libnvram_entry* entry = libnvram_list_deref(it);
		/* legacy format only supports strings and all entries should be null-terminated */
		int r = snprintf((char*) buf + pos, buf_size - pos, row_format, entry->key, entry->value);
		if (r < 0) {
			free(buf);
			return -EINVAL;
		}
		pos += r;
	}

	int r = nvram->interface->write(nvram->interface_priv, buf, buf_size - 1);
	free(buf);
	if (r)
		pr_err("%s: failed writing [%d]: %s\n", nvram->interface->section(nvram->interface_priv), -r, strerror(-r));
	return r;
}

static void legacy_close(struct nvram** nvram)
{
	if (nvram && *nvram) {
		struct nvram *pnvram = *nvram;
		if (pnvram->interface_priv)
			pnvram->interface->destroy(&pnvram->interface_priv);
		free(*nvram);
		*nvram = NULL;
	}
}

/* Exposed by nvram_format.c */
struct nvram_format nvram_legacy_format =
{
	.init = legacy_init,
	.commit = legacy_commit,
	.close = legacy_close,
};
