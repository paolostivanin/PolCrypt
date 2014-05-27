#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <libintl.h>
#include "polcrypt.h"

guchar *calculate_hmac(const gchar *, const guchar *key, size_t, gint);
gint check_pkcs7(guchar *, guchar *);
static void show_error(const gchar *);

void *decrypt_file_gui(struct widget_t *WidgetMain){
	gint algo = -1, fd, number_of_block, block_done = 0, number_of_pkcs7_byte, counterForGoto = 0;	
	guchar *derived_key = NULL, *crypto_key = NULL, *mac_key = NULL, *decBuffer = NULL;
	guchar hex[15] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F}, cipher_text[16], mac_of_file[64] ={0};
	gchar *inputKey = NULL;
	off_t fsize = 0;
	size_t blkLength = 0, keyLength = 0, txtLenght = 16, retval = 0, pwd_len = 0;
	glong current_file_offset, bytes_before_mac;
	FILE *fp, *fpout;
	
	struct metadata_t Metadata;
	struct stat fileStat;
	gcry_cipher_hd_t hd;
	
	gchar *filename = g_strdup(WidgetMain->filename);
	
	decBuffer = gcry_malloc(txtLenght);
	
	gchar *outFilename = NULL, *extBuf = NULL;
	size_t lenFilename = strlen(filename);
	extBuf = malloc(5);
	if(extBuf == NULL){
		fprintf(stderr, _("decrypt_file: error during memory allocation"));
		return;
	}
	memcpy(extBuf, (filename)+lenFilename-4, 4);
	extBuf[4] = '\0';
	if(strcmp(extBuf, ".enc") == 0){
		outFilename = g_malloc(lenFilename-3);
		strncpy(outFilename, filename, lenFilename-4);
		outFilename[lenFilename-4] = '\0';
		free(extBuf);
	}
	else{
		outFilename = g_malloc(lenFilename+5);
		strncpy(outFilename, filename, lenFilename);
		memcpy(outFilename+lenFilename, ".dec", 4);
		outFilename[lenFilename+4] = '\0';
		free(extBuf);
	}


	const gchar *inputWidKey = gtk_entry_get_text(GTK_ENTRY(WidgetMain->pwdEntry));
	pwd_len = strlen(inputWidKey);
	inputKey = gcry_malloc_secure(pwd_len+1);
	strncpy(inputKey, inputWidKey, pwd_len);
	inputKey[pwd_len] = '\0';

	fd = open(filename, O_RDONLY | O_NOFOLLOW);
	if(fd == -1){
		show_error( strerror(errno));
		return;
	}
  	if(fstat(fd, &fileStat) < 0){
  		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
    	close(fd);
    	gcry_free(inputKey);
    	return;
  	}
  	fsize = fileStat.st_size;
  	close(fd);
	
	number_of_block = (fsize - sizeof(struct metadata_t) - 64)/16;
	bytes_before_mac = (number_of_block*16)+sizeof(struct metadata_t);
	
	fp = fopen(filename, "r");
	if(fp == NULL){
		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
		gcry_free(inputKey);
		return;
	}
	if(fseek(fp, 0, SEEK_SET) == -1){
		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
		gcry_free(inputKey);
		return;
	}

	retval = fread(&Metadata, sizeof(struct metadata_t), 1, fp);
	if(retval != 1){
		fprintf(stderr, _("decrypt_file: cannot read file metadata_t\n"));
		gcry_free(inputKey);
		return;
	}
	
	if(Metadata.algo_type == 0){
		algo = gcry_cipher_map_name("aes256");
	}
	else if(Metadata.algo_type == 1){
		algo = gcry_cipher_map_name("serpent256");
	}
	else if(Metadata.algo_type == 2){
		algo = gcry_cipher_map_name("twofish");
		
	}
	else if(Metadata.algo_type == 3){
		algo = gcry_cipher_map_name("camellia256");
	}

	blkLength = gcry_cipher_get_algo_blklen(algo);
	keyLength = gcry_cipher_get_algo_keylen(algo);
	
	gcry_cipher_open(&hd, algo, GCRY_CIPHER_MODE_CBC, 0);
	
	if((derived_key = gcry_malloc_secure(64)) == NULL){
		fprintf(stderr, _("decrypt_file: gcry_malloc_secure failed (derived)\n"));
		gcry_free(inputKey);
		return;
	}
	
	if((crypto_key = gcry_malloc_secure(32)) == NULL){
		fprintf(stderr, _("decrypt_file: gcry_malloc_secure failed (crypto)\n"));
		gcry_free(inputKey);
		gcry_free(derived_key);
		return;
	}
	
	if((mac_key = gcry_malloc_secure(32)) == NULL){
		fprintf(stderr, _("decrypt_file: gcry_malloc_secure failed (mac)\n"));
		gcry_free(crypto_key);
		gcry_free(inputKey);
		gcry_free(derived_key);
		return;
	}

	tryAgainDerive:
	if(gcry_kdf_derive (inputKey, pwd_len+1, GCRY_KDF_PBKDF2, GCRY_MD_SHA512, Metadata.salt, 32, 150000, 64, derived_key) != 0){
		if(counterForGoto == 3){
			fprintf(stderr, _("decrypt_file: Key derivation error\n"));
			gcry_free(derived_key);
			gcry_free(crypto_key);
			gcry_free(mac_key);
			gcry_free(inputKey);
			return;
		}
		counterForGoto += 1;
		goto tryAgainDerive;
	}
	memcpy(crypto_key, derived_key, 32);
	memcpy(mac_key, derived_key + 32, 32);
		
	gcry_cipher_setkey(hd, crypto_key, keyLength);
	gcry_cipher_setiv(hd, Metadata.iv, blkLength);

	if((current_file_offset = ftell(fp)) == -1){
		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;		
	}
	if(fseek(fp, bytes_before_mac, SEEK_SET) == -1){
		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;		
	}
	if(fread(mac_of_file, 1, 64, fp) != 64){
		fprintf(stderr, _("decrypt_file: fread mac error\n"));;
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;
	}
	guchar *hmac = calculate_hmac(filename, mac_key, keyLength, 1);
	if(hmac == (guchar *)1){
		show_error( _("Error during HMAC calculation\n"));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;
	}
	if(memcmp(mac_of_file, hmac, 64) != 0){
		//show_error(_("HMAC doesn't match. This is caused by\n1) wrong password\nor\n2) corrupted file\n"));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		g_free(filename);
		g_free(outFilename);
		g_thread_exit((gpointer)-15);
	}
	free(hmac);
	if(fseek(fp, current_file_offset, SEEK_SET) == -1){
		fprintf(stderr, "decrypt_file: %s\n", strerror(errno));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;		
	}

	fpout = fopen(outFilename, "w");
	if(fpout == NULL){
		show_error( strerror(errno));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;
	}
	
	while(number_of_block > block_done){
		memset(cipher_text, 0, sizeof(cipher_text));
		retval = fread(cipher_text, 1, 16, fp);
		if(!retval) break;
		
		gcry_cipher_decrypt(hd, decBuffer, txtLenght, cipher_text, txtLenght);
		if(block_done == (number_of_block-1)){
			number_of_pkcs7_byte = check_pkcs7(decBuffer, hex);
			fwrite(decBuffer, 1, number_of_pkcs7_byte, fpout);	
			goto end;
		}
		
		fwrite(decBuffer, 1, 16, fpout);
		block_done++;
	}
	
	end:
	
	gcry_cipher_close(hd);
	gcry_free(inputKey);
	gcry_free(derived_key);
	gcry_free(crypto_key);
	gcry_free(mac_key);
	gcry_free(decBuffer);
	g_free(outFilename);
	g_free(filename);
	
	fclose(fp);
	fclose(fpout);
	
	g_thread_exit((gpointer)0);
}

static void show_error(const gchar *message){
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
