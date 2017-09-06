/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * int main(int argc, char *argv[]) --- returns 0 if translation is
 * successful. Diag will exit with 1 if error. 
 *
 * ---The overall strategy of the translation consists of three phases. 
 *
 * 1) read in the whole file as a sequence of tokens, parsing as we go. Most of
 * the trivial C translation such as appending ';' to statements is performed
 * in this phase as is the creation of the symbol table. Item lists maintain
 * the proper token order. Ater a whole block is read in, nontrivial
 * manipulation may be performed on the entire block. 
 *
 * 2) Some blocks and statements can be manipulated only after the entire file
 * has been read in. The solve statement is an example since it can be
 * analysed only after we know what is the type of the associated block.  The
 * kinetic block is another example whose translation depends on the SOLVE
 * method and so cannot be processed until the whole input file has been
 * read. 
 *
 * 3) Output the lists. 
 *
 */

#include "nmodlconf.h"
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "modl.h"
FILE* fin;      /* input file descriptor for  filename.mod */
FILE* fcout;    /* output file descriptor for filename.c */
char* finname;
char* modprefix;

static struct option long_options[] = {
  {"version", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {"outdir", required_argument, 0, 'o'},
  {0,0,0,0}
};

#if LINT
char*        clint;
int          ilint;
Item*        qlint;
#endif

#if NMODL && VECTORIZE
extern int   vectorize;
extern int   numlist;
extern char* nmodl_version_;
extern int   usederivstatearray;
#endif

extern int yyparse();

static void openfiles(char* input_filename, char* output_dir) {
  char  s[NRN_BUFSIZE];
  modprefix = strdup (input_filename);                // we want to keep original string to open input file
  char* first_ext_char = strchr(modprefix, '.');      // find last '.' that delimit file name from extension
  *first_ext_char = '\0';                             // effectively cut the extension from prefix
  if ((fin = fopen(input_filename, "r")) == (FILE *) 0) {
    Sprintf(finname, "%s.mod", modprefix);
      diag("Can't open input file: ", input_filename);
  }
  if (output_dir[0] != '\0')
    Sprintf(s, "%s/%s.c", output_dir, modprefix);
  else
    Sprintf(s, "%s.c", modprefix);

  if ((fcout = fopen(s, "w")) == (FILE *) 0) {
  diag("Can't create C file: ", s);
  }
  Fprintf(stderr, "Translating %s into %s.c\n", input_filename, s);
}

int main(int argc, char** argv) {
  int option        = -1;
  int option_index  = 0;
  char* output_dir = "";
  while ( (option = getopt_long (argc, argv, ":vho:", long_options, &option_index)) != -1) {
    switch (option) {
      case 'v':
        printf("%s\n", nmodl_version_);
        exit(0);
      
      case 'o':
        output_dir = strdup(optarg);
        break;
      
      case 'h':
        fprintf(stderr, "%s source to source compiler from NMODL to C files\n", argv[0]);
        fprintf(stderr, "Usage: %s [options] Inputfile\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\t-o | --outdir <OUTPUT_DIRECTORY>    directory where output files will be written\n");
        fprintf(stderr, "\t-h | --help                         print this message\n");
        fprintf(stderr, "\t-v | --version                      print version number\n");
        exit(0);
      
      case ':':
        fprintf(stderr, "%s: option '-%c' requires an argument\n", argv[0], optopt);
        exit (-1);
      
      case '?':
      default:
        fprintf(stderr, "%s: invalid option `-%c' \n", argv[0], optopt);
        exit (-1);
    }
  }
  if ((argc - optind) > 1) {
    fprintf(stderr, "%s: several input files specified on command line but only one is accepted\n", argv[0]);
    exit(-1);
  }

  init(); /* keywords into symbol table, initialize lists, etc. */
  finname = argv[optind];
  openfiles(finname, output_dir); /* .mrg else .mod,  .var, .c */
  IGNORE(yyparse());
/*
 * At this point all blocks are fully processed except the kinetic
 * block and the solve statements. Even in these cases the 
 * processing doesn't involve syntax since the information is
 * held in intermediate lists of specific structure.
 *
 */
/*
 * go through the list of solve statements and construct the model()
 * code 
 */
  solvhandler();
/* 
 * NAME's can be used in many cases before they were declared and
 * no checking up to this point has been done to make sure that
 * names have been used in only one way.
 *
 */
  consistency();
  chk_thread_safe();
  c_out();   /* print .c file */

  IGNORE(fclose(fcout));

#if NMODL && VECTORIZE
  if (vectorize) {
    Fprintf(stderr, "Thread Safe\n");
  }
  if (usederivstatearray) {
    fprintf(stderr, "Derivatives of STATE array variables are not translated correctly and compile time errors will be generated.\n");
    fprintf(stderr, " %s.c file may be manually edited to fix these errors.\n", modprefix);
  }
#endif

#if LINT
{ /* for lex */
  extern int      yytchar, yylineno;
  extern FILE    *yyin;
  IGNORE(yyin);
  IGNORE(yytchar);
  IGNORE(yylineno);
  IGNORE(yyinput());
  yyunput(ilint);
  yyoutput(ilint);
}
#endif
  return 0;
}
