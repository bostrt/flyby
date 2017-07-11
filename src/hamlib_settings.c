#include <stdlib.h>
#include <string.h>
#include "hamlib.h"
#include "ui.h"
#include "defines.h"
#include <form.h>

#define HAMLIB_SETTINGS_WINDOW_ROW 0
#define HAMLIB_SETTINGS_WINDOW_COL 0

#define HAMLIB_SETTINGS_FIELD_WIDTH 12
#define HAMLIB_SETTINGS_FIELD_HEIGHT 1

#define SPACING 1


//TODO:
//* Remove static fields from structs.
//* Free memory
//* Doc
//* Better code reuse across rigctl and rotctl forms
//* Cleanup hamlib.c/.h: Common error codes? Position reading makes bootstrapping redundant, but still need it, rethink this.
//* Handle hamlib errors in singletrack
//* Smaller quickaccess windows elsewhere?
//* Access to settings from singletrack
//* Read/write convenient settings from/to files

enum field_type {
	TITLE_FIELD,
	DESCRIPTION_FIELD,
	FREE_ENTRY_FIELD,
	DEFAULT_FIELD
};


FIELD *field(enum field_type field_type, int row, int col, const char *content)
{
	int field_width = HAMLIB_SETTINGS_FIELD_WIDTH;
	if (field_type == TITLE_FIELD) {
		field_width = strlen(content);
	}

	FIELD *field = new_field(HAMLIB_SETTINGS_FIELD_HEIGHT, field_width, row, col*(HAMLIB_SETTINGS_FIELD_WIDTH + SPACING), 0, 0);

	int field_attributes = 0;
	switch (field_type) {
		case TITLE_FIELD:
			field_attributes = A_BOLD;
			field_opts_off(field, O_ACTIVE);
			break;
		case DESCRIPTION_FIELD:
			field_attributes = FIELDSTYLE_DESCRIPTION;
			field_opts_off(field, O_ACTIVE);
			break;
		case FREE_ENTRY_FIELD:
			field_attributes = FIELDSTYLE_INACTIVE;
			break;
		default:
			field_opts_off(field, O_ACTIVE);
			break;
	}
	set_field_back(field, field_attributes);

	if (content != NULL) {
		set_field_buffer(field, 0, content);
	}

	return field;
}


struct rotctld_form {
	//FIXME: Reduce FIELDs listed here to those that will be manipulated.
	FORM *form;
	FIELD *title;
	FIELD *host_description;
	FIELD *host;
	FIELD *port_description;
	FIELD *port;
	FIELD *tracking_horizon_description;
	FIELD *tracking_horizon;
	FIELD *update_time_description;
	FIELD *update_time;
	FIELD *connection_status;
	FIELD *aziele_description;
	FIELD *aziele;
	FIELD **field_array;
	int last_row;
	WINDOW *window;
};

int rotctld_form_update_time(struct rotctld_form *form)
{
	char *update_time_string = strdup(field_buffer(form->update_time, 0));
	trim_whitespaces_from_end(update_time_string);
	int update_time = atoi(update_time_string);
	free(update_time_string);
	return update_time;
}

double rotctld_form_horizon(struct rotctld_form *form)
{
	char *horizon_string = strdup(field_buffer(form->tracking_horizon, 0));
	trim_whitespaces_from_end(horizon_string);
	double horizon = strtod(horizon_string, NULL);
	free(horizon_string);
	return horizon;
}

#define ROTOR_FORM_TITLE "Rotor"

struct rotctld_form * rotctld_form_prepare(rotctld_info_t *rotctld, WINDOW *window)
{
	struct rotctld_form *form = (struct rotctld_form *) malloc(sizeof(struct rotctld_form));
	form->window = window;

	int row = 0;
	int col = 0;

	form->title = field(TITLE_FIELD, row, col++, ROTOR_FORM_TITLE);
	col += 2;
	form->connection_status = field(DEFAULT_FIELD, row, col++, NULL);

	row += 2;
	col = 0;
	form->host_description = field(DESCRIPTION_FIELD, row, col++, "Host");
	form->port_description = field(DESCRIPTION_FIELD, row, col++, "Port");
	form->tracking_horizon_description = field(DESCRIPTION_FIELD, row, col++, "Horizon");
	form->aziele_description = field(DESCRIPTION_FIELD, row, col++, "Azi   Ele");
	row++;
	col = 0;

	form->host = field(FREE_ENTRY_FIELD, row, col++, rotctld->host);
	form->port = field(FREE_ENTRY_FIELD, row, col++, rotctld->port);

