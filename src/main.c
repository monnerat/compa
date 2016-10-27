/*
 * compa - Command Output Monitor Panel Applet
 * Copyright (C) 2010-2014 Ofer Kashayov <oferkv@gmail.com>
 * Copyright (C) 2015-2016 Patrick Monnerat <patrick@monnerat.net>
 *
 * compa is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * compa is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#include "config.h"


#define _(s)	gettext(s)
#define N_(s)	s


#define DEFAULT_TEXT	"<span font=\"Italic\" color=\"#00FF00\" "	\
			"bgcolor=\"#333333\">COMPA</span>"
#define ERROR_TEXT	"<span color=\"#FF0000\">Command Error</span>"

#define COMPA_SCHEMA	"org.mate.panel.applet.compa"


typedef struct {
	GtkWidget *	applet;		/* Panel applet */
	GtkWidget *	main_label;	/* Display label */
	GtkWidget *	compa_ebox;	/* Event box */
	GtkWidget *	conf_dialog;	/* Configure dialog */
	GtkWidget *	main_frame;
	GtkWidget *	main_alig;

	gchar *		monitor_cmd;	/* Main command */
	gint		update_int;	/* Update interval */
	gchar *		tooltip_cmd;	/* Tooltip command */
	gchar *		click_cmd;	/* Click action command */

	gchar *		lab_col_str;	/* Label color */
	gboolean	use_color;
	guint		active_mon;	/* Monitor is active */
	GSettings *	gsettings;	/* Configuration settings. */

	gint		frame_type;	/* Frame shadow type */
	gint		padding;
}		compa_data_inst;


#if GTK_MAJOR_VERSION < 3
#define COMPA_LABEL_ALIGN(label, xalign, yalign)			\
		gtk_misc_set_alignment(GTK_MISC(label), (xalign), (yalign))
#define COMPA_TABLE_NEW(rows, columns)					\
		gtk_table_new((rows), (columns), FALSE)
#define COMPA_TABLE_ATTACH(table, child, left, top)			\
		gtk_table_attach(GTK_TABLE(table), (child),		\
				 (left), (left) + 1, (top), (top) + 1,	\
				 GTK_EXPAND | GTK_FILL, GTK_SHRINK, 2, 2)
#define COMPA_NO_STRETCH(align, widget)					\
		*(align) = gtk_alignment_new(0, 0, 0, 0),		\
		gtk_container_add(GTK_CONTAINER(*(align)), (widget))
#define COMPA_ALIGNMENT_SET_PADDING(widget, top, bottom, left, right)	\
		gtk_alignment_set_padding(GTK_ALIGNMENT(widget), (top),	\
					  (bottom), (left), (right))
typedef GdkColor	compa_color;
#define COMPA_COLOR_PARSE(color, spec)	gdk_color_parse((spec), (color))
#define COMPA_COLOR_TO_STRING(color)	gdk_color_to_string(color)
#define COMPA_COLOR_BUTTON_GET_COLOR(button, color)			\
		gtk_color_button_get_color(GTK_COLOR_BUTTON(button), (color))
#define COMPA_COLOR_BUTTON_SET_COLOR(button, color)			\
		gtk_color_button_set_color(GTK_COLOR_BUTTON(button), (color))
#define COMPA_SET_BACKGROUND_COLOR(widget, state, color)		\
		gtk_widget_modify_bg((widget), (state), (color))
#else
#define COMPA_LABEL_ALIGN(label, xalign, yalign)			\
		(gtk_label_set_xalign(GTK_LABEL(label), (xalign)),	\
		 gtk_label_set_yalign(GTK_LABEL(label), (yalign)))
#define COMPA_TABLE_NEW(rows, columns)					\
		gtk_grid_new()
#define COMPA_TABLE_ATTACH(table, child, left, top)			\
		gtk_widget_set_margin_start((child), 2),		\
		gtk_widget_set_margin_end((child), 2),			\
		gtk_widget_set_margin_top((child), 2),			\
		gtk_widget_set_margin_bottom((child), 2),		\
		gtk_widget_set_hexpand((child), TRUE),			\
		gtk_widget_set_vexpand((child), FALSE),			\
		gtk_grid_attach(GTK_GRID(table), (child),		\
				(left), (top), 1, 1)
