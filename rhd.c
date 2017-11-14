/*
   rhd.c - Ranga's hex dump 
   $Id: rhd.c 8 2005-06-18 05:40:25Z ranga $

   Copyright (c) 2003-2017 Sriranga Veeraraghavan <ranga@alum.berkeley.edu>

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/* includes */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>

/* defines */

#define HD_FNAME_STDIN "-"

/* enums */

/* flags for hexdump() */

enum {
    HD_FLAG_NOSTR = 0x00000001, /* don't print the ascii string */
    HD_FLAG_NOPOS = 0x00000010  /* don't print the offset */
};

/* options */

enum {
    HD_OPT_HELP1 = '?',  /* -?: help */
    HD_OPT_NOSTR = 'A',  /* -A: don't print ascii */
    HD_OPT_NOPOS = 'O',  /* -O: don't print offset */
    HD_OPT_CPL   = 'c',  /* -c: number of chars per line */
    HD_OPT_END   = 'e',  /* -e: ending offset */
    HD_OPT_HELP2 = 'h',  /* -h: help */
    HD_OPT_LEN   = 'l',  /* -l: length of bytes to dump */
    HD_OPT_START = 's',  /* -s: starting offset */
    HD_OPT_VERS  = 'v'   /* -v: version */
};

/* defaults */

enum {
    HD_DEF_CPL = 16 /* default chars per line */
};

/* globals */

/* version id */

#ifdef HD_REL
static char *g_version = HD_REL;
#else
static char *g_version = "$Rev: 9 $";
#endif /* HD_REL */

/* lookup table for hex values */

static char *g_hex_table[] = {
    "00", "01", "02", "03", "04", "05", "06", "07",
    "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
    "10", "11", "12", "13", "14", "15", "16", "17",
    "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
    "20", "21", "22", "23", "24", "25", "26", "27",
    "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
    "30", "31", "32", "33", "34", "35", "36", "37",
    "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
    "40", "41", "42", "43", "44", "45", "46", "47",
    "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
    "50", "51", "52", "53", "54", "55", "56", "57",
    "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
    "60", "61", "62", "63", "64", "65", "66", "67",
    "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
    "70", "71", "72", "73", "74", "75", "76", "77",
    "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
    "80", "81", "82", "83", "84", "85", "86", "87",
    "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
    "90", "91", "92", "93", "94", "95", "96", "97",
    "98", "99", "9a", "9b", "9c", "9d", "9e", "9f", 
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", 
    "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
    "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7",
    "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
    "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7",
    "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "d8", "d9", "da", "db", "dc", "dd", "de", "df",
    "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7",
    "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
    "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff",
};

/* prototypes */

static int  hexdump (FILE *outfp, 
                     char *fname,
                     unsigned long offset,
                     long len,
                     unsigned short int cpl,
                     unsigned short int flags);
static int  safechar (char c);
static int  print_error (FILE *stream, 
                         const char *fmt, ...);
static void print_usage (char *cmd);

/* functions */

/* print_error - prints a formatted error message */

static int
print_error (FILE *stream, 
             const char *fmt, ...)
{   
    int rc;
    va_list ap;

    if ((rc = fprintf(stream,"Error: ")) > 0) {
        va_start(ap,fmt);
        rc = vfprintf(stream,fmt,ap);
        va_end(ap);
    }

    return rc;
}

/* print_usage - prints the usage message */

static void
print_usage (char *cmd)
{
    fprintf(stderr,
            "Usage: %s [options] [files]\n\n",
            (cmd == NULL || *cmd == '\0' ? "hd" : cmd));
    fprintf(stderr, 
            "Where [options] are:\n\n");
    fprintf(stderr,
            "  -%c [chars]  \tdump [chars] chars per line\n",
            HD_OPT_CPL);
    fprintf(stderr,
            "  -%c [offset] \tstart dump at offset [offset]\n",
            HD_OPT_START);
    fprintf(stderr,
            "  -%c [offset] \tend dump at offset [offset]\n",
            HD_OPT_END);
    fprintf(stderr,
            "  -%c [len]    \tdump [len] bytes\n",
            HD_OPT_LEN);
    fprintf(stderr,
            "  -%c          \tprint version\n",
            HD_OPT_VERS);
    fprintf(stderr,
            "  -%c          \tprint help\n",
            HD_OPT_HELP2);
    fprintf(stderr,
            "  -%c          \tdon't print the position\n",
            HD_OPT_NOPOS);
    fprintf(stderr,
            "  -%c          \tdon't print the ascii string\n",
            HD_OPT_NOSTR);
    fprintf(stderr,"\n");
}

/* safechar - Given a char, returns a "safe" (printable) version of
              that char. If the char is "unsafe", '.' is returned */
   
static int
safechar (char c)
{
    if (c == '\t') {
        return ' ';
    }
    
    return (c >= ' ' && c <= '~' ? c : '.');
}

/* hexdump - produces a hex dump for the given file starting at [offset]
             upto [len] bytes with [cpl] chars per line. */

