#include <gtk/gtk.h>
#include <glib.h>
#include <gcrypt.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <libintl.h>
#include "polcrypt.h"

guchar *calculate_hmac(const gchar *, const guchar *key, size_t, gint);
gint delete_input_file(const gchar *, size_t);
static void show_error(struct widget_t *, const gchar *);

void *encrypt_file_gui(struct widget_t *WidgetMain){
	struct metadata_t Metadata;
	gint algo = -1,fd, number_of_block, block_done = 0, retcode = 0, counterForGoto = 0;
	guchar hex[15] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F}, plain_text[16];
	guchar *derived_key = NULL, *crypto_key = NULL, *mac_key = NULL, *encBuffer = NULL;
	gchar *inputKey = NULL;
	gfloat result_of_division_by_16, fsize_float;
	off_t fsize = 0;
	size_t blkLength, keyLength, txtLenght = 16, retval = 0, i;
	
	gchar *filename = g_strdup(WidgetMain->filename);
	
	struct stat fileStat;
	gcry_cipher_hd_t hd;
	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widget.r1))){
		algo = gcry_cipher_map_name("aes256");
		Metadata.algo_type = 0;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widget.r2))){
		algo = gcry_cipher_map_name("serpent256");
		Metadata.algo_type = 1;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widget.r3))){
		algo = gcry_cipher_map_name("twofish");
		Metadata.algo_type = 2;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widget.r4))){
		algo = gcry_cipher_map_name("camellia256");
		Metadata.algo_type = 3;
	}

	const gchar *inputWidKey = gtk_entry_get_text(GTK_ENTRY(WidgetMain->pwdEntry));
	size_t len = strlen(inputWidKey);
	inputKey = gcry_malloc_secure(len+1);
	strncpy(inputKey, inputWidKey, len);
	inputKey[len] = '\0';

	gchar *outFilename;
	size_t lenFilename = strlen(filename);
	outFilename = g_malloc(lenFilename+5); // ".enc\0" are 5 char
	strncpy(outFilename, filename, lenFilename);
	memcpy(outFilename+lenFilename, ".enc", 4);
	outFilename[lenFilename+4] = '\0';

	blkLength = gcry_cipher_get_algo_blklen(algo);
	keyLength = gcry_cipher_get_algo_keylen(algo);	
		
	encBuffer = gcry_malloc(txtLenght);

	gcry_create_nonce(Metadata.iv, 16);
	gcry_create_nonce(Metadata.salt, 32);
	
	fd = open(filename, O_RDONLY | O_NOFOLLOW);
	if(fd == -1){
		show_error(WidgetMain, strerror(errno));
		gcry_free(inputKey);
		return;
	}
  	if(fstat(fd, &fileStat) < 0){
		show_error(WidgetMain, strerror(errno));
		gcry_free(inputKey);
    	close(fd);
    	return;
  	}
  	fsize = fileStat.st_size;
  	close(fd);

	fsize_float = (float)fsize;
	result_of_division_by_16 = fsize_float / 16;
	number_of_block = (int)result_of_division_by_16;
	if(result_of_division_by_16 > number_of_block) number_of_block += 1;
	
	FILE *fp = fopen(filename, "r");
	FILE *fpout = fopen(outFilename, "w");
	if(fp == NULL){
		show_error(WidgetMain, strerror(errno));
		gcry_free(inputKey);
		return;
	}
	if(fpout == NULL){
		show_error(WidgetMain, strerror(errno));
		gcry_free(inputKey);
		return;
	}
	
	gcry_cipher_open(&hd, algo, GCRY_CIPHER_MODE_CBC, 0);
	
	if((derived_key = gcry_malloc_secure(64)) == NULL){
		fprintf(stderr, _("encrypt_file: gcry_malloc_secure failed (derived)\n"));
		gcry_free(inputKey);
		return;
	}
	
	if((crypto_key = gcry_malloc_secure(32)) == NULL){
		fprintf(stderr, _("encrypt_file: gcry_malloc_secure failed (crypto)\n"));
		gcry_free(inputKey);
		gcry_free(derived_key);
		return;
	}
	
	if((mac_key = gcry_malloc_secure(32)) == NULL){
		fprintf(stderr, _("encrypt_file: gcry_malloc_secure failed (mac)\n"));
		gcry_free(crypto_key);
		gcry_free(inputKey);
		gcry_free(derived_key);
		return;
	}

	tryAgainDerive:
	if(gcry_kdf_derive (inputKey, len+1, GCRY_KDF_PBKDF2, GCRY_MD_SHA512, Metadata.salt, 32, 150000, 64, derived_key) != 0){
		if(counterForGoto == 3){
			fprintf(stderr, _("encrypt_file: Key derivation error\n"));
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

	fseek(fp, 0, SEEK_SET);

	fwrite(&Metadata, sizeof(struct metadata_t), 1, fpout);

	while(number_of_block > block_done){
		memset(plain_text, 0, sizeof(plain_text));
		retval = fread(plain_text, 1, 16, fp);
		if(!retval) break;
		if(retval < 16){
			for(i=retval; i<16; i++){
				if(retval == 1) plain_text[i] = hex[14];
				if(retval == 2) plain_text[i] = hex[13];
				if(retval == 3) plain_text[i] = hex[12];
				if(retval == 4) plain_text[i] = hex[11];
				if(retval == 5) plain_text[i] = hex[10];
				if(retval == 6) plain_text[i] = hex[9];
				if(retval == 7) plain_text[i] = hex[8];
				if(retval == 8) plain_text[i] = hex[7];
				if(retval == 9) plain_text[i] = hex[6];
				if(retval == 10) plain_text[i] = hex[5];
				if(retval == 11) plain_text[i] = hex[4];
				if(retval == 12) plain_text[i] = hex[3];
				if(retval == 13) plain_text[i] = hex[2];
				if(retval == 14) plain_text[i] = hex[1];
				if(retval == 15) plain_text[i] = hex[0];
			}
		}
		gcry_cipher_encrypt(hd, encBuffer, txtLenght, plain_text, txtLenght);
		
		fwrite(encBuffer, 1, 16, fpout);
		block_done++;
	}
	fclose(fpout);
	fclose(fp);

	guchar *hmac = calculate_hmac(outFilename, mac_key, keyLength, 0);
	if(hmac == (guchar *)1){
		show_error(WidgetMain, _("Error during HMAC calculation"));
		gcry_free(derived_key);
		gcry_free(crypto_key);
		gcry_free(mac_key);
		gcry_free(inputKey);
		return;
	}
	fpout = fopen(outFilename, "a");
	fwrite(hmac, 1, 64, fpout);
	free(hmac);
	
	retcode = delete_input_file(filename, fsize);
	if(retcode == -1)
		show_error(WidgetMain, _("Secure file deletion failed, overwrite it manually"));
	if(retcode == -2)
		show_error(WidgetMain, _("File unlink failed, remove it manually"));
		
	gcry_cipher_close(hd);
	gcry_free(derived_key);
	gcry_free(crypto_key);
	gcry_free(mac_key);
	gcry_free(encBuffer);
	gcry_free(inputKey);
	
	g_free(filename);
	g_free(outFilename);
	
	fclose(fpout);

	//return 0;
}

void show_error(struct widget_t *s_Error, const gchar *message){
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(GTK_WINDOW(s_Error->mainwin),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