#define COMPA_NO_STRETCH(align, widget)					\
		gtk_widget_set_halign((widget), GTK_ALIGN_START),	\
		gtk_widget_set_valign((widget), GTK_ALIGN_START),	\
		*(align) = (widget)
#define COMPA_ALIGNMENT_SET_PADDING(widget, top, bottom, left, right)	\
		gtk_widget_set_margin_top((widget), (top)),		\
		gtk_widget_set_margin_bottom((widget), (bottom)),	\
		gtk_widget_set_margin_start((widget), (left)),		\
		gtk_widget_set_margin_end((widget), (right))
typedef GdkRGBA		compa_color;
#define COMPA_COLOR_PARSE(color, spec)	gdk_rgba_parse((color), (spec))
#define COMPA_COLOR_TO_STRING(color)	gdk_rgba_to_string(color)
#define COMPA_COLOR_BUTTON_GET_COLOR(button, color)			\
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), (color))
#define COMPA_COLOR_BUTTON_SET_COLOR(button, color)			\
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button), (color))
#define COMPA_SET_BACKGROUND_COLOR(widget, state, color)		\
		gtk_widget_override_background_color((widget), (state), (color))
#endif


/*
 *  Action click
 */

gboolean
action_click(GtkWidget * Widget, GdkEventButton * event,
	     compa_data_inst * compa_data)

{
	if (event->type == GDK_BUTTON_PRESS)
		if (event->button == 1) {
			if (compa_data->click_cmd != NULL &&
			    compa_data->click_cmd[0]) {
				char click_cmd_bg[FILENAME_MAX];

				sprintf(click_cmd_bg, "%s &",
					compa_data->click_cmd);

				if (system(click_cmd_bg))
					;			/* Ignore. */
				}

			return TRUE;
			}

	return FALSE;
}


/*
 *  Load config
 */

static void
load_config(compa_data_inst * compa_data)
{
	compa_data->monitor_cmd = g_settings_get_string(compa_data->gsettings,
							"monitor-command");
	compa_data->update_int = g_settings_get_int(compa_data->gsettings,
						    "update-interval");
	compa_data->tooltip_cmd = g_settings_get_string(compa_data->gsettings,
							"tooltip-command");
	compa_data->click_cmd = g_settings_get_string(compa_data->gsettings,
						      "click-command");
	compa_data->frame_type = g_settings_get_enum(compa_data->gsettings,
						     "frame-type");
	compa_data->lab_col_str = g_settings_get_string(compa_data->gsettings,
							"label-color");
	compa_data->padding = g_settings_get_int(compa_data->gsettings,
						 "padding");
	compa_data->use_color = g_settings_get_boolean(compa_data->gsettings,
						      "use-color");
}


/*
 *  Save config
 */

static void
save_config(compa_data_inst * compa_data)

{
	g_settings_set_string(compa_data->gsettings,
			      "monitor-command", compa_data->monitor_cmd);
	g_settings_set_int(compa_data->gsettings,
			   "update-interval", compa_data->update_int);
	g_settings_set_string(compa_data->gsettings,
			      "tooltip-command", compa_data->tooltip_cmd);
	g_settings_set_string(compa_data->gsettings,
			      "click-command", compa_data->click_cmd);
	g_settings_set_enum(compa_data->gsettings,
			    "frame-type", compa_data->frame_type);
	g_settings_set_string(compa_data->gsettings,
			      "label-color", compa_data->lab_col_str);
	g_settings_set_int(compa_data->gsettings,
			   "padding", compa_data->padding);
	g_settings_set_boolean(compa_data->gsettings,
			   "use-color", compa_data->use_color);
	g_settings_sync();
}


/*
 *  Tooltip update
 */

void
tooltip_update(compa_data_inst * compa_data)