	char tracking_horizon_str[MAX_NUM_CHARS];
	snprintf(tracking_horizon_str, MAX_NUM_CHARS, "%f", rotctld->tracking_horizon);
	form->tracking_horizon = field(FREE_ENTRY_FIELD, row, col++, tracking_horizon_str);



	form->aziele = field(DEFAULT_FIELD, row, col++, NULL);

	row += 2;
	col = 0;
	form->update_time_description = field(DESCRIPTION_FIELD, row, col++, "Update time");

	row++;
	col = 0;

	char update_time_str[MAX_NUM_CHARS];
	snprintf(update_time_str, MAX_NUM_CHARS, "%d", rotctld->update_time_interval);
	form->update_time = field(FREE_ENTRY_FIELD, row, col++, update_time_str);

	#define NUM_ROTCTLD_FIELDS 12
	form->field_array = calloc(NUM_ROTCTLD_FIELDS+1, sizeof(FIELD*));
	FIELD *fields[NUM_ROTCTLD_FIELDS+1] = {form->title, form->connection_status,
		form->host_description, form->host, form->port_description, form->port, form->tracking_horizon_description, form->tracking_horizon, form->update_time_description, form->update_time, form->aziele_description, form->aziele, 0};
	memcpy(form->field_array, fields, sizeof(FIELD*)*NUM_ROTCTLD_FIELDS);


	form->form = new_form(form->field_array);
	int rows, cols;
	scale_form(form->form, &rows, &cols);
	set_form_win(form->form, window);
	set_form_sub(form->form, derwin(window, rows, cols, 0, 2));
	post_form(form->form);
	form_driver(form->form, REQ_VALIDATION);

	box(window, 0, 0);
	set_field_buffer(form->title, 0, ROTOR_FORM_TITLE);

	form->last_row = row;

	return form;
}

#define CONNECTED_STYLE COLOR_PAIR(9)
#define DISCONNECTED_STYLE COLOR_PAIR(5)
#define CONNECTING_STYLE COLOR_PAIR(10)

void set_connection_field(FIELD *field, bool connected)
{
	if (connected) {
		set_field_buffer(field, 0, "Connected");
		set_field_back(field, CONNECTED_STYLE);
	} else {
		set_field_buffer(field, 0, "Disconnected");
		set_field_back(field, DISCONNECTED_STYLE);
	}
}

void set_connection_attempt(FIELD *field)
{
	set_field_buffer(field, 0, "Connecting...");
	set_field_back(field, CONNECTING_STYLE);
}

void rotctld_form_attempt_reconnection(struct rotctld_form *form, rotctld_info_t *rotctld)
{
	char *host_field = strdup(field_buffer(form->host, 0));
	trim_whitespaces_from_end(host_field);
	char *port_field = strdup(field_buffer(form->port, 0));
	trim_whitespaces_from_end(port_field);
	rotctld_disconnect(rotctld);
	set_connection_attempt(form->connection_status);
	form_driver(form->form, REQ_VALIDATION);
	wrefresh(form->window);
	rotctld_connect(host_field, port_field, rotctld);
	set_connection_field(form->connection_status, rotctld->connected);
	free(host_field);
	free(port_field);
}

void rotctld_form_update(rotctld_info_t *rotctld, struct rotctld_form *form)
{
	//read current azimuth/elevation from rotctld
	char aziele_string[MAX_NUM_CHARS] = "N/A   N/A";
	if (rotctld->connected) {
		float azimuth = 0, elevation = 0;
		rotctld_error ret_err = rotctld_read_position(rotctld, &azimuth, &elevation);
		if (ret_err == ROTCTLD_NO_ERR) {
			snprintf(aziele_string, MAX_NUM_CHARS, "%3.0f   %3.0f", azimuth, elevation);
		}
	}
	set_field_buffer(form->aziele, 0, aziele_string);

	set_connection_field(form->connection_status, rotctld->connected);

	double horizon = rotctld_form_horizon(form);
	rotctld_set_tracking_horizon(rotctld, horizon);

	int update_time = rotctld_form_update_time(form);
	rotctld_set_update_interval(rotctld, update_time);
}

struct rigctld_form {
	FORM *form;

	FIELD *title;
	FIELD *connection_status;
	FIELD *host_description;
	FIELD *host;
	FIELD *port_description;
	FIELD *port;
	FIELD *vfo_description;
	FIELD *vfo;
	FIELD *frequency_description;
	FIELD *frequency;

	FIELD **field_array;
	WINDOW *window;

	int last_row;
};

