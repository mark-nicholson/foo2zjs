/*

GENERAL
This program converts bilevel PBM, 8-bit PGM, 24-bit PPM, and 32-bit
CMYK PAM files (output by Ghostscript as "pbmraw", "pgmraw", "ppmraw",
and "pamcmyk32" respectively) to HBPL version 1 for the consumption
of various Dell, Epson, and Fuji-Xerox printers.

With this utility, you can print to some Dell and Fuji printers, such as these:
    - Dell 1250c			B/W and Color
    - Dell C1660			B/W and Color
    - Dell C1760			B/W and Color
    - Epson AcuLaser C1700		B/W and Color
    - Fuji-Xerox DocuPrint CP105	B/W and Color

AUTHORS
This program was originally written by Dave Coffin in March 2014.

LICENSE
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

If you want to use this program under different license conditions,
then contact the author for an arrangement.

*/

static char Version[] = "$Id: foo2hbpl1.c,v 1.3 2014/03/30 05:08:32 rick Exp $";

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef linux
    #include <sys/utsname.h>
#endif

/*
 * Command line options
 */
char *ClutFilePath = NULL;
int DocType = 0;
int OutputColor = 0;
int DraftMode = 0;
int NumCopies = 1;
int Trapping = 0;
int Screen = 0;
int	MediaCode = 1;
char	*Username = NULL;
char	*Filename = NULL;
int	Clip[] = { 8,8,8,8 };

char *cb_cmyk = "CMYK";
char *cb_lmh = "LMH";
char ColorBalance[] =
/*   +--- L = Low
     |+-- M = Medium
     ||+- H = High
     |||                  */
    "333"  /* C = Cyan    */
    "333"  /* M = Magenta */
    "333"  /* Y = Yellow  */
    "333"; /* K = Black   */
/*
    0 = Lighter (-3)
    1 = Lighter (-2)
    2 = Lighter (-1)
  * 3 = Normal
    4 = Darker  (+1)
    5 = Darker  (+2)
    6 = Darker  (+3) */

void
usage(void)
{
    fprintf(stderr,
"Usage:\n"
"   foo2hbpl1 [options] <pamcmyk32-file >hbpl-file\n"
"\n"
"	Convert Ghostscript pbmraw, pgmraw, ppmraw, or pamcmyk32\n"
"	format to HBPLv1, for the Dell C1660w and other printers.\n"
"\n"
"	gs -q -dBATCH -dSAFER -dQUIET -dNOPAUSE \\ \n"
"		-sPAPERSIZE=letter -r600x600 -sDEVICE=pamcmyk32 \\ \n"
"		-sOutputFile=- - < testpage.ps \\ \n"
"	| foo2hbpl1 >testpage.zc\n"
"\n"
"Options:\n"
"-a values	Color balance adjustment [default: %s]\n"
"		Argument is a string of 12 adjustment values\n"
"		in range of 0..6 each:\n"
"		  0 = Lighter (-3)\n"
"		  1 = Lighter (-2)\n"
"		  2 = Lighter (-1)\n"
"		  3 = Normal\n"
"		  4 = Darker  (+1)\n"
"		  5 = Darker  (+2)\n"
"		  6 = Darker  (+3)\n"
"		in the following order:\n"
"		  |C|C|C|M|M|M|Y|Y|Y|K|K|K|\n"
"		  |l|m|h|l|m|h|l|m|h|l|m|h|\n"
"		where C=Cyan, M=Magenta, Y=Yellow, K=Black,\n"
"		l=Low, m=Medium, h=High.\n"
"-c		Print ppmraw in color mode.\n"
"		If not set print in black and white.\n"
"		Has no effect if document type (-i option) is 0\n"
"-i type		Document type code for print quality\n"
"		adjustment of ppmraw input [default: %d]\n"
"		  0=No adjustment, 1=Standard, 2=Photos,\n"
"		  3=Presentation, 4=Web Pages, 5=POP, 6=None\n"
"-l path	Pathname of the file containing color\n"
"		look-up tables. Must be specified for\n"
"		document types 1..5 in color mode.\n"
"-m media	Media code to send to printer [default: %d]\n"
"		   1=Plain\n"
"		   2=Plain, side 2\n"
"		   3=Bond\n"
"		   4=Bond, side 2\n"
"		   5=Recycled\n"
"		   6=Recycled, side 2\n"
"		   7=Labels\n"
"		   8=Lightweight Cardstock\n"
"		   9=Lightweight Cardstock, side 2\n"
"		  10=Lightweight Glossy Cardstock\n"
"		  11=Lightweight Glossy Cardstock, side 2\n"
"		  12=Letterhead\n"
"		  13=Letterhead, side 2\n"
"		  14=Pre-Printed\n"
"		  15=Pre-Printed, side 2\n"
"		  16=Hole Punched\n"
"		  17=Hole Punched, side 2\n"
"		  18=Colored\n"
"		  19=Colored, side 2\n"
"		  20=Custom Type\n"
"		  21=Custom Type, side 2\n"
"		  22=Special\n"
"		  23=Special, side 2\n"
"		  24=Envelope\n"
"-n copies	Number of copies, 1..100 [default: %d]\n"
"-s type		Screen type code [default: %d]\n"
"		  0=Auto, 1=Fineness, 2=Standard, 3=Gradation\n"
"-t		Print ppmraw in draft mode.\n"
"		Has no effect if document type (-i option) is 0\n"
"-u left,top,right,bottom\n"
"		Erase margins of specified width\n"
"		[default: %d,%d,%d,%d]\n"
"-J filename	Filename string to send to printer\n"
"-T		Turn trapping on.\n"
"-U username	Username string to send to printer\n"
"-V		Version %s\n"
	, ColorBalance
	, DocType
	, MediaCode
	, NumCopies
	, Screen 
	, Clip[0], Clip[1], Clip[2], Clip[3]
	, Version);
}

