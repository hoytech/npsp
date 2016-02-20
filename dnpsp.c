/**==========================================================================**\
 **
 **	NPSP - Numerical Palindrome Search Program
 **
 **	(c) 2002, defrost @ #c openprojects.net
 **
 **	Inspired / prompted by software of same name / purpose and interface
 **	written:
 **		August 2002 by Fractal @ #c openprojects.net
 **	and:
 **		(C) 2002, HardCore Software
 **
 **	This program is protected by the GNU General Public License.
 **	See the file "COPYING" for details.
 **
\**==========================================================================**/

#include	<ctype.h>
#include	<malloc.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>



/**==========================================================================**\
 **	Version, Usage && Copyright.
\**==========================================================================**/
static const char* const Version	= "1.01" ;
static const char* const VersionDate	= __DATE__ ;
static const char* const VersionHist[]	= {
	"\t1.00\tSat24Aug2002\tFirst coded"	,
	"\t1.01\tMon26Aug2002\tTested & tidied" ,
	NULL
	} ;
static const char* const UsageMsg[]	= {
	"< options >" ,
	"\t-i <input file (Istvan Standard Format)>" ,
	"\t-o <output file (Istvan Standard Format)>" ,
	"\t-n <seed number>" ,
	"\t-m <maximum # of digits to find>" ,
	"\t-b <numeric base>" ,
	"\t-a <seconds between autosaves>" ,
	NULL
	} ;
static const char* const Copyright[]	= {
	"Numerical Palindrome Search Program (NPSP)" ,
	"Copyright: HardCore Software / defrost 2002" ,
	"\tAll Rights Reserved\n" ,
	NULL
	} ;
static char*		 ProgramName	= NULL ; /* to be assigned ... */



/**==========================================================================**\
 **									     **
 **	Numerical Palindrome Core					     **
 **									     **
\**==========================================================================**/
typedef	void (*npc_vfp) (struct palinumeral_type *);

typedef	enum palinumeral_states
	{ NPC_IS_NOTDONE	= 0
	, NPC_IS_SYMMETRIC
	, NPC_IS_MAXDIGITS
	}
	npc_ret;

typedef	struct	palinumeral_type
	{
	unsigned	maxDigits ;
	unsigned	numDigits ;
	unsigned	iterates;
	unsigned 	base	;
	unsigned char	*active	;
	unsigned char	*altern	;

	unsigned	origSeed;
	unsigned	initIter;
	float		percentDigit ;

	npc_ret		state	;

	time_t		timeInit ;
	time_t		timeMark ;

	time_t		progress_gap ;
	npc_vfp		progress_fun ;

	time_t		autosave_gap ;
	npc_vfp		autosave_fun ;
	}
	npc_t ;

/**---------------------------------------------------------------------
 **	npc prototypes
 **/
static	npc_t *	npc_new		( unsigned );
static	npc_t *	npc_free	( npc_t** ) ;

static	int	npc_digit_ingest( npc_t*,unsigned char*,unsigned );
static	int	npc_file_ingest	( npc_t*,FILE*,int );
static	int	npc_string_ingest(npc_t*,char* );
static	int	npc_digit_express(npc_t*,unsigned,unsigned char*,unsigned );
static	int	npc_file_express( npc_t*,FILE* );

static	int	npc_saveFile	( npc_t*,char* );
static	int	npc_loadFile	( npc_t*,char* );

static	int	npc_symmetric	( unsigned char*,unsigned char* );
static	npc_ret	npc_step	( npc_t* );
static	npc_ret	npc_loop	( npc_t* );


/**---------------------------------------------------------------------
 **/
void	npc_def_progress
	(
	npc_t	*npc
	)
	{
	printf(	"\r"
		"Progress: %%%6.2f Digits: %7d Its: %8d Secs: %6u"
		, npc->numDigits * npc->percentDigit
		, npc->numDigits
		, npc->iterates
		, npc->timeMark - npc->timeInit
		);
	fflush( stdout );
	}

/**---------------------------------------------------------------------
 **	npc_new
 **/
