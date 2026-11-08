#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"

#include "bambu_tag.h"

// Same derivation as deriveKeys.py from the RFID Tag Guide: HKDF-SHA256 with the
// UID as input key material, this value as salt and "RFID-A\0" as info.
static const uint8_t SALT[] = {0x9a, 0x75, 0x9c, 0xf2, 0xc4, 0xf7, 0xca, 0xff,
                               0x22, 0x2c, 0xb9, 0x76, 0x9b, 0x41, 0xbc, 0x96};
static const uint8_t INFO[] = "RFID-A"; // sizeof includes the trailing NUL, as required

bool bambu_derive_keys(const uint8_t uid[BAMBU_UID_LEN],
                       uint8_t keys[BAMBU_SECTOR_COUNT][BAMBU_KEY_LEN])
{
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    return mbedtls_hkdf(md, SALT, sizeof(SALT), uid, BAMBU_UID_LEN, INFO, sizeof(INFO),
                        &keys[0][0], BAMBU_SECTOR_COUNT * BAMBU_KEY_LEN) == 0;
}
