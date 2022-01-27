/* Configuration header file for sust */

//static const char* logpath = "habitlog.test";
static const struct habit habits[] = {
	/* frequency, habit */
	{ 2, "foo" },
	{ 3, "bar"},
	{ 4, "baz"},
};

/* AESTHETIC SETTINGS */

/* "ramp" is used for the heatmap, going from low to high.
 * A multibyte sequence can be used,
 * but will be considered one character
 * for the purposes of column calculation. */
//static const char* ramp[] = {"0", "1", "2", "3", "4", "5"};

/* "This task has been completed/was completed recently" */
//static const char* done[] = {"X", "-"};
/* "This task was skipped/is being held off for the moment" */
//static const char* skip[] = {"O", "_"};
/* "This is the last day this task can be done before becoming overdue" */
//static const char* alert = "!";

/* Max columns, task names may be truncated to fit */
/* 0 = no max */
//static const int columns = 80;
/* max no. of months to display */
static const int months = 2;
/* how much space to reserve for tasks */
static const int tabstop = 16;