static	npc_t	*npc_new
	(
	unsigned	maxDigits
	)
	{
	size_t	size	= sizeof( npc_t )
			+ sizeof( char ) * 2 * maxDigits ;
	npc_t	*npc	= ( maxDigits )
			? malloc( size )
			: NULL ;

	if( npc )
		{
		memset( npc, 0, sizeof( npc_t ) );
		memset( npc, 0, size );			/*only for DEBUG*/

		npc->active	= (unsigned char *)npc + sizeof(npc_t) ;
		npc->altern	= npc->active + maxDigits ;
		npc->maxDigits	= maxDigits ;
		npc->base	= 10 ;

		npc->progress_gap = 2 ;
		npc->progress_fun = npc_def_progress ;

		npc->percentDigit = 100.0 / (float)maxDigits ;
		}

	return( npc ) ;
	}

/**---------------------------------------------------------------------
 **	npc_free
 **
 **	free npc_t structure and sub parts addressed by *pp_npc
 **	set *pp_npc to NULL to mark as unused and free
 **	return NULL to drive point home
 **/
static	npc_t	*npc_free
	(
	npc_t	**pp_npc
	)
	{
	if( pp_npc && *pp_npc )
		{
		/**	free / close (*pp_npc) members
		**/
		/* none as yet */

		free( *pp_npc );
		*pp_npc = NULL ;
		}

	return( NULL ) ;
	}

/**---------------------------------------------------------------------
 **	npc_digit_ingest
 **
 **	ingest most significant digit on left representation in base
 **	where 2 <= base <= 36 using [0..9][A..Z] charcaters
 **	to internal npc reversed unsigned char storage
 **/
static	int	npc_digit_ingest
	(
	npc_t		*npc	,
	unsigned char	*buffer	,
	unsigned	index
	)
	{
	while( index )
		{
		unsigned char ch = *buffer ++ ;

		if( ch=='\0' || ch=='\n' )
			break ;

		if( (ch>='0')&&(ch<='9') )
			ch -= '0' ;
		else
		if( (ch>='A')&&(ch<='Z') )
			ch -= ('A'+10) ;
		else
			{
			fprintf( stderr, "ingest number has chars other than [0-9A-Z]\n" );
			break ;
			}

		if( ch >= npc->base )
			{
			fprintf( stderr, "ingest number has digits outside of base represention\n" );
			break ;
			}

		npc->active[ --index ] = ch ;
		npc->numDigits ++ ;
		}

	return( index ) ;
	}

/**---------------------------------------------------------------------
 **	npc_file_ingest
 **
 **	ingest base representation of npc->active number
 **	number begins at FILE *fp, has numDigits chars, and
 **	is represented in base npc->base
 **/
static	int	npc_file_ingest
	(
	npc_t	*npc ,
	FILE	*fp ,
	int	numDigits
	)
	{
	unsigned	n	;
	unsigned char	buffer[ 128 ] ;

	if(	!npc
	||	!fp
	||	(numDigits > npc->maxDigits)
	)
		return( 0 );

	npc->numDigits = 0 ;
	n = numDigits ;

	while(	fgets( buffer, sizeof buffer, fp ) != NULL
	&&	(n = npc_digit_ingest( npc, buffer, n )) != 0
	)	;

	if( (n==0) && (numDigits == npc->numDigits) )
		return( 1 ) ;

	return( 0 );
	}

/**---------------------------------------------------------------------
 **	npc_string_ingest
 **
 **	ingest base representation of npc->active number
 **/
static	int	npc_string_ingest
	(
	npc_t	*npc ,
	char	*string
	)
	{
	unsigned	len ;

	if(	!npc
	||	!string
	||	((len = strlen(string)) > npc->maxDigits)
	)
		return( 0 );

	npc->numDigits = 0 ;

	if( npc_digit_ingest( npc, (unsigned char*)string, len ) == 0 )
		return( 1 );

	return( 0 );
	}

/**---------------------------------------------------------------------
 **	npc_digit_express
 **
 **	copy out, in natural order, transposing on fly,
 **	digits from npc->active to destination string 'dst'.
 **
 **	for one thousand == string "1000" base 10
 **	offset 0 refers to the '1' char, offset 1 to the following '0'
 **
 **	return number of digits copied.
 **/
