/*
** 2003 October 31
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains the C functions that implement date and time
** functions for SQLite.  
**
** There is only one exported symbol in this file - the function
** sqliteRegisterDateTimeFunctions() found at the bottom of the file.
** All other code has file scope.
**
** $Id: date.c,v 1.1 2003/11/01 01:53:54 drh Exp $
**
** NOTES:
**
** SQLite processes all times and dates as Julian Day numbers.  The
** dates and times are stored as the number of days since noon
** in Greenwich on November 24, 4714 B.C. according to the Gregorian
** calendar system.
**
** 1970-01-01 00:00:00 is JD 2440587.5
** 2000-01-01 00:00:00 is JD 2451544.5
**
** This implemention requires years to be expressed as a 4-digit number
** which means that only dates between 0000-01-01 and 9999-12-31 can
** be represented, even though julian day numbers allow a much wider
** range of dates.
**
** The Gregorian calendar system is used for all dates and times,
** even those that predate the Gregorian calendar.  Historians usually
** use the Julian calendar for dates prior to 1582-10-15 and for some
** dates afterwards, depending on locale.  Beware of this difference.
**
** The conversion algorithms are implemented based on descriptions
** in the following text:
**
**      Jean Meeus
**      Astronomical Algorithms, 2nd Edition, 1998
**      ISBM 0-943396-61-1
**      Willmann-Bell, Inc
**      Richmond, Virginia (USA)
*/
#ifndef SQLITE_OMIT_DATETIME_FUNCS
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include "sqliteInt.h"
#include "os.h"

/*
** A structure for holding a single date and time.
*/
typedef struct DateTime DateTime;
struct DateTime {
  double rJD;      /* The julian day number */
  int Y, M, D;     /* Year, month, and day */
  int h, m;        /* Hour and minutes */
  int tz;          /* Timezone offset in minutes */
  double s;        /* Seconds */
  char validYMD;   /* True if Y,M,D are valid */
  char validHMS;   /* True if h,m,s are valid */
  char validJD;    /* True if rJD is valid */
  char validTZ;    /* True if tz is valid */
};


/*
** Convert N digits from zDate into an integer.  Return
** -1 if zDate does not begin with N digits.
*/
static int getDigits(const char *zDate, int N){
  int val = 0;
  while( N-- ){
    if( !isdigit(*zDate) ) return -1;
    val = val*10 + *zDate - '0';
    zDate++;
  }
  return val;
}

/*
** Read text from z[] and convert into a floating point number.  Return
** the number of digits converted.
*/
static int getValue(const char *z, double *pR){
  double r = 0.0;
  double rDivide = 1.0;
  int isNeg = 0;
  int nChar = 0;
  if( *z=='+' ){
    z++;
    nChar++;
  }else if( *z=='-' ){
    z++;
    isNeg = 1;
    nChar++;
  }
  if( !isdigit(*z) ) return 0;
  while( isdigit(*z) ){
    r = r*10.0 + *z - '0';
    nChar++;
    z++;
  }
  if( *z=='.' && isdigit(z[1]) ){
    z++;
    nChar++;
    while( isdigit(*z) ){
      r = r*10.0 + *z - '0';
      rDivide *= 10.0;
      nChar++;
      z++;
    }
    r /= rDivide;
  }
  if( *z!=0 && !isspace(*z) ) return 0;
  *pR = isNeg ? -r : r;
  return nChar;
}

/*
** Parse a timezone extension on the end of a date-time.
** The extension is of the form:
**
**        (+/-)HH:MM
**
** If the parse is successful, write the number of minutes
** of change in *pnMin and return 0.  If a parser error occurs,
** return 0.
**
** A missing specifier is not considered an error.
*/
static int parseTimezone(const char *zDate, DateTime *p){
  int sgn = 0;
  int nHr, nMn;
  while( isspace(*zDate) ){ zDate++; }
  p->tz = 0;
  if( *zDate=='-' ){
    sgn = -1;
  }else if( *zDate=='+' ){
    sgn = +1;
  }else{
    return *zDate!=0;
  }
  zDate++;
  nHr = getDigits(zDate, 2);
  if( nHr<0 || nHr>14 ) return 1;
  zDate += 2;
  if( zDate[0]!=':' ) return 1;
  zDate++;
  nMn = getDigits(zDate, 2);
  if( nMn<0 || nMn>59 ) return 1;
  zDate += 2;
  p->tz = sgn*(nMn + nHr*60);
  while( isspace(*zDate) ){ zDate++; }
  return *zDate!=0;
}

