/*
 * compa - Command Output Monitor Panel Applet
 * Copyright (C) 2010-2014 Ofer Kashayov <oferkv@gmail.com>
 * Copyright (C) 2015-2022 Patrick Monnerat <patrick@monnerat.net>
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
#include <stddef.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>
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
	GtkWidget *		applet;		/* Panel applet. */
	GSettings *		gsettings;	/* Configuration settings. */
	GtkCssProvider *	frame_css;	/* CSS for applet "frame". */

	/* Applet area widgets. */
	GtkWidget *		compa_frame;
	GtkWidget *		compa_eventbox;
	GtkWidget *		compa_label;

	/* Configure dialog widgets. */
	GtkWidget *		configure_dialog;
	GtkWidget *		monitor_entry;
	GtkWidget *		monitor_markup_check;
	GtkWidget *		period_spin;
	GtkWidget *		frame_type_combo;
	GtkWidget *		frame_maximized_check;
	GtkWidget *		padding_spin;
	GtkWidget *		label_color_button;
	GtkWidget *		tooltip_entry;
	GtkWidget *		tooltip_markup_check;
	GtkWidget *		action_entry;

	/* Configuration values. */
	gchar *			monitor_command;
	gboolean		monitor_markup;
	gint			update_period;	/* Seconds. */
	gchar *			tooltip_command;
	gboolean		tooltip_markup;
	gchar *			click_command;
	gchar *			background_color;
	guint			active_monitor;
	gint			frame_type;
	gboolean		frame_maximized;
	gint			padding;
}		compa_t;

/*
 * Gtk builder id to address offset table.
 */
#define IDENTRY(name)	{#name, offsetof(compa_t, name)}
static const struct {
	const char *	id;
	size_t		offset;
}		idtable[] = {
	IDENTRY(compa_label),
	IDENTRY(compa_eventbox),
	IDENTRY(compa_frame),
	IDENTRY(configure_dialog),
	IDENTRY(monitor_entry),
	IDENTRY(monitor_markup_check),
	IDENTRY(period_spin),
	IDENTRY(frame_type_combo),
	IDENTRY(frame_maximized_check),
	IDENTRY(padding_spin),
	IDENTRY(label_color_button),
	IDENTRY(tooltip_entry),
	IDENTRY(tooltip_markup_check),
	IDENTRY(action_entry),
	{NULL, 0}
};


/*
 *  Action click
 */
gboolean
action_click(GtkWidget *widget, GdkEventButton *event, compa_t *p)
{
	(void) widget;

	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		if (p->click_command[0]) {
			char click_cmd_bg[FILENAME_MAX];

			snprintf(click_cmd_bg, sizeof click_cmd_bg, "%s &",
				 p->click_command);

			if (system(click_cmd_bg))
				;			/* Ignore. */
		}

		return TRUE;
	}

	return FALSE;
}


/*
 * Free configuration data.
 */
static void
free_config(compa_t *p)
{
	g_free(p->monitor_command);
	g_free(p->tooltip_command);
	g_free(p->click_command);
	g_free(p->background_color);
	p->monitor_command = NULL;
	p->tooltip_command = NULL;
	p->click_command = NULL;
	p->background_color = NULL;
}


/*
 *  Load config
 */
static void
load_config(compa_t *p)
{
	GSettings *g = p->gsettings;

	p->monitor_command = g_settings_get_string(g, "monitor-command");
	p->monitor_markup = g_settings_get_boolean(g, "monitor-markup");
	p->update_period = g_settings_get_int(g, "update-period");
	p->tooltip_command = g_settings_get_string(g, "tooltip-command");
	p->tooltip_markup = g_settings_get_boolean(g, "tooltip-markup");
	p->click_command = g_settings_get_string(g, "click-command");
	p->frame_type = g_settings_get_enum(g, "frame-type");
	p->frame_maximized = g_settings_get_boolean(g, "frame-maximized");
	p->padding = g_settings_get_int(g, "padding");
	p->background_color = g_settings_get_string(g, "label-color");
}


/*
 *  Save config
 */
static void
save_config(compa_t *p)
{
	GSettings *g = p->gsettings;

	g_settings_set_string(g, "monitor-command", p->monitor_command);
	g_settings_set_boolean(g, "monitor-markup", p->monitor_markup);
	g_settings_set_int(g, "update-period", p->update_period);
	g_settings_set_string(g, "tooltip-command", p->tooltip_command);
	g_settings_set_boolean(g, "tooltip-markup", p->tooltip_markup);
	g_settings_set_string(g, "click-command", p->click_command);
	g_settings_set_enum(g, "frame-type", p->frame_type);
	g_settings_set_boolean(g, "frame-maximized", p->frame_maximized);
	g_settings_set_int(g, "padding", p->padding);
	g_settings_set_string(g, "label-color", p->background_color);
	g_settings_sync();
}


