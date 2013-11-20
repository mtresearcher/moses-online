#! /usr/bin/perl

#******************************************************************************
# Prashant Mathur @ FBK-irst. November 2013
#******************************************************************************
# wrapper for determining number of optimal iterations of online learning in moses
# VERSION 2.0 
# First X sentences as burn in
# Translated the X+1th sentence with {1,2,3 ... N} interations of previous sentence (Xth).
# When the difference in BLEU is negligible between j^{th} and j+1^{th} iteration, we converge
# BLEU is calculated on blocks of B sentences.

use strict;
use warnings;
binmode STDIN, ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";
use constant false => 0;
use constant true  => 1;


# GLOBALS

my @__hist=();
my @__reference=();
my @__source=();
my @__output=();
my @__optimisedHist=();
my $__moses="";
my $__multi_bleu="";
my $__config="";
my $__BURNIN=10;	# number of sentences to leave at the start of online learning to collect statistics
my $__initIter=2;	# initial number of iterations for the sentences under burn in case.
#


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
	read_files($__test, $__postedit);
	
	for(my $sentNum=0;$sentNum < scalar(@__hist); $sentNum++)
	{
# Prepare the optimisedHist list			
		push(@__optimisedHist, $__source[$sentNum]);
		if($sentNum < $__BURNIN){
			for(my $i=0;$i<$__initIter;$i++) {push(@__optimisedHist, $__hist[$sentNum]);}
		}
		else{
			my $__optIter = findOptimalIterations(\@__optimisedHist, $sentNum+1);
			print STDERR "Optimal iterations for Sentence ",$sentNum+1," is $__optIter\n";
			for(my $i=0;$i<$__optIter;$i++) {push(@__optimisedHist, $__hist[$sentNum]);}
		}
	}
}


sub findOptimalIterations
{

## Update
# Online learning :
# 1. print @__hist to temp_input_onlinelearning & @__reference temp_reference to a file
# 2. Update with temp_input_onlinelearning and check BLEU against temp_reference,
# 3. change the temp* files to support 2 iterations of Online Learning. Check BLEU
# 4. go to 3. to support 3, 4, ... N iterations until the difference in BLEU and prevBLEU is < 0.01
# 5. Return N as final number of iterations.
	
	my ($__opt, $__toOptimize) = @_;
	my $iterations=0;
	my $currBLEU=100.0;
	my $prevBLEU=0.0;


# writing to temp.ref
	open TEMPREF, ">temp.ref" or die "cannot create temp ref file\n";
	for(my $k=0;$k<=$__toOptimize; $k++){
		print TEMPREF $__reference[$k]."\n";
	}
	close(TEMPREF);

# stopping criterion
	while(abs($currBLEU - $prevBLEU) > 0.01){ 

		$iterations+=1;

# writing to temp.input	
		open TEMPIN, ">temp.input" or die "cannot create temp input file\n";
		for(my $k=0;$k<scalar(@{$__opt}); $k++){
			print TEMPIN $__opt->[$k]."\n";
		}
		for(my $j=1;$j<$iterations;$j++){
			print TEMPIN $__hist[$__toOptimize-1]."\n";
		}
		print TEMPIN $__source[$__toOptimize]."\n";
		close(TEMPIN);
# translate the temp.input
		my $cmd = "cat temp.input | LD_LIBRARY_PATH=/research/hlt/prashant/MyInstallation/lib64 $__moses -f $__config -w_learningrate 0.05 -w_algorithm onlyMira > temp.output 2> error.log";
		print STDERR $cmd."\n";
		system($cmd);

# calculate the BLEU
		$prevBLEU=$currBLEU;
		$currBLEU=`cat temp.output | perl $__multi_bleu temp.ref | awk '{print \$3}' | perl -pe 's/,//g'`;
		print STDERR "Previous BLEU : $prevBLEU\nCurrent BLEU : $currBLEU\n";
	}

	return $iterations-1;
}

sub read_files
{
	my ($__test, $__postedit) = @_;
	
	open TEST, "<$__test" or die "OOPS! Cannot open the input file\n";
	open POSTEDIT, "<$__postedit" or die "OOPS! Cannot open the postedit file\n";


	while(my $__src = <TEST>)
	{
		chop($__src);
		push(@__source, $__src);
		my $__pe = <POSTEDIT>;
		chop($__pe);
		push(@__reference, $__pe);
		my $__input=$__src."_#_".$__pe;
		push(@__hist, $__input);
	}
}


&main();
