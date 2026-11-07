#!/usr/bin/env perl

my @whitelist = ("CC", "CFLAGS", "AS", "ASFLAGS", "LD", "LDFLAGS", "OBJCOPY", "CPP", "CPPFLAGS", "KERNEL");
my %options = (
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

open(IN, "<", $ARGV[0]) or die;
while(my $l = <IN>){
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

	if($tokens[0] eq "options"){
		my $key = substr($tokens[1], 0, -1);
		my $value = $tokens[2];

		$options{$key} = $value;
	}
}
close(IN);

open(MKCFG, ">", "compile/Makefile.config");
foreach my $option (@whitelist){
	print(MKCFG "$option = $options{$option}\n");
}
close(MKCFG);
