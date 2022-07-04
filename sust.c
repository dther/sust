#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define BUFSIZE 255
#define LENGTH(X) (int)(sizeof (X) / sizeof (X[0]))
#define USAGE \
"usage: sust [COMMAND]\ncommands:\n log\n edit \n edith\n ask\n help\n todo\n"

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

enum STATUS {
	S_UNDEF = 0,
	S_Y = 'y',
	S_N = 'n',
	S_S = 's'
};

void init_tm(void);
void fprint_date(FILE* stream, struct tm *date);
int parse_log(FILE* log);
void print_date(struct tm *date);
void print_habit(int index);
int find_habit(char *habit);
int find_log_index(struct tm *date);
int is_comment(const char *line);
int is_same_date(struct tm *x, struct tm *y);
int split_log_line(char *line, char *fields[3]);

/* user settings */
#include "config.h"
#define MAXDAYS (MONTHS + 1) * 31

struct hlog {
	struct tm due;
	enum STATUS log[MAXDAYS];
};

/* global variables */
/* see time.h(0p) for info on struct tm */
struct tm datetoday = {0}; /* today's date */
struct tm datecutoff = {0}; /* ignore entries older than this */
struct tm datevisible = {0}; /* start of visible entries */
struct hlog hlogs[LENGTH(habits)];
int debug = 0; /* Increases verbosity. Probably going to be removed. */

void print_to_log(struct tm* date, int habit, enum STATUS entry, FILE *logfile)
{
	/* Print habit log entry to file */
	static int firstprint = 1;

	if (firstprint) {
		/* Print a newline for our batch of new habits.
		 * habitctl does this to make manual inspections
		 * of the logfile easier. */
		firstprint = 0;
		fprintf(logfile, "\n");
	}

	fprint_date(logfile, date);
	fprintf(logfile, "\t%s\t%c\n", habits[habit].task, entry);
}

void ask_entries(int index, struct tm *date, FILE *logfile)
{
	char sel, buf[BUFSIZE];
	int i;
	/* ask for entries on each habit that isn't accounted for */
	for (i = 0; i < LENGTH(habits); i++) {
		if (hlogs[i].log[index] != S_UNDEF) {
			continue;
		}
		print_habit(i);
		printf("%s", " [y/n/s/C]? ");
		fgets(buf, BUFSIZE, stdin);
		sel = tolower(buf[0]);
		switch (sel) {
			case S_Y:
			case S_N:
			case S_S:
				print_to_log(date, i, sel, logfile);
				hlogs[i].log[index] = sel;
				break;
			default:
				printf("%s\n", "No entry added.");
				break;
		}
	}
}

int is_missing_entries(int index)
{
	int i;
	for (i = 0; i < LENGTH(habits); i++) {
		if (!hlogs[i].log[index]) {
			return 1;
		}
	}
	return 0;
}

void ask_tasks(void)
{
	/* If there are missing hlog entries
	 * in the past (askdays) days, ask about their completion. */
	struct tm startask = datetoday;
	struct tm tmp = {0};
	int logstart, i;
	FILE* logfile = fopen(logpath, "a");

	if (!logfile) {
		fprintf(stderr, "%s%s%s\n",
			"ERR: Failed to open ", logpath, "for appending.");
		goto fail;
	}

	startask.tm_mday -= askdays - 1;
	tmp = startask;
	if ((logstart = find_log_index(&startask)) == -1) {
		goto fail;
	}

	for (i = 0; i < askdays; i++) {
		mktime(&tmp);
		if (is_missing_entries(logstart + i)) {
			print_date(&tmp);
			printf("%s", ":\n");
			ask_entries(logstart + i, &tmp, logfile);
		}
		tmp.tm_mday++;
	}

fail:
	fclose(logfile);
}

int find_habit(char *habit)
{
	int i;
	for (i = 0; i < LENGTH(habits); i++) {
		if (strcmp(habit, habits[i].task) == 0) {
			return i;
		}
	}
	return -1;
}

