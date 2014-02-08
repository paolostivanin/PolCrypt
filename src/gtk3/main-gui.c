#include <gtk/gtk.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gcrypt.h>
#include "polcrypt.h"

static void file_dialog(struct info *);
static void is_enc(GtkWidget *, struct info *);
static void is_dec(GtkWidget *, struct info *);
static void is_hash(GtkWidget *, struct info *);
static void type_pwd_enc(struct info *);
static void type_pwd_dec(struct info *);
static int do_enc(struct info *);
static int do_dec(struct info *);
static void select_hash_type(struct info *);
static void activate (GtkApplication *, gpointer);
static void startup (GtkApplication *, gpointer);
static void quit (GSimpleAction *, GVariant *, gpointer);
static void about (GSimpleAction *, GVariant *, gpointer);

struct info s_Info;
const gchar *icon = "/usr/share/icons/hicolor/128x128/apps/polcrypt.png";

int main(int argc, char **argv){
	if(!gcry_check_version(GCRYPT_MIN_VER)){
		fputs("libgcrypt min version required: 1.5.0\n", stderr);
		return -1;
	}
	gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	
	GtkApplication *app;
	int status;
	GError *err = NULL;
	GdkPixbuf *logo = gdk_pixbuf_new_from_file(icon, &err);
	gtk_window_set_default_icon(logo);

	app = gtk_application_new ("org.gtk.polcrypt",G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	return status;
}

static void startup (GtkApplication *application, gpointer user_data __attribute__ ((unused)))
{
  static const GActionEntry actions[] = {
    { "about", about },
    { "quit", quit }
  };
  
  GMenu *menu;

  g_action_map_add_action_entries (G_ACTION_MAP (application), actions, G_N_ELEMENTS (actions), application);

  menu = g_menu_new ();
  g_menu_append (menu, "About", "app.about");
  g_menu_append (menu, "Quit",  "app.quit");
  gtk_application_set_app_menu (application, G_MENU_MODEL (menu));
  g_object_unref (menu);
}

static void activate (GtkApplication *app, gpointer user_data __attribute__ ((unused)))
{
	GtkWidget *butEn, *butDe, *butHa, *grid;
	GtkWidget *label;
	GError *err = NULL;
	
	s_Info.mainwin = gtk_application_window_new(app);
	gtk_window_set_application (GTK_WINDOW (s_Info.mainwin), GTK_APPLICATION (app));
	gtk_window_set_position(GTK_WINDOW(s_Info.mainwin), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(s_Info.mainwin), "PolCrypt");
	gtk_window_set_resizable(GTK_WINDOW(s_Info.mainwin), FALSE);
	gtk_window_set_icon_from_file(GTK_WINDOW(s_Info.mainwin), icon, &err);
	gtk_container_set_border_width(GTK_CONTAINER(s_Info.mainwin), 10);
	
	const gchar *str = "Welcome to PolCrypt (v2.0-alpha)";
	label = gtk_label_new(str);
	char *markup;
	markup = g_markup_printf_escaped ("<span foreground=\"black\" size=\"x-large\"><b>%s</b></span>", str); // font grassetto e large
	gtk_label_set_markup (GTK_LABEL (label), markup);
	
	butEn = gtk_button_new_with_label("Encrypt File");
	butDe = gtk_button_new_with_label("Decrypt File");
	butHa = gtk_button_new_with_label("Compute Hash");
	g_signal_connect(butEn, "clicked", G_CALLBACK (is_enc), &s_Info);
	g_signal_connect(butDe, "clicked", G_CALLBACK (is_dec), &s_Info);
	g_signal_connect(butHa, "clicked", G_CALLBACK (is_hash), &s_Info);
	
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(s_Info.mainwin), grid);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
	
	//numero colonna, numero riga, colonne da occupare, righe da occupare. Colonne e righe sono aggiunte automaticamente
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 5, 1);
	g_object_set (label, "margin-bottom", 12, NULL);
	gtk_grid_attach(GTK_GRID(grid), butEn, 1, 1, 3, 1);
	gtk_grid_attach(GTK_GRID(grid), butDe, 1, 2, 3, 1);
	gtk_grid_attach(GTK_GRID(grid), butHa, 1, 3, 3, 1);

	gtk_widget_show_all(s_Info.mainwin);
}

static void is_enc(GtkWidget *ignored __attribute__ ((unused)), struct info *s_Info){
	s_Info->mode = 1;
	file_dialog(s_Info);
}

