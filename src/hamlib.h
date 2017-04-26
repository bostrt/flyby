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

typedef struct {
	///Whether we are connected to a rotctld instance
	bool connected;
	///Socket file identificator
	int socket;
	///Hostname
	char host[MAX_NUM_CHARS];
	///Port
	char port[MAX_NUM_CHARS];
	///Time interval for rotctld update. 0 means that commands will be sent only when angles change
	int update_time_interval;
	///Horizon above which we start tracking
	double tracking_horizon;
	///Previous time at which command was sent
	time_t prev_cmd_time;
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
 * \param update_interval Time interval for rotctld updates. Set to 0 if rotctld should be updated only when (azimuth, elevation) changes. NOTE: Not used internally in rotctld_functions, used externally in SingleTrack
 * \param tracking_horizon Tracking horizon in degrees. NOTE: Not used internally in rotctld_ functions, used externally in SingleTrack
 * \param ret_info Returned rotctld connection instance
 **/
void rotctld_connect(const char *hostname, const char *port, int update_interval, double tracking_horizon, rotctld_info_t *ret_info);

/**
 * Disconnect from rotctld.
 * \param info Rigctld connection instance
 **/
void rotctld_disconnect(rotctld_info_t *info);

/**
 * Send track data to rotctld. Data is sent when current time, azi and ele fulfill the resolution or time
 * requirements with respect to previous time or coordinates.
 *
 * \param info rotctld connection instance
 * \param azimuth Azimuth in degrees
 * \param elevation Elevation in degrees
 **/
void rotctld_track(rotctld_info_t *info, double azimuth, double elevation);

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