{
	if (compa_data->tooltip_cmd && compa_data->tooltip_cmd[0]) {
		FILE * cmd_pipe;
		unsigned int pipe_char;
		gchar tooltip_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(compa_data->tooltip_cmd, "r"))) {
			for (i = 0; i <= FILENAME_MAX; i++) {
				pipe_char = getc(cmd_pipe);

				if (pipe_char == EOF)
					break;

				tooltip_cmd_out[i] = pipe_char;
				}

			if (tooltip_cmd_out[i - 1] == '\n')
				tooltip_cmd_out[i - 1] = 0;
			else
				tooltip_cmd_out[i] = 0;

			(void) pclose(cmd_pipe);
			}

		/* Update label. */

		if (!strlen(tooltip_cmd_out))
			sprintf(tooltip_cmd_out, ERROR_TEXT);

		gtk_widget_set_tooltip_markup(compa_data->compa_ebox,
					      tooltip_cmd_out);
		}
}


/*
 *  Compa update
 */

static gboolean
compa_update(compa_data_inst * compa_data)

{
	if (compa_data->monitor_cmd && compa_data->monitor_cmd[0]) {
		FILE * cmd_pipe;
		unsigned int pipe_char;
		gchar monitor_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(compa_data->monitor_cmd, "r"))) {
			for (i = 0; i < sizeof monitor_cmd_out - 1; i++) {
				pipe_char = getc(cmd_pipe);

				if (pipe_char == EOF)
					break;

				monitor_cmd_out[i] = pipe_char;
				}

			while (i--)
				if (monitor_cmd_out[i] != '\n' &&
				    monitor_cmd_out[i] != '\r')
					break;

			monitor_cmd_out[i + 1] = '\0';
			(void) pclose(cmd_pipe);
			}

		/* Update label */

		if (!strlen(monitor_cmd_out))
			sprintf(monitor_cmd_out, ERROR_TEXT);
		gtk_label_set_markup(GTK_LABEL(compa_data->main_label),
				     monitor_cmd_out);

		return TRUE;
		}
	else
		return FALSE;
}


/*
 *  Menu update
 */

static void
compa_menu_update(GtkAction * action, gpointer user_data)

{
	compa_update((compa_data_inst *) user_data);
}


/*
 *  File open
 */

static void
select_command(GtkWidget * text_ent)

{
	GtkWidget *dialog;

	/* Open file dialog. */

	dialog = gtk_file_chooser_dialog_new(_("Select command"), NULL,
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     _("_Cancel"),
					     GTK_RESPONSE_CANCEL,
					     _("_Open"),
					     GTK_RESPONSE_ACCEPT,
					     NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
					    g_get_home_dir());

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		const gchar * fn;

		fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(GTK_ENTRY(text_ent), fn);
		}

	gtk_widget_destroy(dialog);
}

/*
 *  Dialog response
 */

static void
dialog_response(GtkWidget * dialog)

{
	gtk_widget_hide(dialog);
}


/*
 *  Config dialog
 */

static void
config_dialog(GtkAction * action, gpointer user_data)

