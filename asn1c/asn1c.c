/*-
 * Copyright (c) 2003, 2004, 2005
 * 	Lev Walkin <vlm@lionet.info>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */
/*
 * This is the program that connects the libasn1* libraries together.
 * It uses them in turn to parse, fix and then compile or print the ASN.1 tree.
 */
#include "sys-common.h"

#include <asn1parser.h>		/* Parse the ASN.1 file and build a tree */
#include <asn1fix.h>		/* Fix the ASN.1 tree */
#include <asn1print.h>		/* Print the ASN.1 tree */
#include <asn1compiler.h>	/* Compile the ASN.1 tree */

#include <asn1c_compat.h>	/* Portable basename(3) and dirname(3) */

#undef  COPYRIGHT
#define COPYRIGHT       \
	"Copyright (c) 2003, 2004, 2005 Lev Walkin <vlm@lionet.info>\n"

static void usage(const char *av0);	/* Print the Usage screen and exit */

int
main(int ac, char **av) {
	enum asn1p_flags     asn1_parser_flags	= A1P_NOFLAGS;
	enum asn1f_flags     asn1_fixer_flags	= A1F_NOFLAGS;
	enum asn1c_flags     asn1_compiler_flags= A1C_NO_C99;
	enum asn1print_flags asn1_printer_flags	= APF_NOFLAGS;
	int print_arg__print_out = 0;	/* Don't compile, just print parsed */
	int print_arg__fix_n_print = 0;	/* Fix and print */
	int warnings_as_errors = 0;	/* Treat warnings as errors */
	char *skeletons_dir = NULL;	/* Directory with supplementary stuff */
	asn1p_t *asn = 0;		/* An ASN.1 parsed tree */
	int ret;			/* Return value from misc functions */
	int ch;				/* Command line character */
	int i;				/* Index in some loops */

	/*
	 * Process command-line options.
	 */
	while((ch = getopt(ac, av, "EFf:hLPp:RS:vW:X")) != -1)
	switch(ch) {
	case 'E':
		print_arg__print_out = 1;
		break;
	case 'F':
		print_arg__fix_n_print = 1;
		break;
	case 'f':
		if(strcmp(optarg, "all-defs-global") == 0) {
			asn1_compiler_flags |= A1C_ALL_DEFS_GLOBAL;
		} else if(strcmp(optarg, "bless-SIZE") == 0) {
			asn1_fixer_flags |= A1F_EXTENDED_SizeConstraint;
		} else if(strcmp(optarg, "compound-names") == 0) {
			asn1_compiler_flags |= A1C_COMPOUND_NAMES;
		} else if(strncmp(optarg, "known-extern-type=", 18) == 0) {
			char *known_type = optarg + 18;
			ret = asn1f_make_known_external_type(known_type);
			assert(ret == 0 || errno == EEXIST);
		} else if(
			strcmp(optarg, "native-integers") == 0 /* Legacy */
			|| strcmp(optarg, "native-types") == 0) {
			asn1_compiler_flags |= A1C_USE_NATIVE_TYPES;
		} else if(strcmp(optarg, "no-constraints") == 0) {
			asn1_compiler_flags |= A1C_NO_CONSTRAINTS;
		} else if(strcmp(optarg, "unnamed-unions") == 0) {
			asn1_compiler_flags |= A1C_UNNAMED_UNIONS;
		} else if(strcmp(optarg, "types88") == 0) {
			asn1_parser_flags |= A1P_TYPES_RESTRICT_TO_1988;
		} else {
			fprintf(stderr, "-f%s: Invalid argument\n", optarg);
			exit(EX_USAGE);
		}
		break;
	case 'h':
		usage(av[0]);
	case 'P':
		asn1_compiler_flags |= A1C_PRINT_COMPILED;
		asn1_compiler_flags &= ~A1C_NO_C99;
		break;
	case 'p':
		if(strcmp(optarg, "rint-constraints") == 0) {
			asn1_printer_flags |= APF_DEBUG_CONSTRAINTS;
		} else if(strcmp(optarg, "rint-lines") == 0) {
			asn1_printer_flags |= APF_LINE_COMMENTS;
		} else {
			fprintf(stderr, "-p%s: Invalid argument\n", optarg);
			exit(EX_USAGE);
		}
		break;
	case 'R':
		asn1_compiler_flags |= A1C_OMIT_SUPPORT_CODE;
		break;
	case 'S':
		skeletons_dir = optarg;
		break;
	case 'v':
		fprintf(stderr, "ASN.1 Compiler, v" VERSION "\n" COPYRIGHT);
		exit(0);
		break;
	case 'W':
		if(strcmp(optarg, "error") == 0) {
			warnings_as_errors = 1;
			break;
		} else if(strcmp(optarg, "debug-lexer") == 0) {
			asn1_parser_flags |= A1P_LEXER_DEBUG;
			break;
		} else if(strcmp(optarg, "debug-fixer") == 0) {
			asn1_fixer_flags |= A1F_DEBUG;
			break;
		} else if(strcmp(optarg, "debug-compiler") == 0) {
			asn1_compiler_flags |= A1C_DEBUG;
			break;
		} else {
			fprintf(stderr, "-W%s: Invalid argument\n", optarg);
			exit(EX_USAGE);
		}
		break;
	case 'X':
		print_arg__print_out = 1;	/* Implicit -E */
		print_arg__fix_n_print = 1;	/* Implicit -F */
		asn1_printer_flags |= APF_PRINT_XML_DTD;
		break;
	default:
		usage(av[0]);
	}

	/*
	 * Validate the options combination.
	 */
	if(!print_arg__print_out) {
		if(print_arg__fix_n_print) {
			fprintf(stderr, "Error: -F requires -E\n");
			exit(EX_USAGE);
		}
		if(asn1_printer_flags) {
			fprintf(stderr, "Error: "
				"-print-... arguments require -E\n");
			exit(EX_USAGE);
		}
	}

	/*
	 * Ensure that there are some input files present.
	 */
	if(ac > optind) {
		ac -= optind;
		av += optind;
	} else {
		fprintf(stderr, "%s: No input files specified\n",
			a1c_basename(av[0]));
		exit(1);
	}

	/*
	 * Iterate over input files and parse each.
	 * All syntax trees from all files will be bundled together.
	 */
	for(i = 0; i < ac; i++) {
		asn1p_t *new_asn;

		new_asn = asn1p_parse_file(av[i], asn1_parser_flags);
		if(new_asn == NULL) {
			fprintf(stderr, "Cannot parse \"%s\"\n", av[i]);
			exit(EX_DATAERR);
		}

		/*
		 * Bundle the parsed tree with existing one.
		 */
		if(asn) {
			asn1p_module_t *mod;
			while((mod = TQ_REMOVE(&(new_asn->modules), mod_next)))
				TQ_ADD(&(asn->modules), mod, mod_next);
			asn1p_free(new_asn);
		} else {
			asn = new_asn;
		}

	}

	/*
	 * Dump the parsed ASN.1 tree if -E specified and -F is NOT given.
	 */
	if(print_arg__print_out && !print_arg__fix_n_print) {
		if(asn1print(asn, asn1_printer_flags))
			exit(EX_SOFTWARE);
		return 0;
	}


	/*
	 * Process the ASN.1 specification: perform semantic checks,
	 * expand references, etc, etc.
	 * This function will emit necessary warnings and error messages.
	 */
	ret = asn1f_process(asn, asn1_fixer_flags,
		NULL /* default fprintf(stderr) */);
	switch(ret) {
	case 1:
		if(!warnings_as_errors)
			/* Fall through */
	case 0:
		break;			/* All clear */
	case -1:
		exit(EX_DATAERR);	/* Fatal failure */
	}

	/*
	 * Dump the parsed ASN.1 tree if -E specified and -F is given.
	 */
	if(print_arg__print_out && print_arg__fix_n_print) {
		if(asn1print(asn, asn1_printer_flags))
			exit(EX_SOFTWARE);
		return 0;
	}

	/*
	 * Make sure the skeleton directory is out there.
	 */
	if(skeletons_dir == NULL) {
		struct stat sb;
		skeletons_dir = DATADIR;
		if((av[-optind][0] == '.' || av[-optind][1] == '/')
		&& stat(skeletons_dir, &sb)) {
			/*
			 * The default skeletons directory does not exist,
			 * compute it from my file name:
			 * ./asn1c/asn1c -> ./skeletons
			 */
			char *p;
			size_t len;

			p = a1c_dirname(av[-optind]);

			len = strlen(p) + sizeof("/../skeletons");
			skeletons_dir = alloca(len);
			snprintf(skeletons_dir, len, "%s/../skeletons", p);
			if(stat(skeletons_dir, &sb)) {
				fprintf(stderr,
					"WARNING: skeletons are neither in "
					"\"%s\" nor in \"%s\"!\n",
					DATADIR, skeletons_dir);
				if(warnings_as_errors)
					exit(EX_SOFTWARE);
			}
		}
	}

	/*
	 * Compile the ASN.1 tree into a set of source files
	 * of another language.
	 */
	if(asn1_compile(asn, skeletons_dir, asn1_compiler_flags)) {
		exit(EX_SOFTWARE);
	}

	return 0;
}

