#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "preferences.h"
#include "utils/glipper-i18n.h"

#define GLADE_XML_FILE "glipper-properties.glade"

static GConfClient* global_conf;

extern int hasChanged;
extern GSList* history;
extern void keyhandler(char *keystring, gpointer user_data);
extern GtkTooltips* toolTip; //The trayicon's tooltip
extern GtkWidget* eventbox; //The trayicon's eventbox

GtkWidget* historyLength;
GtkWidget* itemLength;
GtkWidget* primaryCheck;
GtkWidget* defaultCheck;
GtkWidget* markDefaultCheck;
GtkWidget* saveHistCheck;
GtkWidget* keyCombEntry;
GtkWidget* applyButton;
GtkWidget* prefWin;
GtkWidget* helpButton;
GtkWidget* closeButton;

//Callbacks needed to update the GUI and history after a key change

void
max_item_length_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	hasChanged = 1;
	GConfValue *value = gconf_entry_get_value (entry);
	gtk_spin_button_set_value((GtkSpinButton*)itemLength, gconf_value_get_int(value));
}

void
max_elements_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	hasChanged = 1;
	GConfValue *value = gconf_entry_get_value (entry);
	GSList* deleteElement = g_slist_nth(history, gconf_value_get_int(value)-1);
	if (deleteElement != NULL)
	{
		g_slist_free(deleteElement->next);
		deleteElement->next = NULL;
	}
	gtk_spin_button_set_value((GtkSpinButton*)historyLength, gconf_value_get_int(value));
}

void
use_primary_clipboard_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	GConfValue *value = gconf_entry_get_value(entry);
	gtk_toggle_button_set_active((GtkToggleButton*)primaryCheck, gconf_value_get_bool(value));
}
								
void
use_default_clipboard_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	GConfValue *value = gconf_entry_get_value(entry);
	gtk_toggle_button_set_active((GtkToggleButton*)defaultCheck, gconf_value_get_bool(value));
}
		
void
mark_default_entry_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	hasChanged = 1;
	GConfValue *value = gconf_entry_get_value(entry);
	gtk_toggle_button_set_active((GtkToggleButton*)markDefaultCheck, gconf_value_get_bool(value));
}

void
save_history_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	GConfValue *value = gconf_entry_get_value(entry);
	gtk_toggle_button_set_active((GtkToggleButton*)saveHistCheck, gconf_value_get_bool(value));
}

void
key_combination_key_changed_callback(GConfClient *client,
		    guint        cnxn_id,
		    GConfEntry  *entry,
		    gpointer     user_data)
{
	hasChanged = 1;
	GConfValue *value = gconf_entry_get_value(entry);
	keybinder_bind(gconf_value_get_string(value), keyhandler, NULL);
	gtk_tooltips_set_tip(toolTip, eventbox, g_strdup_printf(_("Glipper (%s)\nClipboardmanager"),
						gconf_value_get_string(value)), "Glipper");
	gtk_entry_set_text((GtkEntry*)keyCombEntry, gconf_value_get_string(value));
}

//Sets initial state of widgets
void setWidgets()
{
	gtk_spin_button_set_value((GtkSpinButton*)historyLength, gconf_client_get_int(global_conf, MAX_ELEMENTS_KEY, NULL));
	gtk_spin_button_set_value((GtkSpinButton*)itemLength, gconf_client_get_int(global_conf, MAX_ITEM_LENGTH_KEY, NULL));
	gtk_toggle_button_set_active((GtkToggleButton*)primaryCheck, gconf_client_get_bool(global_conf, USE_PRIMARY_CLIPBOARD_KEY, NULL));
	gtk_toggle_button_set_active((GtkToggleButton*)defaultCheck, gconf_client_get_bool(global_conf, USE_DEFAULT_CLIPBOARD_KEY, NULL));
	gtk_toggle_button_set_active((GtkToggleButton*)markDefaultCheck, gconf_client_get_bool(global_conf, MARK_DEFAULT_ENTRY_KEY, NULL));
	gtk_toggle_button_set_active((GtkToggleButton*)saveHistCheck, gconf_client_get_bool(global_conf, SAVE_HISTORY_KEY, NULL));
	gtk_entry_set_text((GtkEntry*)keyCombEntry, gconf_client_get_string(global_conf, KEY_COMBINATION_KEY, NULL));
	on_defaultCheck_toggled((GtkToggleButton*)defaultCheck,NULL);
}

