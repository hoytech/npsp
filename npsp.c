/* NPSP - Numeric Palindrome Search Program

  (C) 2002, HardCore Software

  This program is protected by the GNU General Public License.
  See the file "COPYING" for details.

  Thanks to defrost for the malloc() tip, and a number of optimizations.

*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>


#define VERSION "1.1"



struct config_struct {
  int memoryused;

  int maxdigits;
  int base;
  int autosavedelay;

  int deathrow; /* Will be non-zero if we've gotten an appropriate signal */

  int currdigits;
  int curriters;

  int origseed;
  int origdigits;
  int origiters;

  int freenumberwhenloaded;
  char *number;
  char *scratch;

  char *infile;
  char *outfile;
};

struct config_struct cfg;



void die() {

  cfg.deathrow=1;

}



void *defrost_malloc(size_t size) {

  void *tp;

  if ((tp = malloc(size + sizeof(size_t))) == NULL) {
    fprintf(stderr, "OUT OF MEMORY!\n");
    exit(0);
  }

  cfg.memoryused += size;
  memcpy(tp, &size, sizeof(size_t));

  return tp+sizeof(size_t);

}


void defrost_free(void *tp) {

  size_t tpsize;

  memcpy(&tpsize, tp-sizeof(size_t), sizeof(size_t));
  cfg.memoryused -= tpsize;

  free(tp - sizeof(size_t));

}
  



int storedigits(char *tostore, char *buf) {

  int i,len;

  len = strlen(tostore);

  if (tostore[0] == '0') {
    fprintf(stderr, "Number has leading 0s.\n");
    exit(0);
  }

  for(i=0; i<len; i++) {
    if ((tostore[i] < 48 || tostore[i] > 58) && (tostore[i] < 65 || tostore[i] > 70)) {
      fprintf(stderr, "Number is not valid.\n");
      exit(0);
    }
    buf[len-i-1] = tostore[i]-48;
    if (buf[len-i-1] > 9) buf[len-i-1] = tostore[i]-65+10;

    if (buf[len-i-1] >= cfg.base) {
      fprintf(stderr, "Number does not appear to be in correct base.\n");
      exit(0);
    }
  }

  return strlen(tostore);
  
}



int oneiter(char *buf, char *scratch, int digits, int base) {

  int i, tp, carry=0;

  for (i=0; i < digits; i++) {

    tp = scratch[i];
    if (carry) tp++;
    carry = 0;

    tp += scratch[digits-i-1];

    if (tp >= base) {
      carry=1;
      tp -= base;
    }

    buf[i] = tp;
    if (i == digits-1 && carry) buf[i+1] = 1;
  }
  
  return digits + carry;

}




char *getbignumber(char *buf, int digits) {

  int i;
  char *tp = (char *) defrost_malloc(digits+10);

  memset(tp, '\0', digits+10);

  for (i=0; i<digits; i++) {
    tp[i] = buf[digits-i-1]+48;
    if (tp[i] >= 58) tp[i] = buf[digits-i-1]+65-10;
  }

  return tp;

}



/* Here's a neat recursive function for ya.
   This, of course, is a real memory killer, and not great on speed,
   but I'll leave it here because it's cool. */
/*
int ispalin(char *buf, int digits) {
  if (digits <= 1) return 1;
  if (buf[0] != buf[digits-1]) return 0;
  return ispalin(buf+1, digits-2);
}
*/


/* defrost's optimized one (Reindented/reformatted by me) */
int ispalin(char *buf, int len) {

  char    *s = buf ;
  char    *e = buf + len - 1 ;

  while( e > s )
    if( *s != *e ) return( 0 );
  else
    ++s, --e ;
  return( 1 ) ;

}



void usage() {

  fprintf(stderr,"Numeric Palindrome Search Program (NPSP) V%s\n", VERSION);
  fprintf(stderr,"(C) 2002, HardCore SoftWare\n\n");
  fprintf(stderr,"usage: npsp <options>\n\n");
  fprintf(stderr,"            -i <input file (Istvan Standard Format)>\n");
  fprintf(stderr,"            -o <output file (Istvan Standard Format)>\n");
  fprintf(stderr,"            -n <seed number>\n");
  fprintf(stderr,"            -m <maximum # of digits to find>\n");
  fprintf(stderr,"            -b <numeric base>\n");
  fprintf(stderr,"            -a <seconds between autosaves>\n\n");

  exit(0);

}




