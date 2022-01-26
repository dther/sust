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
	char *task;
	char complete; /* y, n, or s */
	struct record *next;
};

void init_today(void);
int is_comment(char *line);
struct tm *parse_date(char* date, struct tm *target);
struct record *parse_line(char* line);
int print_date_today(void);
int read_log(FILE* logfile);

/* user settings */
#include "config.h"

/* global variables */
struct tm tm_today = {0};
struct record records[LENGTH(habits)];

void init_today(void)
{
	time_t now = time(NULL);
	localtime_r(&now, &tm_today);
}

int is_comment(char *line)
{
	/* NULL, (almost) empty line, or starts with '#'. */
	return (!line || strlen(line) <= 2 || line[0] == '#');
}

int print_date_today(void) {
	char* buf = malloc(sizeof(char) * BUFSIZE);

	strftime(buf, BUFSIZE, "%F", &tm_today);
	printf("%s\n", buf);
	free(buf);
	return 0;
}

struct tm *parse_date(char *date, struct tm *target)
{
	if (!strptime(date, "%F", target)) {
		goto fail;
	}
	return target;
fail:
	return NULL;
}

struct record *parse_line(char *line)
{
	char *tmp, *datebuf, *taskbuf, *compbuf = NULL;
	struct record *record = malloc(sizeof(struct record));
	struct tm *datetm = malloc(sizeof(struct tm));

	if (!(datebuf = strtok_r(line, "\t", &tmp))
			|| !(taskbuf = strtok_r(NULL, "\t", &tmp))
			|| !(compbuf = strtok_r(NULL, "\t", &tmp))) {
		/* TODO: error msg */
		goto fail;
	}

	if (!parse_date(datebuf, &record->date)) {
		/* TODO: error msg */
		goto fail;
	}
	/* TODO: error checking */
	record->task = malloc(sizeof(char) * (strlen(taskbuf) + 1));
	record->complete = compbuf[0];
	return record;
fail:
	/* Line is invalid. */
	free(record);
	free(datetm);
	return NULL;
}

int print_log()
{
	/* Let's do this... */
	/* Need to print:
	 *  x current date 
	 *  ~ X axis - dont bother for now
	 *  - heatmap
	 *  - one line for each habit */

	print_date_today();

	return 0;
}

int read_log(FILE* logfile)
{
	int lineno = 0;
	char *buf = malloc(sizeof(char) * BUFSIZE);
	struct record *currec;

	while (fgets(buf, BUFSIZE, logfile) && !feof(logfile)) {
		/* do smth */
		lineno++;
		if (is_comment(buf)) {
			continue;
		}
		
		if (!(currec = parse_line(buf))) {
			/* TODO: handle error */
			continue;
		}

		for (int i = 0; i < LENGTH(habits); i++) {
			if (strcmp(currec->task, habits[i].task) == 0) {
				/* FIXME: ??? */
				break;
			}
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	// FILE* habitfile = fopen("habitfile.test", "r");
	//FILE* logfile;
	FILE *logfile = fopen("habitfile.test", "r");

	/* init */
	init_today();
	print_log();
	read_log(logfile);

	return EXIT_FAILURE;
}