void
error(int fatal, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (fatal) exit(fatal);
}

enum doc_type_id {
    DOC_NORMAL,
    DOC_PHOTO,
    DOC_PRESENTATION,
    DOC_WEB,
    DOC_POP,
    DOC_NONE
};

struct doc_type_jobattr
{
    int color, mono;
} screen_jobattr_index[] = {
        {61, 64}, // Fineness
        {62, 65}, // Standard
        {63, 66}  // Gradation
    };

static const struct doc_type
{
    int clut_id;
    struct doc_type_jobattr jobattr;
} doc_type_index[] = {
        { 0, {11, 33}}, // Normal
        { 2, {14, 33}}, // Photo
        { 3, {15, 33}}, // Presentation
        { 5, {17, 33}}, // Web
        { 7, {19, 33}}, // POP
        {18, {31, 33}}  // Off
    };

struct stream
{
    unsigned char *buf;
    int size, off, bits;
};

void
putbits(struct stream *s, unsigned val, int nbits)
{
    if (s->off + 16 > s->size &&
	!(s->buf = realloc(s->buf, s->size += 0x100000)))
	    error (1, "Out of memory\n");
    if (s->bits)
    {
	s->off--;
	val |= s->buf[s->off] >> (8-s->bits) << nbits;
	nbits += s->bits;
    }
    s->bits = nbits & 7;
    while ((nbits -= 8) > 0)
	s->buf[s->off++] = val >> nbits;
    s->buf[s->off++] = val << -nbits;
}

/*
   Runlengths are integers between 1 and 17057 encoded as follows:

	1	00
	2	01 0
	3	01 1
	4	100 0
	5	100 1
	6	101 00
	7	101 01
	8	101 10
	9	101 11
	10	110 0000
	11	110 0001
	12	110 0010
	   ...
	25	110 1111
	26	111 000 000
	27	111 000 001
	28	111 000 010
	29	111 000 011
	   ...
	33	111 000 111
	34	111 001 000
	   ...
	41	111 001 111
	42	111 010 000
	50	111 011 0000
	66	111 100 00000
	98	111 101 000000
	162	111 110 000000000
	674	111 111 00000000000000
	17057	111 111 11111111111111
*/
void
put_len(struct stream *s, unsigned val)
{
    unsigned code[] =
    {
	  1, 0, 2,
	  2, 2, 3,
	  4, 8, 4,
	  6, 0x14, 5,
	 10, 0x60, 7,
	 26, 0x1c0, 9,
	 50, 0x3b0, 10,
	 66, 0x780, 11,
	 98, 0xf40, 12,
	162, 0x7c00, 15,
	674, 0xfc000, 20,
	17058
    };
    int c = 0;

    if (val < 1 || val > 17057) return;
    while (val >= code[c+3]) c += 3;
    putbits(s, val-code[c] + code[c+1], code[c+2]);
}

/*
   CMYK byte differences are encoded as follows:

	 0	000
	+1	001
	-1	010
	 2	011s0	s = 0 for +, 1 for -
	 3	011s1
	 4	100s00
	 5	100s01
	 6	100s10
	 7	100s11
	 8	101s000
	 9	101s001
	    ...
	 14	101s110
	 15	101s111
	 16	110s00000
	 17	110s00001
	 18	110s00010
	    ...
	 46	110s11110
	 47	110s11111
	 48	1110s00000
	 49	1110s00001
	    ...
	 78	1110s11110
	 79	1110s11111
	 80	1111s000000
	 81	1111s000001
	    ...
	 126	1111s101110
	 127	1111s101111
	 128	11111110000
*/
void
put_diff(struct stream *s, signed char val)
{
    static unsigned short code[] =
    {
	 2,  3, 3, 1,
	 4,  4, 3, 2,
	 8,  5, 3, 3,
	16,  6, 3, 5,
	48, 14, 4, 5,
	80, 15, 4, 6,
	129
    };
    int sign, abs, c = 0;

    switch (val)
    {
    case  0:  putbits(s, 0, 3);  return;
    case  1:  putbits(s, 1, 3);  return;
    case -1:  putbits(s, 2, 3);  return;
    }
    abs = ((sign = val < 0)) ? -val:val;
    while (abs >= code[c+4]) c += 4;
    putbits(s, code[c+1], code[c+2]);
    putbits(s, sign, 1);
    putbits(s, abs-code[c], code[c+3]);
}

void
setle(unsigned char *c, int s, int i)
{
    while (s--)
    {
	*c++ = i;
	i >>= 8;
    }
}