void savefile() {

  FILE *fd;
  char *tp;
  char tpbuf[80];
  int i=0,len=0;

  if (cfg.outfile == NULL) return;

  fd = fopen(cfg.outfile,"w");

  if (fd == NULL) {
    fprintf(stderr, "Unable to open %s\n", cfg.outfile);
    exit(0);
  }

  fprintf(fd, "Automatic save #1%c\n",13);
  fprintf(fd, "Initial value:    %d%c\n", cfg.origseed, 13);
  fprintf(fd, "Iteration:        %d%c\n", cfg.curriters+cfg.origiters, 13);
  fprintf(fd, "Number of digits: %d%c\n", cfg.currdigits, 13);

  tp = getbignumber(cfg.number, cfg.currdigits);

  len = strlen(tp);

  memset(tpbuf, '\0', 80);

  printf("Saving file... %%00.00");
  fflush(stdout);

  while(1) {

    if ((i & 0xFF) == 0) {
      printf("\rSaving file... %%%05.2f", 100.*i/len); 
      fflush(stdout);
    }

    strncpy(tpbuf, tp+i, 70);

    i+=70;

    if (i<len) fprintf(fd, "%s%c\n", tpbuf, 13);
    else break;


  }

  fprintf(fd, "%s", tpbuf);

  printf("\rSaving file... Done. \n");
  fflush(stdout);

  defrost_free(tp);
  fclose(fd);


}





void loadfile() {

  FILE *fd;
  char buf[100];
  int currlen=0,tp;

  if (cfg.infile == NULL) return;

  fd = fopen(cfg.infile,"r");

  if (fd == NULL) {
    fprintf(stderr, "Unable to open %s\n", cfg.infile);
    exit(0);
  }

  fgets(buf, sizeof(buf)-1, fd);
  if (strncmp(buf, "Automatic save", 13) != 0) {
    fprintf(stderr, "%s doesn't seem to be Istvan-Standard Formatted.\n", cfg.infile);
    exit(0);
  }

  fgets(buf, sizeof(buf)-1, fd);
  if (strncmp(buf, "Initial value: ", 14) != 0) {
    fprintf(stderr, "%s doesn't seem to be Istvan-Standard Formatted.\n", cfg.infile);
    exit(0);
  }

  for(tp=14; buf[tp] == ' '; tp++)  ;
  cfg.origseed = atoi(buf+tp);

  fgets(buf, sizeof(buf)-1, fd);
  if (strncmp(buf, "Iteration: ", 10) != 0) {
    fprintf(stderr, "%s doesn't seem to be Istvan-Standard Formatted.\n", cfg.infile);
    exit(0);
  }

  for(tp=11; buf[tp] == ' '; tp++)  ;
  cfg.origiters = atoi(buf+tp);

  fgets(buf, sizeof(buf)-1, fd);
  if (strncmp(buf, "Number of digits: ", 17) != 0) {
    fprintf(stderr, "%s doesn't seem to be Istvan-Standard Formatted.\n", cfg.infile);
    exit(0);
  }

  for(tp=18; buf[tp] == ' '; tp++)  ;
  cfg.origdigits = atoi(buf+tp);

  cfg.number = (char *) defrost_malloc(cfg.origdigits+10);
  cfg.freenumberwhenloaded = 1;

  printf("Loading file... %%00.00");
  fflush(stdout);

  while(fgets(buf, sizeof(buf)-1, fd) != NULL) {
    if ((currlen & 0xFF) == 0) {
      printf("\rLoading file... %%%05.2f", 100.*currlen/cfg.origdigits); 
      fflush(stdout);
    }

    for (tp = strlen(buf)-1; buf[tp] == '\n' || buf[tp] == 13; ) {
      buf[tp] = '\0';
      tp--;
    }
    strcpy(cfg.number+currlen, buf);
    currlen+=tp+1;
  }

  printf("\rLoading file... Done. \n");
  fflush(stdout);

  if (currlen != cfg.origdigits) {
    fprintf(stderr, "%s does not have the right number of digits!\n", cfg.infile);
    exit(0);
  }

  fclose(fd);

}


void loaddefaults() {

  cfg.memoryused=0;

  cfg.deathrow=0;

  cfg.maxdigits=100;
  cfg.base=10;
  cfg.autosavedelay=3600; /* Save every hour */

  cfg.currdigits=0;
  cfg.curriters=0;

  cfg.origseed=0;
  cfg.origdigits=0;
  cfg.origiters=0;

  cfg.freenumberwhenloaded=0;
  cfg.number=(char *)NULL;
  cfg.scratch=(char *)NULL;

  cfg.infile=(char *)NULL;
  cfg.outfile=(char *)NULL;

}



int itspersec() {

  static struct timeval lasttime;
  static int lastiters;

  float tp=0;
  struct timeval currtime;

  gettimeofday(&currtime, NULL);

  tp = (float)(cfg.curriters-lastiters);
  tp /= (float)(currtime.tv_sec) + (currtime.tv_usec/1000000.) -
        (float)(lasttime.tv_sec) - (lasttime.tv_usec/1000000.);

  lastiters = cfg.curriters;
  lasttime.tv_sec = currtime.tv_sec;
  lasttime.tv_usec = currtime.tv_usec;

  return (int)(tp+.5);

}

  


void printheartbeat(int state) {

  static char statesbuf[] = "|/-\\";

  printf("%c   ", *(statesbuf+state));

}




