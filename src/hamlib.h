#ifndef FLYBY_HAMLIB_H_DEFINED
#define FLYBY_HAMLIB_H_DEFINED

#include "defines.h"
#include <stdbool.h>
#include <time.h>
#include "string_array.h"

#define ROTCTLD_DEFAULT_HOST "localhost"
#define ROTCTLD_DEFAULT_PORT "4533\0\0"
#define RIGCTLD_UPLINK_DEFAULT_HOST "localhost"
#define RIGCTLD_UPLINK_DEFAULT_PORT "4532\0\0"
#define RIGCTLD_DOWNLINK_DEFAULT_HOST "localhost"
#define RIGCTLD_DOWNLINK_DEFAULT_PORT "4532\0\0"

typedef struct {
	///Whether we are connected to a rotctld instance
	bool connected;
	///Socket file identificator
	int socket;
	///Hostname
	char host[MAX_NUM_CHARS];
	///Port
	char port[MAX_NUM_CHARS];
	///Time interval for rotctld update. Zero means that commands will be sent only when angles change. Defaults to 0.
	int update_time_interval;
	///Horizon above which we start tracking. Defaults to 0.
	double tracking_horizon;
	///Previous time at which command was sent
	time_t prev_cmd_time;
	///Whether first command has been sent, and whether we can guarantee that prev_cmd_azimuth/elevation contain correct values
	bool first_cmd_sent;
	///Previous sent azimuth
	double prev_cmd_azimuth;
	///Previous sent elevation
	double prev_cmd_elevation;
} rotctld_info_t;

typedef struct {
	///Whether we are connected to a rigctld instance
	bool connected;
	///Socket file identificator
	int socket;
	///Hostname
	char host[MAX_NUM_CHARS];
	///Port
	char port[MAX_NUM_CHARS];
	///VFO name
	char vfo_name[MAX_NUM_CHARS];
} rigctld_info_t;

/**
 * Connect to rotctld. 
 *
 * \param hostname Hostname/IP address
 * \param port Port
 * \param ret_info Returned rotctld connection instance
 **/
void rotctld_connect(const char *hostname, const char *port, rotctld_info_t *ret_info);

/**
 * Disconnect from rotctld.
 * \param info Rigctld connection instance
 **/
void rotctld_disconnect(rotctld_info_t *info);

/**
 * Send track data to rotctld.
 *
 * Data is sent only when:
 *  - Input azi/ele differs from previously sent azi/ele
 *  OR
 *  - Difference from previous time for the commands differs more than the time interval for updates
 *
 * \param info rotctld connection instance
 * \param azimuth Azimuth in degrees
 * \param elevation Elevation in degrees
 **/
void rotctld_track(rotctld_info_t *info, double azimuth, double elevation);

/**
 * Set current tracking horizon.
 **/
void rotctld_set_tracking_horizon(rotctld_info_t *info, double horizon);

/**
 * Set current update interval.
 **/
void rotctld_set_update_interval(rotctld_info_t *info, int time_interval);

/**
 * Rigctld-related errors.
 **/
enum rigctld_error_e {
	RIGCTLD_NO_ERR = 0,
	RIGCTLD_GETADDRINFO_ERR = -1,
	RIGCTLD_CONNECTION_FAILED = -2,
	RIGCTLD_SEND_FAILED = -3,
};

typedef enum rigctld_error_e rigctld_error;

/**
 * Make flyby fail and shut down ncurses on rigctld errors.
 **/
void rigctld_fail_on_errors(rigctld_error errorcode);

/**
 * Get error message related to input errorcode.
 **/
const char *rigctld_error_message(rigctld_error errorcode);

/**
 * Connect to rigctld. 
 *
 * \param hostname Hostname/IP address
 * \param port Port
 * \param ret_info Returned rigctld connection instance
 **/
rigctld_error rigctld_connect(const char *hostname, const char *port, rigctld_info_t *ret_info);

/**
 * Set VFO name to be used by this rigctld connection instance. Will not switch VFO in rigctld until set_frequency.
 *
 * \param vfo_name
 **/
rigctld_error rigctld_set_vfo(rigctld_info_t *ret_info, const char *vfo_name);

/**
 * Disconnect from rigctld.
 * \param info Rigctld connection instance
 **/
void rigctld_disconnect(rigctld_info_t *info);

/*
 * Send frequency data to rigctld. 
 *
 * \param info rigctld connection instance
 * \param frequency Frequency in MHz
 **/
rigctld_error rigctld_set_frequency(const rigctld_info_t *info, double frequency);

/**
 * Read frequency from rigctld. 
 *
 * \param info rigctld connection instance
 * \return Current frequency in MHz
 **/
double rigctld_read_frequency(const rigctld_info_t *info);

#endif
