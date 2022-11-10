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

#define G_SETTINGS_ENABLE_BACKEND

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gsettingsbackend.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#include "config.h"


#define _(s)	gettext(s)
#define N_(s)	s


#define DEFAULT_TEXT	"<span font=\"Italic\" color=\"#00FF00\" "	\
			"bgcolor=\"#333333\">COMPA</span>"
#define ERROR_TEXT	"<span color=\"#FF0000\">Command Error</span>"

#define COMPA_SCHEMA	"org.mate.panel.applet.compa"

#define fieldof(t, p, o)	*((t *) (((char *) (p)) + (o)))
#define boolstring(b)		((b)? "true": "false")


typedef struct {
	gchar *			monitor_command;
	gboolean		monitor_markup;
	gint			update_period;	/* Seconds. */
	gchar *			tooltip_command;
	gboolean		tooltip_markup;
	gchar *			click_command;
	gchar *			background_color;
	gint			frame_type;
	gboolean		frame_maximized;
	gint			padding;
}		compa_config_t;

typedef struct {
	GtkWidget *		applet;		/* Panel applet. */
	GSettings *		gsettings;	/* Configuration settings. */
	GtkCssProvider *	frame_css;	/* CSS for applet "frame". */
	guint			active_monitor;	/* Current active monitor. */
	gchar *			command_dir;	/* Last command directory. */
	gchar *			config_dir;	/* Last config directory. */
	gchar *			config_file;	/* Last saved config file. */

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
	GtkWidget *		file_chooser;
	GtkWidget *		file_load_button;
	GtkWidget *		file_save_button;
	GtkWidget *		error_dialog;
	GtkWidget *		error_label;

	/* Configuration values. */
	compa_config_t		config;
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
	IDENTRY(file_chooser),
	IDENTRY(file_load_button),
	IDENTRY(file_save_button),
	IDENTRY(error_dialog),
	IDENTRY(error_label),
	{NULL, 0}
};


/*
 *  Action click
 */
