#define _POSIX_C_SOURCE 2000809L
#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 255
#define LENGTH(X) (int)(sizeof (X) / sizeof (X[0]))

struct habit {
	int freq;
	char *task;
};

int get_longest_freq(void);
void init_tm(void);
int is_comment(const char *line);
int split_log_line(char *line, char *fields[3]);

/* user settings */
#include "config.h"
#define MAXDAYS (MONTHS + 1) * 31

/* global variables */
/* see time.h(0p) for info on struct tm */
struct tm datetoday = {0}; /* today's date */
struct tm datecutoff = {0}; /* ignore entries older than this */
struct tm datevisible = {0}; /* start of visible entries */
char habitlog[LENGTH(habits)][MAXDAYS]; /* FIXME: SCREAM */
int debug = 0;

int find_habit(char *habit)
{
	for (int i = 0; i < LENGTH(habits); i++) {
		if (strcmp(habit, habits[i].task) == 0) {
			return i;
		}
	}
	return -1;
}

void init_tm(void)
{
	/* set global "broken time" structs to midnight of appropriate date */
	/* set datetoday to current local time */
	time_t now = time(NULL);
	localtime_r(&now, &datetoday);
	datetoday.tm_sec = 0;
	datetoday.tm_min = 0;
	datetoday.tm_hour = 0;

	/* set cutoff to first of the farthest month in the past */
	datecutoff.tm_mon = datetoday.tm_mon;
	datecutoff.tm_year = datetoday.tm_year;
	datecutoff.tm_mday = 1;
	datecutoff.tm_mon -= MONTHS;

	datevisible = datecutoff;
	datevisible.tm_mon++;

	/* Fix broken fields (see mktime(3)) */
	mktime(&datetoday);
	mktime(&datecutoff);
	mktime(&datevisible);
}

int is_comment(const char *line)
{
	/* NULL, empty line, or starts with '#'. */
	return (!line || line[0] == '#' || strcspn(line, "\n") == 0 );
}

int is_same_date(struct tm *x, struct tm *y)
{
	return (mktime(x) == mktime(y));
}

int is_too_old(struct tm *date)
{
	return (mktime(&datecutoff) > mktime(date));
}

int insert_habitlog_entry(char *habit, struct tm *date, char complete)
{
	int hindex, lindex = 0;
	struct tm current = datecutoff;

	if ((hindex = find_habit(habit)) == -1) {
		/* invalid habit */
		return 1;
	}

	while (!is_same_date(&current, date)) {
		lindex++;
		current.tm_mday++;
	}

	if (lindex >= MAXDAYS) {
		/* day is definitely in the future */
		printf("hi");
		return 1;
	}

	habitlog[hindex][lindex] = complete;
	return 0;
}

int parse_line(char *line)
{
	char *fields[3];
	struct tm date = {0};

	if (split_log_line(line, fields)) {
		goto fail;
	}

	if (!strptime(fields[0], "%F", &date)) {
		goto fail;
	}

	if (is_too_old(&date)) {
		/* old and irrelevant */
		return 0;
	}

	/* FIXME: insert an entry in the 
	 * approriate index of habitlog[habit]. */
	/* that is, the number of days after datecutoff, 
	 * meaning that entries on datecutoff are index 0. */
	if (insert_habitlog_entry(fields[1], &date, fields[2][0])) {
		goto fail;
	};

	return 0;
fail:
	/* Line was invalid */
	return 1;
}

int parse_log(FILE *log)
{
	char *buf = malloc(sizeof(char) * BUFSIZE);
	int lineno = 0;

	while (fgets(buf, BUFSIZE, log) && !feof(log)) {
		/* parse */
		lineno++;
		if (is_comment(buf)) {
			continue;
		} else if (parse_line(buf)) {
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
	int index = 0;
	char compstat = 'n';
	const char* toprint;
	struct tm currentdate = datecutoff;
	struct tm due = {0};

	printf("%s", habits[habit].task);
	for (int i = strlen(habits[habit].task); i <= tabstop; i++) {
		putchar(' ');
	}

	/*
	for (int i = 0; i < MAXDAYS; i++)
		printf("%c", habitlog[habit][i]);
		*/

	while (mktime(&currentdate) <= mktime(&datetoday)) {
		/* FIXME: lotsa complex logic here */
		switch (habitlog[habit][index]) {
			case 'y':
				toprint = yes[0];
				compstat = 'y';
				due = currentdate;
				due.tm_mday += habits[habit].freq;
				break;
			case 's':
				toprint = skip[0];
				compstat = 'n';
				due = currentdate;
				due.tm_mday += habits[habit].freq;
				break;
			case 'n':
				if (mktime(&currentdate) < mktime(&due)) {
					if (compstat == 'y') {
						toprint = yes[1];
					} else {
						toprint = skip[1];
					}
				} else {
					toprint = " ";
				}
				break;
			default:
				/* Entry is missing */
				toprint = "?";
		}
		if (mktime(&currentdate) >= mktime(&datevisible)) {
			printf("%s", toprint);
		}
		currentdate.tm_mday++;
		index++;
	}

	putchar('\n');
}

void print_heat(void)
{
	/* TODO: print the appropriate ramp[] character as according
	 * to the percentage of tasks that were not overdue on this day. */
	char habitvalid[LENGTH(habits)] = {0};
	struct tm habitdue[LENGTH(habits)] = {0};
	struct tm currentdate = datecutoff;
	int index = 0;
	//const float rampthreshold = 1.0/LENGTH(ramp);
}

void print_log(void)
{
	print_date(&datetoday);
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

int main(int argc, char** argv)
{
	// FILE* habitfile = fopen("habitfile.test", "r");
	//FILE* logfile;
	//FILE *logfile = fopen("habitfile.test", "r");
	FILE *logfile = fopen("test/sim.test", "r");
	//FILE *logfile = fopen("test/habitfile.test", "r");

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
		strftime(buf, BUFSIZE, "%F", &datetoday);
		fprintf(stderr, "today is %s\n", buf);
		strftime(buf, BUFSIZE, "%F", &datecutoff);
		fprintf(stderr, "ignoring entries made before %s\n", buf);
	}
	parse_log(logfile);
	print_log();

	return EXIT_FAILURE;
}