/*
 *  Tooltip update
 */
void
tooltip_update(compa_t *p)
{
	if (p->tooltip_command[0]) {
		gboolean markup = p->tooltip_markup;
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar tooltip_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(p->tooltip_command, "r"))) {
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
		if (!tooltip_cmd_out[0]) {
			snprintf(tooltip_cmd_out, sizeof tooltip_cmd_out,
				 ERROR_TEXT);
			markup = TRUE;
		}

		gtk_widget_set_tooltip_markup(p->compa_eventbox, NULL);
		gtk_widget_set_tooltip_text(p->compa_eventbox, NULL);
		if (markup)
			gtk_widget_set_tooltip_markup(p->compa_eventbox,
						      tooltip_cmd_out);
		else
			gtk_widget_set_tooltip_text(p->compa_eventbox,
						    tooltip_cmd_out);
	}
}


/*
 *  Compa update
 */
static gboolean
compa_update(compa_t *p)
{
	GtkAlign al = p->frame_maximized? GTK_ALIGN_FILL: GTK_ALIGN_CENTER;

	gtk_widget_set_halign(p->compa_frame, al);
	gtk_widget_set_valign(p->compa_frame, al);

	if (p->monitor_command[0]) {
		gboolean markup = p->monitor_markup;
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar monitor_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(p->monitor_command, "r"))) {
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
		if (!monitor_cmd_out[0]) {
			snprintf(monitor_cmd_out, sizeof monitor_cmd_out,
				 ERROR_TEXT);
			markup = TRUE;
		}

		gtk_label_set_markup(GTK_LABEL(p->compa_label), NULL);
		gtk_label_set_text(GTK_LABEL(p->compa_label), NULL);
		if (markup)
			gtk_label_set_markup(GTK_LABEL(p->compa_label),
					     monitor_cmd_out);
		else
			gtk_label_set_text(GTK_LABEL(p->compa_label),
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
compa_menu_update(GtkAction *action, gpointer user_data)
{
	compa_update((compa_t *) user_data);
}


/*
 *  File open
 */
void
select_command(GtkWidget *text_ent)
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
		gchar *fn;

		fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(GTK_ENTRY(text_ent), fn);
		g_free(fn);
	}

	gtk_widget_destroy(dialog);
}

/*
 *  Dialog response
 */
void
dialog_response(GtkWidget *dialog)
{
	gtk_widget_hide(dialog);
}


/*
 * Set padding according to orientation.
 */
static void
handle_orientation(compa_t *p)
{
	guint vertical = 1;

	gtk_widget_set_margin_top(p->compa_frame, 1);
	gtk_widget_set_margin_bottom(p->compa_frame, 1);
	gtk_widget_set_margin_start(p->compa_frame, 1);
	gtk_widget_set_margin_end(p->compa_frame, 1);
	gtk_widget_set_margin_top(p->compa_label, 0);
	gtk_widget_set_margin_bottom(p->compa_label, 0);
	gtk_widget_set_margin_start(p->compa_label, 0);
	gtk_widget_set_margin_end(p->compa_label, 0);

	switch (mate_panel_applet_get_orient(MATE_PANEL_APPLET(p->applet))) {
	case MATE_PANEL_APPLET_ORIENT_LEFT:
		gtk_widget_set_margin_end(p->compa_frame, 0);
		break;

	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		gtk_widget_set_margin_start(p->compa_frame, 0);
		break;

	case MATE_PANEL_APPLET_ORIENT_DOWN:
		gtk_widget_set_margin_top(p->compa_frame, 0);
		vertical = 0;
		break;

	default:	/* MATE_PANEL_APPLET_ORIENT_UP. */
		gtk_widget_set_margin_bottom(p->compa_frame, 0);
		vertical = 0;
		break;
	}

	if (vertical) {
		gtk_widget_set_margin_top(p->compa_label, p->padding);
		gtk_widget_set_margin_bottom(p->compa_label, p->padding);
	}
	else {
		gtk_widget_set_margin_start(p->compa_label, p->padding);
		gtk_widget_set_margin_end(p->compa_label, p->padding);
	}
}


/*
 *  Configure applet.
 */

