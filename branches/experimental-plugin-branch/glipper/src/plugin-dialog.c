#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include "plugin-dialog.h"
#include "plugin.h"
#include "utils/glipper-i18n.h"

#define GLADE_XML_FILE "glipper-plugins.glade"

GtkWidget* pluginWin;
GtkWidget* pluginList;
GtkWidget* startButton;
GtkWidget* refreshButton;
GtkWidget* closeButton;

GtkListStore* pluginStore;

enum {
FILE_COLUMN,
NAME_COLUMN,
DESCRIPTION_COLUMN,
N_COLUMNS
};

void addPluginToList(char* plugin)
{
	plugin_info* info = malloc(sizeof(plugin_info));
	if (get_plugin_info(plugin, info))
	{
		GtkTreeIter iter;
		gtk_list_store_append(pluginStore, &iter);
		gtk_list_store_set(pluginStore, &iter,
			FILE_COLUMN, plugin,
			NAME_COLUMN, info->name,
			DESCRIPTION_COLUMN, info->descr,
			-1);
	}
	else
		g_print("couldn't retrieve informations for plugin %s!\n", plugin);
	free(info);
}

void addDirToList(char* dir)
{
	GDir* plugindir = g_dir_open(dir, 0, NULL);
	if (plugindir == NULL)
		return;
	char* plugin;
	while (plugin = g_dir_read_name(plugindir))
		if (strstr(plugin, ".py"))
		{
			char* tmp = malloc(strlen(plugin)-2);
			strncpy(tmp, plugin, strlen(plugin)-3);
			tmp[strlen(plugin)-3] = '\0';
			addPluginToList(tmp);
			free(tmp);
		}
	g_dir_close(plugindir);
	//Maybe watch for errors?
}

void refreshPluginList()
{
	gtk_list_store_clear(pluginStore);
	addDirToList(PLUGINDIR);
	char* searchModule = g_build_filename(g_get_home_dir(), ".glipper/plugins", NULL);
	addDirToList(searchModule);
	free(searchModule);
}

////////////////////Button signals/////////////////////////////////////////

on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
        GtkTreeIter iter;
        char* file;
	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection(GTK_TREE_VIEW(pluginList)), &pluginStore, &iter))
        {
                gtk_tree_model_get(GTK_TREE_MODEL(pluginStore), &iter, FILE_COLUMN, &file, -1);
                g_print("You want to start plugin %s\n", file);
                free(file);
        }

}

on_refreshButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	refreshPluginList();
}

on_closeButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	//Maybe save something?
	gtk_widget_destroy(pluginWin);
}

void showPluginDialog(gpointer data)
{
	char* glade_file;
	GladeXML* gladeWindow;

	//Load interface from glade file
	glade_file = g_build_filename(GLADEDIR, GLADE_XML_FILE, NULL);

	gladeWindow = glade_xml_new(glade_file, "plugin-dialog", NULL);

	//In case we cannot load glade file
	if (gladeWindow == NULL)
	{
		errorDialog(_("Could not load the preferences interface"), glade_file);
		g_free (glade_file);
		return;
	}

	g_free (glade_file);

	pluginWin = glade_xml_get_widget(gladeWindow, "plugin-dialog");
	pluginList = glade_xml_get_widget(gladeWindow, "pluginList");
	startButton = glade_xml_get_widget(gladeWindow, "startButton");
	refreshButton = glade_xml_get_widget(gladeWindow, "refreshButton");
	closeButton = glade_xml_get_widget(gladeWindow, "closeButton");

	//Connect signals to handlers
	g_signal_connect ((gpointer) startButton, "clicked",
		G_CALLBACK (on_startButton_clicked),
		NULL);
	g_signal_connect ((gpointer) refreshButton, "clicked",
		G_CALLBACK (on_refreshButton_clicked),
		NULL);
	g_signal_connect ((gpointer) closeButton, "clicked",
		G_CALLBACK (on_closeButton_clicked),
		NULL);

	//Set up the plugin list model + widget
	pluginStore = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pluginList), column);
	column = gtk_tree_view_column_new_with_attributes ("Description", renderer,
							   "text", DESCRIPTION_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pluginList), column);
	gtk_tree_view_set_model(GTK_TREE_VIEW (pluginList), GTK_TREE_MODEL (pluginStore));

	refreshPluginList();

	//Show plugin dialog
	gtk_widget_show_all(pluginWin);

	//free the glade data
	g_object_unref(gladeWindow);
}