{
	compa_data_inst * compa_data = user_data;
	GtkWidget * content_area;
	GtkWidget * monitor_lab;
	GtkWidget * interval_lab;
	GtkWidget * tooltip_lab;
	GtkWidget * action_lab;
	GtkWidget * frame_type_lab;
	GtkWidget * padding_lab;
	GtkWidget * col_check;
	GtkWidget * monitor_ent;
	GtkWidget * tooltip_ent;
	GtkWidget * action_ent;
	GtkWidget * interval_spin;
	GtkWidget * interval_alig;
	GtkWidget * padding_spin;
	GtkWidget * padding_alig;
	GtkWidget * monitor_browse_but;
	GtkWidget * tooltip_browse_but;
	GtkWidget * action_browse_but;
	GtkWidget * frame_type_comb;
	GtkWidget * frame_type_alig;
	GtkWidget * label_col_but;
	GtkWidget * label_col_alig;
	GtkWidget * conf_table;
	GtkWidget * frame_alig;
	compa_color lab_color;

	if (compa_data->conf_dialog)
		return;

	/* Dialog. */

	compa_data->conf_dialog =
	    gtk_dialog_new_with_buttons(_("Configure"), NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					_("_Cancel"),
					GTK_RESPONSE_CANCEL,
					_("_OK"),
					GTK_RESPONSE_OK,
					NULL);
	g_signal_connect_swapped(compa_data->conf_dialog, "response",
				 G_CALLBACK(dialog_response),
				 compa_data->conf_dialog);

	/* Labels. */

	monitor_lab = gtk_label_new(_("Monitor command: "));
	interval_lab = gtk_label_new(_("Interval (seconds): "));
	tooltip_lab = gtk_label_new(_("Tooltip command: "));
	action_lab = gtk_label_new(_("Click action: "));
	frame_type_lab = gtk_label_new(_("Shadow type: "));
	padding_lab = gtk_label_new(_("Padding: "));
	COMPA_LABEL_ALIGN(monitor_lab, 0.0, 0.5);
	COMPA_LABEL_ALIGN(interval_lab, 0.0, 0.5);
	COMPA_LABEL_ALIGN(tooltip_lab, 0.0, 0.5);
	COMPA_LABEL_ALIGN(action_lab, 0.0, 0.5);
	COMPA_LABEL_ALIGN(frame_type_lab, 0.0, 0.5);
	COMPA_LABEL_ALIGN(padding_lab, 0.0, 0.5);

	/* Check button. */

	col_check = gtk_check_button_new_with_label(_("Label background: "));

	/* Text entries. */

	monitor_ent = gtk_entry_new();
	tooltip_ent = gtk_entry_new();
	action_ent = gtk_entry_new();

	if (compa_data->monitor_cmd)
		gtk_entry_set_text(GTK_ENTRY(monitor_ent),
				   compa_data->monitor_cmd);

	if (compa_data->tooltip_cmd)
		gtk_entry_set_text(GTK_ENTRY(tooltip_ent),
				   compa_data->tooltip_cmd);

	if (compa_data->click_cmd)
		gtk_entry_set_text(GTK_ENTRY(action_ent),
				   compa_data->click_cmd);

	/* Spin buttons. */

	interval_spin = gtk_spin_button_new_with_range(1, 86400, 1);
	padding_spin = gtk_spin_button_new_with_range(0, 10, 1);
	COMPA_NO_STRETCH(&interval_alig, interval_spin);
	COMPA_NO_STRETCH(&padding_alig, padding_spin);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_spin),
				  compa_data->update_int);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(padding_spin),
				  compa_data->padding);

	/* Buttons. */

	monitor_browse_but = gtk_button_new_with_label(_("Browse"));
	tooltip_browse_but = gtk_button_new_with_label(_("Browse"));
	action_browse_but = gtk_button_new_with_label(_("Browse"));
	g_signal_connect_swapped(G_OBJECT(monitor_browse_but), "clicked",
				 G_CALLBACK(select_command), monitor_ent);
	g_signal_connect_swapped(G_OBJECT(tooltip_browse_but), "clicked",
				 G_CALLBACK(select_command), tooltip_ent);
	g_signal_connect_swapped(G_OBJECT(action_browse_but), "clicked",
				 G_CALLBACK(select_command), action_ent);

	/* Combos. */

	frame_type_comb = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frame_type_comb),
				       _("None"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frame_type_comb),
				       _("In"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frame_type_comb),
				       _("Out"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frame_type_comb),
				       _("Etched in"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frame_type_comb),
				       _("Etched out"));
	COMPA_NO_STRETCH(&frame_type_alig, frame_type_comb);
	gtk_combo_box_set_active(GTK_COMBO_BOX(frame_type_comb),
				 compa_data->frame_type);

	/* Color buttons. */

	label_col_but = gtk_color_button_new();
	COMPA_NO_STRETCH(&label_col_alig, label_col_but);

	if (compa_data->lab_col_str) {
		COMPA_COLOR_PARSE(&lab_color, compa_data->lab_col_str);
		COMPA_COLOR_BUTTON_SET_COLOR(label_col_but, &lab_color);
		}

	if (compa_data->use_color)
		gtk_toggle_button_set_active(
		    GTK_TOGGLE_BUTTON(col_check), TRUE);

	/* Table. */

	conf_table = COMPA_TABLE_NEW(7, 3);
	COMPA_NO_STRETCH(&frame_alig, conf_table);
	COMPA_ALIGNMENT_SET_PADDING(frame_alig, 5, 5, 5, 5);

	/* Monitor command settings. */

	COMPA_TABLE_ATTACH(conf_table, monitor_lab, 0, 0);
	COMPA_TABLE_ATTACH(conf_table, monitor_ent, 1, 0);
	COMPA_TABLE_ATTACH(conf_table, monitor_browse_but, 2, 0);

	/* Interval settings. */

	COMPA_TABLE_ATTACH(conf_table, interval_lab, 0, 1);
	COMPA_TABLE_ATTACH(conf_table, interval_alig, 1, 1);

	/* Frame settings. */

	COMPA_TABLE_ATTACH(conf_table, frame_type_lab, 0, 2);
	COMPA_TABLE_ATTACH(conf_table, frame_type_alig, 1, 2);

	/* Padding settings. */

	COMPA_TABLE_ATTACH(conf_table, padding_lab, 0, 3);
	COMPA_TABLE_ATTACH(conf_table, padding_alig, 1, 3);

	/* Color settings. */

	COMPA_TABLE_ATTACH(conf_table, col_check, 0, 4);
	COMPA_TABLE_ATTACH(conf_table, label_col_alig, 1, 4);

	/* Tooltip settings. */

	COMPA_TABLE_ATTACH(conf_table, tooltip_lab, 0, 5);
	COMPA_TABLE_ATTACH(conf_table, tooltip_ent, 1, 5);
	COMPA_TABLE_ATTACH(conf_table, tooltip_browse_but, 2, 5);

	/* Action settings. */

	COMPA_TABLE_ATTACH(conf_table, action_lab, 0, 6);
	COMPA_TABLE_ATTACH(conf_table, action_ent, 1, 6);
	COMPA_TABLE_ATTACH(conf_table, action_browse_but, 2, 6);

	/* Display dialog. */

	content_area =
	    gtk_dialog_get_content_area(GTK_DIALOG(compa_data->conf_dialog));
	gtk_container_add(GTK_CONTAINER(content_area), frame_alig);
	gtk_widget_show_all(compa_data->conf_dialog);

	if (gtk_dialog_run(GTK_DIALOG(compa_data->conf_dialog)) ==
	    GTK_RESPONSE_OK) {
		/* Update monitor command. */

		if (compa_data->monitor_cmd)
			free(compa_data->monitor_cmd);

		compa_data->monitor_cmd =
		    strdup(gtk_entry_get_text(GTK_ENTRY(monitor_ent)));

		if (strlen(compa_data->monitor_cmd) < 1)
			gtk_label_set_markup(GTK_LABEL(compa_data->main_label),
					     DEFAULT_TEXT);

		/* Update interval. */

		compa_data->update_int =
		    gtk_spin_button_get_value_as_int(
					GTK_SPIN_BUTTON(interval_spin));

		/* Update frame type. */

		compa_data->frame_type =
		    gtk_combo_box_get_active(GTK_COMBO_BOX(frame_type_comb));
		gtk_frame_set_shadow_type(GTK_FRAME(compa_data->main_frame),
					  compa_data->frame_type);

		/* Update padding. */

		compa_data->padding = gtk_spin_button_get_value_as_int(
						GTK_SPIN_BUTTON(padding_spin));
		COMPA_ALIGNMENT_SET_PADDING(compa_data->main_alig,
					    compa_data->padding,
					    compa_data->padding,
					    compa_data->padding,
					    compa_data->padding);

		/* Update colors. */

		COMPA_COLOR_BUTTON_GET_COLOR(label_col_but, &lab_color);

		if (compa_data->lab_col_str)
			free(compa_data->lab_col_str);

		compa_data->lab_col_str =
		    strdup(COMPA_COLOR_TO_STRING(&lab_color));

		if (gtk_toggle_button_get_active(
						GTK_TOGGLE_BUTTON(col_check))) {
			compa_data->use_color = TRUE;
			COMPA_SET_BACKGROUND_COLOR(compa_data->compa_ebox,
						   GTK_STATE_NORMAL,
						   &lab_color);
			}
		else {
			compa_data->use_color = FALSE;
			COMPA_SET_BACKGROUND_COLOR(compa_data->compa_ebox,
						   GTK_STATE_NORMAL, NULL);
			}

		/* Update tooltip command. */

		if (compa_data->tooltip_cmd)
			free(compa_data->tooltip_cmd);

		compa_data->tooltip_cmd =
		    strdup(gtk_entry_get_text(GTK_ENTRY(tooltip_ent)));

		if (strlen(compa_data->tooltip_cmd) < 1)
			gtk_widget_set_tooltip_text(compa_data->compa_ebox, "");

		/* Update click command. */

		if (compa_data->click_cmd)
			free(compa_data->click_cmd);

		compa_data->click_cmd =
		    strdup(gtk_entry_get_text(GTK_ENTRY(action_ent)));

		/* Remove old monitor. */

		if (compa_data->active_mon)
			g_source_remove(compa_data->active_mon);

		/* Add new monitor. */

		if (strlen(compa_data->monitor_cmd) > 0 &&
		    compa_data->update_int > 0) {
			compa_data->active_mon =
			    g_timeout_add_seconds(compa_data->update_int,
						  (GSourceFunc) compa_update,
						  compa_data);
			compa_update(compa_data);
			}

		/* Save config. */

		save_config(compa_data);
		}

	/* Remove dialog. */

	gtk_widget_destroy(compa_data->conf_dialog);
	compa_data->conf_dialog = NULL;
}