static	int	npc_digit_express
	(
	npc_t		*npc ,
	unsigned	offset,
	unsigned char	*dst ,
	unsigned	maxDstDigits
	)
	{
	unsigned	maxCopy ;
	unsigned	numLeft ;
	unsigned char	*src ;

	if(	npc==NULL
	||	dst==NULL
	||	offset>=npc->numDigits
	)
		return( 0 );

	maxCopy = npc->numDigits - offset ;
	src	= npc->active + maxCopy - 1 ;

	if( maxDstDigits < maxCopy )
		maxCopy = maxDstDigits ;
	numLeft = maxCopy ;

	while( numLeft-- )
		{
		unsigned char	ch = *src-- ;

		if( ch < 10 )
			ch += '0' ;
		else
			ch += ('A'-10) ;

		*dst++ = ch ;
		}

	return( maxCopy ) ;
	}

/**---------------------------------------------------------------------
 **	npc_file_express
 **
 **	emit, as base digits, number in npc->active to FILE* fp
 **
 **	number written in 'natural' MSD first order,
 **	broken into multiple lines with digtsPerLine digits per line.
 **
 **	hint: fp = stdout for console output
 **/
static	int	npc_file_express
	(
	npc_t	*npc	,
	FILE	*fp
	)
	{
	int	off		= 0 ;
	int	digitsPerLine	= 70 ;
	char	buffer		[ 128 ] ;

	if(	!npc
	||	!fp
	)
		return( 0 );

	while( 1 )
		{
		int num = npc_digit_express( npc, off, buffer, digitsPerLine );
		if( num )
			{
			buffer[ num ] = '\n' ;
			fwrite( buffer, 1, num+1, fp );
			off += num ;
			}
		else
			break ;
		}

	return( 1 ) ;
	}

/**---------------------------------------------------------------------
 **	npc_saveFile
 **
 **	save npc_t to named file in 'standard' format
 **/
static	int	npc_saveFile
	(
	npc_t	*npc	,
	char	*filename
	)
	{
	FILE	*fp ;

	if( !npc || !filename )
		return( 0 );

	if( NULL == (fp = fopen( filename, "w" )) )
		{
		fprintf( stderr, "npc_saveFile( %s ) failed\n", filename );
		return( 0 );
		}

	fprintf(fp,
		"Automatic save #1\n"
		"Initial value: %u\n"
		"Iteration: %u\n"
		"Number of digits: %u\n"
		, npc->origSeed
		, npc->initIter + npc->iterates
		, npc->numDigits
		) ;

	npc_file_express( npc, fp );

	fclose( fp );
	return( 1 );
	}

/**---------------------------------------------------------------------
 **	npc_loadFile
 **
 **	load standard format npc_t data into npc_t struct
 **/
static	int	npc_loadFile
	(
	npc_t	*npc	,
	char	*filename
	)
	{
	FILE	*fp ;
	int	ret ;

	if( !npc || !filename )
		return( 0 );

	if( NULL==(fp=fopen( filename, "r" )) )
		{
		fprintf( stderr, "npc_loadFile( %s ) failed\n", filename ) ;
		return( 0 );
		}

	ret = fscanf(fp,
		"Automatic save %*s\n"
		"Initial value: %u\n"
		"Iteration: %u\n"
		"Number of digits: %u\n"
		, &(npc->origSeed)
		, &(npc->initIter)
		, &(npc->numDigits)
		) ;

	if( ret != 3 )
		{
		fprintf( stderr, "npc_loadFile( %s ) non standard file\n", filename );
		fclose( fp );
		return( 0 );
		}

	if( npc->numDigits > npc->maxDigits )
		{
		fprintf( stderr, "npc_loadFile( %s ) - too many digits\n",filename ) ;
		fclose( fp );
		return( 0 );
		}

	ret = npc_file_ingest( npc, fp, npc->numDigits );

	fclose( fp ) ;
	return( ret );
	}

/**---------------------------------------------------------------------
 **	npc_symmetric
 **
 **	given beg and end char pointers, where beg <= end
 **	return 1 if char array between pointers is symmetric
 **	return 0 if it is not
 **/
static	int	npc_symmetric
	(
	unsigned char	*beg ,
	unsigned char	*end
	)
	{
	while( end != beg )
		if( *beg != *end )
			return( 0 );
		else
			++beg, --end ;
	return( 1 ) ;
	}

