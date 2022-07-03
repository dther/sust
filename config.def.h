/* Configuration header file for sust */

/* max no. of months to display. One extra month is recorded but hidden,
 * in order to provide context for the earliest printed month. */
#define MONTHS 2
static const char* logpath = "test/sim.test";
static const struct habit habits[] = {
	/* frequency, habit */
	{ 2, "foo" },
	{ 3, "bar"},
	{ 4, "baz"},
};
/* How many days ago to ask for new records, if they don't exist. */
static const int askdays = 7;

/* AESTHETIC SETTINGS */

/* "ramp" is used for the heatmap, going from low to high.
 * A multibyte sequence can be used,
 * but will be considered one character
 * for the purposes of column calculation.
 * Looks best if there's one character for each habit. */
/* Example ramps:
 * .oO0Oo.
 * _.,-='"~
 * .-+*#@
 * UTF-8 or ANSI escape sequences can also be used,
 * and will be treated as 1 character. */
static const char* ramp[] = {".", "o", "O"};

/* "This task has been completed/was completed recently" */
static const char* yes[] = {"x", "-"};
/* "This task was skipped/is being held off for the moment" */
static const char* skip[] = {"o", "."};
/* "This is the last day this task can be done before becoming overdue" */
static const char* alert = "!";
/* Completion status is unknown */
static const char* unknown = "_";

/* Max columns, task names may be truncated to fit */
//static const int columns = 80;
/* how much space to reserve for tasks.
 * pro tip: set this to longest habit + 1 */
static const int tabstop = 16;