static void is_dec(GtkWidget *ignored __attribute__ ((unused)), struct info *s_Info){
	s_Info->mode = 2;
	file_dialog(s_Info);
}

static void is_hash(GtkWidget *ignored __attribute__ ((unused)), struct info *s_Info){
	s_Info->mode = 3;
	file_dialog(s_Info);
}

static void file_dialog(struct info *s_Info){
	s_Info->file_dialog =  gtk_file_chooser_dialog_new("Choose File", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, ("_Cancel"), GTK_RESPONSE_CANCEL, ("_Ok"), GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run (GTK_DIALOG (s_Info->file_dialog)) == GTK_RESPONSE_ACCEPT){
		s_Info->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (s_Info->file_dialog));
		if(s_Info->mode == 1){
			type_pwd_enc(s_Info);
			g_free (s_Info->filename);
		}
		else if(s_Info->mode == 2){
			type_pwd_dec(s_Info);
			g_free (s_Info->filename);
		}
		else if(s_Info->mode == 3){
			select_hash_type(s_Info);
			g_free (s_Info->filename);
		}
	}
	gtk_widget_destroy (s_Info->file_dialog);
}

static void type_pwd_enc(struct info *s_TypePwd){
	gtk_widget_hide(GTK_WIDGET(s_TypePwd->file_dialog));
	GtkWidget *content_area, *grid2, *label, *labelAgain;
   	s_TypePwd->dialog = gtk_dialog_new_with_buttons ("Password", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Quit", GTK_RESPONSE_CLOSE, "_Ok", GTK_RESPONSE_OK, NULL);
   	content_area = gtk_dialog_get_content_area (GTK_DIALOG (s_TypePwd->dialog));
   	
   	label = gtk_label_new("Type password");
   	labelAgain = gtk_label_new("Retype password");
   	s_TypePwd->pwdEntry = gtk_entry_new();
   	s_TypePwd->pwdReEntry = gtk_entry_new();
   	gtk_entry_set_visibility(GTK_ENTRY(s_TypePwd->pwdEntry), FALSE); //input nascosto
   	gtk_entry_set_visibility(GTK_ENTRY(s_TypePwd->pwdReEntry), FALSE);

   	gtk_widget_set_size_request(s_TypePwd->dialog, 150, 100); // richiedo una grandezza minima
   	
   	grid2 = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(grid2), TRUE); // righe stessa altezza
	gtk_grid_set_column_homogeneous(GTK_GRID(grid2), TRUE); // colonne stessa larghezza
	gtk_grid_set_row_spacing(GTK_GRID(grid2), 5); // spazio fra le righe
	
	gtk_grid_attach(GTK_GRID(grid2), label, 0, 0, 3, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_TypePwd->pwdEntry, 0, 1, 3, 1);
	gtk_grid_attach(GTK_GRID(grid2), labelAgain, 0, 2, 3, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_TypePwd->pwdReEntry, 0, 3, 3, 1);		

   	/* Add the grid, and show everything we've added to the dialog */
   	gtk_container_add (GTK_CONTAINER (content_area), grid2);
   	gtk_widget_show_all (s_TypePwd->dialog);
   	
   	s_TypePwd->isSignalActivate = 0;
   	g_signal_connect_swapped(G_OBJECT(s_TypePwd->pwdReEntry), "activate", G_CALLBACK(do_enc), s_TypePwd);
   	gint result = gtk_dialog_run(GTK_DIALOG(s_TypePwd->dialog));
	switch(result){
		case GTK_RESPONSE_OK:
			s_TypePwd->isSignalActivate = -1;
			do_enc(s_TypePwd);
			gtk_widget_destroy(s_TypePwd->dialog);
			break;
		case GTK_RESPONSE_CLOSE:
			g_signal_connect_swapped (s_TypePwd->dialog, "response", G_CALLBACK(gtk_widget_destroy), s_TypePwd->dialog);
			gtk_widget_destroy (s_TypePwd->dialog);	
			break;
	}
}

