#!/usr/bin/env perl

my %makeoptions = {
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
	"KERNEL" => "kernel",
};
my %options = {};
my @includes = ("REPLACED", ".", "sys");
my @files = ();
my @devices = ();
my @devhashes = ();
my @locators = ();

open(IN, "<", "sys/include/sys/device.h");
while(my $l = <IN>){
	$l =~ s/\r?\n//g;

	if($l =~ /locator: ([^ ]+)/){
		push(@locators, $1);
	}
}
close(IN);

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

		if($tokens[1] eq "at"){
			my $dev = $tokens[0];

			$dev =~ s/([0-9]+)$//;
			my $unit = $1;

			if(!(grep /^$dev$/, @devices)){
				push(@devices, $dev);
			}

			my @dev_locators = ();

			for(my $i = 0; $i < @locators; $i++){
				push(@dev_locators, "0");
			}

			for(my $i = 3; $i < @tokens; $i++){
				if(grep(/^$tokens[$i]$/, @locators)){
					my ($index) = grep { $locators[$_] ~~ $tokens[$i] } 0 .. $#locators;
					$dev_locators[$index] = $tokens[$i + 1];
					$i++;
					next;
				}
			}

			push(@devhashes, {
				"dev" => $dev,
				"unit" => $unit,
				"parent" => $tokens[2],
				"locators" => \@dev_locators
			});
		}elsif(($tokens[0] eq "options") or ($tokens[0] eq "makeoptions")){
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

			foreach my $include (@includes){
				if(-f $include . "/" . $path){
					parse_config($include . "/" . $path);
					last;
				}
			}
		}elsif($tokens[0] eq "file"){
			my $path = $_[0];

			$path =~ s/\/[^\/]+$//;
			$path =~ s/\/[^\/]+$//;

			if(!$tokens[2] or (grep /^$tokens[2]$/, @devices)){
				push(@files, $path . "/" . $tokens[1]);
			}
		}
	};

	close($fh);
}

parse_config("sys/conf/std");
parse_config($ARGV[0]);
parse_config("sys/conf/files.std");

open(MKCFG, ">", "compile/Makefile.config");
foreach my $makeoption (sort { $a cmp $b } keys(%makeoptions)){
	print(MKCFG "$makeoption = $makeoptions{$makeoption}\n");
}
close(MKCFG);

open(FILES, ">", "compile/files.kernel");
close(FILES);
foreach my $file (@files){
	system("echo \"$file\" | ./tools/makedep.sh sys >> compile/files.kernel");
}

open(IOCONF, ">", "compile/sys/ioconf.c");
print(IOCONF "#include <sys/device.h>\n");
print(IOCONF "#include <stddef.h>\n");
print(IOCONF "\n");
foreach my $device (@devices){
}
print(IOCONF "struct device_config devices[] = {\n");
foreach my $device (@devhashes){
	print(IOCONF "	{\n");
	print(IOCONF "		\"$device->{dev}\",\n");
	print(IOCONF "		$device->{unit},\n");
	print(IOCONF "		\"$device->{parent}\",\n");
	print(IOCONF "		{\n");
	foreach my $locator (@{$device->{locators}}){
		print(IOCONF "			$locator,\n");
	}
	print(IOCONF "		}\n");
	print(IOCONF "	},\n");
}
print(IOCONF "	{\n");
print(IOCONF "		NULL\n");
print(IOCONF "	}\n");
print(IOCONF "};\n");
close(IOCONF);