char *rigctld_form_vfo(struct rigctld_form *form)
{
	char *vfo_field = strdup(field_buffer(form->vfo, 0));
	trim_whitespaces_from_end(vfo_field);
	return vfo_field;
}


void rigctld_form_attempt_reconnection(struct rigctld_form *form, rigctld_info_t *rigctld)
{
	char *host_field = strdup(field_buffer(form->host, 0));
	trim_whitespaces_from_end(host_field);
	char *port_field = strdup(field_buffer(form->port, 0));
	trim_whitespaces_from_end(port_field);
	rigctld_disconnect(rigctld);
	set_connection_attempt(form->connection_status);
	form_driver(form->form, REQ_VALIDATION);
	wrefresh(form->window);
	rigctld_connect(host_field, port_field, rigctld);
	set_connection_field(form->connection_status, rigctld->connected);
	free(host_field);
	free(port_field);
}

struct rigctld_form * rigctld_form_prepare(const char *title, rigctld_info_t *rigctld, WINDOW *window)
{
	int row = 0;
	int col = 0;

	struct rigctld_form *form = (struct rigctld_form *) malloc(sizeof(struct rigctld_form));
	form->window = window;

	form->title = field(TITLE_FIELD, row, col++, title);
	col += 2;
	form->connection_status = field(DEFAULT_FIELD, row, col++, "N/A");

	row += 2;
	col = 0;
	form->host_description = field(DESCRIPTION_FIELD, row, col++, "Host");
	form->port_description = field(DESCRIPTION_FIELD, row, col++, "Port");
	form->vfo_description = field(DESCRIPTION_FIELD, row, col++, "VFO");
	form->frequency_description = field(DESCRIPTION_FIELD, row, col++, "Frequency");
	row++;
	col = 0;
	form->host = field(FREE_ENTRY_FIELD, row, col++, rigctld->host);
	form->port = field(FREE_ENTRY_FIELD, row, col++, rigctld->port);
	form->vfo = field(FREE_ENTRY_FIELD, row, col++, rigctld->vfo_name);
	form->frequency = field(DEFAULT_FIELD, row, col++, "N/A");


	#define NUM_RIGCTLD_FIELDS 10
	form->field_array = calloc(NUM_RIGCTLD_FIELDS+1, sizeof(FIELD*));
	FIELD *fields[NUM_RIGCTLD_FIELDS+1] = {form->title, form->connection_status, form->host_description, form->host, form->port_description, form->port, form->vfo_description, form->vfo, form->frequency_description, form->frequency, 0};
	memcpy(form->field_array, fields, sizeof(FIELD*)*NUM_RIGCTLD_FIELDS);

	form->form = new_form(form->field_array);
	int rows, cols;
	scale_form(form->form, &rows, &cols);
	set_form_win(form->form, window);
	set_form_sub(form->form, derwin(window, rows, cols, 0, 2));
	post_form(form->form);
	form_driver(form->form, REQ_VALIDATION);

	box(window, 0, 0);
	set_field_buffer(form->title, 0, title);
	form->last_row = row;

	return form;
}

void rigctld_form_update(rigctld_info_t *rigctld, struct rigctld_form *form)
{
	char frequency_string[MAX_NUM_CHARS] = "N/A";
	if (rigctld->connected) {
		double frequency;
		rigctld_error ret_err = rigctld_read_frequency(rigctld, &frequency);
		if (ret_err == RIGCTLD_NO_ERR) {
			snprintf(frequency_string, MAX_NUM_CHARS, "%.3f MHz\n", frequency);
		}
	}
	set_field_buffer(form->frequency, 0, frequency_string);

	set_connection_field(form->connection_status, rigctld->connected);

	char *vfo = rigctld_form_vfo(form);
	rigctld_set_vfo(rigctld, vfo);
	free(vfo);
}


#define RIGCTLD_SETTINGS_WINDOW_HEIGHT 6
#define ROTCTLD_SETTINGS_WINDOW_HEIGHT 9
#define SETTINGS_WINDOW_WIDTH (HAMLIB_SETTINGS_FIELD_WIDTH*4 + 7)
#define WINDOW_SPACING 1

int rownumber(FIELD *field)
{
	int rows, cols, frow, fcol, nrow, nbuf;
	field_info(field, &rows, &cols, &frow, &fcol, &nrow, &nbuf);
	return frow;
}

#define OFFSET 4