int doit() {

  char *tp;
  int heartbeat;
  int nextautosave;
  int tptime;
  /* So we don't have to do a float div every heartbeat - thanks defrost */
  float progfactor=100.0/(cfg.maxdigits-cfg.origdigits);

  heartbeat = time(NULL);
  nextautosave = time(NULL)+cfg.autosavedelay;

  if (cfg.infile == NULL) {
    tp = getbignumber(cfg.number, cfg.currdigits);
    printf("Starting with %s, going to %d digits, using base %d.\n", tp, cfg.maxdigits, cfg.base);
    defrost_free(tp);
  } else {
    printf("Starting with number from \"%s\", going to %d digits, using base %d.\n", cfg.infile, cfg.maxdigits, cfg.base);
  }

  while(1) {
    if (ispalin(cfg.number, cfg.currdigits)) {
      tp = getbignumber(cfg.number, cfg.currdigits);
      printf("\nFOUND (%d ITERS, %d DIGITS): %s\n", cfg.curriters, cfg.currdigits, cfg.outfile == NULL ? tp : cfg.outfile);
      defrost_free(tp);
      break;
    }
    if (cfg.currdigits >= cfg.maxdigits) {
      tp = getbignumber(cfg.number, cfg.currdigits);
      printf("\nUNABLE TO FIND PALINDROME (%d ITERS, %d DIGITS): %s\n", cfg.curriters, cfg.currdigits, cfg.outfile == NULL ? tp : cfg.outfile);
      defrost_free(tp);
      break;
    }

    tp = cfg.number;
    cfg.number = cfg.scratch;
    cfg.scratch = tp;

    cfg.currdigits = oneiter(cfg.number, cfg.scratch, cfg.currdigits, cfg.base);
    cfg.curriters++;

    /* Another of defrost's optimizations */
    if((0 == (cfg.curriters & 0xFF)) && heartbeat <= (tptime = time(NULL))) {
      if (cfg.deathrow) {
        savefile();
        printf("\nGot signal. Saving your file if -o was used...\nExiting.\n");
        exit(0);
      }
      heartbeat = tptime + 1;
      if (nextautosave <= tptime && (cfg.outfile != NULL)) {
        savefile();
        printf("\nAutosaved to \"%s\"\n", cfg.outfile);
        nextautosave = tptime + cfg.autosavedelay; 
      } 
      printf("\r");
      printf("Progress: %%%.2f Its/sec: %d Digits: %d Its: %d Mem: %.1f kB    ", progfactor*(cfg.currdigits-cfg.origdigits), itspersec(), cfg.currdigits, cfg.curriters+cfg.origiters, ((float)cfg.memoryused)/1000);
      printheartbeat(tptime%4);
      fflush(stdout);
    }

  }

  savefile();

  return 0;

}




int main(int argc, char *argv[]) {

  int i;
  char *tp;

  loaddefaults();

  if (argc == 1 || argc%2 == 0) usage();

  for(i=1; i<argc-1; i+=2) {
    if(strncmp(argv[i], "-m", 2) == 0) {
      if(!isdigit(argv[i+1][0])) usage();
      cfg.maxdigits = atoi(argv[i+1]);
    }
    if(strncmp(argv[i], "-b", 2) == 0) {
      if(!isdigit(argv[i+1][0])) usage();
      cfg.base = atoi(argv[i+1]);
    }
    if(strncmp(argv[i], "-a", 2) == 0) {
      if(!isdigit(argv[i+1][0])) usage();
      cfg.autosavedelay = atoi(argv[i+1]);
    }
    if(strncmp(argv[i], "-i", 2) == 0) {
      cfg.infile = argv[i+1];
    }
    if(strncmp(argv[i], "-o", 2) == 0) {
      cfg.outfile = argv[i+1];
    }
    if(strncmp(argv[i], "-n", 2) == 0) {
      cfg.number = argv[i+1]; /* TEMPORARY */
      cfg.freenumberwhenloaded = 0; /* Don't wanna free() an argv... */
    }
  }

  if (cfg.number != NULL) {
    cfg.origseed = atoi(cfg.number);
    cfg.origiters = 0;
    cfg.origdigits = strlen(cfg.number);
  }

  loadfile();

  cfg.origdigits = strlen(cfg.number);
  if (cfg.origdigits >= cfg.maxdigits) {
    fprintf(stderr, "Your number is already %d digits or larger.\n", cfg.maxdigits);
    exit(0);
  }

  tp = (char *) defrost_malloc(cfg.maxdigits+10); /* +10 is just in case */
  cfg.scratch = (char *) defrost_malloc(cfg.maxdigits+10);
  memset(tp, '\0', cfg.maxdigits+10);
  memset(cfg.scratch, '\0', cfg.maxdigits+10);

  memcpy(tp, cfg.number, cfg.origdigits);
  cfg.currdigits = storedigits(cfg.number, tp); 
  if (cfg.freenumberwhenloaded) defrost_free(cfg.number);
  cfg.number = tp;

  printf("Numeric Palindrome Search Program (NPSP) V%s\n", VERSION);
  printf("(C) 2002, HardCore SoftWare\n\n");

  signal(SIGINT, die);
  signal(SIGQUIT, die);
  signal(SIGTERM, die);

  doit();

  return 0;

}