//Every widget needs to update the key in the GConf database when he changes his status
void
on_itemLength_value_changed                   (GtkSpinButton       *spinButton,
                                        		gpointer         user_data)
{
	hasChanged = 1;
	gint value;
	value = gtk_spin_button_get_value (spinButton);
	gconf_client_set_int (global_conf, MAX_ITEM_LENGTH_KEY, value, NULL);
}

void
on_historyLength_value_changed                   (GtkSpinButton       *spinButton,
                                        		gpointer         user_data)
{
	hasChanged = 1;
	gint value;
	value = gtk_spin_button_get_value (spinButton);
	gconf_client_set_int (global_conf, MAX_ELEMENTS_KEY, value, NULL);
}

void
on_keyCombEntry_changed                   (GtkEntry       *entry,
                                        		gpointer         user_data)
{
	hasChanged = 1;
	const gchar* value = gtk_entry_get_text (entry);
	gconf_client_set_string (global_conf, KEY_COMBINATION_KEY, value, NULL);
}

//Set markDefaultCheck checkbox (ctrl+c in blue) active or not
void
on_primaryCheck_toggled                   (GtkToggleButton *toggleButton,
                                        gpointer         user_data)
{
	gint value;
	value = gtk_toggle_button_get_active (toggleButton);
	gconf_client_set_bool (global_conf, USE_PRIMARY_CLIPBOARD_KEY, value, NULL);
}

void
on_defaultCheck_toggled                   (GtkToggleButton *toggleButton,
                                        gpointer         user_data)
{
	//Only permit to distinguish ctrl+c entry if both types of clipboard are present
	if (!(gtk_toggle_button_get_active((GtkToggleButton*)primaryCheck) && 
	  gtk_toggle_button_get_active((GtkToggleButton*)toggleButton)))
		gtk_widget_set_sensitive(markDefaultCheck, FALSE);
	else
		gtk_widget_set_sensitive(markDefaultCheck, TRUE);
	gint value;
	value = gtk_toggle_button_get_active (toggleButton);
	gconf_client_set_bool (global_conf, USE_DEFAULT_CLIPBOARD_KEY, value, NULL);
}

void
on_markDefaultCheck_toggled                   (GtkToggleButton *toggleButton,
                                        gpointer         user_data)
{
	hasChanged = 1;
	gint value;
	value = gtk_toggle_button_get_active (toggleButton);
	gconf_client_set_bool (global_conf, MARK_DEFAULT_ENTRY_KEY, value, NULL);
}

void
on_saveHistCheck_toggled                   (GtkToggleButton *toggleButton,
                                        gpointer         user_data)
{
	gint value;
	value = gtk_toggle_button_get_active (toggleButton);
	gconf_client_set_bool (global_conf, SAVE_HISTORY_KEY, value, NULL);
}

void
on_closeButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(prefWin);
}


void on_helpButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
#ifndef DISABLE_GNOME
    help("preferences");
#endif /*DISABLE_GNOME*/

#ifdef DISABLE_GNOME
    errorDialog(_("Help support is not compiled in."), _("To see the documentation, consult the glipper website or compile glipper with GNOME support (see README file)."));
#endif /*DISABLE_GNOME*/
}

