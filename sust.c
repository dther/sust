#define _POSIX_C_SOURCE 2000809L
#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 255
#define LENGTH(X) (sizeof (X) / sizeof (X[0]))

struct habit {
	int freq;
	char *task;
};

struct record {
	struct tm date;
	int habit;
	char complete; /* y, n, or s */
	int valid; /* task can be considered "completed" (i.e., was done today
		      or within the last frequency cycle)
		     0 = invalid, 1 = valid, -1 = skipped */
	struct record *next;
};

void init_tm(void);
int is_comment(const char *line);
int split_log_line(char *line, char *fields[3]);
void validate_habit(int habit);
void validate_habits(void);

/* user settings */
#include "config.h"

/* global variables */
/* see time.h(0p) for info on struct tm */
struct tm _today = {0}; /* today's date */
struct tm _cutoff = {0}; /* ignore entries older than this */
struct record* records[LENGTH(habits)] = {0};
/* Would this be better as a trinary(?) bitmap for each day? Hm. */
int debug = 0;

void init_tm(void)
{
	time_t now = time(NULL);
	localtime_r(&now, &_today);

	/* set cutoff to first of the farthest month in the past */
	_cutoff.tm_mon = _today.tm_mon;
	_cutoff.tm_year = _today.tm_year;
	_cutoff.tm_mday = 1;
	_cutoff.tm_mon -= (months - 1);
	/* normalise (month 0 = december last year) */
	mktime(&_cutoff);
}

void insert_record(struct record *new)
{
	struct record *found;
	new->next = NULL;

	if (!records[new->habit]) {
		records[new->habit] = new;
	} else {
		found = records[new->habit];
		while (found->next != NULL) {
			found = found->next;
		}
		found->next = new;
	}
}

int is_comment(const char *line)
{
	/* NULL, empty line, or starts with '#'. */
	return (!line || line[0] == '#' || strcspn(line, "\n") == 0 );
}

int is_too_old(struct record *record)
{
	if (debug) {
		fprintf(stderr, "%i %i %i %i %i %i %i %i ",
				record->date.tm_sec,
				record->date.tm_min,
				record->date.tm_hour,
				record->date.tm_mon,
				record->date.tm_year,
				record->date.tm_wday,
				record->date.tm_yday,
				record->date.tm_isdst);
		fprintf(stderr, "Cutoff: %li record: %li\n",
				mktime(&_cutoff), mktime(&record->date));
	}
	return (mktime(&_cutoff) > mktime(&record->date));
}

struct record *parse_line(char *line, struct record *out)
{
	char *fields[3];

	split_log_line(line, fields);
	/* strptime doesn't initialise struct tm */
	out->date = _cutoff;
	if (!strptime(fields[0], "%F", &out->date)) {
		goto fail;
	}

	out->habit = -1;
	for (long unsigned int i = 0; i < LENGTH(habits); i++) {
		/* iterate through habits */
		if (strcmp(habits[i].task, fields[1]) == 0) {
			out->habit = i;
			break;
		}
	}

	out->complete = fields[2][0];
	if (out->complete != 'y' && out->complete != 'n'
			&& out->complete != 's') {
		goto fail;
	}

	return out;
fail:
	/* something was malformed/this isn't a valid line */
	return NULL;
}

int parse_log(FILE *log)
{
	/* initialize records array */
	char *buf = malloc(sizeof(char) * BUFSIZE);
	int lineno = 0;
	struct record *current = malloc(sizeof(struct record));

	while (fgets(buf, BUFSIZE, log) && !feof(log)) {
		/* parse */
		lineno++;
		if (is_comment(buf)) {
			continue;
		}
		if (parse_line(buf, current)) {
			if (is_too_old(current)) {
				continue;
			}
			
			if (debug && current->habit == -1) {
				fprintf(stderr,
					"WARN: Invalid habit on line #%i\n",
					lineno);
			}

			insert_record(current);
			/* FIXME: replace this with calloc too */
			current = malloc(sizeof(struct record));
		} else {
			fprintf(stderr, "ERR: Invalid line (#%i)\n", lineno);
		}
	}

	/* FIXME: check for file errors */
	free(buf);
	return 0;
/* ??? */
	/*
fail:
	return 1;
	*/
}

void print_date(struct tm *date)
{
	/* print a given struct tm in the ISO format (%F, or %Y-%m-%d) */
	char buf[BUFSIZE];
	strftime(buf, BUFSIZE, "%F", date);
	printf(buf);
}