void
start_doc(int color)
{
    char reca[] = { 0x41,0x81,0xa1,0x00,0x82,0xa2,0x07,0x00,0x83,0xa2,0x01,0x00 };
    time_t t;
    struct tm *tmp;
    char datestr[16], timestr[16];
    char cname[128] = "My Computer";
    int i, doc_type_id;
    static const char *doc_type_jobattr_cprucr[] = {"CPR", "UCR"};
    static const char *doc_type_jobattr_trcscr[] = {"TRC", "SCR"};
    static const char *doc_type_jobattr_prefix = "TGI";
    int doc_type_jobattr = 0;
    const struct doc_type_jobattr *s_doc_type_jobattr = NULL;
    char *mname[] =
    {
        "NORMAL",
        "NORMALREV",
        "HIGHQUALITY",
        "HIGHQUALITYREV",
        "RECYCLE",
        "RECYCLEREV",
        "LABEL",
        "THICK",
        "THICKSIDE2",
        "COATEDPAPER2",
        "COATEDPAPER2REV",
        "LETTERHEAD",
        "LETTERHEADREV",
        "PREPRINTED",
        "PREPRINTEDREV",
        "PREPUNCHED",
        "PREPUNCHEDREV",
        "COLOR",
        "COLORREV",
        "USER1",
        "USER1REV",
        "SPECIAL",
        "SPECIALREV",
        "ENVELOPE"
    };

    t = time(NULL);
    tmp = localtime(&t);
    strftime(datestr, sizeof datestr, "%m/%d/%Y", tmp);
    strftime(timestr, sizeof timestr, "%H:%M:%S", tmp);

    #ifdef linux
    {
	struct utsname u;

	uname(&u);
	strncpy(cname, u.nodename, 128);
	cname[127] = 0;
    }
    #endif

/* Lines end with \n, not \r\n */

    printf(
        "\033%%-12345X@PJL JOB MODE=PRINTER\n"
        "@PJL SET STRINGCODESET=UTF8\n"
    );

    printf("@PJL COMMENT DATE=%s\n", datestr);
    printf("@PJL COMMENT TIME=%s\n", timestr);
    printf("@PJL COMMENT DNAME=%s\n", Filename ? Filename : "");
    printf("@PJL SET JOBATTR=\"@LUNA=%s\"\n", Username ? Username : "");
    printf("@PJL SET COPIES=%d\n", NumCopies);

    printf(
        "@PJL SET JOBATTR=\"@TRCH=OFF\"\n"
        "@PJL SET DUPLEX=OFF\n"
    );

    for (i = 0; i < 12; i++) {
        printf("@PJL SET JOBATTR=\"@AJ%c%c=%c\"\n",
            cb_cmyk[i / 3], cb_lmh[i % 3], ColorBalance[i]);
    }

    printf("@PJL SET JOBATTR=\"@APSP=OFF\"\n");

    printf("@PJL SET JOBATTR=\"@MSIP=%s\"\n", mname[MediaCode - 1]);
    printf("@PJL SET RENDERMODE=%s\n", color ? "COLOR" : "GRAYSCALE");
    printf("@PJL SET ECONOMODE=%s\n", DraftMode ? "ON" : "OFF");

    printf("@PJL SET JOBATTR=\"@IREC=OFF\"\n");

    printf("@PJL SET JOBATTR=\"@TRAP=%s\"\n",
        color && Trapping ? "ON" : "OFF");

    // "ON" only if 'Image Enhancement' option is enabled in original Xerox's driver.
    printf("@PJL SET RET=%s\n", "OFF");

    printf(
        "@PJL SET JOBATTR=\"@TDFT=0\"\n"
        "@PJL SET JOBATTR=\"@GDFT=0\"\n"
        "@PJL SET JOBATTR=\"@IDFT=0\"\n"
        "@PJL SET JOBATTR=\"@NLPP=1\"\n"
        "@PJL SET JOBATTR=\"@HOAD=IC0A809\"\n"
    );

    printf("@PJL SET JOBATTR=\"@JOAU=%s\"\n", Username ? Username : "");
    printf("@PJL SET JOBATTR=\"@CNAM=%s\"\n", cname);

    printf(
        "@PJL SET OUTBIN=FACEDOWN\n"
        "@PJL SET JOBATTR=\"@PODR=NORMAL\"\n"
        "@PJL SET SLIPSHEET=OFF\n"
        "@PJL SET PAPERDIRECTION=SEF\n"
        "@PJL SET RESOLUTION=600\n"
        "@PJL SET BITSPERPIXEL=8\n"
        "@PJL SET JOBATTR=\"@DRDM=RASTER\"\n"
    );

    doc_type_id = DocType == 0 ? 0 : DocType - 1;

    if (color) {
        for (i = 0; i < 6; i++) {
            printf("@PJL SET JOBATTR=\"@%c%s=%d\"\n",
                doc_type_jobattr_prefix[i % 3],
                doc_type_jobattr_cprucr[i / 3],
                doc_type_index[doc_type_id].jobattr.color);
        }
    }

    if (Screen == 0) {
        s_doc_type_jobattr = &doc_type_index[doc_type_id].jobattr;
    } else {
        s_doc_type_jobattr = &screen_jobattr_index[Screen - 1];
    }
    doc_type_jobattr = color ?
        s_doc_type_jobattr->color : s_doc_type_jobattr->mono;
    for (i = 0; i < 6; i++) {
        printf("@PJL SET JOBATTR=\"@%c%s=%d\"\n",
            doc_type_jobattr_prefix[i % 3],
            doc_type_jobattr_trcscr[i / 3],
            doc_type_jobattr);
    }

    if (!color) {
        printf("@PJL SET JOBATTR=\"@PBLK=OFF\"\n");
    }

    printf("@PJL SET JOBATTR=\"@BANR=DEVICE\"\n");

    if (doc_type_id == DOC_NONE) {
        printf("@PJL SET JOBATTR=\"@FCMS=ON\"\n");
    }

    printf("@PJL ENTER LANGUAGE=HBPL\n");

    fwrite (reca, 1, sizeof reca, stdout);
}

#define IP (((int *)image) + off)
#define CP (((char *)image) + off)
#define DP (((char *)image) + off*deep)
#define BP(x) ((blank[(off+x) >> 3] << ((off+x) & 7)) & 128)
#define put_token(s,x) putbits(s, huff[hsel][x] >> 4, huff[hsel][x] & 15)