static void
applet_configure(compa_t *p)
{
	gchar *css;

	static gchar const frame_style_format[] =
		"#compa-frame {background-color: %s;"
			      "border-color: %s;"
			      "border-width: %u;"
			      "border-style: %s;}";
	const struct css_params {
		const char *	border_style;
		const char *	border_color;
		int		border_width;
	}		*b;

	static const struct css_params params[] = {
		{	"none",		"rgba(0,0,0,0)",	0	},
		{	"inset",	"rgba(90,90,90,0.6)",	2	},
		{	"outset",	"rgba(90,90,90,0.6)",	2	},
		{	"groove",	"rgba(90,90,90,0.6)",	3	},
		{	"ridge",	"rgba(90,90,90,0.6)",	3	},
		{	"solid",	"rgba(0,0,0,0.4)",	2	},
	};

	/* Remove old monitor. */
	if (p->active_monitor)
		g_source_remove(p->active_monitor);
	p->active_monitor = 0;

	/* Preset default content. */
	gtk_label_set_text(GTK_LABEL(p->compa_label), NULL);
	gtk_label_set_markup(GTK_LABEL(p->compa_label), DEFAULT_TEXT);
	gtk_widget_set_tooltip_text(p->compa_eventbox, "");

	/* Update frame type and background. */
	b = params + p->frame_type;
	css = g_strdup_printf(frame_style_format, p->background_color,
			      b->border_color, b->border_width,
			      b->border_style);
	gtk_css_provider_load_from_data(p->frame_css, css, -1, NULL);
	g_free(css);

	/* Update padding. */
	handle_orientation(p);

	/* Update displayed data. */
	compa_update(p);

	/* Add new monitor. */
	if (p->monitor_command[0] && p->update_period)
		p->active_monitor =
		    g_timeout_add_seconds(p->update_period,
					  (GSourceFunc) compa_update, p);
}


/*
 *  Load configuration data into dialog.
 */
static void
load_config_dialog_data(compa_t *p)
{
	GdkRGBA color;

	gtk_entry_set_text(GTK_ENTRY(p->monitor_entry),
			   p->monitor_command? p->monitor_command: "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->monitor_markup_check),
				     p->monitor_markup);
	gtk_entry_set_text(GTK_ENTRY(p->tooltip_entry),
			   p->tooltip_command? p->tooltip_command: "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->tooltip_markup_check),
				     p->tooltip_markup);
	gtk_entry_set_text(GTK_ENTRY(p->action_entry),
			   p->click_command? p->click_command: "");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->period_spin),
				  p->update_period);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->padding_spin),
				  p->padding);
	gdk_rgba_parse(&color, p->background_color);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(p->label_color_button),
				  &color);
	gtk_combo_box_set_active(GTK_COMBO_BOX(p->frame_type_combo),
				 p->frame_type);
	gtk_toggle_button_set_active(
	    GTK_TOGGLE_BUTTON(p->frame_maximized_check), p->frame_maximized);
}


/*
 *  Retrieve configure dialog data.
 */
static void
retrieve_config_dialog_data(compa_t *p)
{
	GdkRGBA color;

	free_config(p);

	/* Retrieve monitor command. */
	p->monitor_command = g_strdup(gtk_entry_get_text(
				GTK_ENTRY(p->monitor_entry)));
	p->monitor_markup = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->monitor_markup_check));

	/* Retrieve update period. */
	p->update_period = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(p->period_spin));

	/* Retrieve frame type. */
	p->frame_type = gtk_combo_box_get_active(
				GTK_COMBO_BOX(p->frame_type_combo));
	p->frame_maximized = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->frame_maximized_check));

	/* Retrieve padding. */
	p->padding = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(p->padding_spin));

	/* Retrieve background color. */
	gtk_color_chooser_get_rgba(
		GTK_COLOR_CHOOSER(p->label_color_button), &color);
	p->background_color = gdk_rgba_to_string(&color);

	/* Retrieve tooltip command. */
	p->tooltip_command = g_strdup(gtk_entry_get_text(
				GTK_ENTRY(p->tooltip_entry)));
	p->tooltip_markup = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->tooltip_markup_check));

	/* Retrieve click command. */
	p->click_command =
	     g_strdup(gtk_entry_get_text(GTK_ENTRY(p->action_entry)));
}


/*
 *  Config dialog
 */
static void
config_dialog(GtkAction *action, compa_t *p)
{
	gtk_widget_show_all(p->configure_dialog);

	switch (gtk_dialog_run(GTK_DIALOG(p->configure_dialog))) {
	case GTK_RESPONSE_OK:
		retrieve_config_dialog_data(p);
		applet_configure(p);
		save_config(p);
		break;
	default:
		load_config_dialog_data(p);
		break;
	}
}


/*
 *  About dialog
 */
