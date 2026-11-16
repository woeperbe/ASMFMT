// asmfmt.c - line up the columns of 8080/Z80/M80/asmx assembler source files
//
//   label:  opcode  operands                ; comment
//
// usage: asmfmt [options] file...
//   -s          use spaces instead of tabs
//   -t          use tabs (default)
//   -w n        tab width (default 8)
//   -c o,a,c    columns for opcode, operands and comment (default 8,16,32)
//   -u          uppercase opcodes
//   -i          rewrite the files in place (the original is kept as file.bak)
//   -o file     write to this file (only with one input file)
//   without -i or -o the result goes to standard output
//
// Rules:
//   - a line starting in column 1 with ';' or '*' is a comment line: left alone
//   - a line whose first non-blank character is ';' is left alone too
//   - anything else in column 1 is a label (with or without ':' or '::')
//   - text in quotes and M80 <...> brackets is never split or changed
//   - line endings (CR/LF or LF) are kept, trailing blanks are removed
//   - everything from a CP/M end-of-file mark (^Z) on is copied unchanged
//
// Build:  cc -O2 -o asmfmt asmfmt.c
// (keep it out of the asmx source folder: the asmx Makefile compiles every .c file there)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int useTabs  = 1;
static int tabWidth = 8;
static int colOp    = 8;
static int colArg   = 16;
static int colCmt   = 32;
static int upperOp  = 0;

#define MAXLINE 4096

// ------------------------------------------------------------------

// display column after text, starting at column col
static int ColAfter(const char *s, int len, int col)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (s[i] == '\t') col = (col / tabWidth + 1) * tabWidth;
        else              col++;
    }
    return col;
}

// add blanks to move from column col to column target (always at least one blank)
static int Pad(char *out, int *n, int col, int target)
{
    if (col >= target)
    {   // no room: single separator
        if (useTabs) { out[(*n)++] = '\t'; return (col / tabWidth + 1) * tabWidth; }
        out[(*n)++] = ' ';
        return col + 1;
    }
    if (useTabs)
    {
        while ((col / tabWidth + 1) * tabWidth <= target)
        {
            out[(*n)++] = '\t';
            col = (col / tabWidth + 1) * tabWidth;
        }
    }
    while (col < target)
    {
        out[(*n)++] = ' ';
        col++;
    }
    return col;
}

// skip a quoted string or M80 <...> starting at s[i], returns index after it
// (an unmatched quote, like in Z80 "EX AF,AF'", is treated as a normal character)
static int SkipQuoted(const char *s, int i, int len)
{
    char q = s[i];
    int  j, depth;

    if (q == '\'' || q == '"')
    {
        for (j = i + 1; j < len; j++)
            if (s[j] == q)
            {
                if (j + 1 < len && s[j+1] == q) { j++; continue; }   // '' inside string
                return j + 1;
            }
        return i + 1;
    }

    if (q == '<')
    {   // only treat as brackets when a matching '>' exists
        depth = 0;
        for (j = i; j < len; j++)
        {
            if (s[j] == '<') depth++;
            else if (s[j] == '>' && --depth == 0) return j + 1;
            else if (s[j] == '\'' || s[j] == '"') j = SkipQuoted(s, j, len) - 1;
        }
        return i + 1;
    }

    return i + 1;
}

// position of the comment ';' in s, or len if there is none
static int FindComment(const char *s, int len)
{
    int i = 0;
    while (i < len)
    {
        if (s[i] == ';') return i;
        if (s[i] == '\'' || s[i] == '"')
            i = SkipQuoted(s, i, len);
        else if (s[i] == '<' && (i == 0 || s[i-1] == ',' || isspace((unsigned char)s[i-1])))
            i = SkipQuoted(s, i, len);
        else
            i++;
    }
    return len;
}

static int IsBlank(char c) { return c == ' ' || c == '\t' || c == 12; }

// format one line (without its line ending)
static void FormatLine(const char *in, int len, char *out, int *outLen)
{
    int  n = 0, col = 0;
    int  i, start, end, cmt;
    int  codeEnd;

    // remove trailing blanks
    while (len > 0 && IsBlank(in[len-1])) len--;

    *outLen = 0;
    if (len == 0) return;

    // comment lines and lines starting with a blank followed by ';': leave alone
    i = 0;
    while (i < len && IsBlank(in[i])) i++;
    if (in[0] == ';' || in[0] == '*' || in[i] == ';')
    {
        memcpy(out, in, len);
        *outLen = len;
        return;
    }

    cmt = FindComment(in, len);
    codeEnd = cmt;
    while (codeEnd > 0 && IsBlank(in[codeEnd-1])) codeEnd--;

    i = 0;

    // ---- label
    if (!IsBlank(in[0]))
    {
        start = 0;
        while (i < codeEnd && !IsBlank(in[i]) && in[i] != ':') i++;
        while (i < codeEnd && in[i] == ':') i++;     // keep ':' or '::'
        memcpy(out + n, in + start, i - start);
        n  += i - start;
        col = ColAfter(in + start, i - start, 0);
    }

    // ---- opcode
    while (i < codeEnd && IsBlank(in[i])) i++;
    if (i < codeEnd)
    {
        col = Pad(out, &n, col, colOp);
        start = i;
        while (i < codeEnd && !IsBlank(in[i])) i++;
        end = i;
        for (; start < end; start++)
            out[n++] = upperOp ? toupper((unsigned char)in[start]) : in[start];
        col = ColAfter(out, n, 0);

        // ---- operands (kept as written, inner blanks included)
        while (i < codeEnd && IsBlank(in[i])) i++;
        if (i < codeEnd)
        {
            col = Pad(out, &n, col, colArg);
            memcpy(out + n, in + i, codeEnd - i);
            n  += codeEnd - i;
            col = ColAfter(out, n, 0);
        }
    }

    // ---- comment
    if (cmt < len)
    {
        if (n > 0)
            col = Pad(out, &n, col, colCmt);
        memcpy(out + n, in + cmt, len - cmt);
        n += len - cmt;
    }

    *outLen = n;
}