void
encode_page(int color, int width, int height, char *image)
{
    unsigned char head[90] =
    {
	0x43,0x91,0xa1,0x00,0x92,0xa1,0x01,0x93,0xa1,0x01,0x94,0xa1,
	0x00,0x95,0xc2,0x00,0x00,0x00,0x00,0x96,0xa1,0x00,0x97,0xc3,
	0x00,0x00,0x00,0x00,0x98,0xa1,0x00,0x99,0xa4,0x01,0x00,0x00,
	0x00,0x9a,0xc4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x9b,
	0xa1,0x00,0x9c,0xa1,0x01,0x9d,0xa1,0x00,0x9e,0xa1,0x02,0x9f,
	0xa1,0x05,0xa0,0xa1,0x08,0xa1,0xa1,0x00,0xa2,0xc4,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x51,0x52,0xa3,0xa1,0x00,0xa4,
	0xb1,0xa4
    };
    unsigned char body[52] =
    {
	0x20,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x10,0x32,0x04,0x00,
	0xa1,0x42,0x00,0x00,0x00,0x00,0xff
    };
    static short papers[] =
    {	// Official sizes to nearest 1/600 inch
	// will accept +-1.5mm (35/600 inch) tolerance
	  0, 5100, 6600,	// Letter
	  2, 5100, 8400,	// Legal
	  4, 4961, 7016,	// A4
	  6, 4350, 6300,	// Executive
	 13, 2475, 5700,	// #10 envelope
	 15, 2325, 4500,	// Monarch envelope
	 17, 3827, 5409,	// C5 envelope
	 19, 2599, 5197,	// DL envelope
//	 ??, 4158, 5906,	// B5 ISO
	 22, 4299, 6071,	// B5 JIS
	 30, 3496, 4961,	// A5
	410, 5100, 7800,	// Folio
    };
    static const unsigned short huff[2][8] =
    {
	{ 0x01,0x63,0x1c5,0x1d5,0x1e5,0x22,0x3e6 }, // for text & graphics
	{ 0x22,0x63,0x1c5,0x1d5,0x1e5,0x01,0x3e6 }, // for images
    };
    unsigned char *blank;
    static int pagenum = 0;
    struct stream stream[5] = { { 0 } };
    int dirs[] = { -1,0,-1,1,2 }, rotor[] = { 0,1,2,3,4 };
    int i, j, row, col, deep, dir, run, try, bdir, brun, total;
    int paper = 510, hsel = 0, off = 0, bit = 0, stat = 0;
    int margin = width-96;

    for (i = 0; i < sizeof papers / sizeof *papers; i+=3) {
	if (abs(width-papers[i+1]) < 36 && abs(height-papers[i+2]) < 36) {
	    paper = papers[i];
            break;
        }
    }
    if (!MediaCode)
	MediaCode = paper & 1 ? 24 : 1;
    if (!pagenum)
	start_doc(color);
    head[12] = paper >> 1;
    if (paper == 510)
    {
	setle (head+15, 2,  (width*254+300)/600);  // units of 0.1mm
	setle (head+17, 2, (height*254+300)/600);
	head[21] = 2;
    }
    width = -(-width & -8);
    setle (head+33, 4, ++pagenum);
    setle (head+39, 4, width);
    setle (head+43, 4, height);
    setle (head+70, 4, width);
    setle (head+74, 4, height);
    head[55] = 9 + color*130;
    if (color)	body[6] = 1;
    else	body[4] = 8;

    deep = 1 + color*3;
    for (i=1; i < 5; i++)
	dirs[i] -= width;
    if (!color) dirs[4] = -8;

    blank = calloc(height+2, width/8);
    memset (blank++, -color, width/8+1);
    for (row = 1; row <= height; row++)
    {
	for (col = deep; col < deep*2; col++)
	    image[row*width*deep + col] = -1;
	for (col = 8; col < width*deep; col += 4)
	    if (*(int *)(image + row*width*deep + col))
	    {
		for (col = 12; col < margin/8; col++)
		    blank[row*(width/8)+col] = -1;
		blank[row*(width/8)+col] = -2 << (~margin & 7);
		break;
	    }
    }
    memset (image, -color, (width+1)*deep);
    image += (width+1)*deep;
    blank += width/8;

    while (off < width * height)
    {
	for (bdir = brun = dir = 0; dir < 5; dir++)
	{
	    try = dirs[rotor[dir]];
	    for (run = 0; run < 17057; run++, try++)
	    {
		if (color)
		{
		    if (IP[run] != IP[try]) break;
		}
		else
		    if (CP[run] != CP[try]) break;

		if (BP(run) != BP(try)) break;
	    }
	    if (run > brun)
	    {
		bdir = dir;
		brun = run;
	    }
	}
	if (brun == 0)
	{
	    put_token(stream, 5);
	    for (i = 0; i < deep; i++)
		put_diff(stream+1+i, DP[i] - DP[i-deep]);
	    bit = 0;
	    off++;
	    stat--;
	    continue;
	}
	if (brun > width * height - off)
	    brun = width * height - off;
	if (bdir)
	{
	    j = rotor[bdir];
	    for (i = bdir; i; i--)
		rotor[i] = rotor[i-1];
	    rotor[0] = j;
	}
	if ((off-1+brun)/width != (off-1)/width)
	{
	    if (abs(stat) > 8 && ((stat >> 31) & 1) != hsel)
	    {
		hsel ^= 1;
		put_token(stream, 6);
	    }
	    stat = 0;
	}
	stat += bdir == bit;
	put_token(stream, bdir - bit);
	put_len(stream, brun);
	bit = brun < 17057;
	off += brun;
    }

    putbits(stream, 0xff, 8);
    for (total = 48, i = 0; i <= deep; i++)
    {
	putbits(stream+i, 0xff, 8);
	stream[i].off--;
	setle (body+32 + i*4, 4, stream[i].off);
	total += stream[i].off;
    }
    head[85] = 0xa2 + (total > 0xffff)*2;
    setle (head+86, 4, total);
    fwrite(head, 1, 88+(total > 0xffff)*2, stdout);
    fwrite(body, 1, 48, stdout);
    for (i = 0; i <= deep; i++)
    {
	fwrite(stream[i].buf, 1, stream[i].off, stdout);
	free(stream[i].buf);
    }
    free(blank-width/8-1);
    printf("SD");
}
#undef IP
#undef CP
#undef DP
#undef BP
#undef put_token