/*
** Parse times of the form HH:MM or HH:MM:SS or HH:MM:SS.FFFF.
** The HH, MM, and SS must each be exactly 2 digits.  The
** fractional seconds FFFF can be one or more digits.
**
** Return 1 if there is a parsing error and 0 on success.
*/
static int parseHhMmSs(const char *zDate, DateTime *p){
  int h, m, s;
  double ms = 0.0;
  h = getDigits(zDate, 2);
  if( h<0 || zDate[2]!=':' ) return 1;
  zDate += 3;
  m = getDigits(zDate, 2);
  if( m<0 || m>59 ) return 1;
  zDate += 2;
  if( *zDate==':' ){
    s = getDigits(&zDate[1], 2);
    if( s<0 || s>59 ) return 1;
    zDate += 3;
    if( *zDate=='.' && isdigit(zDate[1]) ){
      double rScale = 1.0;
      zDate++;
      while( isdigit(*zDate) ){
        ms = ms*10.0 + *zDate - '0';
        rScale *= 10.0;
        zDate++;
      }
      ms /= rScale;
    }
  }else{
    s = 0;
  }
  p->validJD = 0;
  p->validHMS = 1;
  p->h = h;
  p->m = m;
  p->s = s + ms;
  if( parseTimezone(zDate, p) ) return 1;
  p->validTZ = p->tz!=0;
  return 0;
}

/*
** Convert from YYYY-MM-DD HH:MM:SS to julian day.  We always assume
** that the YYYY-MM-DD is according to the Gregorian calendar.
**
** Reference:  Meeus page 61
*/
static void computeJD(DateTime *p){
  int Y, M, D, A, B, X1, X2;

  if( p->validJD ) return;
  if( p->validYMD ){
    Y = p->Y;
    M = p->M;
    D = p->D;
  }else{
    Y = 2000;
    M = 1;
    D = 1;
  }
  if( M<=2 ){
    Y--;
    M += 12;
  }
  A = Y/100;
  B = 2 - A + (A/4);
  X1 = 365.25*(Y+4716);
  X2 = 30.6001*(M+1);
  p->rJD = X1 + X2 + D + B - 1524.5;
  p->validJD = 1;
  p->validYMD = 0;
  if( p->validHMS ){
    p->rJD += (p->h*3600.0 + p->m*60.0 + p->s)/86400.0;
    if( p->validTZ ){
      p->rJD += p->tz*60/86400.0;
      p->validHMS = 0;
      p->validTZ = 0;
    }
  }
}

/*
** Parse dates of the form
**
**     YYYY-MM-DD HH:MM:SS.FFF
**     YYYY-MM-DD HH:MM:SS
**     YYYY-MM-DD HH:MM
**     YYYY-MM-DD
**
** Write the result into the DateTime structure and return 0
** on success and 1 if the input string is not a well-formed
** date.
*/
static int parseYyyyMmDd(const char *zDate, DateTime *p){
  int Y, M, D;

  Y = getDigits(zDate, 4);
  if( Y<0 || zDate[4]!='-' ) return 1;
  zDate += 5;
  M = getDigits(zDate, 2);
  if( M<=0 || M>12 || zDate[2]!='-' ) return 1;
  zDate += 3;
  D = getDigits(zDate, 2);
  if( D<=0 || D>31 ) return 1;
  zDate += 2;
  while( isspace(*zDate) ){ zDate++; }
  if( isdigit(*zDate) ){
    if( parseHhMmSs(zDate, p) ) return 1;
  }else if( *zDate==0 ){
    p->validHMS = 0;
  }else{
    return 1;
  }
  p->validJD = 0;
  p->validYMD = 1;
  p->Y = Y;
  p->M = M;
  p->D = D;
  if( p->validTZ ){
    computeJD(p);
  }
  return 0;
}

/*
** Attempt to parse the given string into a Julian Day Number.  Return
** the number of errors.
**
** The following are acceptable forms for the input string:
**
**      YYYY-MM-DD HH:MM:SS.FFF  +/-HH:MM
**      DDDD.DD 
**      now
**
** In the first form, the +/-HH:MM is always optional.  The fractional
** seconds extension (the ".FFF") is optional.  The seconds portion
** (":SS.FFF") is option.  The year and date can be omitted as long
** as there is a time string.  The time string can be omitted as long
** as there is a year and date.
*/
static int parseDateOrTime(const char *zDate, DateTime *p){
  int i;
  memset(p, 0, sizeof(*p));
  for(i=0; isdigit(zDate[i]); i++){}
  if( i==4 && zDate[i]=='-' ){
    return parseYyyyMmDd(zDate, p);
  }else if( i==2 && zDate[i]==':' ){
    return parseHhMmSs(zDate, p);
    return 0;
  }else if( i==0 && sqliteStrICmp(zDate,"now")==0 ){
    double r;
    if( sqliteOsCurrentTime(&r)==0 ){
      p->rJD = r;
      p->validJD = 1;
      return 0;
    }
    return 1;
  }else if( sqliteIsNumber(zDate) ){
    p->rJD = atof(zDate);
    p->validJD = 1;
    return 0;
  }
  return 1;
}

