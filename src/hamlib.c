#include "hamlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>

void bailout(const char *msg);

int sock_readline(int sockd, char *message, size_t bufsize)
{
	int len=0, pos=0;
	char c='\0';

	if (message!=NULL) {
		message[bufsize-1]='\0';
	}

	do {
		len = recv(sockd, &c, 1, MSG_WAITALL);
		if (len <= 0) {
			break;
		}
		if (message!=NULL) {
			message[pos]=c;
			message[pos+1]='\0';
		}
		pos+=len;
	} while (c!='\n' && pos<bufsize-2);

	return pos;
}

rotctld_error rotctld_connect(const char *rotctld_host, const char *rotctld_port, rotctld_info_t *ret_info)
{
	struct addrinfo hints, *servinfo, *servinfop;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int rotctld_socket = 0;
	int retval = getaddrinfo(rotctld_host, rotctld_port, &hints, &servinfo);
	if (retval != 0) {
		return ROTCTLD_GETADDRINFO_ERR;
	}

	for(servinfop = servinfo; servinfop != NULL; servinfop = servinfop->ai_next) {
		if ((rotctld_socket = socket(servinfop->ai_family, servinfop->ai_socktype,
			servinfop->ai_protocol)) == -1) {
			continue;
		}
		if (connect(rotctld_socket, servinfop->ai_addr, servinfop->ai_addrlen) == -1) {
			close(rotctld_socket);
			continue;
		}

		break;
	}
	if (servinfop == NULL) {
		return ROTCTLD_CONNECTION_FAILED;
	}
	freeaddrinfo(servinfo);
	/* TrackDataNet() will wait for confirmation of a command before sending
	   the next so we bootstrap this by asking for the current position */
	send(rotctld_socket, "p\n", 2, 0);
	sock_readline(rotctld_socket, NULL, 256);

	ret_info->socket = rotctld_socket;
	ret_info->connected = true;
	strncpy(ret_info->host, rotctld_host, MAX_NUM_CHARS);
	strncpy(ret_info->port, rotctld_port, MAX_NUM_CHARS);
	ret_info->tracking_horizon = 0;

	ret_info->update_time_interval = 0;
	ret_info->prev_cmd_time = 0;
	ret_info->prev_cmd_azimuth = 0;
	ret_info->prev_cmd_elevation = 0;
	ret_info->first_cmd_sent = false;

	return ROTCTLD_NO_ERR;
}

const char *rotctld_error_message(rotctld_error errorcode)
{
	switch (errorcode) {
		case ROTCTLD_NO_ERR:
			return "No error.";
		case ROTCTLD_GETADDRINFO_ERR:
			return "getaddrinfo error.";
		case ROTCTLD_CONNECTION_FAILED:
			return "Unable to connect to rotctld.";
		case ROTCTLD_SEND_FAILED:
			return "Unable to send to rotctld.";
	}
	return "Unsupported error code.";
}

void rotctld_fail_on_errors(rotctld_error errorcode)
{
	if (errorcode != ROTCTLD_NO_ERR) {
		bailout(rotctld_error_message(errorcode));
		exit(-1);
	}
}

void rotctld_set_tracking_horizon(rotctld_info_t *info, double horizon)
{
	info->tracking_horizon = horizon;
}

void rotctld_set_update_interval(rotctld_info_t *info, int time_interval)
{
	if (time_interval >= 0) {
		info->update_time_interval = time_interval;
	}
}

bool angles_differ(double prev_angle, double angle)
{
	return (int)round(prev_angle) != (int)round(angle);
}

bool rotctld_directions_differ(rotctld_info_t *info, double azimuth, double elevation)
{
	bool azimuth_differs = angles_differ(info->prev_cmd_azimuth, azimuth);
	bool elevation_differs = angles_differ(info->prev_cmd_elevation, elevation);
	return azimuth_differs || elevation_differs;
}

