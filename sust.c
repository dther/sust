#define _POSIX_C_SOURCE 2000809L
#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 255
#define LENGTH(X) (int)(sizeof (X) / sizeof (X[0]))
#define USAGE "USAGE:\n\tsust [command]\nCommands:\n\tnone, right now.\n"

struct habit {
	int freq;
	char *task;
};

enum COMMAND {
	C_UNDEF,
	C_LOG,
	C_EDIT,
	C_EDITH,
	C_ASK,
	C_HELP,
	C_TODO
};

void init_tm(void);
int parse_log(FILE* log);
void print_date(struct tm *date);
int find_habit(char *habit);
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
char habitlog[LENGTH(habits)][MAXDAYS];
int debug = 0; /* Increases verbosity. Probably going to be removed. */

void ask_tasks(void)
{
	/* If there are missing habitlog entries
	 * in the past (askdays) days, ask about their completion. */
	int dindex = 0;
	struct tm tmp = datecutoff;
	struct tm startask = datetoday;
	startask.tm_mday -= askdays - 1;

	/* Find which index... */
	while (mktime(&tmp) < mktime(&startask)) {
		dindex++;
		tmp.tm_mday++;
	}
	
	/* TODO: actually ask */
	printf("%s", "Asking about the following days:\n");
	for (int i = 0; i < askdays; i++) {
		mktime(&tmp);
		print_date(&tmp);
		putchar('\n');
		tmp.tm_mday++;
	}
}

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

	/* insert an entry in the 
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

	/* TODO: check for file errors */
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

void print_ramp(int heat)
{
	const float rampthresh = 1.0/LENGTH(ramp);
	const char* toprint = ramp[0];
	float heatfrac = (heat * 1.0)/LENGTH(habits);

	for (int i = 0; i < LENGTH(ramp); i++) {
		toprint = ramp[i];
		if ((rampthresh * i) >= heatfrac) {
			break;
		}
	}

	printf("%s", toprint);
}

void print_heat(void)
{
	/* TODO: print the appropriate ramp[] character as according
	 * to the percentage of tasks that were not overdue on this day. */
	struct tm habitdue[LENGTH(habits)] = {0};
	struct tm currentdate = datecutoff;
	int dindex = 0;
	int heat;

	for (int i = 0; i <= tabstop; i++)
		putchar(' ');
	
	while (mktime(&currentdate) <= mktime(&datetoday)) {
		heat = 0;
		for (int i = 0; i < LENGTH(habits); i++) {
			switch (habitlog[i][dindex]) {
				case 'y':
				case 's':
					heat++;
					habitdue[i] = currentdate;
					habitdue[i].tm_mday += habits[i].freq;
					break;
				case 'n':
				default:
					if (mktime(&currentdate)
						< mktime(&habitdue[i])) {
						heat++;
					}
			}
		}

		if (mktime(&currentdate) >= mktime(&datevisible)) {
			print_ramp(heat);
		}
		currentdate.tm_mday++;
		dindex++;
	}
	putchar('\n');
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
	enum COMMAND cmd = C_ASK;
	char* cmdarg;
	// FILE* habitfile = fopen("habitfile.test", "r");
	//FILE* logfile;
	//FILE *logfile = fopen("habitfile.test", "r");
	FILE *logfile = fopen("test/sim.test", "r");
	//FILE *logfile = fopen("test/habitfile.test", "r");

	/* TODO: implement help and version -h -v using getopt(3) */
	/* TODO: more intelligent subcommand finding */
	if (argc > 1) {
		cmdarg = argv[1];
		if (strcmp(cmdarg, "log") == 0) {
			cmd = C_LOG;
		} else if (strcmp(cmdarg, "help") == 0) {
			cmd = C_HELP;
		} else if (strcmp(cmdarg, "ask") == 0) {
			cmd = C_ASK;
		} else if (strcmp(cmdarg, "todo") == 0) {
			cmd = C_TODO;
		} else if (strcmp(cmdarg, "edit") == 0) {
			cmd = C_EDIT;
		} else if (strcmp(cmdarg, "edith") == 0) {
			cmd = C_EDITH;
		} else {
			cmd = C_UNDEF;
		}
	}

	/* init */
	init_tm();
	parse_log(logfile);
	fclose(logfile);

	switch (cmd) {
		case C_ASK:
			/* Should ask for task status, then... */
			ask_tasks();
		case C_LOG:
			print_log();
			exit(EXIT_SUCCESS);
		case C_TODO:
		case C_EDIT:
		case C_EDITH:
			fprintf(stderr, "ERR: Unimplemented.\n");
			break;
		case C_UNDEF:
			fprintf(stderr, "Invalid usage.\n");
			fprintf(stderr, USAGE);
			break;
		case C_HELP:
			fprintf(stderr, USAGE);
	}

	return EXIT_FAILURE;
}