#define CLUT_BLK0_OFFSET 0x374
#define CLUT_OFFSET 0x308
#define CLUT_SIZE (17 * 17 * 17 * 4)
#define CLUT_BLK_SIZE ((CLUT_OFFSET) + (CLUT_SIZE))
#define FILE_CLUT_OFFSET(i) ((CLUT_BLK0_OFFSET) + (CLUT_BLK_SIZE) * (i) + (CLUT_OFFSET))

#define CLUT_X_FACT (1 * 4)
#define CLUT_Y_FACT (17 * 4)
#define CLUT_Z_FACT (17 * 17 * 4)

#define RGB_TO_GRAY(r, g, b) ((1229 * (r) + 2417 * (g) + 451 * (b)) >> 12)

unsigned char*
load_clut(const int clut_id, const char *clut_fname)
{
    unsigned char *clut;
    FILE *clut_fp;

    if ((clut = malloc(CLUT_SIZE * sizeof(char))) == NULL) {
        error(1, "Can't allocate memory for CLUT\n");
    }

    if (!(clut_fp = fopen(clut_fname, "r"))) {
        error(1, "Can't open '%s' for reading\n", clut_fname);
    }

    if (fseek(clut_fp, FILE_CLUT_OFFSET(clut_id), SEEK_SET) != 0
        || fread(clut, sizeof(char), CLUT_SIZE, clut_fp) < CLUT_SIZE) {
        fclose(clut_fp);
        error(1, "Can't read CLUT #%d from '%s'\n", clut_id, clut_fname);
    }

    fclose(clut_fp);

    return clut;
}

// Borrowed from Little Color Management System, http://www.littlecms.com
// Copyright (c) 1998-2012 Marti Maria Saguer
// Licensed under the MIT license
// http://opensource.org/licenses/mit-license.php

// Tetrahedral interpolation, using Sakamoto algorithm.
void
rgb_to_kcmy(const unsigned char clut[], unsigned char kcmy[], const unsigned char rgb[])
{
    int i;
    int rx, ry, rz;
    int x0, y0, z0;
    int x1, y1, z1;
    const unsigned char *plane;
    int c0, c1, c2, c3;

    rx = rgb[0] & 0xf; ry = rgb[1] & 0xf; rz = rgb[2] & 0xf;

    if (rgb[0] == 255) {
        x0 = CLUT_X_FACT << 4; x1 = 0;
    } else {
        x0 = (rgb[0] >> 4) * (x1 = CLUT_X_FACT);
    }

    if (rgb[1] == 255) {
        y0 = CLUT_Y_FACT << 4; y1 = 0;
    } else {
        y0 = (rgb[1] >> 4) * (y1 = CLUT_Y_FACT);
    }

    if (rgb[2] == 255) {
        z0 = CLUT_Z_FACT << 4; z1 = 0;
    } else {
        z0 = (rgb[2] >> 4) * (z1 = CLUT_Z_FACT);
    }

    clut = &clut[x0 + y0 + z0];

    if (rx >= ry) {
        if (ry >= rz) {
            y1 += x1;
            z1 += y1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3]; // in KCMY order
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c3 -= c2;
                c2 -= c1;
                c1 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        } else if (rz >= rx) {
            x1 += z1;
            y1 += x1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3];
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c2 -= c1;
                c1 -= c3;
                c3 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        } else {
            z1 += x1;
            y1 += z1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3];
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c2 -= c3;
                c3 -= c1;
                c1 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        }
    } else {
        if (rx >= rz) {
            x1 += y1;
            z1 += x1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3];
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c3 -= c1;
                c1 -= c2;
                c2 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        } else if (ry >= rz) {
            z1 += y1;
            x1 += z1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3];
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c1 -= c3;
                c3 -= c2;
                c2 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        } else {
            y1 += z1;
            x1 += y1;
            for (i = 0; i < 4; i++) {
                plane = &clut[(i - 1) & 3];
                c1 = plane[x1];
                c2 = plane[y1];
                c3 = plane[z1];
                c0 = plane[0];
                c1 -= c2;
                c2 -= c3;
                c3 -= c0;
                kcmy[i] = c0 + ((c1 * rx + c2 * ry + c3 * rz) >> 4);
            }
        }
    }

    return;
}

enum proc_mode {
    PROC_BITMAP,
    PROC_GRAY,
    PROC_RGB_NOADJ,
    PROC_RGB_ADJ_COLOR,
    PROC_RGB_ADJ_COLOR_NONE,
    PROC_RGB_ADJ_COLOR_DRAFT,
    PROC_RGB_ADJ_COLOR_NONE_DRAFT,
    PROC_RGB_ADJ_MONO,
    PROC_RGB_ADJ_MONO_DRAFT,
    PROC_CMYK
};

enum margin_pos {
    LEFT,
    TOP,
    RIGHT,
    BOTTOM,
};

static unsigned char trc_rgb_color_std[] = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14,
    15, 16, 18, 19, 20, 21, 22, 23, 25, 26, 27, 28, 29, 31, 32, 33, 34, 35,
    36, 37, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94,
    95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
    125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
    153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166,
    167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
    209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
    223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236,
    237, 238, 239, 240, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
    250, 251, 252, 253, 254, 255
};

// Identity transformation
static unsigned char trc_rgb_color_none[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
    74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
    164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
    178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
    206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
    234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
    248, 249, 250, 251, 252, 253, 254, 255
};

static unsigned char trc_rgb_gray_std[] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 6, 7, 10, 12, 13, 15, 16, 19, 20,
    23, 23, 26, 27, 28, 31, 32, 34, 35, 36, 38, 40, 41, 43, 44, 45, 46, 48,
    50, 51, 52, 54, 55, 56, 57, 59, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71,
    74, 75, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93,
    94, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
    125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 141, 142, 143, 144, 145, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174, 175, 176, 177,
    178, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186, 187, 188, 189,
    190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200, 201,
    202, 203, 204, 204, 205, 206, 207, 208, 209, 209, 210, 211, 212, 213,
    214, 215, 215, 216, 217, 218, 219, 220, 220, 221, 222, 223, 224, 225,
    225, 226, 227, 228, 229, 229, 230, 231, 232, 233, 234, 234, 235, 236,
    237, 238, 238, 239, 240, 240, 241, 241, 242, 243, 244, 245, 245, 246,
    247, 248, 249, 249, 250, 251, 252, 253, 253, 254, 255
};