//field_status kan brukes til å sjekke om noe er blitt modda
void hamlib_settings(rotctld_info_t *rotctld, rigctld_info_t *downlink, rigctld_info_t *uplink)
{
	clear();
	refresh();

	attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);
	mvprintw(0,0,"                                                                                ");
	mvprintw(1,0,"  flyby Hamlib Settings                                                         ");
	mvprintw(2,0,"                                                                                ");


	WINDOW *rotctld_window = newwin(ROTCTLD_SETTINGS_WINDOW_HEIGHT, SETTINGS_WINDOW_WIDTH, OFFSET, HAMLIB_SETTINGS_WINDOW_COL);
	WINDOW *downlink_window = newwin(RIGCTLD_SETTINGS_WINDOW_HEIGHT, SETTINGS_WINDOW_WIDTH, OFFSET+ROTCTLD_SETTINGS_WINDOW_HEIGHT+WINDOW_SPACING, HAMLIB_SETTINGS_WINDOW_COL);
	WINDOW *uplink_window = newwin(RIGCTLD_SETTINGS_WINDOW_HEIGHT, SETTINGS_WINDOW_WIDTH, OFFSET+ROTCTLD_SETTINGS_WINDOW_HEIGHT + RIGCTLD_SETTINGS_WINDOW_HEIGHT + 2*WINDOW_SPACING, HAMLIB_SETTINGS_WINDOW_COL);

	halfdelay(HALF_DELAY_TIME);
	struct rigctld_form *downlink_form = rigctld_form_prepare("Downlink", downlink, downlink_window);
	struct rigctld_form *uplink_form = rigctld_form_prepare("Uplink", uplink, uplink_window);
	struct rotctld_form *rotctld_form = rotctld_form_prepare(rotctld, rotctld_window);


	#define NUM_FORMS 3
	int curr_form_index = 0;
	FORM *forms[NUM_FORMS] = {rotctld_form->form, downlink_form->form, uplink_form->form};
	int last_rows[NUM_FORMS] = {rotctld_form->last_row, downlink_form->last_row, uplink_form->last_row};

	FIELD *curr_field = current_field(forms[curr_form_index]);
	set_field_back(curr_field, FIELDSTYLE_ACTIVE);

	bool should_run = true;
	while (should_run) {
		FORM *curr_form = forms[curr_form_index];

		rigctld_form_update(downlink, downlink_form);
		rigctld_form_update(uplink, uplink_form);
		rotctld_form_update(rotctld, rotctld_form);

		wrefresh(uplink_window);
		wrefresh(downlink_window);
		wrefresh(rotctld_window);
		int key = getch();

		switch (key) {
			case KEY_UP:
				form_driver(curr_form, REQ_UP_FIELD);

				//jump to previous form if at start of form
				if (rownumber(current_field(curr_form)) == last_rows[curr_form_index]) {
					curr_form_index--;
					if (curr_form_index < 0) {
						curr_form_index = 0;
					}
				}
				break;
			case KEY_DOWN:
				//jump to next form if at end of current form
				if (rownumber(current_field(curr_form)) == last_rows[curr_form_index]) {
					curr_form_index++;
					if (curr_form_index >= NUM_FORMS) {
						curr_form_index = NUM_FORMS-1;
					}
				} else {
					form_driver(curr_form, REQ_DOWN_FIELD);
				}
				break;
			case KEY_LEFT:
				form_driver(curr_form, REQ_LEFT_FIELD);
				break;
			case KEY_RIGHT:
				form_driver(curr_form, REQ_RIGHT_FIELD);
				break;
			case 10:
				if (curr_form == rotctld_form->form) {
					rotctld_form_attempt_reconnection(rotctld_form, rotctld);
				} else if (curr_form == downlink_form->form) {
					rigctld_form_attempt_reconnection(downlink_form, downlink);
				} else if (curr_form == uplink_form->form) {
					rigctld_form_attempt_reconnection(uplink_form, uplink);
				}
				break;
			case KEY_BACKSPACE:
				form_driver(curr_form, REQ_DEL_PREV);
				form_driver(curr_form, REQ_VALIDATION);
				break;
			case 27:
				should_run = false;
				break;
			default:
				form_driver(curr_form, key);
				form_driver(curr_form, REQ_VALIDATION);
				break;
		}

		//highlight current active field
		FIELD *curr_field_candidate = current_field(forms[curr_form_index]);
		if (curr_field_candidate != curr_field) {
			set_field_back(curr_field, FIELDSTYLE_INACTIVE);
			set_field_back(curr_field_candidate, FIELDSTYLE_ACTIVE);
			curr_field = curr_field_candidate;
		}


	}

	delwin(uplink_window);
	delwin(downlink_window);
}
