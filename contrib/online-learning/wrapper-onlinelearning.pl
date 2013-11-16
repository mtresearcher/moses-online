#! /usr/bin/perl

#******************************************************************************
# Prashant Mathur @ FBK-irst. November 2013
#******************************************************************************
# wrapper for determining number of optimal iterations of online learning in moses


use strict;
use warnings;
binmode STDIN, ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";

sub main {
my $__usage = "
USAGE
-----
perl wrapper-onlinelearning.pl --input=<filename> --postedit=<filename> --moses=<path to moses binary> --config=<path to moses config> [--initIter=<Number of initial iterations>]
-----
";

my $__debug;
my $__help;
my $__test="";
my $__postedit="";
my $__moses="";
my $__multi_bleu="";
my $__config="";
my $__initIter=10;
GetOptions ('debug' => \$__debug, 
	'input=s' => \$__test,
	'postedit=s' => \$__postedit,
	'moses=s' => \$__moses,
	'config=s' => \$__config,
	'multi-bleu=s' => \$__multi_bleu,
	'initIter=i' => \$__initIter,
	'help' => \$__help);

if($__help) { die "$__usage\n\n"; }

print STDERR "
******************************
Moses : $__moses
Config : $__config
Input : $__test
Postedited : $__postedit
";

open TEST, "<$__test" or die "OOPS! Cannot open the input file\n$__usage";
open POSTEDIT, "<$__postedit" or die "OOPS! Cannot open the postedit file\n$__usage";

@__hist=();


while(my $__src = <TEST>)
{
	chop($__src);
print STDERR " 
Translating : $__src
";
	my $__pe = <POSTEDIT>;
	chop($__pe);
	my $__input=$__src."_#_".$__pe;

	my $cmd = "echo \"$__input\" | LD_LIBRARY_PATH=/research/hlt/prashant/MyInstallation/lib64 $__moses -f $__config -w_learningrate 0.05 -w_algorithm onlyMira > $temp.out 2> error.log";
	my $output = `$cmd`;
print STDERR "
Output : $output
";

	$cmd = "cat $filename | LD_LIBRARY_PATH=/research/hlt/prashant/MyInstallation/lib64 $__moses -f $__config -w_learningrate 0.05 -w_algorithm onlyMira > $temp.out 2> error.log";
	system($cmd);



	$cmd = "cat $temp.out | perl $__multi_bleu $reference | awk '{print $3}' | perl -pe 's/,//g'";
	my $score=`$cmd`;
}

}


&main();