rotctld_error rotctld_track(rotctld_info_t *info, double azimuth, double elevation)
{
	time_t curr_time = time(NULL);
	bool use_update_interval = (info->update_time_interval > 0);
	bool coordinates_differ = rotctld_directions_differ(info, azimuth, elevation);

	if (!info->first_cmd_sent) {
		coordinates_differ = true;
		info->first_cmd_sent = true;
	}

	//send when coordinates differ or when a update interval has been specified
	if ((coordinates_differ && !use_update_interval) || (use_update_interval && ((curr_time - info->update_time_interval) >= info->prev_cmd_time))) {
		info->prev_cmd_azimuth = azimuth;
		info->prev_cmd_elevation = elevation;
		info->prev_cmd_time = curr_time;

		char message[30];

		/* If positions are sent too often, rotctld will queue
		   them and the antenna will lag behind. Therefore, we wait
		   for confirmation from last command before sending the
		   next. */
		sock_readline(info->socket, message, sizeof(message));

		sprintf(message, "P %.2f %.2f\n", azimuth, elevation);
		int len = strlen(message);
		if (send(info->socket, message, len, 0) != len) {
			return ROTCTLD_SEND_FAILED;
		}
	}

	return ROTCTLD_NO_ERR;
}

rigctld_error rigctld_send_message(int socket, char *message)
{
	int len;
	len = strlen(message);
	if (send(socket, message, len, 0) != len) {
		return RIGCTLD_SEND_FAILED;
	}
	return RIGCTLD_NO_ERR;
}

rigctld_error rigctld_bootstrap_response(int socket)
{
	char message[256];
	sprintf(message, "f\n");
	return rigctld_send_message(socket, message);
}

rigctld_error rigctld_get_current_vfo(rigctld_info_t *info, int string_buffer_length, char *current_vfo);

rigctld_error rigctld_get_vfo_names(rigctld_info_t *info, string_array_t *vfo_names);

rigctld_error rigctld_connect(const char *rigctld_host, const char *rigctld_port, rigctld_info_t *ret_info)
{
	struct addrinfo hints, *servinfo, *servinfop;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int rigctld_socket = 0;
	int retval = getaddrinfo(rigctld_host, rigctld_port, &hints, &servinfo);
	if (retval != 0) {
		return RIGCTLD_GETADDRINFO_ERR;
	}

	for(servinfop = servinfo; servinfop != NULL; servinfop = servinfop->ai_next) {
		if ((rigctld_socket = socket(servinfop->ai_family, servinfop->ai_socktype,
			servinfop->ai_protocol)) == -1) {
			continue;
		}
		if (connect(rigctld_socket, servinfop->ai_addr, servinfop->ai_addrlen) == -1) {
			close(rigctld_socket);
			continue;
		}

		break;
	}
	if (servinfop == NULL) {
		return RIGCTLD_CONNECTION_FAILED;
	}
	freeaddrinfo(servinfo);
	/* FreqDataNet() will wait for confirmation of a command before sending
	   the next so we bootstrap this by asking for the current frequency */
	rigctld_error ret_err = rigctld_bootstrap_response(rigctld_socket);
	if (ret_err != RIGCTLD_NO_ERR) {
		return ret_err;
	}

	ret_info->socket = rigctld_socket;
	ret_info->connected = true;
	strncpy(ret_info->host, rigctld_host, MAX_NUM_CHARS);
	strncpy(ret_info->port, rigctld_port, MAX_NUM_CHARS);

	return RIGCTLD_NO_ERR;
}

rigctld_error rigctld_send_vfo_command(int socket, const char *vfo_name)
{
	if (strlen(vfo_name) > 0)	{
		char message[256];
		sprintf(message, "V %s\n", vfo_name);
		usleep(100); // hack: avoid VFO selection racing

		rigctld_error ret_err = rigctld_send_message(socket, message);
		if (ret_err != RIGCTLD_NO_ERR) {
			return ret_err;
		}
		sock_readline(socket, message, sizeof(message));
	}
	return RIGCTLD_NO_ERR;
}

