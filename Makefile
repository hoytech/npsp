CC=gcc
CFLAGS= -O2 -Wall -I/usr/local/include -s

LIBS= -L/usr/local/lib

BINNAME=npsp
DBINNAME=dnpsp

all:
	#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	#  Numerical Palindrome Search Program
	#
	#  This program is (C) 2002 by HardCore Software and defrost.
	#  This program is protected by the GNU General Public
	#  Licence. See the file COPYING for details.
	#
	#  Please choose which target you want to make:
	#
	#  make npsp
	#    -Fractal's original version, with many of defrost's
	#     optimizations.
	#
	#  make dnpsp
	#    -defrost's modification and complete code clean up
	#     on Fractal's code.
	#
	#  Refer to README if you aren't sure
	#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

npsp:
	rm -f $(BINNAME) $(BINNAME).exe
	$(CC) $(CFLAGS) -o $(BINNAME) npsp.c $(LIBS)

dnpsp:
	rm -f $(DBINNAME) $(DBINNAME).exe
	$(CC) $(CFLAGS) -o $(DBINNAME) dnpsp.c $(LIBS)

clean:
	rm -f $(BINNAME) *.o *.core *.out $(BINNAME).exe core core.* *~
	rm -f $(DBINNAME) *.o *.core *.out $(DBINNAME).exe core core.* *~
