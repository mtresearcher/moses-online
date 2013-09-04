#! /usr/bin/perl

#******************************************************************************
# Prashant Mathur @ FBK-irst. September 2013
#******************************************************************************
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
perl build.pl --corpus=name.trgln --omodel=filename
-----
";

my $__debug;
my $__corpus="";
my $__omodel="";
my $__help;
my %model=();
my $__counter=0;
GetOptions ('debug' => \$__debug, 
				'corpus=s' => \$__corpus,
				'omodel=s' => \$__omodel,
            'help' => \$__help);

if($__help) { die "$__usage\n\n"; }

print STDERR "
******************************
CORPUS = $__corpus
Building the model...";

open __CORP, "$__corpus" or die "OOPS! Couldn't open the file\n";

while(<__CORP>)
{
	$__counter++;
	if($__counter%1000==0){print STDERR ".";}
	chop();
	my @__TOKENS = split(/ /);
	for(my $i=0;$i<@__TOKENS; $i++)
	{
		for(my $j=$i+1;$j<@__TOKENS; $j++)
		{
			$model{$__TOKENS[$i]}{$__TOKENS[$j]}++;
			$model{$__TOKENS[$j]}{$__TOKENS[$i]}++;
		}
	}
}
print STDERR "
******************************
";
&normalize(\%model);
&DumpModel(\%model, $__omodel);

}

sub normalize{
print STDERR "
******************************
Normalizing the values ..
";
	my ($model)=@_;
	foreach my $__token1(keys \%{$model})
	{
		foreach my $__token2(keys \%{$model->{$__token1}})
		{
			my $temp1=$model->{$__token1}->{$__token2};
			my $temp2=scalar keys (\%{$model->{$__token1}});
			$model->{$__token1}->{$__token2}=sprintf '%.3f',($temp1/$temp2);
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
			print MODEL $__token1,"|||",$__token2,"|||",$model->{$__token1}->{$__token2},"\n";
		}
	}
}

&main();