static unsigned char trc_rgb_gray_none[] = {
    0, 1, 1, 2, 2, 3, 4, 6, 7, 8, 10, 11, 13, 15, 16, 18, 20, 21, 23, 24,
    26, 27, 29, 30, 32, 33, 34, 36, 37, 39, 40, 41, 43, 44, 45, 47, 48, 49,
    50, 52, 53, 54, 55, 57, 58, 59, 60, 62, 63, 64, 65, 66, 68, 69, 70, 71,
    72, 73, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
    109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
    137, 138, 139, 140, 141, 142, 143, 144, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 158, 159, 160, 161, 162,
    163, 164, 165, 166, 167, 168, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 177, 178, 179, 180, 181, 182, 183, 184, 184, 185, 186, 187,
    188, 189, 190, 191, 191, 192, 193, 194, 195, 196, 197, 197, 198, 199,
    200, 201, 202, 203, 203, 204, 205, 206, 207, 208, 208, 209, 210, 211,
    212, 213, 214, 214, 215, 216, 217, 218, 219, 219, 220, 221, 222, 223,
    224, 224, 225, 226, 227, 228, 228, 229, 230, 231, 232, 233, 233, 234,
    235, 236, 237, 237, 238, 239, 240, 241, 241, 242, 243, 244, 245, 245,
    246, 247, 248, 249, 249, 250, 251, 252, 253, 253, 254, 255
};

static unsigned char trc_draft[] = {
    0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 14, 14,
    15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 23, 24, 25, 25, 26, 27,
    28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 37, 38, 39, 39,
    40, 41, 42, 42, 43, 44, 44, 45, 46, 46, 47, 48, 49, 49, 50, 51, 51, 52,
    53, 53, 54, 55, 56, 56, 57, 58, 58, 59, 60, 60, 61, 62, 62, 63, 64, 65,
    65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 72, 73, 74, 74, 75, 76, 77, 77,
    78, 79, 79, 80, 81, 81, 82, 83, 84, 84, 85, 86, 86, 87, 88, 88, 89, 90,
    91, 91, 92, 93, 93, 94, 95, 95, 96, 97, 98, 98, 99, 100, 100, 101, 102,
    102, 103, 104, 105, 105, 106, 107, 107, 108, 109, 109, 110, 111, 112,
    112, 113, 114, 114, 115, 116, 116, 117, 118, 118, 119, 120, 121, 121,
    122, 123, 123, 124, 125, 125, 126, 127, 128, 128, 129, 130, 130, 131,
    132, 133, 133, 134, 135, 135, 136, 137, 137, 138, 139, 140, 140, 141,
    142, 142, 143, 144, 144, 145, 146, 147, 147, 148, 149, 149, 150, 151,
    151, 152, 153, 154, 154, 155, 156, 156, 157, 158, 158, 159, 160, 161,
    161, 162, 163, 163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 170,
    171, 172, 172, 173, 174, 175, 175, 176, 177, 177, 178
};

void
detect_rgb_content(unsigned char *const image,
    const int wide, const int high, const int margins[4],
    int *is_color, int *is_blank)
{
    const int deep = 3;
    const int byte = wide * deep;
    int i, m[4];
    int start_row, end_row;
    int start_col, end_col;
    int start_col_off, end_col_off;
    unsigned char *rowp, *end_rowp, *startp, *endp, *p;
    int color_detected = 0;
    int gray_detected = 0;

    for (i = 0; i < 4; i++) {
        m[i] = margins[i] < 0 ? 0 : margins[i];
    }

    start_row = m[TOP];
    end_row = high - m[BOTTOM];
    start_col = m[LEFT];
    end_col = wide - m[RIGHT];

    if (start_row < end_row && start_col < end_col) {
        end_rowp = image + (end_row - 1) * byte;
        start_col_off = start_col * deep;
        end_col_off = (end_col - 1) * deep;

        for (rowp = image + start_row * byte;; rowp += byte) {
            startp = rowp + start_col_off;
            endp = rowp + end_col_off;

            for (p = startp;; p += deep) {
                if (p[0] != p[1] || p[1] != p[2]) {
                    color_detected = 1;
                    goto end_search;
                } else if (p[0] != 255) {
                    gray_detected = 1;
                }
                if (p == endp) {
                    break;
                }
            }
            if (rowp == end_rowp) {
                break;
            }
        }
    }

end_search: *is_color = color_detected;
    *is_blank = !(color_detected || gray_detected);

    return;
}