/*
** Compute the Year, Month, and Day from the julian day number.
*/
static void computeYMD(DateTime *p){
  int Z, A, B, C, D, E, X1;
  if( p->validYMD ) return;
  Z = p->rJD + 0.5;
  A = (Z - 1867216.25)/36524.25;
  A = Z + 1 + A - (A/4);
  B = A + 1524;
  C = (B - 122.1)/365.25;
  D = 365.25*C;
  E = (B-D)/30.6001;
  X1 = 30.6001*E;
  p->D = B - D - X1;
  p->M = E<14 ? E-1 : E-13;
  p->Y = p->M>2 ? C - 4716 : C - 4715;
  p->validYMD = 1;
}

/*
** Compute the Hour, Minute, and Seconds from the julian day number.
*/
static void computeHMS(DateTime *p){
  int Z, s;
  if( p->validHMS ) return;
  Z = p->rJD + 0.5;
  s = (p->rJD + 0.5 - Z)*86400000.0 + 0.5;
  p->s = 0.001*s;
  s = p->s;
  p->s -= s;
  p->h = s/3600;
  s -= p->h*3600;
  p->m = s/60;
  p->s += s - p->m*60;
  p->validHMS = 1;
}

/*
** Process a modifier to a date-time stamp.  The modifiers are
** as follows:
**
**     NNN days
**     NNN hours
**     NNN minutes
**     NNN.NNNN seconds
**     NNN months
**     NNN years
**     start of month
**     start of year
**     start of week
**     start of day
**     weekday N
**     unixepoch
**
** Return 0 on success and 1 if there is any kind of error.
*/
static int parseModifier(const char *zMod, DateTime *p){
  int rc = 1;
  int n;
  double r;
  char z[30];
  for(n=0; n<sizeof(z)-1; n++){
    z[n] = tolower(zMod[n]);
  }
  z[n] = 0;
  switch( z[0] ){
    case 'u': {
      /*
      **    unixepoch
      **
      ** Treat the current value of p->rJD as the number of
      ** seconds since 1970.  Convert to a real julian day number.
      */
      if( strcmp(z, "unixepoch")==0 && p->validJD ){
        p->rJD = p->rJD/86400.0 + 2440587.5;
        p->validYMD = 0;
        p->validHMS = 0;
        p->validTZ = 0;
        rc = 0;
      }
      break;
    }
    case 'w': {
      /*
      **    weekday N
      **
      ** Move the date to the beginning of the next occurrance of
      ** weekday N where 0==Sunday, 1==Monday, and so forth.  If the
      ** date is already on the appropriate weekday, this is equivalent
      ** to "start of day".
      */
      if( strncmp(z, "weekday ", 8)==0 && getValue(&z[8],&r)>0
                 && (n=r)==r && n>=0 && r<7 ){
        int Z;
        computeYMD(p);
        p->validHMS = 0;
        p->validTZ = 0;
        p->validJD = 0;
        computeJD(p);
        Z = p->rJD + 1.5;
        Z %= 7;
        if( Z>n ) Z -= 7;
        p->rJD += n - Z;
        p->validYMD = 0;
        p->validHMS = 0;
        rc = 0;
      }
      break;
    }
    case 's': {
      /*
      **    start of TTTTT
      **
      ** Move the date backwards to the beginning of the current day,
      ** or month or year.
      */
      if( strncmp(z, "start of ", 9)!=0 ) break;
      zMod = &z[9];
      computeYMD(p);
      p->validHMS = 1;
      p->h = p->m = 0;
      p->s = 0.0;
      p->validTZ = 0;
      p->validJD = 0;
      if( strcmp(zMod,"month")==0 ){
        p->D = 1;
        rc = 0;
      }else if( strcmp(zMod,"year")==0 ){
        computeYMD(p);
        p->M = 1;
        p->D = 1;
        rc = 0;
      }else if( strcmp(zMod,"day")==0 ){
        rc = 0;
      }
      break;
    }
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      n = getValue(z, &r);
      if( n<=0 ) break;
      zMod = &z[n];
      while( isspace(zMod[0]) ) zMod++;
      n = strlen(zMod);
      if( n>10 || n<3 ) break;
      strcpy(z, zMod);
      if( z[n-1]=='s' ){ z[n-1] = 0; n--; }
      computeJD(p);
      rc = 0;
      if( n==3 && strcmp(z,"day")==0 ){
        p->rJD += r;
      }else if( n==4 && strcmp(z,"hour")==0 ){
        computeJD(p);
        p->rJD += r/24.0;
      }else if( n==6 && strcmp(z,"minute")==0 ){
        computeJD(p);
        p->rJD += r/(24.0*60.0);
      }else if( n==6 && strcmp(z,"second")==0 ){
        computeJD(p);
        p->rJD += r/(24.0*60.0*60.0);
      }else if( n==5 && strcmp(z,"month")==0 ){
        int x, y;
        computeYMD(p);
        p->M += r;
        x = p->M>0 ? (p->M-1)/12 : (p->M-12)/12;
        p->Y += x;
        p->M -= x*12;
        p->validJD = 0;
        computeJD(p);
        y = r;
        if( y!=r ){
          p->rJD += (r - y)*30.0;
        }
      }else if( n==4 && strcmp(z,"year")==0 ){
        computeYMD(p);
        p->Y += r;
        p->validJD = 0;
        computeJD(p);
      }else{
        rc = 1;
      }
      p->validYMD = 0;
      p->validHMS = 0;
      p->validTZ = 0;
      break;
    }
    default: {
      break;
    }
  }
  return rc;
}