static void type_pwd_dec(struct info *s_TypePwdDec){
	gtk_widget_hide(GTK_WIDGET(s_TypePwdDec->file_dialog));
	GtkWidget *content_area, *grid2, *label;
   	s_TypePwdDec->dialog = gtk_dialog_new_with_buttons ("Password", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Quit", GTK_RESPONSE_CLOSE, "_Ok", GTK_RESPONSE_OK, NULL);
   	content_area = gtk_dialog_get_content_area (GTK_DIALOG (s_TypePwdDec->dialog));
   	
   	label = gtk_label_new("Type password");
   	s_TypePwdDec->pwdEntry = gtk_entry_new();
   	gtk_entry_set_visibility(GTK_ENTRY(s_TypePwdDec->pwdEntry), FALSE); //input nascosto

   	gtk_widget_set_size_request(s_TypePwdDec->dialog, 150, 100); // richiedo una grandezza minima
   	
   	grid2 = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(grid2), TRUE); // righe stessa altezza
	gtk_grid_set_column_homogeneous(GTK_GRID(grid2), TRUE); // colonne stessa larghezza
	gtk_grid_set_row_spacing(GTK_GRID(grid2), 5); // spazio fra le righe
	
	gtk_grid_attach(GTK_GRID(grid2), label, 0, 0, 3, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_TypePwdDec->pwdEntry, 0, 1, 3, 1);

   	/* Add the grid, and show everything we've added to the dialog */
   	gtk_container_add (GTK_CONTAINER (content_area), grid2);
   	gtk_widget_show_all (s_TypePwdDec->dialog);
   	
   	s_TypePwdDec->isSignalActivate = 0;
   	g_signal_connect_swapped(G_OBJECT(s_TypePwdDec->pwdEntry), "activate", G_CALLBACK(do_dec), s_TypePwdDec);
   	gint result = gtk_dialog_run(GTK_DIALOG(s_TypePwdDec->dialog));
	switch(result){
		case GTK_RESPONSE_OK:
			s_TypePwdDec->isSignalActivate = -1;
			do_dec(s_TypePwdDec);
			gtk_widget_destroy(s_TypePwdDec->dialog);
			break;
		case GTK_RESPONSE_CLOSE:
			g_signal_connect_swapped (s_TypePwdDec->dialog, "response", G_CALLBACK(gtk_widget_destroy), s_TypePwdDec->dialog);
			gtk_widget_destroy (s_TypePwdDec->dialog);	
			break;
	}
}

static int do_enc(struct info *s_InfoCheckPwd){
	const gchar *pw1 = gtk_entry_get_text(GTK_ENTRY(s_InfoCheckPwd->pwdEntry));
	const gchar *pw2 = gtk_entry_get_text(GTK_ENTRY(s_InfoCheckPwd->pwdReEntry));
	if(g_strcmp0(pw1, pw2) != 0){
		g_print("pwd diverse\n");
		return -1;
	}
	encrypt_file_gui(s_InfoCheckPwd);
	if(s_InfoCheckPwd->isSignalActivate == 0) gtk_widget_destroy (GTK_WIDGET(s_InfoCheckPwd->dialog));
	return 0;
}

static int do_dec(struct info *s_InfoDecPwd){
	decrypt_file_gui(s_InfoDecPwd);
	if(s_InfoDecPwd->isSignalActivate == 0) gtk_widget_destroy (GTK_WIDGET(s_InfoDecPwd->dialog));
	return 0;
}

