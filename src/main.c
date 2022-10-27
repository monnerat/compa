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
	GtkWidget *	applet;		/* Panel applet */
	GtkWidget *	main_label;	/* Display label */
	GtkWidget *	compa_ebox;	/* Event box */
	GtkWidget *	conf_dialog;	/* Configure dialog */
	GtkWidget *	main_frame;

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
}		compa_t;


/*
 *  Action click
 */
gboolean
action_click(GtkWidget *Widget, GdkEventButton *event, compa_t *p)
{
	if (event->type == GDK_BUTTON_PRESS)
		if (event->button == 1) {
			if (p->click_cmd != NULL && p->click_cmd[0]) {
				char click_cmd_bg[FILENAME_MAX];

				sprintf(click_cmd_bg, "%s &",
					p->click_cmd);

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
load_config(compa_t *p)
{
	p->monitor_cmd = g_settings_get_string(p->gsettings, "monitor-command");
	p->update_int = g_settings_get_int(p->gsettings, "update-interval");
	p->tooltip_cmd = g_settings_get_string(p->gsettings, "tooltip-command");
	p->click_cmd = g_settings_get_string(p->gsettings, "click-command");
	p->frame_type = g_settings_get_enum(p->gsettings, "frame-type");
	p->lab_col_str = g_settings_get_string(p->gsettings, "label-color");
	p->padding = g_settings_get_int(p->gsettings, "padding");
	p->use_color = g_settings_get_boolean(p->gsettings, "use-color");
}


/*
 *  Save config
 */
static void
save_config(compa_t *p)
{
	g_settings_set_string(p->gsettings, "monitor-command", p->monitor_cmd);
	g_settings_set_int(p->gsettings, "update-interval", p->update_int);
	g_settings_set_string(p->gsettings, "tooltip-command", p->tooltip_cmd);
	g_settings_set_string(p->gsettings, "click-command", p->click_cmd);
	g_settings_set_enum(p->gsettings, "frame-type", p->frame_type);
	g_settings_set_string(p->gsettings, "label-color", p->lab_col_str);
	g_settings_set_int(p->gsettings, "padding", p->padding);
	g_settings_set_boolean(p->gsettings, "use-color", p->use_color);
	g_settings_sync();
}


/*
 *  Draw background
 */
gboolean
draw_widget_background(GtkWidget *widget, cairo_t *cr, gpointer user_data)

{
	compa_t *p = (compa_t *) user_data;

	if (p->use_color) {
		GdkRGBA lab_color;

		gdk_rgba_parse(&lab_color, p->lab_col_str);
		cairo_set_source_rgb(cr, lab_color.red,
                                     lab_color.green, lab_color.blue);
		cairo_paint(cr);
		}

	return FALSE;
}


/*
 *  Tooltip update
 */
void
tooltip_update(compa_t *p)
{
	if (p->tooltip_cmd && p->tooltip_cmd[0]) {
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar tooltip_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(p->tooltip_cmd, "r"))) {
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

		gtk_widget_set_tooltip_markup(p->compa_ebox, tooltip_cmd_out);
	}
}


/*
 *  Compa update
 */
static gboolean
compa_update(compa_t *p)
{
	if (p->monitor_cmd && p->monitor_cmd[0]) {
		FILE *cmd_pipe;
		unsigned int pipe_char;
		gchar monitor_cmd_out[FILENAME_MAX];
		int i;

		if ((cmd_pipe = popen(p->monitor_cmd, "r"))) {
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
		gtk_label_set_markup(GTK_LABEL(p->main_label),
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
static void
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
		const gchar *fn;

		fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(GTK_ENTRY(text_ent), fn);
	}

	gtk_widget_destroy(dialog);
}

/*
 *  Dialog response
 */
static void
dialog_response(GtkWidget *dialog)
{
	gtk_widget_hide(dialog);
}


static GtkWidget *
new_label(const gchar *str)
{
	GtkWidget *label = gtk_label_new(str);

	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	return label;
}

static void
grid_attach(GtkWidget *grid, GtkWidget *child, int left, int top)
{
	gtk_widget_set_margin_start(child, 2);
	gtk_widget_set_margin_end(child, 2);
	gtk_widget_set_margin_top(child, 2);
	gtk_widget_set_margin_bottom(child, 2);
	gtk_widget_set_hexpand(child, TRUE);
	gtk_widget_set_vexpand(child, FALSE);
	gtk_grid_attach(GTK_GRID(grid), child, left, top, 1, 1);
}

static void
set_margins(GtkWidget *widget, int top, int bottom, int left, int right)
{
	gtk_widget_set_margin_top(widget, top);
	gtk_widget_set_margin_bottom(widget, bottom);
	gtk_widget_set_margin_start(widget, left);
	gtk_widget_set_margin_end(widget, right);
}


/*
 *  Config dialog
 */
static void
config_dialog(GtkAction *action, gpointer user_data)
{
	compa_t *p = (compa_t *) user_data;
	GtkWidget *content_area;
	GtkWidget *monitor_lab;
	GtkWidget *interval_lab;
	GtkWidget *tooltip_lab;
	GtkWidget *action_lab;
	GtkWidget *frame_type_lab;
	GtkWidget *padding_lab;
	GtkWidget *col_check;
	GtkWidget *monitor_ent;
	GtkWidget *tooltip_ent;
	GtkWidget *action_ent;
	GtkWidget *interval_spin;
	GtkWidget *padding_spin;
	GtkWidget *monitor_browse_but;
	GtkWidget *tooltip_browse_but;
	GtkWidget *action_browse_but;
	GtkWidget *frame_type_comb;
	GtkWidget *label_col_but;
	GtkWidget *conf_grid;
	GdkRGBA lab_color;

	if (p->conf_dialog)
		return;

	/* Dialog. */
	p->conf_dialog =
	    gtk_dialog_new_with_buttons(_("Configure"), NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					_("_Cancel"),
					GTK_RESPONSE_CANCEL,
					_("_OK"),
					GTK_RESPONSE_OK,
					NULL);
	g_signal_connect_swapped(p->conf_dialog, "response",
				 G_CALLBACK(dialog_response),
				 p->conf_dialog);

	/* Labels. */
	monitor_lab = new_label(_("Monitor command: "));
	interval_lab = new_label(_("Interval (seconds): "));
	tooltip_lab = new_label(_("Tooltip command: "));
	action_lab = new_label(_("Click action: "));
	frame_type_lab = new_label(_("Shadow type: "));
	padding_lab = new_label(_("Padding: "));

	/* Check button. */
	col_check = gtk_check_button_new_with_label(_("Label background: "));

	/* Text entries. */
	monitor_ent = gtk_entry_new();
	tooltip_ent = gtk_entry_new();
	action_ent = gtk_entry_new();

	if (p->monitor_cmd)
		gtk_entry_set_text(GTK_ENTRY(monitor_ent), p->monitor_cmd);

	if (p->tooltip_cmd)
		gtk_entry_set_text(GTK_ENTRY(tooltip_ent), p->tooltip_cmd);

	if (p->click_cmd)
		gtk_entry_set_text(GTK_ENTRY(action_ent), p->click_cmd);

	/* Spin buttons. */
	interval_spin = gtk_spin_button_new_with_range(1, 86400, 1);
	gtk_widget_set_halign(interval_spin, GTK_ALIGN_START);
	gtk_widget_set_valign(interval_spin, GTK_ALIGN_START);
	padding_spin = gtk_spin_button_new_with_range(0, 10, 1);
	gtk_widget_set_halign(padding_spin, GTK_ALIGN_START);
	gtk_widget_set_valign(padding_spin, GTK_ALIGN_START);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_spin),
				  p->update_int);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(padding_spin), p->padding);

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
	gtk_widget_set_halign(frame_type_comb, GTK_ALIGN_START);
	gtk_widget_set_valign(frame_type_comb, GTK_ALIGN_START);
	gtk_combo_box_set_active(GTK_COMBO_BOX(frame_type_comb), p->frame_type);

	/* Color buttons. */
	label_col_but = gtk_color_button_new();
	gtk_widget_set_halign(label_col_but, GTK_ALIGN_START);
	gtk_widget_set_valign(label_col_but, GTK_ALIGN_START);

	if (p->lab_col_str) {
		gdk_rgba_parse(&lab_color, p->lab_col_str);
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(label_col_but),
					   &lab_color);
	}

	if (p->use_color)
		gtk_toggle_button_set_active(
		    GTK_TOGGLE_BUTTON(col_check), TRUE);

	/* Grid. */
	conf_grid = gtk_grid_new();
	gtk_widget_set_halign(conf_grid, GTK_ALIGN_START);
	gtk_widget_set_valign(conf_grid, GTK_ALIGN_START);
	set_margins(conf_grid, 5, 5, 5, 5);

	/* Monitor command settings. */
	grid_attach(conf_grid, monitor_lab, 0, 0);
	grid_attach(conf_grid, monitor_ent, 1, 0);
	grid_attach(conf_grid, monitor_browse_but, 2, 0);

	/* Interval settings. */
	grid_attach(conf_grid, interval_lab, 0, 1);
	grid_attach(conf_grid, interval_spin, 1, 1);

	/* Frame settings. */
	grid_attach(conf_grid, frame_type_lab, 0, 2);
	grid_attach(conf_grid, frame_type_comb, 1, 2);

	/* Padding settings. */
	grid_attach(conf_grid, padding_lab, 0, 3);
	grid_attach(conf_grid, padding_spin, 1, 3);

	/* Color settings. */
	grid_attach(conf_grid, col_check, 0, 4);
	grid_attach(conf_grid, label_col_but, 1, 4);

	/* Tooltip settings. */
	grid_attach(conf_grid, tooltip_lab, 0, 5);
	grid_attach(conf_grid, tooltip_ent, 1, 5);
	grid_attach(conf_grid, tooltip_browse_but, 2, 5);

	/* Action settings. */
	grid_attach(conf_grid, action_lab, 0, 6);
	grid_attach(conf_grid, action_ent, 1, 6);
	grid_attach(conf_grid, action_browse_but, 2, 6);

	/* Display dialog. */
	content_area =
	    gtk_dialog_get_content_area(GTK_DIALOG(p->conf_dialog));
	gtk_container_add(GTK_CONTAINER(content_area), conf_grid);
	gtk_widget_show_all(p->conf_dialog);

	if (gtk_dialog_run(GTK_DIALOG(p->conf_dialog)) == GTK_RESPONSE_OK) {
		/* Update monitor command. */
		if (p->monitor_cmd)
			g_free(p->monitor_cmd);

		p->monitor_cmd =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(monitor_ent)));

		if (strlen(p->monitor_cmd) < 1)
			gtk_label_set_markup(GTK_LABEL(p->main_label),
					     DEFAULT_TEXT);

		/* Update interval. */
		p->update_int =
		    gtk_spin_button_get_value_as_int(
					GTK_SPIN_BUTTON(interval_spin));

		/* Update frame type. */
		p->frame_type =
		    gtk_combo_box_get_active(GTK_COMBO_BOX(frame_type_comb));
		gtk_frame_set_shadow_type(GTK_FRAME(p->main_frame),
					  p->frame_type);

		/* Update padding. */
		p->padding = gtk_spin_button_get_value_as_int(
						GTK_SPIN_BUTTON(padding_spin));
		set_margins(p->main_label,
			    p->padding, p->padding, p->padding, p->padding);

		/* Update colors. */
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(label_col_but),
					   &lab_color);

		if (p->lab_col_str)
			g_free(p->lab_col_str);

		p->lab_col_str = gdk_rgba_to_string(&lab_color);
		p->use_color = gtk_toggle_button_get_active(
						GTK_TOGGLE_BUTTON(col_check));

		/* Update tooltip command. */
		if (p->tooltip_cmd)
			g_free(p->tooltip_cmd);

		p->tooltip_cmd =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(tooltip_ent)));

		if (strlen(p->tooltip_cmd) < 1)
			gtk_widget_set_tooltip_text(p->compa_ebox, "");

		/* Update click command. */
		if (p->click_cmd)
			g_free(p->click_cmd);

		p->click_cmd =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(action_ent)));

		/* Remove old monitor. */
		if (p->active_mon)
			g_source_remove(p->active_mon);

		/* Add new monitor. */
		if (strlen(p->monitor_cmd) > 0 && p->update_int > 0) {
			p->active_mon =
			    g_timeout_add_seconds(p->update_int,
						  (GSourceFunc) compa_update,
						  p);
			compa_update(p);
		}

		/* Save config. */
		save_config(p);
	}

	/* Remove dialog. */

	gtk_widget_destroy(p->conf_dialog);
	p->conf_dialog = NULL;
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
	/* Remove config dialog if active. */
	if (p->conf_dialog)
		gtk_widget_destroy(GTK_WIDGET(p->conf_dialog));

	/* Remove an existing monitor. */
	if (p->active_mon)
		g_source_remove(p->active_mon);

	if (p->gsettings)
		g_object_unref(p->gsettings);

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
 *  Compa init
 */
