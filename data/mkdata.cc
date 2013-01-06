// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "../gleri/config.h"
#include <stdio.h>

#define OUTDIR	".o/data/"

int main (int argc, const char* const* argv)
{
    FILE* dsrc = fopen (OUTDIR "data.cc", "w");
    FILE* dhdr = fopen (OUTDIR "data.h", "w");
    fprintf (dhdr, "#pragma once\n\n");
    fprintf (dsrc, "#include \"data.h\"\n\n");
    for (int i = 1; i < argc; ++i) {
	char aname[64];
	snprintf (ArrayBlock(aname), "File_%s", argv[i]+strlen(OUTDIR));
	*strchr(aname, '.') = 0;

	FILE* f = fopen (argv[i], "rb");
	fseek (f, 0, SEEK_END);
	long fsz = ftell (f);
	fseek (f, 0, SEEK_SET);
	fprintf (dhdr, "extern const unsigned char %s [%ld];\n", aname, fsz);
	fprintf (dsrc, "//{{{ %s\nconst unsigned char %s [%ld] = {\n", aname, aname, fsz);
	for (long j = 0; j < fsz; ++j)
	    fprintf (dsrc, "%s%s%hhu", ","+!j, "\n"+(15!=j%16), fgetc(f));
	fprintf (dsrc, "\n};\n//}}}-------------------------------------------------------------------\n");
	fclose (f);
    }
    fclose (dsrc);
    fclose (dhdr);
    return (EXIT_SUCCESS);
}
