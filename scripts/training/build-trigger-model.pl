#! /usr/bin/perl

#******************************************************************************
# Prashant Mathur @ FBK-irst. September 2013
#******************************************************************************
# Interlingual single trigger model
# build-model.pl <corpus>


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
perl build.pl --corpus=name --srcln=en --trgln=fr --omodel=filename [--dist-normalize]
-----
";

my $__debug;
my $__corpus="";
my $__srcln="";
my $__trgln="";
my $__omodel="";
my $__help;
my $__norm;
my %model=();
my $__counter=0;
my (%srcfreq, %trgfreq)=();
GetOptions ('debug' => \$__debug, 
				'corpus=s' => \$__corpus,
				'srcln=s' => \$__srcln,
				'trgln=s' => \$__trgln,
				'omodel=s' => \$__omodel,
				'dist-normalize' => \$__norm,
            'help' => \$__help);

if($__help) { die "$__usage\n\n"; }

print STDERR "
******************************
CORPUS = $__corpus
Cleaning the corpus...";
open __SRCCORPUS, "$__corpus.$__srcln" or die "OOPS! Couldn't open the source corpus file\n";
open __TRGCORPUS, "$__corpus.$__trgln" or die "OOPS! Couldn't open the target corpus file\n";

while(my $__src=<__SRCCORPUS>)
{
	my $__trg=<__TRGCORPUS>;
	chop($__src);
	chop($__trg);
	my @__TOKENS_SRC = split(/ /,$__src);
	my @__TOKENS_TRG = split(/ /,$__trg);
	for(my $i=0;$i<@__TOKENS_SRC; $i++)
	{
		$srcfreq{$__TOKENS_SRC[$i]}++;
	}
	for(my $i=0;$i<@__TOKENS_TRG; $i++)
	{
		$trgfreq{$__TOKENS_TRG[$i]}++;
	}
}
my $count=0;
foreach my $key(sort {$srcfreq{$b} <=> $srcfreq{$a}} keys %srcfreq)
{
	delete $srcfreq{$key};
	$count++;
	if($count>100) {last;}
}
$count=0;
foreach my $key(sort {$trgfreq{$b} <=> $trgfreq{$a}} keys %trgfreq)
{
	delete $trgfreq{$key};
	$count++;
	if($count>100) {last;}
}


close(__SRCCORPUS);
close(__TRGCORPUS);

print STDERR "
******************************
CORPUS = $__corpus
Building the model...";

open __SRCCORPUS, "$__corpus.$__srcln" or die "OOPS! Couldn't open the source corpus file\n";
open __TRGCORPUS, "$__corpus.$__trgln" or die "OOPS! Couldn't open the target corpus file\n";


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
		for(my $j=0;$j<@__TOKENS_TRG; $j++)
		{
			if(exists $srcfreq{$__TOKENS_SRC[$i]} && exists $trgfreq{$__TOKENS_TRG[$j]}){
				if($__norm){$model{$__TOKENS_TRG[$j]}{$__TOKENS_SRC[$i]}+=SigMod(abs($i-$j));}
				else {$model{$__TOKENS_TRG[$j]}{$__TOKENS_SRC[$i]}++;}

			}
		}
	}
}
print STDERR "
******************************
";
&normalize(\%model,$__norm);
&DumpModel(\%model, $__omodel);

}
sub normalize{
print STDERR "
******************************
Normalizing the values ..
";
	my ($model,$__norm)=@_;
	if($__norm)
	{
	foreach my $__token1(keys \%{$model})
	{
		my $sum=0;
		foreach my $__token2(keys \%{$model->{$__token1}})
		{
			$sum+==$model->{$__token1}->{$__token2};
		}
		foreach my $__token2(keys \%{$model->{$__token1}})
		{
		  my $temp1=$model->{$__token1}->{$__token2};
		  $model->{$__token1}->{$__token2}=($temp1/$sum);
		}
	}
	}
	else
	{
	foreach my $__token1(keys \%{$model})
	{
		foreach my $__token2(keys \%{$model->{$__token1}})
		{
			my $temp1=$model->{$__token1}->{$__token2};
			my $temp2=scalar keys (\%{$model->{$__token1}});
			$model->{$__token1}->{$__token2}=($temp1/$temp2);
		}
	}
	}
print STDERR "
******************************
";
}

sub DumpModel
{
	my ($model, $output)=@_;
	open MODEL, ">", $output or die "Cannot open the model in write mode, Do you have permissions?\n";
	foreach my $__token1(keys \%{$model})
	{
		foreach my $__token2(keys \%{$model->{$__token1}})
		{
			print MODEL $__token2,"|||",$__token1,"|||",$model->{$__token1}->{$__token2},"\n";
		}
	}
}

sub SigMod
{
	my $i=shift;
	return ($i/1+abs($i));
}

&main();