/**---------------------------------------------------------------------
 **	npc_step
 **/
static	npc_ret	npc_step
	(
	npc_t	*npc
	)
	{
	unsigned	carry	;
	unsigned	count	;
	unsigned	base	;
	unsigned char	*srcBeg	= npc->active ;
	unsigned char	*srcEnd	= npc->active + npc->numDigits - 1 ;
	unsigned char	*dst	;

	if( npc_symmetric( srcBeg, srcEnd ) )
		return( npc->state = NPC_IS_SYMMETRIC );

	if( (count = npc->numDigits) == npc->maxDigits )
		return( npc->state = NPC_IS_MAXDIGITS );

	dst	= npc->altern ;
	base	= npc->base ;
	carry	= 0 ;

	while( count-- )
		{
		unsigned sum	= *srcBeg ++
				+ *srcEnd --
				+ carry ;

		if( (carry = (sum >= base) ? 1 : 0) )
			sum -= base ;

		*dst ++	= sum ;
		}
	if( carry )
		{
		*dst = 1 ;
		++( npc->numDigits ) ;
		}

	/**	swap active / alternate buffers
	**/
	srcBeg	= npc->active ;
	npc->active = npc->altern ;
	npc->altern = srcBeg ;

	++( npc->iterates ) ;

	return( NPC_IS_NOTDONE );
	}

/**---------------------------------------------------------------------
 **	npc_loop
 **
 **	continously iterate numerical palindrome in runs of stepChunk steps
 **	until either a symmetric number, expressed in base, is reached
 **	or until the maximum number of digits is reached
 **
 **/
static	npc_ret	npc_loop
	(
	npc_t	*npc
	)
	{
	int	stepChunk	= 256 ;

	time_t	timeStat ;
	time_t	timeSave ;

	npc->timeInit= npc->timeMark = time( NULL );

	timeStat= ((npc->progress_gap > 0) && (npc->progress_fun != NULL))
		? ( npc->timeInit + npc->progress_gap )
		: 0 ;

	timeSave= ((npc->autosave_gap > 0) && (npc->autosave_fun != NULL))
		? ( npc->timeInit + npc->autosave_gap )
		: 0 ;

	if( timeStat )
		npc->progress_fun( npc ) ;

	while(  1  )
		{
		int	chunk	= stepChunk ;

		while( chunk-- && (npc_step( npc )==NPC_IS_NOTDONE) )
			/** null body **/ ;

		npc->timeMark = time( NULL );

		if( timeStat && (timeStat <= npc->timeMark) )
			{
			npc->progress_fun( npc ) ;
			timeStat = npc->timeMark + npc->progress_gap ;
			}

		if((timeSave && (timeSave <= npc->timeMark))
		|| (npc->state != NPC_IS_NOTDONE)
		)	{
			if( npc->autosave_fun )
				npc->autosave_fun( npc ) ;

			if( npc->state != NPC_IS_NOTDONE )
				break ;

			timeSave = npc->timeMark + npc->autosave_gap ;
			}
		}

	return( npc->state );
	}

/**==========================================================================**\
 **	End of Numerical Palindrome Core
\**==========================================================================**/



/**==========================================================================**\
 **	Version, Usage && Copywrite puts routines
\**==========================================================================**/
static	void 	putps( char ** p_str )
	{
	if( p_str ) while( *p_str ) puts( *p_str++ ) ;
	}
static	void	copywrite( void )
	{
	puts( "" ) ;
	putps( (char**)Copyright ) ;
	}
static	void	usage( void )
	{
	printf("\nusage: %s ",ProgramName) ;
	putps( (char**)UsageMsg ) ;
	}
static	void	version( void )
	{
	printf("\nVersion:%s - compiled %s\n", Version, VersionDate ) ;
	putps( (char**)VersionHist ) ;
	}
/**==========================================================================**\
 **	End of Version routines
\**==========================================================================**/


/**==========================================================================**\
 **	Miscellaneous util functions
\**==========================================================================**/
static	char	*strupdate( char **p_str, char *new_str )
	{
	if( p_str && new_str )
		{
		if( *p_str )
			free( *p_str );
		return( *p_str = strdup( new_str ) );
		}
	return( NULL );
	}
