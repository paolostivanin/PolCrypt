/* Sviluppatore: Paolo Stivanin
 * Copyright: 2013
 * Licenza: GNU GPL v3 <http://www.gnu.org/licenses/gpl-3.0.html>
 * Sito web: <https://github.com/polslinux/PolCrypt>
 */

#ifndef POLCRYPT_H_INCLUDED
#define POLCRYPT_H_INCLUDED

int check_pkcs7(unsigned char *, unsigned char *);
int encrypt_file(void);
int decrypt_file(void);

struct metadata{
	char header[32];
	unsigned char salt[32];
	unsigned char iv[16];
};
extern struct metadata s_mdata;

#endif