rigctld_error rigctld_set_frequency(const rigctld_info_t *info, double frequency)
{
	char message[256];

	/* If frequencies is sent too often, rigctld will queue
	   them and the radio will lag behind. Therefore, we wait
	   for confirmation from last command before sending the
	   next. */
	sock_readline(info->socket, message, sizeof(message));

	rigctld_error ret_err = rigctld_send_vfo_command(info->socket, info->vfo_name);
	if (ret_err != RIGCTLD_NO_ERR) {
		return ret_err;
	}

	sprintf(message, "F %.0f\n", frequency*1000000);
	return rigctld_send_message(info->socket, message);
}

void rigctld_fail_on_errors(rigctld_error errorcode)
{
	if (errorcode != RIGCTLD_NO_ERR) {
		bailout(rigctld_error_message(errorcode));
		exit(-1);
	}
}

const char *rigctld_error_message(rigctld_error errorcode)
{
	switch (errorcode) {
		case RIGCTLD_NO_ERR:
			return "No error.";
		case RIGCTLD_GETADDRINFO_ERR:
			return "getaddrinfo error.";
		case RIGCTLD_CONNECTION_FAILED:
			return "Unable to connect to rigctld.";
		case RIGCTLD_SEND_FAILED:
			return "Unable to send to rigctld.";
	}
	return "Unsupported error code.";
}

double rigctld_read_frequency(const rigctld_info_t *info)
{
	char message[256];
	double freq;

	//read pending return message
	sock_readline(info->socket, message, sizeof(message));

	rigctld_send_vfo_command(info->socket, info->vfo_name);

	sprintf(message, "f\n");
	rigctld_send_message(info->socket, message);

	sock_readline(info->socket, message, sizeof(message));
	freq=atof(message)/1.0e6;

	//prepare new pending reply
	rigctld_bootstrap_response(info->socket);

	return freq;
}

rigctld_error rigctld_get_current_vfo(rigctld_info_t *info, int string_buffer_length, char *current_vfo)
{
	char message[256];

	//pending message
	sock_readline(info->socket, message, sizeof(message));

	sprintf(message, "v\n");
	rigctld_error ret_err = rigctld_send_message(info->socket, message);
	if (ret_err != RIGCTLD_NO_ERR) {
		return ret_err;
	}

	sock_readline(info->socket, message, sizeof(message));
	strncpy(current_vfo, message, string_buffer_length);

	//prepare new pending reply
	return rigctld_bootstrap_response(info->socket);
}

rigctld_error rigctld_get_vfo_names(rigctld_info_t *info, string_array_t *vfo_names)
{
	char message[256];

	//pending return message
	sock_readline(info->socket, message, sizeof(message));

	//get VFO names
	sprintf(message, "V ?\n");
	rigctld_error ret_err = rigctld_send_message(info->socket, message);
	if (ret_err != RIGCTLD_NO_ERR) {
		return ret_err;
	}

	sock_readline(info->socket, message, sizeof(message));

	//split names in output
	for (int i=0; i < strlen(message); i++) {
		if ((message[i] == ' ') || (message[i] == '\n')) {
			message[i] = ':';
		}
	}
	stringsplit(message, vfo_names);

	//prepare new pending reply
	return rigctld_bootstrap_response(info->socket);
}

rigctld_error rigctld_set_vfo(rigctld_info_t *ret_info, const char *vfo_name)
{
	strncpy(ret_info->vfo_name, vfo_name, MAX_NUM_CHARS);
	return RIGCTLD_NO_ERR;
}

void rigctld_disconnect(rigctld_info_t *info)
{
	if (info->connected) {
		send(info->socket, "q\n", 2, 0);
		close(info->socket);
		info->connected = false;
	}
}

void rotctld_disconnect(rotctld_info_t *info)
{
	if (info->connected) {
		send(info->socket, "q\n", 2, 0);
		close(info->socket);
		info->connected = false;
	}
}