static void
about_dialog(void)
{
	static const char transcred[] = N_("translator-credits");
	const char *translator = _(transcred);

	const gchar *authors[] = {
		"Ofer Kashayov",
		"Patrick Monnerat",
		NULL
	};

	const gchar *artists[] = {
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
						   "2015-2022 Patrick Monnerat",
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
compa_destroy(GtkWidget *widget, compa_t *p)
{
	/* Remove configure dialog. */
	if (p->configure_dialog)
		gtk_widget_destroy(p->configure_dialog);

	/* Remove an existing monitor. */
	if (p->active_monitor)
		g_source_remove(p->active_monitor);

	if (p->gsettings)
		g_object_unref(p->gsettings);

	/* Remove frame CSS. */
	g_object_unref(p->frame_css);

	free_config(p);
	g_free(p);
}


/*
 *  init_popup:
 * This is isolated in a separate function to override necessary deprecated
 * structures and calls.
 */
static void
init_popup_menu(MatePanelApplet *applet, compa_t *p)
{
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	GtkActionGroup *action_group;
	static const char menu[] =
	    "   <menuitem name=\"update_item\" action=\"update_verb\" />"
	    "   <menuitem name=\"configure_item\" action=\"configure_verb\" />"
	    "   <menuitem name=\"about_item\" action=\"about_verb\" />";
	static const GtkActionEntry verbs[] = {
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

	action_group = gtk_action_group_new(_("Compa Applet Actions"));
	gtk_action_group_set_translation_domain(action_group, "compa");
	gtk_action_group_add_actions(action_group, verbs,
				     G_N_ELEMENTS(verbs), p);
	mate_panel_applet_setup_menu(applet, menu, action_group);
	g_object_unref(action_group);
	G_GNUC_END_IGNORE_DEPRECATIONS
}


/*
 * Disable configuration dialog vertical resizing.
 */
void
configure_dialog_size_allocate(GtkWidget *dialog, GdkRectangle *allocation,
			       gpointer user_data)
{
	GdkGeometry hints;

	(void) allocation;
	(void) user_data;

	hints.min_width = 0;
        hints.max_width = G_MAXINT;
        hints.min_height = 0;
        hints.max_height = 1;
	gtk_window_set_geometry_hints(GTK_WINDOW (dialog), (GtkWidget *) NULL,
				      &hints, (GdkWindowHints)
				      (GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}


/*
 *  Compa init
 */
static void
compa_init(MatePanelApplet *applet)
{
	compa_t *p;
	GtkBuilder *builder;
	GtkStyleContext *context;

	size_t i;

	/* i18n. */
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE_NAME, "utf-8");
	textdomain(PACKAGE_NAME);

	/* Init. */
	g_set_application_name("Compa");
	gtk_window_set_default_icon_name("gtk-properties");

	/* This instance data. */
	p = g_new0(compa_t, 1);
	p->applet = GTK_WIDGET(applet);

	/* Configuration data. */
	p->gsettings = mate_panel_applet_settings_new(
						MATE_PANEL_APPLET(p->applet),
						COMPA_SCHEMA);

	/* Load config. */
	load_config(p);

	/* GUI builder. */
	builder = gtk_builder_new_from_file(PKGDATADIR "/compa.glade");
	gtk_builder_connect_signals(builder, p);

	/* Identify widgets of interest. */
	for (i = 0; idtable[i].id; i++) {
		GtkWidget **w = (GtkWidget **) ((char *) p + idtable[i].offset);
		*w = GTK_WIDGET(gtk_builder_get_object(builder, idtable[i].id));
	}

	/* Set-up client area. */
	mate_panel_applet_set_flags(MATE_PANEL_APPLET(p->applet),
				    MATE_PANEL_APPLET_EXPAND_MINOR);
	gtk_container_add(GTK_CONTAINER(p->applet), p->compa_frame);
	g_object_unref(G_OBJECT(builder));

	/* Add a CSS to the applet "frame". */
	context = gtk_widget_get_style_context(p->compa_frame);
	p->frame_css = gtk_css_provider_new();
	gtk_style_context_add_provider(context,
				       GTK_STYLE_PROVIDER(p->frame_css),
				       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	/* Display configured data. */
	applet_configure(p);

	/* Complete configure dialog with current configuration data. */
	load_config_dialog_data(p);

	/* Popup menu. */
	init_popup_menu(applet, p);

	g_signal_connect_swapped(G_OBJECT(applet), "change-orient",
			 G_CALLBACK(handle_orientation), p);
	g_signal_connect(G_OBJECT(applet), "destroy",
			 G_CALLBACK(compa_destroy), p);
	gtk_widget_show_all(GTK_WIDGET(p->applet));
}


/*
 *  Applet factory
 */
static gboolean
applet_factory(MatePanelApplet *applet, const gchar *iid, gpointer user_data)
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
