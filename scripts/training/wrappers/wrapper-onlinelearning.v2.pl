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
# Works only on cluster

use strict;
use POSIX;
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
my $__workingDir="/tmp";
my $__tercom="tercom_v6b.pl";
my $__config="";
my $__metric="BLEU";
my $__metric_script="multi-bleu.pl";
my $__initWeightOnline=0.5;
my $__BURNIN=10;	# number of sentences to leave at the start of online learning to collect statistics
my $__initIter=10;	# initial number of iterations for the sentences under burn in case.
my $__blockSize=10;
#


sub main {
	my $__usage = "
USAGE
-----
perl wrapper-onlinelearning.pl 
            --input=<filename> 
            --postedit=<filename> 
            --moses=<path to moses binary> 
            --config=<path to moses config> 
            --metric=X (where X = BLEU or TER)
            --metric-script=<filename> (path to bleu script or tercom perl script)
            --working-dir=<DIR> (default=/tmp)
            --init-online-weight=<float> (initial online weight, default=0.5)
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
    'metric=s' => \$__metric,
    'metric-script=s' => \$__metric_script,
	'config=s' => \$__config,
	'working-dir=s' => \$__workingDir,
	'init-online-weight' => \$__initWeightOnline,
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
	my $TotalBlocks = ceil(scalar(@__source)/$__blockSize);
	my $prevJob="trans.0";
	my $best=0;
	my @bestIteration=();	
################################################
	for(my $blockNum=0; $blockNum < $TotalBlocks; $blockNum++)
	{
		if($blockNum>0){
			$best = $bestIteration[$blockNum-1];
		}

################################################
		for(my $iter=0; $iter<=10; $iter++){
			# Prepare the optimisedHist list
			my ($tm, $rm, $lm, $wp, $wol, $bool)=(); 
			my $prevBlock = $blockNum-1;
			if($blockNum>0){
    			($tm, $rm, $lm, $wp, $wol) = getWeights("$__workingDir/$prevBlock/weights.$best"); # get weights from weights.err
                if($wol == 0) {$wol = 0.09;}
                $bool=true;
			}
			else {
				$bool = false;
			}
			my $cmd="";
			# create the modified input 
			system("mkdir -p $__workingDir/$blockNum");
			saveToFile($blockNum, $iter, $__workingDir);	# 
			# translate the FILE using qsub
			if($bool == true){
				$cmd = "echo \"$__moses -f $__config -w_learningrate 0.05 -w_algorithm mira -f_learningrate 0.1 -slack 0.001 -dump-weights-online $__workingDir/$blockNum/weights.$iter -dump-online-learning-model $__workingDir/$blockNum/online-model.$blockNum.$iter -read-online-learning-model $__workingDir/$blockNum/online-model.$blockNum.$best -tm $tm -d $rm -lm $lm -w $wp -weight-ol $wol < $__workingDir/$blockNum/$iter.iter.input > $__workingDir/$blockNum/$iter.iter.input.trans\" | qsub -q bld.q,bld-ib.q,mmap.q -l mf=5G -o $__workingDir/trans.$blockNum.$iter.out -e $__workingDir/trans.$blockNum.$iter.err -N trans.$blockNum -S /bin/bash";
			}
			else{
				$cmd = "echo \"$__moses -f $__config -w_learningrate 0.05 -w_algorithm mira -f_learningrate 0.1 -slack 0.001 -dump-weights-online $__workingDir/$blockNum/weights.$iter -dump-online-learning-model $__workingDir/$blockNum/online-model.$blockNum.$iter -weight-ol $__initWeightOnline < $__workingDir/$blockNum/$iter.iter.input > $__workingDir/$blockNum/$iter.iter.input.trans\" | qsub -q bld.q,bld-ib.q,mmap.q -l mf=5G -o $__workingDir/trans.$blockNum.$iter.out -e $__workingDir/trans.$blockNum.$iter.err -N trans.$blockNum -S /bin/bash";
			}

			print STDERR $cmd."\n";
			system($cmd);
		}
################################################

        CheckQsub("trans.$blockNum");

################################################
# calculate value of $best using BLEU on batch
		my $maxBLEU=0.0;
        $maxBLEU=200.0 if($__metric eq "TER");
        my $currBLEU=0.0;
        for(my $iter=0; $iter<=10; $iter++){
            if($__metric eq "BLEU"){
    		    $currBLEU=`cat $__workingDir/$blockNum/$iter.iter.input.trans | perl $__metric_script $__workingDir/$blockNum/reference | awk '{print \$3}' | perl -pe 's/,//g'`;
                print STDERR "Running command : cat $__workingDir/$blockNum/$iter.iter.input.trans | perl $__metric_script $__workingDir/$blockNum/reference | awk '{print \$3}' | perl -pe 's/,//g'";
                print STDERR "\nCurrent BLEU : $currBLEU\n";
            }
            elsif($__metric eq "TER"){
                system("sed -e 's/\$/(S-1)/' -i $__workingDir/$blockNum/$iter.iter.input.trans");
                $currBLEU=`perl $__metric_script -r $__workingDir/$blockNum/reference -h $__workingDir/$blockNum/$iter.iter.input.trans -o doc_err | awk '{print \$1}' | perl -pe 's/\s//g'`;
                print STDERR "Running command : perl $__metric_script -r $__workingDir/$blockNum/reference -h $__workingDir/$blockNum/$iter.iter.input.trans -o doc_err\n";
                print STDERR "\nCurrent TER : $currBLEU\n";
            }

            if($__metric eq "TER"){
		    	if($currBLEU < $maxBLEU){
		     		$best = $iter;
		    		$maxBLEU=$currBLEU;
		    	} 
            }
            if($__metric eq "BLEU"){
		    	if($currBLEU > $maxBLEU){
		     		$best = $iter;
		    		$maxBLEU=$currBLEU;
		    	} 
            }
        }
################################################
        print STDERR "best iteration was $best\n";
        $bestIteration[$blockNum]=$best;
    }
################################################
}

