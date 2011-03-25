#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uint8;

#define BUFSIZE 1000

uint8 buf[BUFSIZE];
uint8 *p;
uint8 *q;

int prefix = 0;

#define P printf
#define Q(...) (printf (" "), printf(__VA_ARGS__))
#define R printf ("\n")

#define ERR(format, args...) do {									\
		fprintf (stderr, format " at byte %d\n", ##args, p - buf);	\
		exit (1);													\
	} while (0)

void
print_prefix (int c)
{
	int i;
	for (i = 0; i < c; i++)
		printf ("%02hhx ", p[i]);
	for (; i < 5; i++)
		printf ("   ");
	for (i = 0; i < prefix; i++)
		printf ("    ");
}

int
get_value (void)
{
	switch (*p & 0x03) {
	case 0x00: return 0;
	case 0x01: return *(p+1);
	case 0x02: return *((short int *)(p + 1));
	case 0x03: return *((int *)(p + 1));
	}
	return 0;
}


void
parse_value (char *name)
{
	switch (*p & 0x03) {
	case 0x00:
		print_prefix (1);
		p++;
		P ("%s", name);
		break;
	case 0x01:
		print_prefix (2);
		p++;
		P ("%s (0x%02hhx %hhd)", name, *p, *p);
		p++;
		break;
	case 0x02:
		print_prefix (3);
		p++;
		P ("%s (0x%04hx %hd)", name, *((short int *)p), *((short int *)p));
		p += 2;
		break;
	case 0x03:
		print_prefix (5);
		p++;
		P ("%s (0x%08x %d)", name, *((int *)p), *((int *)p));
		p += 4;
		break;
	}
}

void
parse_iof (char *name)
{
	int val = get_value();
	
	parse_value (name);
	
	if (val & 0x01) Q ("Cons"); else Q ("Data");
	if (val & 0x02) Q ("Var"); else Q ("Array");
	if (val & 0x04) Q ("Rel"); else Q ("Abs");
	if (val & 0x08) Q ("Wrap");
	if (val & 0x10) Q ("NonLin");
	if (val & 0x20) Q ("NonPref");
	if (val & 0x40) Q ("Null");
	if (val & 0x80) Q ("Vol");
	if (val & 0x0100) Q ("BufferedBytes");
}


void
parse_collection (void)
{
	switch (get_value()) {
	case 0x00: parse_value ("COLLECTION"); Q ("Physical"); break;
	case 0x01: parse_value ("COLLECTION");  Q ("Application"); break;
	case 0x02: parse_value ("COLLECTION"); Q ("Logical"); break;
	case 0x03: parse_value ("COLLECTION"); Q ("Report"); break;
	case 0x04: parse_value ("COLLECTION"); Q ("Named Array"); break;
	case 0x05: parse_value ("COLLECTION"); Q ("Usage Switch"); break;
	case 0x06: parse_value ("COLLECTION"); Q ("Usage Modifier"); break;
	default: parse_value ("COLLECTION"); Q ("RESERVED"); break;
	}
}


void
parse_main (void)
{
	switch (*p & 0xF0) {
	case 0x80: parse_iof ("INPUT"); R; break;
	case 0x90: parse_iof ("OUTPUT"); R; break;
	case 0xB0: parse_iof ("FEATURE"); R; break;
	case 0xA0: parse_collection(); prefix++; R; break;
	case 0xC0: 
		prefix--; 
		parse_value ("END_COLLECTION"); 
		R; 
		if (prefix == 0)
			P ("\f\n");
		break;
	default: ERR ("Unknown main tag %x", *p & 0xF0);
	}
}

void 
parse_global (void)
{
	switch (*p & 0xF0) {
	case 0x00: parse_value ("USAGE PAGE"); break;
	case 0x10: parse_value ("LOGICAL MINIMUM"); break;
	case 0x20: parse_value ("LOGICAL MAXIMUM"); break;
	case 0x30: parse_value ("PHYSICAL MINIMUM"); break;
	case 0x40: parse_value ("PHYSICAL MAXIMUM"); break;
	case 0x50: parse_value ("UNIT EXPONENT"); break;
	case 0x60: parse_value ("UNIT"); break;
	case 0x70: parse_value ("REPORT SIZE"); break;
	case 0x80: parse_value ("REPORT ID"); break;
	case 0x90: parse_value ("REPORT COUNT"); break;
	case 0xA0: parse_value ("PUSH"); break;
	case 0xB0: parse_value ("POP"); break;
	default: ERR ("Unknown global tag %x", *p & 0xF0);
	}
}

void
parse_local (void)
{
	switch (*p & 0xF0) {
	case 0x00: parse_value ("USAGE"); break;
	case 0x10: parse_value ("USAGE MINIMUM"); break;
	case 0x20: parse_value ("USAGE MAXIMUM"); break;
	case 0x30: parse_value ("DESIGNATOR INDEX"); break;
	case 0x40: parse_value ("DESIGNATOR MINIMUM"); break;
	case 0x50: parse_value ("DESIGNATOR MAXIMUM"); break;
	case 0x70: parse_value ("STRING INDEX"); break;
	case 0x80: parse_value ("STRING MINIMUM"); break;
	case 0x90: parse_value ("STRING MAXIMUM"); break;
	case 0xA0: parse_value ("DELIMITER"); break;
	default: ERR ("Unknown local tag %x", *p & 0xF0);
	}
}

void
parse_long (void)
{
	ERR ("Sorry, can't understand LONG items");
}


void
parse (void)
{
	R;
	while (p != q) {
		switch (*p & 0x0C) {
		case 0x00: parse_main(); break;
		case 0x04: parse_global(); break;
		case 0x08: parse_local(); break;
		case 0x0C: parse_long(); break;
		default: ERR ("Unknown type");
		}
		R;
	}
}


int
main (int argc, char **argv)
{
	size_t i = 0;
	int num;
	
	while (i < BUFSIZE) {
		num = scanf (" %2hhx ", &buf[i]);
		if (num == EOF || num == 0)
			break;
		i++;
	}

	p = buf;
	q = buf + i;
	parse();
	
	return 0;
}
