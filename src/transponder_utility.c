#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include "defines.h"
#include <math.h>
#include "string_array.h"
#include "qth_config.h"
#include "tle_db.h"
#include "xdg_basedirs.h"
#include "transponder_db.h"
#include <libgen.h>
#include "option_help.h"
#include <math.h>

/**
 * Print differences between transponder database entries to terminal.
 *
 * \param old_db_entry Old database entry
 * \param new_db_entry New database entry
 **/
void print_transponder_entry_differences(const struct sat_db_entry *old_db_entry, const struct sat_db_entry *new_db_entry);

int main(int argc, char **argv)
{
	string_array_t transponder_db_filenames = {0}; //TLE files to be used to update the TLE databases
	bool force_changes = false;
	bool ignore_changes = false;
	bool silent_mode = false;

	//command line options
	struct option_extended options[] = {
		{{"add-transponder-file",	required_argument,	0,	'a'},
			"FILE", "Add transponder entries from specified transponder database FILE to flyby's transponder database. Ignores entries for which no corresponding TLE exists in the TLE database."},
		{{"force-changes",		no_argument,		0,	'f'},
			NULL, "Accept all database changes. The program will otherwise ask the user whether changes should be accepted or not."},
		{{"ignore-changes",		no_argument,		0,	'i'},
			NULL, "Add all new database entries but ignore any changes to existing entries"},
		{{"help",			no_argument,		0,	'h'},
			NULL, "Display help"},
		{{"silent",			no_argument,		0,	's'},
			NULL, "Silent mode, print verbose output only when encountering a change to an existing entry and -f or -i are not enabled"},
		{{0, 0, 0, 0}, NULL, NULL}
	};
	struct option *long_options = extended_to_longopts(options);
	char short_options[] = "a:fish";
	char usage_instructions[MAX_NUM_CHARS];
	snprintf(usage_instructions, MAX_NUM_CHARS, "Flyby transponder database utility\n\nUsage: %s [OPTIONS]", argv[0]);

	//parse options
	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, short_options, long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'a': //transponder file
				string_array_add(&transponder_db_filenames, optarg);
				break;
			case 'f': //force changes
				force_changes = true;
				break;
			case 'i': //ignore changes
				ignore_changes = true;
				break;
			case 's': //silent mode
				silent_mode = true;
				break;
			case 'h': //help
				getopt_long_show_help(usage_instructions, options, short_options);
				return 0;
				break;
		}
	}

	//read TLE database
	struct tle_db *tle_db = tle_db_create();
	tle_db_from_search_paths(tle_db);
	if (tle_db->num_tles == 0) {
		fprintf(stderr, "No TLEs defined.\n");
	}

	//read current transponder database
	struct transponder_db *transponder_db = transponder_db_create(tle_db);
	transponder_db_from_search_paths(tle_db, transponder_db);

	//get transponders from input database file
	for (int i=0; i < string_array_size(&transponder_db_filenames); i++) {
		const char *filename = string_array_get(&transponder_db_filenames, i);
		struct transponder_db *file_db = transponder_db_create(tle_db);
		if (transponder_db_from_file(filename, tle_db, file_db, LOCATION_TRANSIENT) != TRANSPONDER_SUCCESS) {
			if (!silent_mode) fprintf(stderr, "Could not read file: %s\n", filename);
			continue;
		}

		//compare entries
		for (int j=0; j < file_db->num_sats; j++) {
			struct sat_db_entry *new_db_entry = &(file_db->sats[j]);
			struct sat_db_entry *old_db_entry = &(transponder_db->sats[j]);
			if (!transponder_db_entry_empty(new_db_entry) && !transponder_db_entry_equal(old_db_entry, new_db_entry)) {
				if (transponder_db_entry_empty(old_db_entry)) {
					//add new entry
					if (!silent_mode) fprintf(stderr, "Adding new transponder entries to %s\n", tle_db->tles[j].name);
					transponder_db_entry_copy(old_db_entry, new_db_entry);
				} else if (!ignore_changes) {
					//update existing entry
					if (!silent_mode) fprintf(stderr, "Updating transponder entries for %s:\n", tle_db->tles[j].name);
					bool do_update = false;
					if (!force_changes) {
						//prompt user for acceptance
						print_transponder_entry_differences(old_db_entry, new_db_entry);
						fprintf(stderr, "Accept change for %s? (y/n) ", tle_db->tles[j].name);
						while (true) {
							int c = getchar();
							if (c == 'y') {
								do_update = true;
								break;
							} else if (c == 'n') {
								break;
							}
						}
					} else {
						do_update = true;
					}
					if (do_update) {
						transponder_db_entry_copy(old_db_entry, new_db_entry);
					}
				}
			}
		}

		transponder_db_destroy(&file_db);
	}

	//update user file
	transponder_db_write_to_default(tle_db, transponder_db);

	//free memory
	tle_db_destroy(&tle_db);
	transponder_db_destroy(&transponder_db);
	free(long_options);
}

void print_transponder_entry_differences(const struct sat_db_entry *old_db_entry, const struct sat_db_entry *new_db_entry)
{
	for (int i=0; i < fmax(old_db_entry->num_transponders, new_db_entry->num_transponders); i++) {
		struct transponder transponder_new = new_db_entry->transponders[i];
		struct transponder transponder_old = old_db_entry->transponders[i];

		if (transponder_empty(old_db_entry->transponders[i])) {
			fprintf(stderr, "New entry: %s, %f->%f, %f->%f\n", transponder_new.name, transponder_new.uplink_start, transponder_new.uplink_end, transponder_new.downlink_start, transponder_new.downlink_end);
			continue;
		}

		if (strncmp(transponder_old.name, transponder_new.name, MAX_NUM_CHARS) != 0) {
			fprintf(stderr, "Names differ: `%s` -> `%s`\n", transponder_old.name, transponder_new.name);
		}

		double old_value = transponder_old.uplink_start;
		double new_value = transponder_new.uplink_start;
		if (old_value != new_value) fprintf(stderr, "Uplink start differs: %f -> %f\n", old_value, new_value);

		old_value = transponder_old.uplink_end;
		new_value = transponder_new.uplink_end;
		if (old_value != new_value) fprintf(stderr, "Uplink end differs: %f -> %f\n", old_value, new_value);

		old_value = transponder_old.downlink_start;
		new_value = transponder_new.downlink_start;
		if (old_value != new_value) fprintf(stderr, "Downlink start differs: %f -> %f\n", old_value, new_value);

		old_value = transponder_old.downlink_end;
		new_value = transponder_new.downlink_end;
		if (old_value != new_value) fprintf(stderr, "Downlink end differs: %f -> %f\n", old_value, new_value);
	}
}