sub getWeights
{
	my $weights_file = shift;

	my (@tm, @d, @lm, @wp, @ol);
	my $v=join(" ",@tm);
	my $w=join(" ",@d);
	my $x=join(" ",@lm);
	my $y=join(" ",@wp);
	my $z=join(" ",@ol);

	open FILE, $weights_file or die "Cannot open $weights_file\n"; 
	while(<FILE>)
	{
		chop();
		split(/ /);
		if($_[1] eq "d"){
			push(@d, $_[2]);
		}
		elsif($_[1] eq "tm"){
			push(@tm, $_[2]);
		}
		elsif($_[1] eq "w"){
			push(@wp, $_[2]);
		}
		elsif($_[1] eq "lm"){
			push(@lm, $_[2]);
		}
		elsif($_[1] eq "ol"){
			push(@ol, $_[2]);
		}
	}
	$v=join(" ",@tm);
	$w=join(" ",@d);
	$x=join(" ",@lm);
	$y=join(" ",@wp);
	$z=join(" ",@ol);

	return ($v, $w, $x, $y, $z);
}

sub saveToFile
{
	my ($blockNum, $iter, $file) = @_;
	open FILE, ">$file/$blockNum/$iter.iter.input" or die "Cannot write to $file/$blockNum/$iter.iter.input\n";
# print the modified input file to FILE
	my $offset=$blockNum*$__blockSize + 1;
	if($blockNum==0){$offset=0;}
	for(my $i=$offset; $i<$offset+$__blockSize && $i < scalar(@__source); $i++){
		print FILE $__source[$i]."\n";
		for(my $j=0; $j<$iter; $j++){
			print FILE $__hist[$i]."\n";
		}
	}
	close(FILE);
	open FILE, ">$file/$blockNum/reference" or die "Cannot write to $file/$blockNum/reference\n";
	for(my $i=$offset; $i<$offset+$__blockSize && $i < scalar(@__source); $i++){
		print FILE $__reference[$i]."\n" if($__metric eq "BLEU");
		print FILE $__reference[$i]." (S-$i)\n" if($__metric eq "TER");
	}
	close(FILE);
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


sub CheckQsub{

	my $job_name = shift;
	my $stop = 1;
	while($stop){
		my $command =  "qstat";
		my $status=`$command`;
		my @jobsStatus=split(/\n/,$status);
		my @targetJobStatus  = grep(/$job_name/,@jobsStatus);  #Here we get only the status of the of interest
		if(scalar(@targetJobStatus )== 0) #if no job is running
		{
			$stop=0;
		}
		sleep(10);
	}
}


&main();
