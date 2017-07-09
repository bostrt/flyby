#include <stdlib.h>
#include <string.h>
#include "hamlib.h"
#include "ui.h"
#include "defines.h"
#include <form.h>

#define HAMLIB_SETTINGS_WINDOW_ROW 0
#define HAMLIB_SETTINGS_WINDOW_COL 0

#define HAMLIB_SETTINGS_FIELD_WIDTH 10
#define HAMLIB_SETTINGS_FIELD_HEIGHT 1

#define SPACING 3

FIELD *field(int row, int col, const char *content)
{
	FIELD *field = new_field(HAMLIB_SETTINGS_FIELD_HEIGHT, HAMLIB_SETTINGS_FIELD_WIDTH, row, col*(HAMLIB_SETTINGS_FIELD_WIDTH + SPACING), 0, 0);

	if (content != NULL) {
		set_field_buffer(field, 0, content);
	}

	return field;
}


struct rotctld_form {
	FORM *form;
	FIELD *title;
	FIELD *host;
	FIELD *port;
	FIELD *tracking_horizon_description;
	FIELD *tracking_horizon;
	FIELD *update_time_description;
	FIELD *update_time;
	FIELD *connection_status;
	FIELD *azimuth_description;
	FIELD *azimuth;
	FIELD *elevation_description;
	FIELD *elevation;
	FIELD **field_array;
};

struct rotctld_form * rotctld_form_prepare(rotctld_info_t *rotctld, WINDOW *window)
{
	struct rotctld_form *form = (struct rotctld_form *) malloc(sizeof(struct rotctld_form));

	int row = 0;
	int col = 0;

	form->title = field(row, col++, "Rotor controller");
	form->connection_status = field(row, col++, NULL);

	row++;
	col = 0;
	form->host = field(row, col++, rotctld->host);
	form->port = field(row, col++, rotctld->port);

	form->tracking_horizon_description = field(row, col++, "Tracking horizon");

	char tracking_horizon_str[MAX_NUM_CHARS];
	snprintf(tracking_horizon_str, MAX_NUM_CHARS, "%f", rotctld->tracking_horizon);
	form->tracking_horizon = field(row, col++, tracking_horizon_str);

	form->update_time_description = field(row, col++, "Update time interval");
	char update_time_str[MAX_NUM_CHARS];
	snprintf(update_time_str, MAX_NUM_CHARS, "%d", rotctld->update_time_interval);
	form->update_time = field(row, col++, update_time_str);

	form->azimuth_description = field(row, col++, "Azimuth");
	form->elevation_description = field(row, col++, "Elevation");
	form->azimuth = field(row, col++, NULL);
	form->elevation = field(row, col++, NULL);

	#define NUM_ROTCTLD_FIELDS 14
	form->field_array = calloc(NUM_ROTCTLD_FIELDS+1, sizeof(FIELD*));
	FIELD *fields[NUM_ROTCTLD_FIELDS+1] = {form->title, form->connection_status,
		form->host, form->port, form->tracking_horizon_description, form->tracking_horizon, form->update_time_description, form->update_time, form->azimuth_description, form->azimuth, form->elevation_description, form->elevation, 0};
	memcpy(form->field_array, fields, sizeof(FIELD*)*NUM_ROTCTLD_FIELDS);


	form->form = new_form(form->field_array);
	int rows, cols;
	scale_form(form->form, &rows, &cols);
	set_form_win(form->form, window);
	set_form_sub(form->form, derwin(window, rows, cols, 2, 2));
	post_form(form->form);
	form_driver(form->form, REQ_VALIDATION);

	return form;
}

void rotctld_form_update(rotctld_info_t *rotctld, struct rotctld_form *form)
{
	if (rotctld->connected) {
		set_field_buffer(form->connection_status, 0, "Connected");
	} else {
		set_field_buffer(form->connection_status, 0, "Disconnected");
	}

	char azimuth_string[MAX_NUM_CHARS] = "N/A";
	char elevation_string[MAX_NUM_CHARS] = "N/A";
	if (rotctld->connected) {
		float azimuth = 0, elevation = 0;
		rotctld_read_position(rotctld, &azimuth, &elevation);
		snprintf(azimuth_string, MAX_NUM_CHARS, "%f", azimuth);
		snprintf(elevation_string, MAX_NUM_CHARS, "%f", elevation);
	}
	set_field_buffer(form->azimuth, 0, azimuth_string);
	set_field_buffer(form->elevation, 0, elevation_string);
}