/*
** Process time function arguments.  argv[0] is a date-time stamp.
** argv[1] and following are modifiers.  Parse them all and write
** the resulting time into the DateTime structure p.  Return 0
** on success and 1 if there are any errors.
*/
static int isDate(int argc, const char **argv, DateTime *p){
  int i;
  if( argc==0 ) return 1;
  if( parseDateOrTime(argv[0], p) ) return 1;
  for(i=1; i<argc; i++){
    if( parseModifier(argv[i], p) ) return 1;
  }
  return 0;
}


/*
** The following routines implement the various date and time functions
** of SQLite.
*/

/*
**    julianday( TIMESTRING, MOD, MOD, ...)
**
** Return the julian day number of the date specified in the arguments
*/
static void juliandayFunc(sqlite_func *context, int argc, const char **argv){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    computeJD(&x);
    sqlite_set_result_double(context, x.rJD);
  }
}

/*
**    datetime( TIMESTRING, MOD, MOD, ...)
**
** Return YYYY-MM-DD HH:MM:SS
*/
static void datetimeFunc(sqlite_func *context, int argc, const char **argv){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeYMD(&x);
    computeHMS(&x);
    sprintf(zBuf, "%04d-%02d-%02d %02d:%02d:%02d",x.Y, x.M, x.D, x.h, x.m,
           (int)(x.s));
    sqlite_set_result_string(context, zBuf, -1);
  }
}

/*
**    time( TIMESTRING, MOD, MOD, ...)
**
** Return HH:MM:SS
*/
static void timeFunc(sqlite_func *context, int argc, const char **argv){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeHMS(&x);
    sprintf(zBuf, "%02d:%02d:%02d", x.h, x.m, (int)x.s);
    sqlite_set_result_string(context, zBuf, -1);
  }
}

/*
**    date( TIMESTRING, MOD, MOD, ...)
**
** Return YYYY-MM-DD
*/
static void dateFunc(sqlite_func *context, int argc, const char **argv){
  DateTime x;
  if( isDate(argc, argv, &x)==0 ){
    char zBuf[100];
    computeYMD(&x);
    sprintf(zBuf, "%04d-%02d-%02d", x.Y, x.M, x.D);
    sqlite_set_result_string(context, zBuf, -1);
  }
}

