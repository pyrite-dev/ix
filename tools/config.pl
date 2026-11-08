#!/usr/bin/env perl

my %makeoptions = (
	"CC" => "cc",
	"CFLAGS" => "",
	"AS" => "as",
	"ASFLAGS" => "",
	"LD" => "ld",
	"LDFLAGS" => "",
	"OBJCOPY" => "objcopy",
	"OCFLAGS" => "",
	"CPP" => "cpp",
	"CPPFLAGS" => "",
	"KERNEL" => "kernel"
);
my %options = ();
my @includes = ("REPLACED", ".", "sys");

sub parse_config {
	open(my $fh, "<", $_[0]) or die;
	while(my $l = <$fh>){
		$l =~ s/[\r\n]//g;
		my @tokens = $l =~ /("(?:\\.|[^"\\])*"|[^\s"]+)/g;
		my $discard = 0;
		
		map { s/^"(.*)"$/$1/g } @tokens;
		map { s/\\(.)/$1/g } @tokens;
		@tokens = grep {
			if($discard){
				0;
			}else{
				my $m = $_ =~ /#/;
	
				if($m){
					$discard = 1;
				}
	
				!$m;
			}
		} @tokens;
		map { s/#.+// } @tokens;
	
		if(($tokens[0] eq "options") or ($tokens[0] eq "makeoptions")){
			my $key = substr($tokens[1], 0, -1);
			my $value = $tokens[2];
	
			if($tokens[0] eq "options"){
				$options{$key} = $value;
			}else{
				$makeoptions{$key} = $value;
			}
		}elsif($tokens[0] eq "include"){
			my $path = $tokens[1];

			$includes[0] = $_[0];
			$includes[0] =~ s/\/[^\/]+$//;

			print("$includes[0]\n");

			foreach my $include (@includes){
				if(-f $include . "/" . $path){
					parse_config($include . "/" . $path);
				}
			}
		}
	};

	close($fh);
}

parse_config("sys/conf/std");
parse_config($ARGV[0]);

open(MKCFG, ">", "compile/Makefile.config");
foreach my $makeoption (sort { $a cmp $b } keys(%makeoptions)){
	print(MKCFG "$makeoption = $makeoptions{$makeoption}\n");
}
close(MKCFG);
