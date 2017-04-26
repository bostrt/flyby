#ifndef FLYBY_HAMLIB_H_DEFINED
#define FLYBY_HAMLIB_H_DEFINED

#include "defines.h"
#include <stdbool.h>
#include <time.h>

#define ROTCTLD_DEFAULT_HOST "localhost"
#define ROTCTLD_DEFAULT_PORT "4533\0\0"
#define RIGCTLD_UPLINK_DEFAULT_HOST "localhost"
#define RIGCTLD_UPLINK_DEFAULT_PORT "4532\0\0"
#define RIGCTLD_DOWNLINK_DEFAULT_HOST "localhost"
#define RIGCTLD_DOWNLINK_DEFAULT_PORT "4532\0\0"

//Default rotor angle precision at 1 degree
#define ROTCTLD_DEFAULT_PRECISION 1

typedef struct {
	///Whether we are connected to a rotctld instance
	bool connected;
	///Socket file identificator
	int socket;
	///Hostname
	char host[MAX_NUM_CHARS];
	///Port
	char port[MAX_NUM_CHARS];
	///Time interval for rotctld update. 0 means that commands will be sent only when angles change. Defaults to 0.
	int update_time_interval;
	///Horizon above which we start tracking. Defaults to 0.
	double tracking_horizon;
	///Previous time at which command was sent
	time_t prev_cmd_time;
	///Previous sent azimuth
	double prev_cmd_azimuth;
	///Previous sent elevation
	double prev_cmd_elevation;
	///How much angles should differ before we send an update to rotctld
	double angle_precision;
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
 *  - Input azi/ele differs from previously sent azi/ele more than the angle precision
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
 * Set current rotor precision in degrees.
 **/
void rotctld_set_precision(rotctld_info_t *info, double precision);

/**
 * Connect to rigctld. 
 *
 * \param hostname Hostname/IP address
 * \param port Port
 * \param vfo_name VFO name
 * \param ret_info Returned rigctld connection instance
 **/
void rigctld_connect(const char *hostname, const char *port, const char *vfo_name, rigctld_info_t *ret_info);

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
void rigctld_set_frequency(const rigctld_info_t *info, double frequency);

/**
 * Read frequency from rigctld. 
 *
 * \param info rigctld connection instance
 * \return Current frequency in MHz
 **/
double rigctld_read_frequency(const rigctld_info_t *info);

#endif
