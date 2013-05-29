##############################################################################
# Copyright (c) 1998-2007,2008 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
# $Id: MKcodes.awk,v 1.1 2009-05-21 08:33:45 steven Exp $
function large_item(value) {
	result = sprintf("%d,", offset);
	offset = offset + length(value) + 1;
	offcol = offcol + length(result) + 2;
	if (offcol > 70) {
		result = result "\n";
		offcol = 0;
	} else {
		result = result " ";
	}
	bigstr = bigstr sprintf("\"%s\\0\" ", value);
	bigcol = bigcol + length(value) + 5;
	if (bigcol > 70) {
		bigstr = bigstr "\\\n";
		bigcol = 0;
	}
	return result;
}

function small_item(value) {
	return sprintf("\t\t\"%s\",\n", value);
}

function print_strings(name,value) {
	printf  "DCL(%s) = {\n", name
	print  value
	print  "\t\t(NCURSES_CONST char *)0,"
	print  "};"
	print  ""
}

function print_offsets(name,value) {
	printf  "static const short _nc_offset_%s[] = {\n", name
	printf "%s",  value
	print  "};"
	print  ""
	printf "static NCURSES_CONST char ** ptr_%s = 0;\n", name
	print  ""
}

BEGIN	{
		print  "/* This file was generated by MKcodes.awk */"
		print  ""
		print  "#include <curses.priv.h>"
		print  ""
		print  "#define IT NCURSES_CONST char * const"
		print  ""
		offset = 0;
		offcol = 0;
		bigcol = 0;
	}

$1 ~ /^#/		{next;}

$1 == "SKIPWARN"	{next;}

$3 == "bool"	{
			small_boolcodes = small_boolcodes small_item($4);
			large_boolcodes = large_boolcodes large_item($4);
		}

$3 == "num"	{
			small_numcodes = small_numcodes small_item($4);
			large_numcodes = large_numcodes large_item($4);
		}

$3 == "str"	{
			small_strcodes = small_strcodes small_item($4);
			large_strcodes = large_strcodes large_item($4);
		}

END	{
		print  ""
		print  "#if BROKEN_LINKER || USE_REENTRANT"
		print  ""
		print  "#include <term.h>"
		print  ""
		if (bigstrings) {
			printf "static const char _nc_code_blob[] = \n"
			printf "%s;\n", bigstr;
			print_offsets("boolcodes", large_boolcodes);
			print_offsets("numcodes", large_numcodes);
			print_offsets("strcodes", large_strcodes);
			print  ""
			print  "static IT *"
			print  "alloc_array(NCURSES_CONST char ***value, const short *offsets, unsigned size)"
			print  "{"
			print  "	if (*value == 0) {"
			print  "		if ((*value = typeCalloc(NCURSES_CONST char *, size + 1)) != 0) {"
			print  "			unsigned n;"
			print  "			for (n = 0; n < size; ++n) {"
			print  "				(*value)[n] = _nc_code_blob + offsets[n];"
			print  "			}"
			print  "		}"
			print  "	}"
			print  "	return *value;"
			print  "}"
			print  ""
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return alloc_array(&ptr_##it, _nc_offset_##it, SIZEOF(_nc_offset_##it)); }"
		} else {
			print  "#define DCL(it) static IT data##it[]"
			print  ""
			print_strings("boolcodes", small_boolcodes);
			print_strings("numcodes", small_numcodes);
			print_strings("strcodes", small_strcodes);
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return data##it; }"
		}
		print  ""
		print  "FIX(boolcodes)"
		print  "FIX(numcodes)"
		print  "FIX(strcodes)"
		print  ""
		print  "#define FREE_FIX(it) if (ptr_##it) { FreeAndNull(ptr_##it); }"
		print  ""
		print  "#if NO_LEAKS"
		print  "NCURSES_EXPORT(void)"
		print  "_nc_codes_leaks(void)"
		print  "{"
		if (bigstrings) {
		print  "FREE_FIX(boolcodes)"
		print  "FREE_FIX(numcodes)"
		print  "FREE_FIX(strcodes)"
		}
		print  "}"
		print  "#endif"
		print  ""
		print  "#else"
		print  ""
		print  "#define DCL(it) NCURSES_EXPORT_VAR(IT) it[]"
		print  ""
		print_strings("boolcodes", small_boolcodes);
		print_strings("numcodes", small_numcodes);
		print_strings("strcodes", small_strcodes);
		print  ""
		print  "#endif /* BROKEN_LINKER */"
	}