/*
 *  About dialog
 */

static void
about_dialog(void)

{
	static const char transcred[] = N_("translator-credits");
	const char * translator = _(transcred);

	const gchar * authors[] = {
		"Ofer Kashayov",
		"Patrick Monnerat",
		NULL
	};

	const gchar * artists[] = {
		"Hava Saban",
		NULL
	};

	if (!strcmp(transcred, translator) || !*translator)
		translator = NULL;

	gtk_show_about_dialog(NULL,
			      "program-name", "Compa",
			      "comments",
				_("Command Output Monitor Panel Applet"),
			      "version", VERSION,
			      "copyright",
				"Copyright \xC2\xA9 2010-2014 Ofer Kashayov, "
						   "2015 Patrick Monnerat",
			      "license", "GPLv3+",
			      "logo-icon-name", "compa",
			      "website", "https://github.com/monnerat/compa",
			      "authors", authors,
			      "artists", artists,
			       transcred, translator,
			       NULL);
}


/*
 *  Compa Destroy
 */

static void
compa_destroy(GtkWidget * widget, compa_data_inst * compa_data)

{
	/* Remove config dialog if active. */
	if (compa_data->conf_dialog)
		gtk_widget_destroy(GTK_WIDGET(compa_data->conf_dialog));

	/* Remove an existing monitor. */
	if (compa_data->active_mon)
		g_source_remove(compa_data->active_mon);

	if (compa_data->gsettings)
		g_object_unref(compa_data->gsettings);

	g_free(compa_data);
}


