#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

typedef struct func_t {
    char *name;
    int private;
    char *desc;
    char *version;
    int num_args;
    char **args;
    char *returns;
} func_t;

func_t *functions = NULL;

void bail_error(const char *fmt, ...)
{
    va_list ap;
    char tmp[64] = { 0 };

    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    fprintf(stderr, "Error: %s\n", tmp);
    exit(1);
}

char *ltrim_string(char *str)
{
    /* Skip initial comment indentation */
    if (strncmp(str, " * ", 3) == 0)
        str += 3;

    /* Now skip spaces */
    while (*str == ' ')
        str++;

    /* TABs are not acceptable! */
    if (*str == '\t')
        bail_error("Indent with spaces, not TABs");

    return str;
}

void parse_comment(char *line, int func_num, int *arg_num)
{
    char *ltrimmed = ltrim_string(line);

    if (strncmp(ltrimmed, "Function name:", 14) == 0) {
        functions[func_num].name = strdup( ltrim_string( ltrimmed + 14) );
        functions[func_num].private = 0;
    }
    else if (strncmp(ltrimmed, "Private function name:", 22) == 0) {
        functions[func_num].name = strdup( ltrim_string( ltrimmed + 22) );
        functions[func_num].private = 1;
    }
    else if (strncmp(ltrimmed, "Since version:", 14) == 0) {
        functions[func_num].version = strdup( ltrim_string( ltrimmed + 14) );
    }
    else if (strncmp(ltrimmed, "Description:", 12) == 0) {
        functions[func_num].desc = strdup( ltrim_string( ltrimmed + 12) );
    }
    else if (strncmp(ltrimmed, "Arguments:", 10) == 0) {
        char *str = ltrim_string(ltrimmed + 11);
        if (arg_num == NULL)
            return;

        functions[func_num].num_args = 1;
        functions[func_num].args = malloc( sizeof(char *) );
        functions[func_num].args[0] = malloc( (strlen(str) + 1) * sizeof(char) );
        strcpy(functions[func_num].args[0], str);

        *arg_num = 1;
    }
    else if (strncmp(ltrimmed, "Returns:", 8) == 0) {
        char *str = ltrim_string(ltrimmed + 7);

        functions[func_num].returns = malloc( (strlen(str) + 1) * sizeof(char));
        strcpy(functions[func_num].returns, str);
    }
    else if ((arg_num != NULL) && (*arg_num > 0)) {
        functions[func_num].num_args++;
        functions[func_num].args = realloc( functions[func_num].args,
                                            functions[func_num].num_args * sizeof(char *));
        functions[func_num].args[functions[func_num].num_args-1] = malloc((strlen(ltrimmed) + 1) * sizeof(char) );
        strcpy(functions[func_num].args[functions[func_num].num_args-1], ltrimmed);

        *arg_num = *arg_num + 1;
    }
}

char *get_lpart(char *str)
{
    if (!str || strcmp(str, "None") == 0)
        return str;
    char *new = strdup(str);
    char *tmp = strchr(str, ':');
    if (!tmp)
        return str;

    new[ strlen(new) - strlen(tmp) ] = 0;

    return new;
}

char *get_rpart(char *str)
{
    if (!str || strcmp(str, "None") == 0)
        return str;
    char *tmp = strchr(str, ':');

    if (!tmp)
        return str;

    return (++tmp);
}

void free_functions(int function_number)
{
    int i, j;

    for (i = 0; i <= function_number; i++) {
        for (j = 0; j < functions[i].num_args; j++)
            free(functions[i].args[j]);
        free(functions[i].args);
        free(functions[i].name);
        free(functions[i].desc);
        free(functions[i].returns);
    }
    free(functions);
}

int count_functions(int num_funcs, int private)
{
    int i, num = 0;

    for (i = 0; i < num_funcs; i++)
        if ((functions[i].name != NULL) && (functions[i].private == private))
            num++;

    return num;
}


static void
parse_source(const char *in,
             int *function_number)
{
    FILE *fp;
    char line[1024] = { 0 };
    bool in_comment = false;
    int arg_number = 0;

    if (access(in, R_OK) != 0)
        bail_error("Cannot open file %s", in);

    if (!functions &&
        !(functions = (func_t *)calloc(sizeof(func_t), 1)))
        bail_error("Out of memory");

    if (!(fp = fopen(in, "r")))
        bail_error("Error while opening %s", in);

    while (true) {
        memset(line, 0, sizeof(line));

        if (!fgets(line, sizeof(line), fp)) {
            if (feof(fp))
                break;
            bail_error("Unable to read form %s", in);
        }

        /* Strip new line characters */
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;

        if (strcmp(line, "/*") == 0) {
            (*function_number)++;
            in_comment = 1;
            if (!(functions = (func_t *) realloc(functions,
                                                 sizeof(func_t) * (*function_number + 1))))
                bail_error("Out of memory");
            memset(functions + *function_number, 0, sizeof(*functions));
            functions[*function_number].name = NULL;
            functions[*function_number].num_args = 0;
        } else {
            if (strcmp(line, " */") == 0)
                in_comment = 0;
            else if (in_comment)
                parse_comment(line, *function_number, &arg_number);
        }
    }
    fclose(fp);
}