void print_habit(int habit)
{
	/* print a chart of this habit's record */
	struct tm current_day = _cutoff;
	struct record *current_record;

	printf("%s", habits[habit].task);
	for (int i = strlen(habits[habit].task); i <= tabstop; i++) {
		putchar(' ');
	}

	while (mktime(&current_day) < mktime(&_today)) {
		/* search records */
		current_record = records[habit];
		while (current_record &&
				mktime(&current_record->date)
					!= mktime(&current_day)) {
			current_record = current_record->next;
		}

		if (current_record) {
			switch (current_record->complete) {
				case 'y':
					printf("%s", yes[0]);
					break;
				case 's':
					printf("%s", skip[0]);
					break;
				default:
					switch (current_record->valid) {
						case 1:
							printf("%s", yes[1]);
							break;
						case -1:
							printf("%s", skip[1]);
							break;
						default:
							putchar(' ');
					}
			}
		} else {
			putchar('?');
		}
		current_day.tm_mday++;
	}
	putchar('\n');
}

void test_valid(void)
{
	struct record *current;
	for (unsigned long i = 0; i < LENGTH(habits); i++) {
		current = records[i];
		while (current) {
			printf("%s record on ", habits[i].task);
			print_date(&current->date);
			printf(" has validity %i\n", current->valid);
			current = current->next;
		}
	}
}

void print_heat(void)
{
	/* TODO: print the appropriate ramp[] character as according
	 * to the percentage of tasks that were "valid" on this day. */
	validate_habits();
	/* TODO: write test code for validate_habits */
	if (debug) {
		test_valid();
	}
}

void print_log(void)
{
	print_date(&_today);
	putc('\n', stdout);

	print_heat();

	for (unsigned long i = 0; i < LENGTH(habits); i++) {
		print_habit(i);
	}
}

int split_log_line(char *line, char *fields[3])
{
	char* tmp = NULL;

	fields[0] = strtok_r(line, "\t", &tmp);
	for (int i = 1; i < 3; i++) {
		if (!(fields[i] = strtok_r(NULL, "\t", &tmp))) {
			/* Not enough fields. */
			return 1;
		}
	}

	return 0;
}

int is_overdue(struct tm* ys_date, struct tm* n_date, int freq)
{
	/* returns 0 if n_date is within freq days of ys_date
	 * 1 if it's more than freq days */
	struct tm due_by = *ys_date;
	due_by.tm_mday += freq - 1;
	return (mktime(&due_by) < mktime(n_date));
}

void validate_habit(int habit)
{
	/* set each record's "valid" member appropriately based on the
	 * last time "y" or "s" was set and habits[habit].freq. */
	/* TODO: TEST THIS HOT MESS */
	struct record* current = records[habit];
	struct record* last_ys = NULL;

	while (current) {
		switch (current->complete) {
			case 'y':
				current->valid = 1;
				last_ys = current;
				break;
			case 's':
				current->valid = -1;
				last_ys = current;
				break;
			case 'n':
			default:
				if (last_ys && !is_overdue(&last_ys->date,
							&current->date,
							habits[habit].freq)) {
					current->valid = last_ys->valid;
				} else {
					current->valid = 0;
				}
				break;

		}
		current = current->next;
	}
}

void validate_habits(void)
{
	for (unsigned long i = 0; i < LENGTH(habits); i++) {
		validate_habit(i);
	}
}

int main(int argc, char** argv)
{
	// FILE* habitfile = fopen("habitfile.test", "r");
	//FILE* logfile;
	//FILE *logfile = fopen("habitfile.test", "r");
	FILE *logfile = fopen("test/sim.test", "r");

	/* FIXME: handle args */
	if (argc > 1) {
		printf("No arguments implemented for %s...\n", argv[0]);
		printf("Assuming you want to debug stuff.\n");

		debug = 1;
	}

	/* init */
	init_tm();

	if (debug) {
		/* FIXME: put this somewhere more reasonable */
		char buf[BUFSIZE];
		strftime(buf, BUFSIZE, "%F", &_today);
		fprintf(stderr, "today is %s\n", buf);
		strftime(buf, BUFSIZE, "%F", &_cutoff);
		fprintf(stderr, "ignoring entries made before %s\n", buf);
	}
	parse_log(logfile);
	print_log();

	return EXIT_FAILURE;
}