void
apply_trapping(unsigned char *image, int wide, int high, const int margins[4])
{
    int byte;
    int i, j, k, m[4], n[24];
    int skip_mlr, eff_byte;
    unsigned char *p, *row_endp, *img_endp, *np;

    for (i = 0; i < 4; i++) {
        m[i] = margins[i] > 1 ? margins[i] : 2;
    }

    if (m[TOP] < high - m[BOTTOM] && m[LEFT] < wide - m[RIGHT]) {

        byte = wide * 4;

        // Array `n` contains relative offsets
        // of the neighbor pixels in a 5x5 window.
        //
        // +----+----+----+----+----+
        // |  0 |  1 |  2 |  3 |  4 |
        // +----+----+----+----+----+
        // |  5 |  6 |  7 |  8 |  9 |
        // +----+----+---------+----+
        // | 10 | 11 |  p | 12 | 13 |
        // +----+----+---------+----+
        // | 14 | 15 | 16 | 17 | 18 |
        // +----+----+----+----+----+
        // | 19 | 20 | 21 | 22 | 23 |
        // +----+----+----+----+----+

        for (k = 0, i = -2; i < 3; i++) {
            for (j = -2; j < 3; j++) {
                if (i || j) {
                    n[k++] =  byte * i + 4 * j;
                }
            }
        }

        p = image + m[TOP] * byte + m[LEFT] * 4;
        img_endp = image + (high - m[BOTTOM]) * byte - m[RIGHT] * 4;
        skip_mlr = (m[LEFT] + m[RIGHT]) * 4;
        eff_byte = byte - skip_mlr;
        row_endp = p + eff_byte;

        for (; p != img_endp; p += 4) {
            if (p == row_endp) {
                p += skip_mlr;
                row_endp = p + eff_byte;
            }

            if (p[0] < 204) {
                continue;
            }

            for (i = 0; i < 24; i++) {
                np = p + n[i];
                if (np[0] < 204) {
                    for (j = 1; j < 4; j++) {
                        if (p[j] < np[j]) {
                            p[j] = np[j];
                        }
                    }
                }
            }
        }
    }

    return;
}

int
getint(FILE *fp)
{
    int c, ret;

    for (;;)
    {
	while (isspace(c = fgetc(fp)));
	if (c == '#')
	    while (fgetc(fp) != '\n');
	else break;
    }
    if (!isdigit(c)) return -1;
    for (ret = c-'0'; isdigit(c = fgetc(fp)); )
	ret = ret*10 + c-'0';
    return ret;
}

void
do_file(FILE *fp)
{
    int type, iwide, ihigh, ideep, imax, ibyte;
    int wide, deep, byte, row, col, i, k;
    char tupl[128], line[128];
    unsigned char *image, *sp, *dp;
    int buf_size, ibuf_size;
    unsigned char *sstart, *dstart;
    int doc_type_id = 0;
    const struct doc_type *s_doc_type;
    unsigned char *clut = NULL;
    enum proc_mode proc_mode;
    unsigned char *trc = trc_rgb_color_none;
    int is_color = 0;
    int is_blank = 0;

    while ((type = fgetc(fp)) != EOF)
    {
	type = ((type - 'P') << 8) | fgetc(fp);
	tupl[0] = iwide = ihigh = ideep = deep = imax = ibyte = -1;
	switch (type)
	{
	case '4':
	    deep = 1 + (ideep = 0);
            proc_mode = PROC_BITMAP;
	    goto six;
	case '5':
	    deep = ideep = 1;
            proc_mode = PROC_GRAY;
	    goto six;
	case '6':
            if (DocType != 0) {
                doc_type_id = DocType - 1;
                if (OutputColor) {
                    deep = 1 + (ideep = 3);
                    if (doc_type_id == DOC_NONE) {
                        proc_mode = DraftMode ?
                            PROC_RGB_ADJ_COLOR_NONE_DRAFT : PROC_RGB_ADJ_COLOR_NONE;
                    } else {
			if (!ClutFilePath) {
                            error(1, "CLUT file path (option -l) is not set.\n");
                        }
                        proc_mode = DraftMode ?
                            PROC_RGB_ADJ_COLOR_DRAFT : PROC_RGB_ADJ_COLOR;
                    }
                } else {
                    ideep = 3;
                    deep = 1;
                    proc_mode = DraftMode ?
                        PROC_RGB_ADJ_MONO_DRAFT : PROC_RGB_ADJ_MONO;
                }
            } else {
                deep = 1 + (ideep = 3);
                proc_mode = PROC_RGB_NOADJ;
            }
six:	    iwide = getint(fp);
	    ihigh = getint(fp);
	    imax = type == '4' ? 255 : getint(fp);
	    break;
	case '7':
	    do
	    {
		if (!fgets(line, 128, fp)) goto fail;
		if (!strncmp(line, "WIDTH ",6))
		    iwide = atoi(line + 6);
		if (!strncmp(line, "HEIGHT ",7))
		    ihigh = atoi(line + 7);
		if (!strncmp(line, "DEPTH ",6))
		    deep = ideep = atoi(line + 6);
		if (!strncmp(line, "MAXVAL ",7))
		    imax = atoi(line + 7);
		if (!strncmp(line, "TUPLTYPE ",9))
		    strcpy (tupl, line + 9);
	    } while (strcmp(line, "ENDHDR\n"));
	    if (ideep != 4 || strcmp(tupl, "CMYK\n")) goto fail;
            proc_mode = PROC_CMYK;
	    break;
	default:
	    goto fail;
	}
	if (iwide <= 0 || ihigh <= 0 || imax != 255) goto fail;
	wide = -(-iwide & -8);
        if (ideep)
	    ibyte = iwide * ideep;
	else
	    ibyte = wide >> 3;
	byte = wide * deep;

        buf_size = (ibyte > byte ? ibyte : byte) * ihigh + byte * 2;

        if ((image = malloc(buf_size)) == NULL) {
            error(1, "Can't allocate memory for page buffer\n");
        }

        dstart = image + byte;
        ibuf_size = ibyte * ihigh;
        sstart = image + (buf_size - ibuf_size);

        if (fread (sstart, ibuf_size, 1, fp) != 1) {
            error(1, "Error reading image data\n");
        }

        if (ideep == 3 && DocType != 0) {
            detect_rgb_content(sstart, iwide, ihigh, Clip,
                &is_color, &is_blank);

            if (doc_type_id == DOC_NONE) {
                trc = is_color ? trc_rgb_color_none : trc_rgb_gray_none;
            } else {
                s_doc_type = &doc_type_index[doc_type_id];
                clut = load_clut(s_doc_type->clut_id, ClutFilePath);
                trc = is_color ? trc_rgb_color_std : trc_rgb_gray_std;
            }
        }

	for (row = 1; row <= ihigh; row++)
	{
	    sp = sstart;
	    dp = dstart;
	    for (col = 0; col < iwide; col++)
	    {
		dp += deep;
		switch (proc_mode)
		{
		case PROC_BITMAP:
		    *dp = ((sp[col >> 3] >> (~col & 7)) & 1) * 255;
		    break;
		case PROC_GRAY:
		    *dp = ~*sp;
		    break;
		case PROC_RGB_NOADJ:
		    for (k = sp[2], i = 0; i < 2; i++)
			if (k < sp[i]) k = sp[i];
		    *dp = ~k;
		    for (i = 0; i < 3; i++)
			dp[i+1] = k ? (k - sp[i]) * 255 / k : 255;
		    break;
		case PROC_RGB_ADJ_COLOR:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    rgb_to_kcmy(clut, dp, sp);
		    break;
		case PROC_RGB_ADJ_COLOR_NONE:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    for (k = sp[2], i = 0; i < 2; i++)
                        if (k < sp[i]) k = sp[i];
                    *dp = ~k;
                    for (i = 0; i < 3; i++)
                        dp[i+1] = k - sp[i];
                    break;
		case PROC_RGB_ADJ_COLOR_DRAFT:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    rgb_to_kcmy(clut, dp, sp);
                    for (i = 0; i < 4; i++) {
                        dp[i] = trc_draft[dp[i]];
                    }
		    break;
		case PROC_RGB_ADJ_COLOR_NONE_DRAFT:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    for (k = sp[2], i = 0; i < 2; i++)
                        if (k < sp[i]) k = sp[i];
                    *dp = ~k;
                    for (i = 0; i < 3; i++)
                        dp[i+1] = k - sp[i];
                    for (i = 0; i < 4; i++) {
                        dp[i] = trc_draft[dp[i]];
                    }
                    break;
		case PROC_RGB_ADJ_MONO:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    *dp = ~RGB_TO_GRAY(sp[0], sp[1], sp[2]);
		    break;
		case PROC_RGB_ADJ_MONO_DRAFT:
                    for (i = 0; i < 3; i++) {
                        sp[i] = trc[sp[i]];
                    }
                    *dp = ~RGB_TO_GRAY(sp[0], sp[1], sp[2]);
                    for (i = 0; i < 4; i++) {
                        dp[i] = trc_draft[dp[i]];
                    }
		    break;
		case PROC_CMYK:
		    for (i=0; i < 4; i++)
			dp[i] = sp[((i-1) & 3)];
		    break;
		}
		sp += ideep;
	    }
	    sstart += ibyte;
	    dstart += byte;
	    for (i = 0; i < deep*Clip[0]; i++)
		image[row*byte + deep+i] = 0;
	    for (i = deep*(iwide-Clip[2]); i < byte; i++)
		image[row*byte + deep+i] = 0;
	}
	memset(image+deep, 0, byte*(Clip[1]+1));
	memset(image+deep + byte*(ihigh-Clip[3]+1), 0, byte*Clip[3]);
        if (deep == 4 && Trapping) {
            apply_trapping(image + byte + deep, iwide, ihigh, Clip);
        }
	encode_page(deep > 1, iwide, ihigh, (char *) image);
	free(image);
        if (ideep == 3 && DocType != 0 && doc_type_id != DOC_NONE) {
            free(clut);
        }
    }
    return;
fail:
    fprintf (stderr, "Not an acceptable PBM, PPM or PAM file!!!\n");
}