/*
 * Print the usage screen and exit(EX_USAGE).
 */
static void
usage(const char *av0) {
	fprintf(stderr,
"ASN.1 Compiler, v" VERSION "\n" COPYRIGHT
"Usage: %s [options] file ...\n"
"Options:\n"
"  -E                    Run only the ASN.1 parser and print out the tree\n"
"  -F                    During -E operation, also perform tree fixing\n"
"\n"
"  -P                    Concatenate and print the compiled text\n"
"  -R                    Restrict output (tables only, no support code)\n"
"  -S <dir>              Directory with support (skeleton?) files\n"
"                        (Default is \"%s\")\n"
"  -X                    Generate and print the XML DTD\n"
"\n"

"  -Werror               Treat warnings as errors; abort if any warning\n"
"  -Wdebug-lexer         Enable verbose debugging output from lexer\n"
"  -Wdebug-fixer         --//-- semantics processor\n"
"  -Wdebug-compiler      --//-- compiler\n"
"\n"

"  -fall-defs-global     Don't make the asn1_DEF_'s of structure members \"static\"\n"
"  -fbless-SIZE          Allow SIZE() constraint for INTEGER etc (non-std.)\n"
"  -fcompound-names      Disambiguate C's struct NAME's inside top-level types\n"
"  -fknown-extern-type=<name>    Pretend this type is known\n"
"  -fnative-types        Use \"long\" instead of INTEGER_t whenever possible, etc.\n"
"  -fno-constraints      Do not generate constraint checking code\n"
"  -funnamed-unions      Enable unnamed unions in structures\n"
"  -ftypes88             Pretend to support only ASN.1:1988 embedded types\n"
"\n"

"  -print-constraints    Explain subtype constraints (debug)\n"
"  -print-lines          Generate \"-- #line\" comments in -E output\n"

	,
	a1c_basename(av0), DATADIR
	);
	exit(EX_USAGE);
}

