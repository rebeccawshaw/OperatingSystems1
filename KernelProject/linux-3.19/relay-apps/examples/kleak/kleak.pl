#!/usr/bin/perl
# This script processes the kleak per-cpu relay files output from
# kleak and prints a summary of kmalloc/kfree activity.

# Usage: To use this script, you need to first collect kleak data
# (see below) then run the script on the output:
#
# mount -t debugfs debugfs /debug
# insmod ./kleak.ko
# kleak
# Ctrl-c
# ./kleak.pl

$tracefile = "kleak.all";
$symbolfile = "/proc/kallsyms";

my $kmallocs;
my $kfrees;

use constant KMALLOC_ID => 0x1;
use constant KFREE_ID => 0x2;

open(SYMBOLFILE, $symbolfile) or die "can't open $symbolfile: $!";
while (<SYMBOLFILE>) {
    chomp;
    @fields = split;
    $symbols{hex $fields[0]} = $fields[2];
}
@sorted_keys = sort(keys(%symbols));

system("cat cpu* > $tracefile");
open(TRACEFILE, $tracefile) or die "can't open $tracefile: $!";
binmode($tracefile);
while (read(TRACEFILE, $buf, 4)) {
    ($eventid) = unpack("C", $buf);
    if ($eventid == KMALLOC_ID) {
	read(TRACEFILE, $buf, 16);
	($addr, $caller, $size, $objsize) = unpack("IIII", $buf);
#	printf("%x %x %x %d %d\n", $eventid, $addr, $caller, $size, $objsize);
	$kmallocs++;
	$bytes_kmalloced+=$size;
	$objbytes_kmalloced+=$objsize;
	$kmallocs_by_caller{$caller}++;
	$kmalloced_bytes_by_caller{$caller}+=$size;
	$kmalloced_objbytes_by_caller{$caller}+=$objsize;
	$kmalloc_addrs{$addr} = $caller;
    } else {
	read(TRACEFILE, $buf, 12);
	($addr, $caller, $objsize) = unpack("III", $buf);
#	printf("%x %x %x %d %d\n", $eventid, $addr, $caller, $objsize);
	$kfrees++;
	$bytes_kfreed+=$objsize;
	$kfrees_by_caller{$caller}++;
	$kfreed_bytes_by_caller{$caller}+=$objsize;
	$kfree_addrs{$addr} = $caller;
    }
}
summarize();

sub lookup_symbol {
    my ($caller) = @_;
    $symbol = $cached_callers{$caller};
    if ($symbol) {
	return $symbol;
    }
    for($i = 0; $i < scalar(@sorted_keys) - 1; $i++) {
	if ($caller >= $sorted_keys[$i] && $caller < $sorted_keys[$i+1]) {
	    $symbol = sprintf("%s+0x%x", $symbols{$sorted_keys[$i]}, $offset);
	    $cached_callers{$caller} = $symbol;
	    $offset = $caller - $sorted_keys[$i];
	    break;
	}
    }
    if (!$symbol) {
	$symbol = "unknown";
    }

    return $symbol;
}

sub summarize {
    print "Total kmallocs: $kmallocs\n";
    print "Total kfrees: $kfrees\n";

    print "\nTotal bytes kmalloced: $bytes_kmalloced [$objbytes_kmalloced with slack]\n";
    print "Total bytes kfreed: $bytes_kfreed\n";

    print "\nTotal kmallocs by caller:\n";
    while (($caller, $count) = each %kmallocs_by_caller) {
	$symbol = lookup_symbol($caller);
	printf("  %x [%s]: %d\n", $caller, $symbol, $count);
    }
    print "Total kfrees by caller:\n";
    while (($caller, $count) = each %kfrees_by_caller) {
	$symbol = lookup_symbol($caller);
	printf("  %x [%s]: %d\n", $caller, $symbol, $count);
    }

    print "\nTotal kmalloced bytes by caller:\n";
    while (($caller, $count) = each %kmalloced_bytes_by_caller) {
	$symbol = lookup_symbol($caller);
	printf("  %x [%s]: %d [%d]\n", $caller, $symbol, $count, $kmalloced_objbytes_by_caller{$caller});
    }
    print "Total kfreed bytes by caller:\n";
    while (($caller, $count) = each %kfreed_bytes_by_caller) {
	$symbol = lookup_symbol($caller);
	printf("  %x [%s]: %d\n", $caller, $symbol, $count);
    }

    print "\nUnfreed kmallocs:\n";
    while (($addr, $caller) = each %kmalloc_addrs) {
	if (!$kfree_addrs{$addr}) {
	    $symbol = lookup_symbol($caller);
	    $unfreed_kmallocs{$symbol}++;
	}
    }
    while (($symbol, $count) = each %unfreed_kmallocs) {
	    printf("  %s: %d\n", $symbol, $count);
    }
    print "Unmalloced kfrees:\n";
    while (($addr, $caller) = each %kfree_addrs) {
	if (!$kmalloc_addrs{$addr}) {
	    $symbol = lookup_symbol($caller);
	    $unmalloced_kfrees{$symbol}++;
	}
    }
    while (($symbol, $count) = each %unmalloced_kfrees) {
	    printf("  %s: %d\n", $symbol, $count);
    }
    close(TRACEFILE);
    close(SYMBOL_FILE);
}
