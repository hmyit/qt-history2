#!
#! This is a custom template for creating a Makefile for the moc.
#!
#${
    StdInit();
    $tsrc = $project{"TOOLSRC"} . " " . $project{"MOCGEN"};
    $tobj = Objects($tsrc);
    $tobj =~ s=\.\.[\\/]tools[\\/]==g;
    $project{"OBJECTS"} = $tobj;
#$}
#$ IncludeTemplate("app.t");

####### Lex/yacc programs and options

LEX	=	flex
YACC	=	#$ $text = ($is_unix ? "yacc -d" : "byacc -d");

####### Lex/yacc files

LEXIN	=	#$ Expand("LEXINPUT");
LEXOUT  =	lex.yy.c
YACCIN	=	#$ Expand("YACCINPUT");
YACCOUT =	y.tab.c
YACCHDR =	y.tab.h
MOCGEN  =	#$ Expand("MOCGEN");

####### Process lex/yacc files

$(LEXOUT): $(LEXIN)
	$(LEX) $(LEXIN)

$(MOCGEN): moc.y $(LEXOUT)
	$(YACC) moc.y
	#$ $text = ($is_unix ? "-rm -f " : "-del ") . '$(MOCGEN)';
	#$ $text = ($is_unix ? "-mv " : "-ren ") . '$(YACCOUT) $(MOCGEN)'; 

####### Compile the C++ sources

#$ $text = Objects($project{"MOCGEN"}) . ": " . $project{"MOCGEN"};
#$ $text = "\t" . '$(CC) -c $(CFLAGS) $(INCPATH) -o ' . Objects($project{"MOCGEN"}) . " " . $project{"MOCGEN"};

#${
    if ( $is_unix ) {
	$td = "../tools/";
    } else {
	$td = "..\\tools\\";
    }
    @s = split(/\s+/,$project{"TOOLSRC"});
    foreach ( @s ) {
	$text && ($text .= "\n");
	$src = $td . $_;
	$text .= Objects($_) . ": " . $src . "\n\t";
	$text .= '$(CC) -c $(CFLAGS) $(INCPATH) -o ';
	$text .= Objects($_) . " " . $src . "\n";
    }
#$}