/**==========================================================================**\
 **	End of Miscellaneous
\**==========================================================================**/



/**==========================================================================**\
 **	Program Parameters
\**==========================================================================**/

typedef	enum	parameter_help_flags
	{
        PARAM_COPYRIGHT	= 0x01 ,
	PARAM_VERSION	= 0x02 ,
	PARAM_USAGE	= 0x04 ,
	PARAM_EXIT	= 0x08
	}
	param_f ;

typedef	struct	parameter_struct
	{
	param_f	help ;
	int	numParams ;
	char	*loadFile ;
	char	*saveFile ;
	char	*seedNumber ;
	unsigned baseNumber ;
	unsigned maxDigits ;
	time_t	 savePeriod ;
	}
	param_t ;

/**---------------------------------------------------------------------
 **/
static	void	param_default
	(
	param_t	*param
	)
	{
	if( !param )
		return ;

	memset( param, 0, sizeof( *param ) ) ;

	param->help |= PARAM_COPYRIGHT ;
	param->help |= PARAM_USAGE ;
	param->help |= PARAM_EXIT ;

	param->baseNumber = 10 ;
	param->maxDigits  = 100 ;
	}

/**---------------------------------------------------------------------
 **/
static	void	param_usage
	(
	param_t	*param
	)
	{
	if( !param )
		return ;
	if( param->help & PARAM_COPYRIGHT )	copywrite() ;
	if( param->help & PARAM_VERSION )	version() ;
	if( param->help & PARAM_USAGE )		usage() ;
	if( param->help & PARAM_EXIT )		exit( 0 );
	}

/**---------------------------------------------------------------------
 **/
static	void	param_verify
	(
	param_t	*param
	)
	{
	char *msgStack[10] ;
	char **msg = msgStack ;

	if( param->loadFile==NULL && param->seedNumber==NULL )
		{
		}
	/** complete me
	**/

	if( param->numParams == 0 )
		param_usage( param ) ;
	}
/**==========================================================================**\
 **	End of Program Parameters
\**==========================================================================**/



/**==========================================================================**\
 **	M A IN
\**==========================================================================**/
int	main
	(
	int argc,
	char *argv[]
	)
	{
	param_t	param	;
	npc_t	*npc	= NULL ;

	ProgramName	= *argv++ ; --argc ;
	param_default( &param );

	while( argv[0] && argv[1] && argv[0][0]=='-' )
		{
		switch( argv[0][1] )
			{
		case 'n': param.seedNumber= argv[1];  break;
		case 'm': param.maxDigits = atoi( argv[1] ); break;
		case 'b': param.baseNumber= atoi( argv[1] ); break;
		case 'a': param.savePeriod= atoi( argv[1] ); break;
		case 'i': strupdate(&(param.loadFile), argv[1] ); break;
		case 'o': strupdate(&(param.saveFile), argv[1] ); break;

		case 'v': param.help |= PARAM_VERSION ;
			  param_usage( &param ) ;

		default	: fprintf( stderr, "unknown option: '%c'\n" );
			  param_usage( &param );
			}
		param.numParams ++ ;
		argc -= 2 ;
		argv += 2 ;
		}

	param_verify( &param );

	if( (npc = npc_new( param.maxDigits )) == NULL )
		{
		fprintf( stderr, "err npc_new\n" );
		return( 0 );
		}
	npc->base = param.baseNumber ;

	if( param.loadFile )
		{
		if( !npc_loadFile( npc, param.loadFile ) )
			return( 0 );
		}
	else
		{
        	if( !npc_string_ingest( npc, param.seedNumber ) )
			{
			fprintf( stderr, "err loading seedNumber\n" );
			return( 0 );
			}
		else
			npc->origSeed =	atoi( param.seedNumber ) ;
		}

	copywrite() ;

	npc_loop( npc ) ;

	printf(	"\n"
		"Progress: %%%6.2f Digits: %7d Its: %8d Secs: %6u\n"
		, 100. * npc->numDigits / npc->maxDigits
		, npc->numDigits
		, npc->iterates
		, npc->timeMark - npc->timeInit
		) ;

	if( param.saveFile )
		npc_saveFile( npc, param.saveFile ) ;

	return 0;
	}

/**=======================================================[  End of File  ]==**/