gboolean
action_click(GtkWidget *widget, GdkEventButton *event, compa_t *p)
{
	compa_config_t *config = &p->config;

	(void) widget;

	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		if (config->click_command[0]) {
			char click_cmd_bg[FILENAME_MAX];

			snprintf(click_cmd_bg, sizeof click_cmd_bg, "%s &",
				 config->click_command);

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
free_config(compa_config_t *config)
{
	g_free(config->monitor_command);
	g_free(config->tooltip_command);
	g_free(config->click_command);
	g_free(config->background_color);
	config->monitor_command = NULL;
	config->tooltip_command = NULL;
	config->click_command = NULL;
	config->background_color = NULL;
}


/*
 *  Load config
 */
static void
load_config(compa_config_t *config, GSettings *g)
{
	config->monitor_command = g_settings_get_string(g, "monitor-command");
	config->monitor_markup = g_settings_get_boolean(g, "monitor-markup");
	config->update_period = g_settings_get_int(g, "update-period");
	config->tooltip_command = g_settings_get_string(g, "tooltip-command");
	config->tooltip_markup = g_settings_get_boolean(g, "tooltip-markup");
	config->click_command = g_settings_get_string(g, "click-command");
	config->frame_type = g_settings_get_enum(g, "frame-type");
	config->frame_maximized = g_settings_get_boolean(g, "frame-maximized");
	config->padding = g_settings_get_int(g, "padding");
	config->background_color = g_settings_get_string(g, "label-color");
}


/*
 *  Save config
 */
static void
commit_config(compa_config_t *config, GSettings *g)
{
	g_settings_set_string(g, "monitor-command", config->monitor_command);
	g_settings_set_boolean(g, "monitor-markup", config->monitor_markup);
	g_settings_set_int(g, "update-period", config->update_period);
	g_settings_set_string(g, "tooltip-command", config->tooltip_command);
	g_settings_set_boolean(g, "tooltip-markup", config->tooltip_markup);
	g_settings_set_string(g, "click-command", config->click_command);
	g_settings_set_enum(g, "frame-type", config->frame_type);
	g_settings_set_boolean(g, "frame-maximized", config->frame_maximized);
	g_settings_set_int(g, "padding", config->padding);
	g_settings_set_string(g, "label-color", config->background_color);
	g_settings_sync();
}


/*
 *  Tooltip update
 */
void
tooltip_update(compa_t *p)
{
	compa_config_t *config = &p->config;

	if (config->tooltip_command[0]) {
		gboolean markup = config->tooltip_markup;
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar tooltip_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(config->tooltip_command, "r"))) {
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
	compa_config_t *config = &p->config;
	GtkAlign al = config->frame_maximized? GTK_ALIGN_FILL: GTK_ALIGN_CENTER;

	gtk_widget_set_halign(p->compa_frame, al);
	gtk_widget_set_valign(p->compa_frame, al);

	if (config->monitor_command[0]) {
		gboolean markup = config->monitor_markup;
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar monitor_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(config->monitor_command, "r"))) {
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


static gchar *
run_file_chooser(compa_t *p, GtkFileChooserAction action, const char *title,
		 char **path, char **basename)
{
	GtkFileChooser *fc = GTK_FILE_CHOOSER(p->file_chooser);
	gchar *filename = NULL;

	if (!*path)
		*path = g_strdup(g_get_home_dir());

	if (basename) {
		if (!*basename)
			*basename = g_strdup("untitled.compa");
	}

	gtk_file_chooser_set_action(fc, action);
	gtk_window_set_title(GTK_WINDOW(fc), title);
	gtk_file_chooser_set_current_folder(fc, *path);

	gtk_widget_set_visible(p->file_load_button,
			       action != GTK_FILE_CHOOSER_ACTION_SAVE);
	gtk_widget_set_visible(p->file_save_button,
			       action == GTK_FILE_CHOOSER_ACTION_SAVE);
	if (basename)
		gtk_file_chooser_set_current_name(fc, *basename);

	if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(fc);
		g_free(*path);
		*path = g_path_get_dirname(filename);
		if (basename) {
			g_free(*basename);
			*basename = g_path_get_basename(filename);
		}
	}

	gtk_widget_hide(p->file_chooser);
	return filename;
}


/*
 *  File open
 */
void
select_command(GtkWidget *widget, compa_t *p)
{
	gchar *filename = run_file_chooser(p, GTK_FILE_CHOOSER_ACTION_OPEN,
					   _("Select command"),
					   &p->command_dir, NULL);

	if (filename) {
		gpointer text_entry = g_object_get_data(G_OBJECT(widget),
							"compa-target");

		gtk_entry_set_text(GTK_ENTRY(text_entry), filename);
		g_free(filename);
	}
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
	compa_config_t *config = &p->config;
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
		gtk_widget_set_margin_top(p->compa_label, config->padding);
		gtk_widget_set_margin_bottom(p->compa_label, config->padding);
	}
	else {
		gtk_widget_set_margin_start(p->compa_label, config->padding);
		gtk_widget_set_margin_end(p->compa_label, config->padding);
	}
}


/*
 *  Configure applet.
 */

static void
applet_configure(compa_t *p)
{
	compa_config_t *config = &p->config;
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
	b = params + config->frame_type;
	css = g_strdup_printf(frame_style_format, config->background_color,
			      b->border_color, b->border_width,
			      b->border_style);
	gtk_css_provider_load_from_data(p->frame_css, css, -1, NULL);
	g_free(css);

	/* Update padding. */
	handle_orientation(p);

	/* Update displayed data. */
	compa_update(p);

	/* Add new monitor. */
	if (config->monitor_command[0] && config->update_period)
		p->active_monitor =
		    g_timeout_add_seconds(config->update_period,
					  (GSourceFunc) compa_update, p);
}


/*
 *  Load configuration data into dialog.
 */
static void
load_config_dialog_data(compa_t *p)
{
	compa_config_t *c = &p->config;
	GdkRGBA color;

	gtk_entry_set_text(GTK_ENTRY(p->monitor_entry),
			   c->monitor_command? c->monitor_command: "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->monitor_markup_check),
				     c->monitor_markup);
	gtk_entry_set_text(GTK_ENTRY(p->tooltip_entry),
			   c->tooltip_command? c->tooltip_command: "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->tooltip_markup_check),
				     c->tooltip_markup);
	gtk_entry_set_text(GTK_ENTRY(p->action_entry),
			   c->click_command? c->click_command: "");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->period_spin),
				  c->update_period);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->padding_spin),
				  c->padding);
	gdk_rgba_parse(&color, c->background_color);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(p->label_color_button),
				  &color);
	gtk_combo_box_set_active(GTK_COMBO_BOX(p->frame_type_combo),
				 c->frame_type);
	gtk_toggle_button_set_active(
	    GTK_TOGGLE_BUTTON(p->frame_maximized_check), c->frame_maximized);
}


/*
 *  Retrieve configure dialog data.
 */
static void
retrieve_config_dialog_data(compa_t *p, compa_config_t *c)
{
	GdkRGBA color;

	free_config(c);

	/* Retrieve monitor command. */
	c->monitor_command = g_strdup(gtk_entry_get_text(
				GTK_ENTRY(p->monitor_entry)));
	c->monitor_markup = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->monitor_markup_check));

	/* Retrieve update period. */
	c->update_period = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(p->period_spin));

	/* Retrieve frame type. */
	c->frame_type = gtk_combo_box_get_active(
				GTK_COMBO_BOX(p->frame_type_combo));
	c->frame_maximized = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->frame_maximized_check));

	/* Retrieve padding. */
	c->padding = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(p->padding_spin));

	/* Retrieve background color. */
	gtk_color_chooser_get_rgba(
		GTK_COLOR_CHOOSER(p->label_color_button), &color);
	c->background_color = gdk_rgba_to_string(&color);

	/* Retrieve tooltip command. */
	c->tooltip_command = g_strdup(gtk_entry_get_text(
				GTK_ENTRY(p->tooltip_entry)));
	c->tooltip_markup = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(p->tooltip_markup_check));

	/* Retrieve click command. */
	c->click_command =
	     g_strdup(gtk_entry_get_text(GTK_ENTRY(p->action_entry)));
}


