#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nvram_interface.h"

#define xstr(a) str(a)
#define str(a) #a

#define NVRAM_ENV_FILE_USER_A "NVRAM_FILE_USER_A"
#define NVRAM_ENV_FILE_USER_B "NVRAM_FILE_USER_B"
#define NVRAM_ENV_FILE_SYSTEM_A "NVRAM_FILE_SYSTEM_A"
#define NVRAM_ENV_FILE_SYSTEM_B "NVRAM_FILE_SYSTEM_B"
#define NVRAM_ENV_MTD_USER_A "NVRAM_MTD_USER_A"
#define NVRAM_ENV_MTD_USER_B "NVRAM_MTD_USER_B"
#define NVRAM_ENV_MTD_SYSTEM_A "NVRAM_MTD_SYSTEM_A"
#define NVRAM_ENV_MTD_SYSTEM_B "NVRAM_MTD_SYSTEM_B"
#define NVRAM_ENV_EFI_USER_A "NVRAM_EFI_USER_A"
#define NVRAM_ENV_EFI_USER_B "NVRAM_EFI_USER_B"
#define NVRAM_ENV_EFI_SYSTEM_A "NVRAM_EFI_SYSTEM_A"
#define NVRAM_ENV_EFI_SYSTEM_B "NVRAM_EFI_SYSTEM_B"

#if NVRAM_INTERFACE_FILE > 0
/* nvram_interface_file.c*/
extern struct nvram_interface nvram_file_interface;
#endif
#if NVRAM_INTERFACE_MTD > 0
/* nvram_interface_mtd.c*/
extern struct nvram_interface nvram_mtd_interface;
#endif
#if NVRAM_INTERFACE_EFI > 0
/* nvram_interface_efi.c*/
extern struct nvram_interface nvram_efi_interface;
#endif

static const char* get_env_str(const char* env, const char* def)
{
	const char *str = getenv(env);
	if (str)
		return str;
	return def;
}

struct nvram_interface* nvram_get_interface(const char* interface_name)
{
#if NVRAM_INTERFACE_FILE > 0
	if (!strcmp("file", interface_name))
		return &nvram_file_interface;
#endif
#if NVRAM_INTERFACE_MTD > 0
	if (!strcmp("mtd", interface_name))
		return &nvram_mtd_interface;
#endif
#if NVRAM_INTERFACE_EFI > 0
	if (!strcmp("efi", interface_name))
		return &nvram_efi_interface;
#endif
	return NULL;
}

const char* nvram_get_interface_section(const char* interface_name, enum section section)
{
#if NVRAM_INTERFACE_FILE > 0
	if (!strcmp("file", interface_name)) {
		switch (section) {
		case SYSTEM_A:
			return get_env_str(NVRAM_ENV_FILE_SYSTEM_A, xstr(NVRAM_FILE_SYSTEM_A));
		case SYSTEM_B:
			return get_env_str(NVRAM_ENV_FILE_SYSTEM_B, xstr(NVRAM_FILE_SYSTEM_B));
		case USER_A:
			return get_env_str(NVRAM_ENV_FILE_USER_A, xstr(NVRAM_FILE_USER_A));
		case USER_B:
			return get_env_str(NVRAM_ENV_FILE_USER_B, xstr(NVRAM_FILE_USER_B));
		}
	}
#endif
#if NVRAM_INTERFACE_MTD > 0
	if (!strcmp("mtd", interface_name)) {
		switch (section) {
		case SYSTEM_A:
			return get_env_str(NVRAM_ENV_MTD_SYSTEM_A, xstr(NVRAM_MTD_SYSTEM_A));
		case SYSTEM_B:
			return get_env_str(NVRAM_ENV_MTD_SYSTEM_B, xstr(NVRAM_MTD_SYSTEM_B));
		case USER_A:
			return get_env_str(NVRAM_ENV_MTD_USER_A, xstr(NVRAM_MTD_USER_A));
		case USER_B:
			return get_env_str(NVRAM_ENV_MTD_USER_B, xstr(NVRAM_MTD_USER_B));
		}
	}
#endif
#if NVRAM_INTERFACE_EFI > 0
	if (!strcmp("efi", interface_name)) {
		switch (section) {
		case SYSTEM_A:
			return get_env_str(NVRAM_ENV_EFI_SYSTEM_A, xstr(NVRAM_EFI_SYSTEM_A));
		case SYSTEM_B:
			return get_env_str(NVRAM_ENV_EFI_SYSTEM_B, xstr(NVRAM_EFI_SYSTEM_B));
		case USER_A:
			return get_env_str(NVRAM_ENV_EFI_USER_A, xstr(NVRAM_EFI_USER_A));
		case USER_B:
			return get_env_str(NVRAM_ENV_EFI_USER_B, xstr(NVRAM_EFI_USER_B));
		}
	}
#endif
	return NULL;
}