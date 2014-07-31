#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gcrypt.h>
#include <errno.h>
#include <fcntl.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <libintl.h>
#include <libnotify/notify.h>
#include <nettle/gosthash94.h>
#include <sys/mman.h>
#include "polcrypt.h"


static goffset get_file_size (const gchar *);


void
compute_gost94 (struct hashWidget_t *HashWidget)
{
   	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (HashWidget->hashCheck[7])))
   	{
		gtk_entry_set_text (GTK_ENTRY (HashWidget->hashEntry[7]), "");
		goto fine;
	}
	else if (g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (HashWidget->hashEntry[7])), -1) == 32)
		goto fine;

	struct gosthash94_ctx ctx;
	guint8 digest[GOSTHASH94_DIGEST_SIZE];
	gint fd, i, retVal;
	goffset fileSize, doneSize = 0, diff = 0, offset = 0;
	gchar hash[33];
	guint8 *fAddr;
	GError *err = NULL;
	
	fd = g_open (HashWidget->filename, O_RDONLY | O_NOFOLLOW);
	if (fd == -1)
	{
		g_printerr ("gost94: %s\n", g_strerror (errno));
		return;
	}
	
  	fileSize = get_file_size (HashWidget->filename);
       
	gosthash94_init (&ctx);

	if (fileSize < BUF_FILE)
	{
		fAddr = mmap (NULL, fileSize, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
		if (fAddr == MAP_FAILED)
		{
			g_printerr ("gost94: %s\n", g_strerror (errno));
			return;
		}
		gosthash94_update (&ctx, fileSize, fAddr);
		retVal = munmap (fAddr, fileSize);
		if(retVal == -1)
		{
			g_printerr ("gost94: munmap error\n");
			return;
		}
		goto nowhile;
	}

	while (fileSize > doneSize)
	{
		fAddr = mmap (NULL, BUF_FILE, PROT_READ, MAP_FILE | MAP_SHARED, fd, offset);
		if (fAddr == MAP_FAILED)
		{
			g_printerr ("gost94: %s\n", g_strerror (errno));
			return;
		}
		gosthash94_update (&ctx, BUF_FILE, fAddr);
		doneSize += BUF_FILE;
		diff = fileSize - doneSize;
		offset += BUF_FILE;
		if (diff < BUF_FILE && diff > 0)
		{
			fAddr = mmap (NULL, diff, PROT_READ, MAP_FILE | MAP_SHARED, fd, offset);
			if (fAddr == MAP_FAILED)
			{
				g_printerr ("gost94: %s\n", g_strerror (errno));
				return;
			}
			gosthash94_update (&ctx, diff, fAddr);
			retVal = munmap (fAddr, diff);
			if (retVal == -1)
			{
				g_printerr ("gost94: munmap error\n");
				return;
			}
			break;
		}
		retVal = munmap (fAddr, BUF_FILE);
		if (retVal == -1)
		{
			g_printerr ("gost94: munmap error\n");
			return;
		}
	}
	
	nowhile:	
	gosthash94_digest (&ctx, GOSTHASH94_DIGEST_SIZE, digest);
 	for (i=0; i<16; i++)
		g_sprintf (hash+(i*2), "%02x", digest[i]);

 	hash[32] = '\0';
 	gtk_entry_set_text (GTK_ENTRY (HashWidget->hashEntry[7]), hash);
 	
	g_close(fd, &err);
	
	fine:
	return;
}


static goffset
get_file_size (const gchar *filePath)
{
	GFileInfo *info;
	GFile *file;
	GError *error = NULL;
	const gchar *attributes = "standard::*";
	GFileQueryInfoFlags flags = G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS;
	GCancellable *cancellable = NULL;
	goffset fileSize;

	file = g_file_new_for_path (filePath);
	info = g_file_query_info (file, attributes, flags, cancellable, &error);
	fileSize = g_file_info_get_size (info);

	g_object_unref(file);
	
	return fileSize;
}