/*
 *  Compa init
 */

static void
compa_init(MatePanelApplet * applet)

{
	GtkActionGroup * action_group;
	compa_data_inst * compa_data;
	static const char compa_menu[] =
	    "   <menuitem name=\"update_item\" action=\"update_verb\" />"
	    "   <menuitem name=\"configure_item\" action=\"configure_verb\" />"
	    "   <menuitem name=\"about_item\" action=\"about_verb\" />";
	static const GtkActionEntry compa_verbs[] = {
		{
			"update_verb", GTK_STOCK_REFRESH, N_("_Update now"),
			NULL, NULL, G_CALLBACK(compa_menu_update)
		},
		{
			"configure_verb", GTK_STOCK_PROPERTIES,
			N_("_Configure"), NULL, NULL, G_CALLBACK(config_dialog)
		},
		{
			"about_verb", GTK_STOCK_ABOUT, N_("_About"),
			NULL, NULL, G_CALLBACK(about_dialog)
		}
	};

	/* i18n. */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE_NAME, "utf-8");
	textdomain(PACKAGE_NAME);

	/* Init. */
	g_set_application_name("Compa");
	gtk_window_set_default_icon_name("gtk-properties");

	/* This instance data. */
	compa_data = g_new0(compa_data_inst, 1);
	compa_data->applet = GTK_WIDGET(applet);
	g_signal_connect(G_OBJECT(applet), "destroy",
			 G_CALLBACK(compa_destroy), compa_data);

	/* Configuration data. */
	compa_data->gsettings = mate_panel_applet_settings_new(
					MATE_PANEL_APPLET(compa_data->applet),
					COMPA_SCHEMA);

	/* Popup menu. */
	action_group = gtk_action_group_new(_("Compa Applet Actions"));
	gtk_action_group_set_translation_domain(action_group, "compa");
	gtk_action_group_add_actions(action_group, compa_verbs,
				     G_N_ELEMENTS(compa_verbs), compa_data);
	mate_panel_applet_setup_menu(MATE_PANEL_APPLET(compa_data->applet),
				     compa_menu, action_group);
	g_object_unref(action_group);

	/* Label. */
	compa_data->main_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(compa_data->main_label), DEFAULT_TEXT);
	gtk_label_set_justify(GTK_LABEL(compa_data->main_label),
			      GTK_JUSTIFY_CENTER);

	/* Alignment. */
	COMPA_NO_STRETCH(&compa_data->main_alig, compa_data->main_label);

	/* Frame. */
	compa_data->main_frame = gtk_frame_new(NULL);

	/* Event box. */
	compa_data->compa_ebox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(compa_data->compa_ebox), "button_press_event",
			 G_CALLBACK(action_click), compa_data);
	g_signal_connect_swapped(G_OBJECT(compa_data->compa_ebox),
				 "enter-notify-event",
				 G_CALLBACK(tooltip_update), compa_data);

	/* Place widgets. */
	gtk_container_add(GTK_CONTAINER(compa_data->compa_ebox),
			  compa_data->main_alig);
	gtk_container_add(GTK_CONTAINER(compa_data->main_frame),
			  compa_data->compa_ebox);
	gtk_container_add(GTK_CONTAINER(compa_data->applet),
			  compa_data->main_frame);
	gtk_widget_show_all(GTK_WIDGET(compa_data->applet));

	/* Load config. */
	load_config(compa_data);

	/* Add monitor. */
	if (compa_data->monitor_cmd > 0 && compa_data->update_int > 0) {
		compa_data->active_mon =
		    g_timeout_add_seconds(compa_data->update_int,
					  (GSourceFunc) compa_update,
					  compa_data);
		compa_update(compa_data);
		}

	/* Set frame type. */
	gtk_frame_set_shadow_type(GTK_FRAME(compa_data->main_frame),
				  compa_data->frame_type);

	/* Update padding. */
	COMPA_ALIGNMENT_SET_PADDING(compa_data->main_alig,
				    compa_data->padding, compa_data->padding,
				    compa_data->padding, compa_data->padding);

	/* Update colors. */
	if (compa_data->use_color) {
		compa_color lab_color;

		COMPA_COLOR_PARSE(&lab_color, compa_data->lab_col_str);
		COMPA_SET_BACKGROUND_COLOR(compa_data->compa_ebox,
					   GTK_STATE_NORMAL, &lab_color);
		}
}


/*
 *  Applet factory
 */

static gboolean
applet_factory(MatePanelApplet *applet, const gchar * iid, gpointer user_data)

{
	if (strcmp(iid, "compaApplet"))
		return FALSE;

	compa_init(applet);
	return TRUE;
}


MATE_PANEL_APPLET_OUT_PROCESS_FACTORY(
	"compaAppletFactory",
	PANEL_TYPE_APPLET,
	"Compa",
	applet_factory,
	NULL
);