void showPreferences(gpointer data)
{
	char* glade_file;
	GladeXML* gladeWindow;

	//Load interface from glade file
	glade_file = g_build_filename(GLADEDIR, GLADE_XML_FILE, NULL);

	gladeWindow = glade_xml_new(glade_file, "preferences-dialog", NULL);

	//In case we cannot load glade file
	if (gladeWindow == NULL)
	{
		errorDialog(_("Could not load the preferences interface"), glade_file);
		g_free (glade_file);
		return;
	}

	g_free (glade_file);

	//Get the widgets that we will need
	historyLength = glade_xml_get_widget(gladeWindow, "historyLength");
	itemLength = glade_xml_get_widget(gladeWindow, "itemLength");
	primaryCheck = glade_xml_get_widget(gladeWindow, "primaryCheck");
	defaultCheck = glade_xml_get_widget(gladeWindow, "defaultCheck");
	markDefaultCheck = glade_xml_get_widget(gladeWindow, "markDefaultCheck");
	saveHistCheck = glade_xml_get_widget(gladeWindow, "saveHistCheck");
	keyCombEntry = glade_xml_get_widget(gladeWindow, "keyComb");
	helpButton = glade_xml_get_widget(gladeWindow, "helpButton");
	closeButton = glade_xml_get_widget(gladeWindow, "closeButton");
	prefWin = glade_xml_get_widget(gladeWindow, "preferences-dialog");

	//Connect signals to handlers
	g_signal_connect ((gpointer) primaryCheck, "toggled",
		G_CALLBACK (on_primaryCheck_toggled),
                          NULL);
	g_signal_connect ((gpointer) defaultCheck, "toggled",
		G_CALLBACK (on_defaultCheck_toggled),
		NULL);
	g_signal_connect ((gpointer) helpButton, "clicked",
		G_CALLBACK (on_helpButton_clicked),
		NULL);
	g_signal_connect ((gpointer) closeButton, "clicked",
		G_CALLBACK (on_closeButton_clicked),
		NULL);
	g_signal_connect ((gpointer) itemLength, "value_changed",
		G_CALLBACK (on_itemLength_value_changed),
		NULL);
	g_signal_connect ((gpointer) historyLength, "value_changed",
		G_CALLBACK (on_historyLength_value_changed),
		NULL);
	g_signal_connect ((gpointer) markDefaultCheck, "toggled",
		G_CALLBACK (on_markDefaultCheck_toggled),
		NULL);
	g_signal_connect ((gpointer) saveHistCheck, "toggled",
		G_CALLBACK (on_saveHistCheck_toggled),
		NULL);
	g_signal_connect ((gpointer) keyCombEntry, "changed",
		G_CALLBACK (on_keyCombEntry_changed),
		NULL);
	//Show preferences dialog
	setWidgets();
	gtk_widget_show_all(prefWin);

	//free the glade data
	g_object_unref(gladeWindow);
}

void initPreferences(GConfClient* conf)
{
	g_return_if_fail (conf != NULL);
	g_return_if_fail (global_conf == NULL);

	global_conf = conf;
	g_object_ref (G_OBJECT (global_conf));
	
	gconf_client_add_dir (conf, PATH,
                           GCONF_CLIENT_PRELOAD_NONE,
                           NULL);

	gconf_client_notify_add (conf,
                           MAX_ITEM_LENGTH_KEY,
                           max_item_length_key_changed_callback,
                           NULL,
                           NULL, NULL);

	gconf_client_notify_add (conf,
                           MAX_ELEMENTS_KEY,
                           max_elements_key_changed_callback,
                           NULL,
                           NULL, NULL);

	gconf_client_notify_add (conf,
                           USE_PRIMARY_CLIPBOARD_KEY,
                           use_primary_clipboard_key_changed_callback,
                           NULL,
                           NULL, NULL);

	gconf_client_notify_add (conf,
                           USE_DEFAULT_CLIPBOARD_KEY,
                           use_default_clipboard_key_changed_callback,
                           NULL,
                           NULL, NULL);

	gconf_client_notify_add (conf,
                           MARK_DEFAULT_ENTRY_KEY,
                           mark_default_entry_key_changed_callback,
                           NULL,
                           NULL, NULL);
                           
	gconf_client_notify_add (conf,
                           SAVE_HISTORY_KEY,
                           save_history_key_changed_callback,
                           NULL,
                           NULL, NULL);
                           
	gconf_client_notify_add (conf,
                           KEY_COMBINATION_KEY,
                           key_combination_key_changed_callback,
                           NULL,
                           NULL, NULL);
}