static int
hexdump (FILE *outfp, 
         char *fname, 
         unsigned long offset, 
         long len, 
         unsigned short int cpl, 
         unsigned short int flags)
{
    unsigned char *rawchars = NULL, *buf2 = NULL, *ucp = NULL;
    size_t i = 0, j = 0, k = 0;
    size_t size_rawchars = 0, size_buf2 = 0;
    size_t toread, numread = 0;
    off_t pos = 0;
    long end = -1;
    FILE *fp = NULL;

    /* make sure chars per line is greater than 0 */

    if (cpl <= 0) {
        return -1;
    }
  
    if (outfp == NULL) {
        outfp = stdout;
    }

    /* if no file name is specified or the filename is -, use stdin as
       the file pointer, otherwise fopen the file and use that as the
       file pointer */

    if (fname == NULL || *fname == '\0' || 
        strcmp(fname,HD_FNAME_STDIN) == 0) {
        fp = stdin;
    } else {
        if ((fp = fopen(fname,"r")) == NULL) {
            print_error(stderr,
                        "Cannot open %s: %s\n",
                        fname,
                        strerror(errno));
        return -1;
        }
    }

    /* if an offset is specified, fseek there and update the position */

    if (offset > 0) {
        if (fseek(fp, offset, SEEK_SET) < 0) {
            print_error(stderr,
                        "Cannot set starting offset of %s to %ld: %s\n",
                        fname,
                        offset,
                        strerror(errno));

            if (fp != stdin) {
                fclose(fp);
            }

            return -1;
        }
        
        pos += offset;
    } 

    /* if a length is specified, set the ending position */

    if (len > 0) {
        end = pos + len;
    }
    
    /* allocate memory for the read buffer (rawchars) and the ascii
       printout buffer */

    size_rawchars = cpl;
    size_buf2 = size_rawchars + 1;

    /* divide [cpl] in half because we want to print an extra space half
       way through the hex dump to make the easier to read */

    cpl /= 2;

    /* allocate memory for reading in the file's contents */
    
    if (((rawchars = malloc(sizeof(char) * size_rawchars)) == NULL) ||
        (((buf2 = malloc(sizeof(char) * size_buf2))) == NULL)) {

        if (rawchars) {
            free(rawchars);
        }

        print_error(stderr,
                    "Cannot allocate memory: %s\n",
                    strerror(errno));

        if (fp != stdin) {
            fclose(fp);
        }
        
        return -1;
    }

    /* read and dump the file */

    while (!ferror(fp) && !feof(fp)) {

        /* update the current position */
    
        pos += numread;
        
        /* stop dumping when the requested end position has been reached */
        
        if (end > 0 && pos > end) {
            break;
        }

        /* clear the buffers */

        memset(rawchars, '\0', size_rawchars);
        memset(buf2, ' ', size_rawchars);
        buf2[size_rawchars] = '\0';

        /* figure out the number of chars to read (maybe fewer than
           size_rawchars near the end of the requested range) */
    
        toread = size_rawchars;
        k = (size_t)(end - pos);
        if (end > 0 && k < size_rawchars) {
            toread = k;
        }
        
        /* read the bytes */

        if ((numread = fread(rawchars, sizeof(char), toread, fp)) == 0) {
            break;
        }
        
        /* print the current position, unless HD_FLAG_NOPOS was given */
    
        if (!(flags && ((flags | HD_FLAG_NOPOS) == flags))) {
            fprintf(outfp,"%08lx: ",(unsigned long int)pos);
        }
        
        /* convert the characters in rawchars to hex values using the
           g_hex_table table */

        for (i = 0; i < numread; i++) {
            ucp = rawchars + i;

            fprintf(outfp,
                    "%s %s",
                    g_hex_table[*ucp],
                    (i + 1 == cpl && cpl % 2 == 0 ? " " : ""));
            
            /* convert the char to a "safe" (printable) value */
            
            buf2[i] = safechar(rawchars[i]);
        }
    
        /* print padding so that output is nicely formatted */
    
        if (numread < size_rawchars) {
            for (i = 0, j = size_rawchars - numread; i < j; i++) {
                fprintf(outfp,
                        "   %s",
                        (i + 1 < j &&
                         i + 1 == cpl &&
                         cpl % 2 == 0 ? " " : ""));
            }
        }
    
        /* print the ascii version of the bytes, unless HD_FLAG_NOSTR is
           given */

        if (flags && ((flags | HD_FLAG_NOSTR) == flags)) {
            fprintf(outfp,"\n");
        } else {
            fprintf(outfp,"| %s |\n",buf2);
        }
    }

    /* close files, free buffers and return */

    if (fp != stdin) {
        fclose(fp);
    }
    
    if (buf2) {
        free(buf2);
    }

    if (rawchars) {
        free(rawchars);
    }

    return 0;
}