// ------------------------------------------------------------------

static char *ReadFile(const char *name, long *size)
{
    FILE *f = fopen(name, "rb");
    char *buf;

    if (!f) { perror(name); return NULL; }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(*size + 1);
    if (!buf || fread(buf, 1, *size, f) != (size_t)*size)
    {
        fprintf(stderr, "%s: read error\n", name);
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    return buf;
}

static int FormatFile(const char *name, FILE *fo)
{
    long  size, pos, eof, lineStart, lineEnd;
    char *buf;
    char  out[MAXLINE * 2];
    int   outLen;
    char *z;

    buf = ReadFile(name, &size);
    if (!buf) return 1;

    // stop at CP/M end of file (^Z), the rest is copied unchanged
    z = memchr(buf, 0x1A, size);
    eof = z ? (z - buf) : size;

    pos = 0;
    while (pos < eof)
    {
        lineStart = pos;
        while (pos < eof && buf[pos] != '\n') pos++;
        lineEnd = pos;
        if (lineEnd > lineStart && buf[lineEnd-1] == '\r') lineEnd--;

        if (lineEnd - lineStart >= MAXLINE)
            fwrite(buf + lineStart, 1, lineEnd - lineStart, fo);  // too long: unchanged
        else
        {
            FormatLine(buf + lineStart, (int)(lineEnd - lineStart), out, &outLen);
            fwrite(out, 1, outLen, fo);
        }

        // keep the original line ending
        if (lineEnd < pos)     fputc('\r', fo);
        if (pos < eof)       { fputc('\n', fo); pos++; }
    }

    if (eof < size)
        fwrite(buf + eof, 1, size - eof, fo);

    free(buf);
    return 0;
}

static void Usage(void)
{
    fprintf(stderr,
        "asmfmt - line up assembler source columns\n"
        "usage: asmfmt [options] file...\n"
        "  -s          use spaces instead of tabs\n"
        "  -t          use tabs (default)\n"
        "  -w n        tab width (default 8)\n"
        "  -c o,a,c    columns for opcode, operands, comment (default 8,16,32)\n"
        "  -u          uppercase opcodes\n"
        "  -i          rewrite files in place (original kept as file.bak)\n"
        "  -o file     output file (one input file only)\n"
        "  default output is standard output\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int   i, inPlace = 0, err = 0, nfiles = 0;
    char *outName = NULL;
    char  bak[1024], tmp[1024];
    FILE *fo;

    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++)
    {
        switch (argv[i][1])
        {
            case 's': useTabs = 0; break;
            case 't': useTabs = 1; break;
            case 'u': upperOp = 1; break;
            case 'i': inPlace = 1; break;
            case 'w':
                if (++i >= argc) Usage();
                tabWidth = atoi(argv[i]);
                if (tabWidth < 1) Usage();
                break;
            case 'c':
                if (++i >= argc) Usage();
                if (sscanf(argv[i], "%d,%d,%d", &colOp, &colArg, &colCmt) != 3) Usage();
                if (colOp < 1 || colArg <= colOp || colCmt <= colArg) Usage();
                break;
            case 'o':
                if (++i >= argc) Usage();
                outName = argv[i];
                break;
            default:
                Usage();
        }
    }

    nfiles = argc - i;
    if (nfiles < 1) Usage();
    if (outName && (nfiles != 1 || inPlace)) Usage();

    for (; i < argc; i++)
    {
        if (inPlace)
        {
            snprintf(tmp, sizeof(tmp), "%s.fmt", argv[i]);
            snprintf(bak, sizeof(bak), "%s.bak", argv[i]);
            fo = fopen(tmp, "wb");
            if (!fo) { perror(tmp); err = 1; continue; }
            if (FormatFile(argv[i], fo)) { fclose(fo); remove(tmp); err = 1; continue; }
            fclose(fo);
            remove(bak);
            if (rename(argv[i], bak) || rename(tmp, argv[i]))
            {
                perror(argv[i]);
                err = 1;
            }
        }
        else if (outName)
        {
            fo = fopen(outName, "wb");
            if (!fo) { perror(outName); return 1; }
            err |= FormatFile(argv[i], fo);
            fclose(fo);
        }
        else
            err |= FormatFile(argv[i], stdout);
    }

    return err;
}