static void
generate_output(const char *out,
                int function_number,
                int private)
{
    FILE *fp;
    int i;

    if (!(fp = fopen(out, "w"))) {
        free_functions(function_number);
        bail_error("Cannot write %s", out);
    }

    fprintf(fp, "<?xml version=\"1.0\"?>\n<html>\n  <body>\n");

    fprintf(fp,"<h1>%s API Reference guide</h1>\n\n    <h3>Functions</h3>\n\n    <!-- Links -->\n", (private == 0) ? "PHP" : "Developer's");
    fprintf(fp, "<pre>Total number of functions: %d. Functions supported are:<br /><br />\n", count_functions(function_number, private));
    for (i = 0; i <= function_number; i++) {
        if ((functions[i].name != NULL) && (functions[i].private == private)) {
            int j;

            fprintf(fp, "\t<code class=\"docref\">%s</code>(", functions[i].name);

            for (j = 0; j < functions[i].num_args; j++) {
                if (strcmp(functions[i].args[j], "None") != 0) {
                    char *new = get_lpart(functions[i].args[j]);
                    char *part;
                    int decrement;

                    if (new[0] == '@')
                        new++;

                    part = strchr(new, ' ');
                    decrement = (part != NULL) ? strlen( part ) : 0;

                    if (j > 0)
                        fprintf(fp, ", ");

                    new[ strlen(new) - decrement ] = 0;
                    fprintf(fp, "$%s", new);
                }
            }

            fprintf(fp, ")<br />\n");
        }
    }

    fprintf(fp, "</pre>\n");

    for (i = 0; i <= function_number; i++) {
        if ((functions[i].name != NULL) && (functions[i].private == private)) {
            int j;

            fprintf(fp, "<h3><a name=\"%s\"><code>%s</code></a></h3>\n", functions[i].name, functions[i].name);
            fprintf(fp, "<pre class=\"programlisting\">%s(", functions[i].name);

            for (j = 0; j < functions[i].num_args; j++) {
                if (strcmp(functions[i].args[j], "None") != 0) {
                    char *new = get_lpart(functions[i].args[j]);
                    char *part;
                    int decrement;

                    if (new[0] == '@')
                        new++;

                    part = strchr(new, ' ');
                    decrement = (part != NULL) ? strlen( part ) : 0;

                    if (j > 0)
                        fprintf(fp, ", ");

                    new[ strlen(new) - decrement ] = 0;
                    fprintf(fp, "$%s", new);
                }
            }

            fprintf(fp, ")</pre>\n");
            fprintf(fp, "<p>[Since version %s]</p>\n", functions[i].version);
            fprintf(fp, "<p>%s.</p>", functions[i].desc);
            fprintf(fp, "<div class=\"variablelist\">\n");
            fprintf(fp, "\t<table border=\"0\">\n");
            fprintf(fp, "\t\t<col align=\"left\" />\n");
            fprintf(fp, "\t\t<tbody>\n");

            for (j = 0; j < functions[i].num_args; j++) {
                if (strcmp(functions[i].args[j], "None") != 0) {
                    fprintf(fp, "\t\t  <tr>\n");
                    fprintf(fp, "\t\t    <td>\n");
                    fprintf(fp, "\t\t\t<span class=\"term\"><i><tt>%s</tt></i>:</span>\n", get_lpart(functions[i].args[j]) );
                    fprintf(fp, "\t\t    </td>\n");
                    fprintf(fp, "\t\t    <td>\n");
                    fprintf(fp, "\t\t\t%s\n", get_rpart(functions[i].args[j]));
                    fprintf(fp, "\t\t    </td>\n");
                    fprintf(fp, "\t\t  </tr>\n");
                }
            }

            fprintf(fp, "\t\t  <tr>\n");
            fprintf(fp, "\t\t    <td>\n");
            fprintf(fp, "\t\t\t<span class=\"term\"><i><tt>Returns</tt></i>:</span>\n");
            fprintf(fp, "\t\t    </td>\n");
            fprintf(fp, "\t\t    <td>\n");
            fprintf(fp, "\t\t\t%s\n", functions[i].returns);
            fprintf(fp, "\t\t    </td>\n");
            fprintf(fp, "\t\t  </tr>\n");
            fprintf(fp, "\t\t</tbody>\n");
            fprintf(fp, "\t</table>\n");
            fprintf(fp, "</div>\n");
        }
    }
    fclose(fp);
}


int main(int argc, char *argv[])
{
    int function_number = -1;
    int private = 0;
    char *output = NULL;
    int arg;
    struct option opt[] = {
        {"private", no_argument, NULL, 'p'},
        {"output", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };

    while ((arg = getopt_long(argc, argv, ":o:p", opt, NULL)) != -1) {
        switch (arg) {
        case 'p':
            private = 1;
            break;
        case 'o':
            free(output);
            if (!(output = strdup(optarg)))
                bail_error("Out of memory");
            break;
        case '?':
            bail_error("Unknown option");
        }
    }

    if (argc == optind) {
        fprintf(stderr, "Syntax: %s [-p|--private] source-file output-in-file\n", argv[0]);
        return 1;
    }

    while (argv[optind])
        parse_source(argv[optind++], &function_number);

    if (!output &&
        !(output = strdup("/dev/stdout")))
        bail_error("Out of memory");

    generate_output(output, function_number, private);

    free_functions(function_number);
    free(output);
    printf("Documentation has been generated successfully\n");
    return 0;
}