int
main (int argc, char **argv)
{
    int i;
    char optstr[16], *ep;
    extern char *optarg;
    extern int optind;
    unsigned long ulval, offset_start = 0, offset_end = 0;
    unsigned short int cpl = HD_DEF_CPL, err = 0, ver = 0, help = 0;
    unsigned short int have_len = 0, have_end = 0, flags = 0;
    long len = -1;
    int ch;

    memset(optstr,'\0',sizeof(optstr));
    snprintf(optstr,
             sizeof(optstr),
             "%c:%c:%c:%c:%c%c%c%c%c",
             HD_OPT_CPL,
             HD_OPT_START,
             HD_OPT_LEN,
             HD_OPT_END,
             HD_OPT_VERS,
             HD_OPT_HELP1,
             HD_OPT_HELP2,
             HD_OPT_NOSTR,
             HD_OPT_NOPOS);

    while ((ch = getopt(argc,argv,optstr)) != -1) {
        switch (ch) {
            
            case HD_OPT_NOSTR:

                if (!(flags && ((flags | HD_FLAG_NOSTR) == flags))) {
                    flags |= HD_FLAG_NOSTR;
                }
                break;
                
            case HD_OPT_NOPOS:
                
                if (!(flags && ((flags | HD_FLAG_NOPOS) == flags))) {
                    flags |= HD_FLAG_NOPOS;
                }
                break;
                
            case HD_OPT_CPL:
                
                if (optarg && *optarg != '\0') {
                    errno = 0;
                    ulval = strtoul(optarg,&ep,0);
                    if (optarg[0] == '\0' || *ep != '\0' ||
                        (errno == ERANGE && ulval > 32) || ulval == 0) {
                        print_error(stderr,
                                    "Invalid number of chars per line: %s\n",
                                    optarg);
                        err++;
                    } else {
                        cpl = (unsigned short int)ulval;
                    }
                } else {
                    err++;
                }
                break;
                
            case HD_OPT_START:
                
                if (optarg && optarg != '\0') {
                    errno = 0;
                    ulval = strtoul(optarg,&ep,0);
                    if (optarg[0] == '\0' || *ep != '\0' ||
                        (errno == ERANGE && ulval > ULONG_MAX)) {
                        print_error(stderr,
                                    "Invalid starting offset: %s\n",
                                    optarg);
                        err++;
                    } else {
                        offset_start = ulval;
                    }
                } else {
                    err++;
                }
                break;
                
            case HD_OPT_END:

                if (optarg && optarg != '\0' && have_len == 0) {
                    errno = 0;
                    ulval = strtoul(optarg,&ep,0);
                    if (optarg[0] == '\0' || *ep != '\0' ||
                        (errno == ERANGE && ulval == ULONG_MAX)) {
                        print_error(stderr,
                                    "Invalid ending offset: %s\n",
                                    optarg);
                        err++;
                    } else {
                        offset_end = ulval;
                        have_end = 1;
                    }
                } else {
                    if (have_len) {
                        print_error(stderr,
                                    "Cannot specify both -%c and -%c\n",
                                    HD_OPT_END,
                                    HD_OPT_LEN);
                    }
                    err++;
                }
                break;
                
            case HD_OPT_LEN:
                
                if (optarg && *optarg != '\0' && have_end == 0) {
                    errno = 0;
                    len = strtol(optarg,&ep,0);
                    if (optarg[0] == '\0' || *ep != '\0' ||
                        (errno == ERANGE &&
                         (len == LONG_MAX || len == LONG_MIN))) {
                            print_error(stderr,
                                        "Invalid length: %s\n",
                                        optarg);
                            err++;
                    }
                    have_len = 1;
                } else {
                    if (have_end) {
                        print_error(stderr,
                                    "Cannot specify both -%c and -%c\n",
                                    HD_OPT_END,
                                    HD_OPT_LEN);
                    }
                    err++;
                }
                break;
                
            case HD_OPT_VERS:

                ver = 1;
                break;
                
            case HD_OPT_HELP1:
            case HD_OPT_HELP2:

                help = 1;
                break;
                
            default:
                
                print_error(stderr,"Unknown option: -%c\n",ch);
                err++;
        }

        if (err || ver || help) {
            break;
        }
    }

    if (ver) {
        fprintf(stdout,"Version: %s\n",g_version);
        exit(0);
    }
  
    if (help) {
        print_usage(argv[0]);
        exit(0);
    }

    if (have_len && have_end) {
        err++;
    }
    
    if (err) {
        print_usage(argv[0]);
        exit(1);
    }

    argc -= optind;
    argv += optind;
  
    if (have_end) {
        len = offset_end - offset_start;
        if (len < 0) {
            print_error(stderr,
                        "Ending offset is greater than starting offset\n");
            exit(1);
        }
    }

    /* if no file is specified, dump STDIN
       if 1 file is specified, dump that file
       if more than 1 file is specified, dump each file, with a separator */
    
    if (argc == 0) {
        hexdump(stdout,NULL,offset_start,len,cpl,flags);
    } else if (argc == 1) {
        hexdump(stdout,argv[0],offset_start,len,cpl,flags);
    } else {
        for (i=0;i<argc;i++) {
            fprintf(stdout,"::::::::::::::\n%s\n::::::::::::::\n",argv[i]);
            hexdump(stdout,argv[i],offset_start,len,cpl,flags);
        }
    }

    exit(0);
}