int find_log_index(struct tm *date)
{
	struct tm tmp = datecutoff;
	int out = 0;

	while (!is_same_date(date, &tmp)) {
		out++;
		tmp.tm_mday++;
		if (out >= MAXDAYS) {
			/* failed to find */
			fprintf(stderr, "%s\n",
"ERR: Internal log math error! Is askdays set to a number less than 1?");
			return -1;
		}
	}

	return out;
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

	/* Don't display the oldest month, it's only there for context. */
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

int insert_hlog_entry(char *habit, struct tm *date, char complete)
{
	int hi, li = 0;
	enum STATUS status;

	if ((hi = find_habit(habit)) == -1) {
		/* ignore invalid habit */
		/* we might have just removed it from the list */
		return 0;
	}

	if ((li = find_log_index(date)) < 0) {
		/* Math error. Constant in config.h may be set wrongly. */
		return 1;
	}

	status = tolower(complete);
	switch (status) {
		case S_Y:
		case S_S:
			hlogs[hi].due = *date;
			hlogs[hi].due.tm_mday += habits[hi].freq;
			/* fallthrough */
		case S_N:
			hlogs[hi].log[li] = status;
			break;
		default:
			/* Invalid entry character. */
			return -1;
	}

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
	 * approriate index of hlog[habit]. */
	/* that is, the number of days after datecutoff, 
	 * meaning that entries on datecutoff are index 0. */
	if (insert_hlog_entry(fields[1], &date, fields[2][0])) {
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

void fprint_date(FILE *stream, struct tm *date)
{
	/* print ISO date to a specified stream */
	char buf[BUFSIZE];
	strftime(buf, BUFSIZE, "%Y-%m-%d", date);
	fprintf(stream, buf);
}

void print_date(struct tm *date)
{
	fprint_date(stdout, date);
}

void print_habit(int habit)
{
	/* print a chart of this habit's record */
	int i, index = 0;
	char compstat = 'n';
	const char* toprint;
	struct tm currentdate = datecutoff;
	struct tm due = {0};

	printf("%s", habits[habit].task);
	for (i = strlen(habits[habit].task); i <= tabstop; i++) {
		putchar(' ');
	}

	while (mktime(&currentdate) <= mktime(&datetoday)) {
		/* FIXME: lotsa complex logic here */
		switch (hlogs[habit].log[index]) {
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
				/* if this is the due date,
				 * print alert symbol */
				if (habits[habit].freq > 0 &&
						is_same_date(&currentdate,
							&due)) {
					toprint = alert;
				} else {
					toprint = unknown;
				}
		}
		if (mktime(&currentdate) >= mktime(&datevisible)) {
			printf("%s", toprint);
		}
		currentdate.tm_mday++;
		index++;
	}

	/* At the end of the log,
	 * print alert if the date today is a due date */
	if (habits[habit].freq > 0 && is_same_date(&due, &datetoday)) {
		printf(" %s", alert);
	}
	putchar('\n');
}

int total_non_optional_habits(void) {
	/* Returns the number of habits which are non-optional
	 * (i.e. freq > 0) */
	static int total = 0;
	int i;
	if (total > 0) {
		return total;
	}

	for (i = 0; i < LENGTH(habits); i++) {
		if (habits[i].freq > 0) {
			total++;
		}
	}
	return total;
}

void print_ramp(int heat)
{
	const float rampthresh = 1.0/LENGTH(ramp);
	const char* toprint = ramp[0];
	float heatfrac = (heat * 1.0)/total_non_optional_habits();
	int i;

	for (i = 0; i < LENGTH(ramp); i++) {
		if ((rampthresh * i) >= heatfrac) {
			break;
		}
		toprint = ramp[i];
	}

	printf("%s", toprint);
}

void print_heat(void)
{
	/* print the appropriate ramp[] character as according
	 * to the percentage of tasks that were not overdue on this day. */
	/* disincludes habits with frequency 0 */
	struct tm satisfied[LENGTH(habits)] = {0};
	struct tm currentdate = datecutoff;
	int dindex = 0;
	int heat, i;

	for (i = 0; i <= tabstop; i++)
		putchar(' ');
	
	while (mktime(&currentdate) <= mktime(&datetoday)) {
		heat = 0;
		for (i = 0; i < LENGTH(habits); i++) {
			if (habits[i].freq < 1) {
				/* Don't include habits which are optional */
				continue;
			}
			switch (hlogs[i].log[dindex]) {
				case 'y':
				case 's':
					heat++;
					satisfied[i] = currentdate;
					satisfied[i].tm_mday += habits[i].freq;
					break;
				case 'n':
				default:
					if (mktime(&currentdate)
						< mktime(&satisfied[i])) {
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
	int i;
	print_date(&datetoday);
	putc('\n', stdout);

	print_heat();

	for (i = 0; i < LENGTH(habits); i++) {
		print_habit(i);
	}
}

int split_log_line(char *line, char *fields[3])
{
	int i;
	char* tmp = NULL;

	fields[0] = strtok_r(line, "\t", &tmp);
	for (i = 1; i < 3; i++) {
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
	FILE *logfile = fopen(logpath, "r");

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

	if (!logfile) {
		fprintf(stderr, "No logfile detected! Please create it.\n");
		return EXIT_FAILURE;
	}
	/* init */
	init_tm();
	parse_log(logfile);
	fclose(logfile);

	switch (cmd) {
		case C_ASK:
			ask_tasks();
			/* fallthrough */
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
			/* fallthrough */
		case C_HELP:
			fprintf(stderr, USAGE);
	}

	return EXIT_FAILURE;
}

