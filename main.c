#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int percentage = 101;
double timeout = 0.5;

static void usage(const char *program_name) {
	fprintf(stderr,
			"Usage:\n"
			"  %s -p 100 -t 1.5  to show 100%% for 1.5 sec,\n"
			"  %s -v             for version information,\n"
			"  %s                for default behavior\n"
			,
			program_name, program_name, program_name);
	exit(1);
}



int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("albatpop-"VERSION);
			exit(0);
		}
		else if (i + 1 == argc)  /* the next arguments take values */
			usage(argv[0]);
		else if (!strcmp(argv[i], "-p"))
			percentage = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-t"))
			timeout = atof(argv[++i]);
		else
			usage(argv[0]);
	}
	return 0;
}