static void
error(compa_t *p, const char *format, ...)
{
	gchar *text;
	va_list args;

	va_start(args, format);
	text = g_strdup_vprintf(format, args);
	va_end(args);

	gtk_label_set_text(GTK_LABEL(p->error_label), text);
	g_free(text);

	gtk_dialog_run(GTK_DIALOG(p->error_dialog));
	gtk_widget_hide(p->error_dialog);
}


GSettings *
gsettings_file(const char *filename)
{
	GString *path = g_string_new("/");
	GSettings *g = NULL;

	g_string_append(path, COMPA_SCHEMA);
	g_string_append(path, "/");
	g_string_replace(path, ".", "/", 0);
	GSettingsBackend *backend = g_keyfile_settings_backend_new(filename,
	    path->str, "compa");

	if (backend)
		g = g_settings_new_with_backend_and_path(COMPA_SCHEMA,
							 backend, path->str);

	g_string_free(path, TRUE);
	return g;
}


/*
 * Load configuration from file.
 */
void
load_pressed(compa_t *p)
{
	gchar *filename = run_file_chooser(p, GTK_FILE_CHOOSER_ACTION_OPEN,
					   _("Load Compa configuration file"),
					   &p->config_dir, NULL);

	if (filename) {
		GSettings *g = gsettings_file(filename);

		if (!g)
			error(p, _("Cannot load file `%s'"), filename);
		else {
			load_config(&p->config, g);
			load_config_dialog_data(p);
			g_object_unref(g);
		}

		g_free(filename);
	}
}


/*
 * Save configuration to file.
 */
void
save_pressed(compa_t *p)
{
	gchar *filename = run_file_chooser(p, GTK_FILE_CHOOSER_ACTION_SAVE,
					   _("Save Compa configuration file"),
					   &p->config_dir, &p->config_file);

	if (filename) {
		GSettings *g = gsettings_file(filename);

		if (!g)
			error(p, _("Cannot save configuration to file `%s'"),
			      filename);
		else {
			compa_config_t config;

			memset(&config, 0, sizeof config);
			retrieve_config_dialog_data(p, &config);
			commit_config(&config, g);
			g_object_unref(g);
		}

		g_free(filename);
	}
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
		retrieve_config_dialog_data(p, &p->config);
		applet_configure(p);
		commit_config(&p->config, p->gsettings);
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
	/* Remove dialogs. */
	gtk_widget_destroy(p->configure_dialog);
	gtk_widget_destroy(p->error_dialog);
	gtk_widget_destroy(p->file_chooser);
	g_free(p->command_dir);
	g_free(p->config_dir);
	g_free(p->config_file);

	/* Remove an existing monitor. */
	if (p->active_monitor)
		g_source_remove(p->active_monitor);

	if (p->gsettings)
		g_object_unref(p->gsettings);

	/* Remove frame CSS. */
	g_object_unref(p->frame_css);

	free_config(&p->config);
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
	gtk_action_group_set_translation_domain(action_group, PACKAGE_NAME);
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
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");

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
	load_config(&p->config, p->gsettings);

	/* GUI builder. */
	builder = gtk_builder_new_from_file(PKGDATADIR "/compa.glade");
	gtk_builder_connect_signals(builder, p);

	/* Identify widgets of interest. */
	for (i = 0; idtable[i].id; i++) {
		GtkWidget **w = &fieldof(GtkWidget *, p, idtable[i].offset);
		*w = GTK_WIDGET(gtk_builder_get_object(builder, idtable[i].id));
	}

	/* Attach custom data. */
	g_object_set_data(gtk_builder_get_object(builder,
						 "monitor_browse_button"),
			  "compa-target", p->monitor_entry);
	g_object_set_data(gtk_builder_get_object(builder,
						 "tooltip_browse_button"),
			  "compa-target", p->tooltip_entry);
	g_object_set_data(gtk_builder_get_object(builder,
						 "action_browse_button"),
			  "compa-target", p->action_entry);

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


/*
 * Dirty hack to force UTF-8 locale.
 */
#define MATE_PANEL_GETTEXT_DOMAIN	"mate-panel"

#undef _MATE_PANEL_APPLET_SETUP_GETTEXT
#define _MATE_PANEL_APPLET_SETUP_GETTEXT(out)				\
	do { \
		bindtextdomain (MATE_PANEL_GETTEXT_DOMAIN, MATELOCALEDIR); \
		bind_textdomain_codeset (MATE_PANEL_GETTEXT_DOMAIN, "UTF-8"); \
		if (out) {						\
			setlocale(LC_ALL, "");				\
			textdomain (MATE_PANEL_GETTEXT_DOMAIN);		\
		}							\
	} while (0)


/* Main program. */
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY(
	"compaAppletFactory",
	PANEL_TYPE_APPLET,
	"Compa",
	applet_factory,
	NULL
);