static void
compa_init(MatePanelApplet *applet)
{
	compa_t *p;

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
	g_signal_connect(G_OBJECT(applet), "destroy",
			 G_CALLBACK(compa_destroy), p);

	/* Configuration data. */
	p->gsettings = mate_panel_applet_settings_new(
						MATE_PANEL_APPLET(p->applet),
						COMPA_SCHEMA);

	/* Popup menu. */
	init_popup_menu(applet, p);

	/* Label. */
	p->main_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(p->main_label), DEFAULT_TEXT);
	gtk_label_set_justify(GTK_LABEL(p->main_label), GTK_JUSTIFY_CENTER);
	gtk_widget_set_halign(p->main_label, GTK_ALIGN_START);
	gtk_widget_set_valign(p->main_label, GTK_ALIGN_START);

	/* Frame. */
	p->main_frame = gtk_frame_new(NULL);

	/* Event box. */
	p->compa_ebox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(p->compa_ebox), "button_press_event",
			 G_CALLBACK(action_click), p);
	g_signal_connect(G_OBJECT(p->compa_ebox), "draw",
			 G_CALLBACK(draw_widget_background), p);
	g_signal_connect_swapped(G_OBJECT(p->compa_ebox),
				 "enter-notify-event",
				 G_CALLBACK(tooltip_update), p);

	/* Place widgets. */
	gtk_container_add(GTK_CONTAINER(p->compa_ebox), p->main_label);
	gtk_container_add(GTK_CONTAINER(p->main_frame), p->compa_ebox);
	gtk_container_add(GTK_CONTAINER(p->applet), p->main_frame);
	gtk_widget_show_all(GTK_WIDGET(p->applet));

	/* Load config. */
	load_config(p);

	/* Add monitor. */
	if (p->monitor_cmd && p->monitor_cmd[0] && p->update_int > 0) {
		p->active_mon =
		    g_timeout_add_seconds(p->update_int,
					  (GSourceFunc) compa_update,
					  p);
		compa_update(p);
	}

	/* Set frame type. */
	gtk_frame_set_shadow_type(GTK_FRAME(p->main_frame), p->frame_type);

	/* Update padding. */
	set_margins(p->main_label,
		    p->padding, p->padding, p->padding, p->padding);
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