/*
**    strftime( FORMAT, TIMESTRING, MOD, MOD, ...)
**
** Return a string described by FORMAT.  Conversions as follows:
**
**   %d  day of month
**   %f  ** fractional seconds  SS.SSS
**   %H  hour 00-24
**   %j  day of year 000-366
**   %J  ** Julian day number
**   %m  month 01-12
**   %M  minute 00-59
**   %s  seconds since 1970-01-01
**   %S  seconds 00-59
**   %w  day of week 0-6  sunday==0
**   %W  week of year 00-53
**   %Y  year 0000-9999
**   %%  %
*/
static void strftimeFunc(sqlite_func *context, int argc, const char **argv){
  DateTime x;
  int n, i, j;
  char *z;
  const char *zFmt = argv[0];
  char zBuf[100];
  if( isDate(argc-1, argv+1, &x) ) return;
  for(i=0, n=1; zFmt[i]; i++, n++){
    if( zFmt[i]=='%' ){
      switch( zFmt[i+1] ){
        case 'd':
        case 'H':
        case 'm':
        case 'M':
        case 'S':
        case 'W':
          n++;
          /* fall thru */
        case 'w':
        case '%':
          break;
        case 'f':
          n += 8;
          break;
        case 'j':
          n += 3;
          break;
        case 'Y':
          n += 8;
          break;
        case 's':
        case 'J':
          n += 50;
          break;
        default:
          return;  /* ERROR.  return a NULL */
      }
      i++;
    }
  }
  if( n<sizeof(zBuf) ){
    z = zBuf;
  }else{
    z = sqliteMalloc( n );
    if( z==0 ) return;
  }
  computeJD(&x);
  computeYMD(&x);
  computeHMS(&x);
  for(i=j=0; zFmt[i]; i++){
    if( zFmt[i]!='%' ){
      z[j++] = zFmt[i];
    }else{
      i++;
      switch( zFmt[i] ){
        case 'd':  sprintf(&z[j],"%02d",x.D); j+=2; break;
        case 'f': {
          int s = x.s;
          int ms = (x.s - s)*1000.0;
          sprintf(&z[j],"%02d.%03d",s,ms);
          j += strlen(&z[j]);
          break;
        }
        case 'H':  sprintf(&z[j],"%02d",x.h); j+=2; break;
        case 'W': /* Fall thru */
        case 'j': {
          int n;
          DateTime y = x;
          y.validJD = 0;
          y.M = 1;
          y.D = 1;
          computeJD(&y);
          n = x.rJD - y.rJD + 1;
          if( zFmt[i]=='W' ){
            sprintf(&z[j],"%02d",(n+6)/7);
            j += 2;
          }else{
            sprintf(&z[j],"%03d",n);
            j += 3;
          }
          break;
        }
        case 'J':  sprintf(&z[j],"%.16g",x.rJD); j+=strlen(&z[j]); break;
        case 'm':  sprintf(&z[j],"%02d",x.M); j+=2; break;
        case 'M':  sprintf(&z[j],"%02d",x.m); j+=2; break;
        case 's': {
          sprintf(&z[j],"%d",(int)((x.rJD-2440587.5)*86400.0));
          j += strlen(&z[j]);
          break;
        }
        case 'S':  sprintf(&z[j],"%02d",(int)x.s); j+=2; break;
        case 'w':  z[j++] = (((int)(x.rJD+1.5)) % 7) + '0'; break;
        case 'Y':  sprintf(&z[j],"%04d",x.Y); j+=strlen(&z[j]); break;
        case '%':  z[j++] = '%'; break;
      }
    }
  }
  z[j] = 0;
  sqlite_set_result_string(context, z, -1);
  if( z!=zBuf ){
    sqliteFree(z);
  }
}


#endif /* !defined(SQLITE_OMIT_DATETIME_FUNCS) */

/*
** This function registered all of the above C functions as SQL
** functions.  This should be the only routine in this file with
** external linkage.
*/
void sqliteRegisterDateTimeFunctions(sqlite *db){
  static struct {
     char *zName;
     int nArg;
     int dataType;
     void (*xFunc)(sqlite_func*,int,const char**);
  } aFuncs[] = {
#ifndef SQLITE_OMIT_DATETIME_FUNCS
    { "julianday", -1, SQLITE_NUMERIC, juliandayFunc   },
    { "date",      -1, SQLITE_TEXT,    dateFunc        },
    { "time",       1, SQLITE_TEXT,    timeFunc        },
    { "datetime",  -1, SQLITE_TEXT,    datetimeFunc    },
    { "strftime",  -1, SQLITE_TEXT,    strftimeFunc    },
#endif
  };
  int i;

  for(i=0; i<sizeof(aFuncs)/sizeof(aFuncs[0]); i++){
    sqlite_create_function(db, aFuncs[i].zName,
           aFuncs[i].nArg, aFuncs[i].xFunc, 0);
    if( aFuncs[i].xFunc ){
      sqlite_function_type(db, aFuncs[i].zName, aFuncs[i].dataType);
    }
  }
}
