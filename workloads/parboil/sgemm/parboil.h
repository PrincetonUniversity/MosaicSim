
#include <unistd.h>

/* Command line parameters for benchmarks */
struct pb_Parameters {
  char *outFile;		/* If not NULL, the raw output of the
				 * computation should be saved to this
				 * file. The string is owned. */
  char **inpFiles;		/* A NULL-terminated array of strings
				 * holding the input file(s) for the
				 * computation.  The array and strings
				 * are owned. */
};

/* Read command-line parameters.
 *
 * The argc and argv parameters to main are read, and any parameters
 * interpreted by this function are removed from the argument list.
 *
 * A new instance of struct pb_Parameters is returned.
 * If there is an error, then an error message is printed on stderr
 * and NULL is returned.
 */
struct pb_Parameters *
pb_ReadParameters(int *_argc, char **argv);

/* Free an instance of struct pb_Parameters.
 */
void
pb_FreeParameters(struct pb_Parameters *p);

/* Count the number of input files in a pb_Parameters instance.
 */
int
pb_Parameters_CountInputs(struct pb_Parameters *p);