struct rigctld_form {
	FORM *form;

	FIELD *title;
	FIELD *connection_status;
	FIELD *host;
	FIELD *port;
	FIELD *vfo_description;
	FIELD *vfo;
	FIELD *frequency_description;
	FIELD *frequency;

	FIELD **field_array;
};

struct rigctld_form * rigctld_form_prepare(const char *title, rigctld_info_t *rigctld, WINDOW *window)
{
	int row = 0;
	int col = 0;

	struct rigctld_form *form = (struct rigctld_form *) malloc(sizeof(struct rigctld_form));

	form->title = field(row, col++, title);
	form->connection_status = field(row, col++, "N/A");

	row++;
	col = 0;
	form->host = field(row, col++, rigctld->host);
	form->port = field(row, col++, rigctld->port);
	form->vfo_description = field(row, col++, "VFO");
	form->vfo = field(row, col++, rigctld->vfo_name);
	form->frequency_description = field(row, col++, "Frequency (MHz)");
	form->frequency = field(row, col++, "N/A");


	#define NUM_RIGCTLD_FIELDS 8
	form->field_array = calloc(NUM_RIGCTLD_FIELDS+1, sizeof(FIELD*));
	FIELD *fields[NUM_RIGCTLD_FIELDS+1] = {form->title, form->connection_status, form->host, form->port, form->vfo_description, form->vfo, form->frequency_description, form->frequency, 0};
	memcpy(form->field_array, fields, sizeof(FIELD*)*NUM_RIGCTLD_FIELDS);

	form->form = new_form(form->field_array);
	int rows, cols;
	scale_form(form->form, &rows, &cols);
	set_form_win(form->form, window);
	set_form_sub(form->form, derwin(window, rows, cols, 2, 2));
	post_form(form->form);
	form_driver(form->form, REQ_VALIDATION);

	return form;
}

void rigctld_form_update(rigctld_info_t *rigctld, struct rigctld_form *form)
{
	char frequency_string[MAX_NUM_CHARS] = "N/A";
	if (rigctld->connected) {
		float frequency = rigctld_read_frequency(rigctld);
		snprintf(frequency_string, MAX_NUM_CHARS, "%f\n", frequency);
	}
	set_field_buffer(form->frequency, 0, frequency_string);

	if (rigctld->connected) {
		set_field_buffer(form->connection_status, 0, "Connected");
	} else {
		set_field_buffer(form->connection_status, 0, "Disconnected");
	}
}

void hamlib_settings(rotctld_info_t *rotctld, rigctld_info_t *downlink, rigctld_info_t *uplink)
{
	WINDOW *rotctld_window = newwin(LINES/3, COLS, 0, HAMLIB_SETTINGS_WINDOW_COL);
	WINDOW *downlink_window = newwin(LINES/3, COLS, LINES/3, HAMLIB_SETTINGS_WINDOW_COL);
	WINDOW *uplink_window = newwin(LINES/3, COLS, 2*LINES/3, HAMLIB_SETTINGS_WINDOW_COL);

	halfdelay(HALF_DELAY_TIME);
	struct rigctld_form *downlink_form = rigctld_form_prepare("Downlink", downlink, downlink_window);
	struct rigctld_form *uplink_form = rigctld_form_prepare("Uplink", uplink, uplink_window);
	struct rotctld_form *rotctld_form = rotctld_form_prepare(rotctld, rotctld_window);

	while (true) {
		rigctld_form_update(downlink, downlink_form);
		rigctld_form_update(uplink, uplink_form);
		rotctld_form_update(rotctld, rotctld_form);

		wrefresh(uplink_window);
		wrefresh(downlink_window);
		wrefresh(rotctld_window);
		char key = getch();
		if (key >= 0) {
			break;
		}
	}

	delwin(uplink_window);
	delwin(downlink_window);
}