int
main(int argc, char *argv[])
{
    int	c, i;

    while ( (c = getopt(argc, argv, "a:ci:l:m:s:tu:J:TU:V")) != EOF)
	switch (c)
	{
	case 'a':
            for (i = 0; i < 12 && optarg[i]; i++) {
                if (optarg[i] >= '0' && optarg[i] <= '6') {
                    ColorBalance[i] = optarg[i];
                } else {
                    error(1, "Color balance values must be in range 0..6\n");
                }
            }
            break;
	case 'c':
		OutputColor = 1;
		break;
	case 'i':
		if (sscanf(optarg, "%d", &DocType) != 1
			|| DocType < 0 || DocType > 6
		) {
		    error(1, "Document type code must be in range 1..6.\n");
		}
		break;
	case 'l':  if (optarg[0]) ClutFilePath = optarg; break;
	case 'm':
            if (sscanf(optarg, "%d", &MediaCode) != 1
                || MediaCode < 1 || MediaCode > 24
            ) {
                error(1, "Media code must be in range 1..24.\n");
            }
            break;
	case 'n':
            if (sscanf(optarg, "%d", &NumCopies) != 1
                || NumCopies < 1 || NumCopies > 24
            ) {
                error(1, "Number of copies must be in range 1..100.\n");
            }
            break;
	case 's':
            if (sscanf(optarg, "%d", &Screen) != 1
                || Screen < 0 || Screen > 3
            ) {
		error(1, "Screen type code must be in range 0..3.\n");
            }
            break;
        case 't':
            DraftMode = 1;
            break;
	case 'u':  if (sscanf(optarg, "%d,%d,%d,%d",
			Clip, Clip+1, Clip+2, Clip+3) != 4)
		      error(1, "Must specify four clipping margins!\n");
		   break;
	case 'J':  if (optarg[0]) Filename = optarg; break;
        case 'T':
            Trapping = 1;
            break;
	case 'U':  if (optarg[0]) Username = optarg; break;
	case 'V':  printf("%s\n", Version); return 0;
	default:   usage(); return 1;
	}

    argc -= optind;
    argv += optind;

    if (argc == 0)
    {
	do_file(stdin);
    }
    else
    {
	for (i = 0; i < argc; ++i)
	{
	    FILE *ifp;

	    if (!(ifp = fopen(argv[i], "r")))
		error(1, "Can't open '%s' for reading\n", argv[i]);
	    do_file(ifp);
	    fclose(ifp);
	}
    }
    puts("B\033%-12345X@PJL EOJ");
    return 0;
}