static void select_hash_type(struct info *s_InfoHash){
	gtk_widget_hide(GTK_WIDGET(s_InfoHash->file_dialog));
	struct hashes s_HashType;
	GtkWidget *content_area, *grid2;
   	s_InfoHash->dialog = gtk_dialog_new_with_buttons ("Select Hash", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Quit", GTK_RESPONSE_CLOSE, NULL);
   	content_area = gtk_dialog_get_content_area (GTK_DIALOG (s_InfoHash->dialog));
   	
   	s_HashType.checkMD5 = gtk_check_button_new_with_label("MD5");
   	s_HashType.checkS1 = gtk_check_button_new_with_label("SHA-1");
   	s_HashType.checkS256 = gtk_check_button_new_with_label("SHA-256");
   	s_HashType.checkS512 = gtk_check_button_new_with_label("SHA-512");
   	s_HashType.checkWhir = gtk_check_button_new_with_label("Whirlpool");
   	s_HashType.checkRMD = gtk_check_button_new_with_label("RMD160");
   	
   	s_HashType.entryMD5 = gtk_entry_new();
   	s_HashType.entryS1 = gtk_entry_new();
   	s_HashType.entryS256 = gtk_entry_new();
   	s_HashType.entryS512 = gtk_entry_new();
   	s_HashType.entryWhir = gtk_entry_new();
   	s_HashType.entryRMD = gtk_entry_new();
   	
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryMD5), FALSE);
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryS1), FALSE);
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryS256), FALSE);
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryS512), FALSE);
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryWhir), FALSE);
   	gtk_editable_set_editable(GTK_EDITABLE(s_HashType.entryRMD), FALSE);

   	gtk_widget_set_size_request(s_InfoHash->dialog, 250, 150); // richiedo una grandezza minima
   	
   	grid2 = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(grid2), TRUE); // righe stessa altezza
	gtk_grid_set_column_homogeneous(GTK_GRID(grid2), TRUE); // colonne stessa larghezza
	gtk_grid_set_row_spacing(GTK_GRID(grid2), 5); // spazio fra le righe
	
	// numero colonna, numero riga, colonne da occupare, righe da occupare
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkMD5, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryMD5, 2, 0, 6, 1);
	
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkS1, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryS1, 2, 1, 6, 1);
	
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkS256, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryS256, 2, 2, 6, 1);
	
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkS512, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryS512, 2, 3, 6, 1);
	
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkWhir, 0, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryWhir, 2, 4, 6, 1);
	
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.checkRMD, 0, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid2), s_HashType.entryRMD, 2, 5, 6, 1);

   	/* Add the grid, and show everything we've added to the dialog */
   	gtk_container_add (GTK_CONTAINER (content_area), grid2);
   	gtk_widget_show_all (s_InfoHash->dialog);
   	
   	s_HashType.filename = malloc(strlen(s_InfoHash->filename)+1);
   	strcpy(s_HashType.filename, s_InfoHash->filename);
   	
   	g_signal_connect_swapped(s_HashType.checkMD5, "clicked", G_CALLBACK(compute_md5), &s_HashType);
   	g_signal_connect_swapped(s_HashType.checkS1, "clicked", G_CALLBACK(compute_sha1), &s_HashType);
   	g_signal_connect_swapped(s_HashType.checkS256, "clicked", G_CALLBACK(compute_sha256), &s_HashType);
   	g_signal_connect_swapped(s_HashType.checkS512, "clicked", G_CALLBACK(compute_sha512), &s_HashType);
   	g_signal_connect_swapped(s_HashType.checkWhir, "clicked", G_CALLBACK(compute_whirlpool), &s_HashType);
   	g_signal_connect_swapped(s_HashType.checkRMD, "clicked", G_CALLBACK(compute_rmd160), &s_HashType);

   	gint result = gtk_dialog_run(GTK_DIALOG(s_InfoHash->dialog));
	switch(result){
		case GTK_RESPONSE_CLOSE:
			g_signal_connect_swapped (s_InfoHash->dialog, "response", G_CALLBACK(gtk_widget_destroy), s_InfoHash->dialog);
			gtk_widget_destroy(s_InfoHash->dialog);
			break;
	}
	free(s_HashType.filename);
}

static void about (GSimpleAction *action __attribute__ ((unused)), GVariant *parameter __attribute__ ((unused)), gpointer user_data __attribute__ ((unused)))
{
        const gchar *authors[] = /* Qui definisco gli autori*/
        {
                "Paolo Stivanin <info@paolostivanin.com>",
                NULL,
        };
        
        GError *error = NULL;
        GdkPixbuf *logo_about = gdk_pixbuf_new_from_file_at_size(icon, 64, 64, &error);
        
        GtkWidget *a_dialog = gtk_about_dialog_new ();
        gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (a_dialog), "PolCrypt");
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(a_dialog), logo_about);
        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (a_dialog), "2.0-alpha");
        gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (a_dialog), "Copyright (C) 2014");
        gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (a_dialog), "With this software you can encrypt and decrypt file with AES-256 CBC using HMAC-SHA512 for message authentication or you can compute various type of hashes");
        gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(a_dialog),
"This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License along with this program.\n"
"If not, see http://www.gnu.org/licenses\n"
"\n"
"PolCrypt is Copyright (C) 2014 by Paolo Stivanin.\n");
        gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(a_dialog), TRUE);
        gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (a_dialog), "https://github.com/polslinux/PolCrypt");
        gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (a_dialog), authors);

        gtk_dialog_run(GTK_DIALOG (a_dialog)); /* Avvio il dialog a_dialog */
        gtk_widget_destroy(a_dialog); /* Alla pressione del pulsante chiudi il widget viene chiuso */
}

static void quit (GSimpleAction *action __attribute__ ((unused)), GVariant *parameter __attribute__ ((unused)), gpointer user_data __attribute__ ((unused)))
{
   GApplication *application = user_data;
   g_application_quit (application);
}
