#! /usr/bin/perl

#******************************************************************************
# Prashant Mathur @ FBK-irst. September 2013
#******************************************************************************
# Interlingual single trigger model
# build-trigger-model.pl --corpus <corpus> --srcln en --trgln fr --omodel ISTM.<corpus> 


use strict;
use warnings;
use open ':utf8';
binmode STDIN, ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";

sub main {
my $__usage = "
USAGE
-----
perl build-trigger-model.pl --corpus=name --srcln=en --trgln=fr --omodel=filename [--prune]
-----
";

my $__debug;
my $__corpus="";
my $__srcln="";
my $__trgln="";
my $__omodel="";
my $__help;
my $__prune;
my %__model=(); #store the model here
my %__joint_counts=();
my $__pairwise_counts=0;
my $__src_counts=0;
my $__trg_counts=0;
my $__counter=0;
my (%__srcfreq, %__trgfreq)=();
GetOptions ('debug' => \$__debug, 
				'corpus=s' => \$__corpus,
				'srcln=s' => \$__srcln,
				'trgln=s' => \$__trgln,
				'omodel=s' => \$__omodel,
				'prune' => \$__prune,
            'help' => \$__help);

if($__help) { die "$__usage\n\n"; }

print STDERR "
******************************
CORPUS = $__corpus
Collecting counts";
open __SRCCORPUS, "$__corpus.$__srcln" or die "OOPS! Couldn't open the source corpus file\n";
open __TRGCORPUS, "$__corpus.$__trgln" or die "OOPS! Couldn't open the target corpus file\n";

while(my $__src=<__SRCCORPUS>)
{
	my $__trg=<__TRGCORPUS>;
	$__counter++;
	if($__counter%1000==0){print STDERR ".";}
	chop($__src);
	chop($__trg);
	my @__TOKENS_SRC = split(/ /,$__src);
	my @__TOKENS_TRG = split(/ /,$__trg);
	for(my $i=0;$i<@__TOKENS_SRC; $i++)
	{
		$__srcfreq{$__TOKENS_SRC[$i]}++;
		$__pairwise_counts+=scalar(@__TOKENS_TRG);
		$__src_counts++;
	}
	for(my $i=0;$i<@__TOKENS_TRG; $i++)
	{
		$__trgfreq{$__TOKENS_TRG[$i]}++;
		$__pairwise_counts+=scalar(@__TOKENS_SRC);
		$__trg_counts++;
	}
	if(!$__prune)
	{
	  for(my $i=0;$i<@__TOKENS_SRC; $i++)
	  {
		 for(my $j=0;$j<@__TOKENS_TRG; $j++)
		 {
			if(exists $__srcfreq{$__TOKENS_SRC[$i]} && exists $__trgfreq{$__TOKENS_TRG[$j]}){
			  $__joint_counts{$__TOKENS_SRC[$i]}{$__TOKENS_TRG[$j]}++;  # joint counts  
			}
		 }
	  }
	}
}
# -- Pruning top.k --
if($__prune){
  foreach my $key(sort {$__srcfreq{$b} <=> $__srcfreq{$a}} keys %__srcfreq)
  {
	 if($key !~ m/^[a-zA-Z]+$/) {delete $__srcfreq{$key};}
  }
  my $count=0;
  foreach my $key(sort {$__trgfreq{$b} <=> $__trgfreq{$a}} keys %__trgfreq)
  {
	 delete $__trgfreq{$key};
	 $count++;
	 if($count>100) {last;}
  }
}


close(__SRCCORPUS);
close(__TRGCORPUS);

if($__prune)
{
	$__counter=0;
	open __SRCCORPUS, "$__corpus.$__srcln" or die "OOPS! Couldn't open the source corpus file\n";
	open __TRGCORPUS, "$__corpus.$__trgln" or die "OOPS! Couldn't open the target corpus file\n";
print STDERR "
******************************
Pruning";
	while(my $__src=<__SRCCORPUS>)
	{
		my $__trg=<__TRGCORPUS>;
		chop($__src);
		chop($__trg);
		$__counter++;
		if($__counter%1000==0){print STDERR ".";}
		my @__TOKENS_SRC = split(/ /,$__src);
		my @__TOKENS_TRG = split(/ /,$__trg);
		for(my $i=0;$i<@__TOKENS_SRC; $i++)
		{
			if(defined $__srcfreq{$__TOKENS_SRC[$i]}){
				for(my $j=0;$j<@__TOKENS_TRG; $j++)
				{
					if(defined $__trgfreq{$__TOKENS_TRG[$j]} && abs(length($__TOKENS_SRC[$i]) - length($__TOKENS_TRG[$j])) < 3 ){
					  $__joint_counts{$__TOKENS_SRC[$i]}{$__TOKENS_TRG[$j]}++;	# joint counts  
					}
				}
			}
		}
	}
	print STDERR "
******************************
";
	close(__SRCCORPUS);
	close(__TRGCORPUS);
}

&normalize($__pairwise_counts, \%__joint_counts, \%__srcfreq, \%__trgfreq, $__src_counts, $__trg_counts, \%__model);

&DumpModel(\%__model, $__omodel);

print STDERR "
******************************
Zipping .. 
";
system("gzip $__omodel");
print STDERR "
******************************
";
}
sub normalize{
print STDERR "
******************************
Calculating PMI values";
	my (%__joint_prob, %__prob_x, %__prob_y)=();
	my ($__pairwise_counts, $__joint_counts, $__srcfreq, $__trgfreq, $__src_counts, $__trg_counts, $__model)=@_;
	foreach my $__token1(keys \%{$__srcfreq})
	{
		$__prob_x{$__token1}=($__srcfreq->{$__token1} * 1.0)/($__src_counts*1.0);
	}
	foreach my $__token2(keys \%{$__trgfreq})
	{
		$__prob_y{$__token2}=($__trgfreq->{$__token2}*1.0)/($__trg_counts*1.0);
	}
	foreach my $__token1(keys \%{$__joint_counts})
	{
		foreach my $__token2(keys \%{$__joint_counts->{$__token1}})
		{
			$__joint_prob{$__token1}{$__token2}=($__joint_counts->{$__token1}->{$__token2}*1.0)/($__pairwise_counts*1.0);
			$__model->{$__token1}->{$__token2}=log($__joint_prob{$__token1}{$__token2})-(log($__prob_x{$__token1})+log($__prob_y{$__token2}));
		}
	}
				
print STDERR "
******************************
";
}

sub DumpModel
{

print STDERR "
******************************
Dumping Model 
";
	my ($__model, $output)=@_;
	open MODEL, ">", $output or die "Cannot open the model in write mode, Do you have permissions?\n";
	foreach my $__token1(sort keys \%{$__model})
	{
		foreach my $__token2(sort keys \%{$__model->{$__token1}})
		{
			print MODEL $__token1,"|||",$__token2,"|||",$__model->{$__token1}->{$__token2},"\n";
		}
	}
print STDERR "
******************************
";
}

&main();
